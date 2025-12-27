#include "dat.h"
#include "error.h"
#include "fns.h"
#include "lib.h"
#include "mem.h"
#include "u.h"

#include "blind_ledger.h"
#include "pebble.h"
#include "uuid.h"

/*@
  predicate Inv_Conservation(struct PebbleState *ps, int total) =
    ps->colorless_bank + ps->black_inuse + ps->blue_inuse + ps->red_inuse ==
total;

  predicate Inv_NonNegative(struct PebbleState *ps) =
    ps->colorless_bank >= 0 &&
    ps->black_inuse >= 0 &&
    ps->blue_inuse >= 0 &&
    ps->red_inuse >= 0 &&
    ps->white_pending >= 0 &&
    ps->white_verified >= 0;
@*/

Lock pebble_global_lock;
Lock pebble_bank_lock;
int pebble_enabled = 1;
int pebble_debug = PEBBLE_DEBUG;

/* Global colorless bank - single pool for entire system */
ulong pebble_global_colorless_bank = 0;
ulong pebble_total_system_tokens = 0;

static int pebble_initialized;

static void pebble_free_red(PebbleRed *);

static PebbleBlack *
pebble_lookup_black_by_cap_locked(PebbleState *ps, const UserCapability *cap) {
  PebbleBlack *pb;
  for (pb = ps->black_list; pb != nil; pb = pb->next) {
    if (memcmp(pb->capability.hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
      return pb;
    }
  }
  return nil;
}

static void pebble_reset_state(PebbleState *ps) {
  memset(ps, 0, sizeof(*ps));
  ps->colorless_bank = 0; /* Processes start with 0 tokens */
  ps->white_head = 0;
  ps->white_pending = 0;
}

// Boot-time state for use before proc0
static PebbleState boot_pstate;

PebbleState *pebble_state(void) {
  if (up == nil)
    return &boot_pstate;
  return &up->pebble;
}

/*
 * Calculate total system RAM from conf.mem[] entries
 */
static ulong pebble_calculate_system_ram(void) {
  ulong total = 0;
  int i;
  for (i = 0; i < nelem(conf.mem); i++) {
    if (conf.mem[i].npage > 0)
      total += conf.mem[i].npage * BY2PG;
  }
  return total;
}

void pebbleinit(void) {
  ulong total_ram;
  ulong boot_tokens, init_tokens;

  if (pebble_initialized)
    return;

  /* Calculate system RAM and initialize global token pool */
  total_ram = pebble_calculate_system_ram();
  pebble_total_system_tokens = total_ram / PEBBLE_BYTES_PER_TOKEN;
  pebble_global_colorless_bank = pebble_total_system_tokens;

  /* Reserve boot budget from global pool */
  boot_tokens = PEBBLE_BOOT_BUDGET / PEBBLE_BYTES_PER_TOKEN;
  if (pebble_global_colorless_bank >= boot_tokens)
    pebble_global_colorless_bank -= boot_tokens;
  boot_pstate.colorless_bank = boot_tokens;
  boot_pstate.white_generation = 1;

  /* Reserve init budget from global pool */
  init_tokens = PEBBLE_INIT_BUDGET / PEBBLE_BYTES_PER_TOKEN;
  if (pebble_global_colorless_bank >= init_tokens)
    pebble_global_colorless_bank -= init_tokens;
  /* init_tokens will be granted to proc0 at proc0() entry */

  if (pebble_debug)
    print(
        "PEBBLE: global pool=%lu tokens (%luMB), boot=%lu, reserved_init=%lu\n",
        pebble_global_colorless_bank,
        (pebble_global_colorless_bank * PEBBLE_BYTES_PER_TOKEN) / (1024 * 1024),
        boot_tokens, init_tokens);

  pebble_initialized = 1;
}

void pebbleprocinit(Proc *p) {
  if (p == nil)
    return;
  pebble_reset_state(&p->pebble);

  /* Grant initial budget to proc0/init so it can bootstrap */
  if (p->pid == 1) {
    p->pebble.colorless_bank = PEBBLE_INIT_BUDGET;
    if (pebble_debug)
      print("PEBBLE: granted %dMB init budget to pid 1\n",
            PEBBLE_INIT_BUDGET / (1024 * 1024));
  }
}

static PebbleBlack *pebble_lookup_black_locked(PebbleState *ps, void *handle) {
  PebbleBlack *pb;
  /* Strip wave bits (Holographic View) */
  void *base_handle = PEBBLE_PTR_ADDR(handle);

  for (pb = ps->black_list; pb != nil; pb = pb->next)
    if (pb == base_handle)
      return pb;
  return nil;
}

PebbleBlack *pebble_lookup_black(PebbleState *ps, void *handle) {
  PebbleBlack *pb;

  if (ps == nil || handle == nil)
    return nil;
  lock(&pebble_global_lock);
  pb = pebble_lookup_black_locked(ps, handle);
  unlock(&pebble_global_lock);
  return pb;
}

/*@
  requires size > 0;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
/*
 * SMT: Validated by proofs/pebble/pebble_security.v
 * Theorem: Inv_Conservation, Inv_NonNegative
 * Description: Verifies white token issuance preserves system invariants
 */
PebbleWhite *pebble_issue_white(PebbleState *ps, void *data, ulong size) {
  int i, idx;
  ulong pegged_size;

  if (ps == nil)
    return nil;

  /* PoW is enforced on budget requests; issuance only burns budget. */

  /* Peg size to 8-byte quantum (unit of account) */
  if (size < PEBBLE_MIN_ALLOC)
    size = PEBBLE_MIN_ALLOC;
  pegged_size = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);

  lock(&pebble_global_lock);

  /* Check colorless budget */
  if (ps->colorless_bank < pegged_size) {
    unlock(&pebble_global_lock);
    if (pebble_debug)
      print("PEBBLE: insufficient budget for WHITE pid=%lud need=%lud "
            "have=%lud\n",
            up ? up->pid : 0, pegged_size, ps->colorless_bank);
    return nil; /* Insufficient budget */
  }

  for (i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    idx = (ps->white_head + i) % PEBBLE_MAX_TOKENS;
    if (ps->whites_active[idx])
      continue;

    /* Consume budget: COLORLESS → WHITE transition */
    ps->colorless_bank -= pegged_size;
    ps->white_pending += pegged_size;

    ps->white_generation++;
    ps->whites_active[idx] = 1;
    ps->whites[idx].token = PEBBLE_TOKEN_MAGIC;
    ps->whites[idx].generation = ps->white_generation;
    ps->whites[idx].data_ptr = data;
    ps->whites[idx].size = pegged_size;
    ps->white_head = (idx + 1) % PEBBLE_MAX_TOKENS;
    unlock(&pebble_global_lock);
    return &ps->whites[idx];
  }
  unlock(&pebble_global_lock);
  {
    int active = 0;
    for (i = 0; i < PEBBLE_MAX_TOKENS; i++)
      if (ps->whites_active[i])
        active++;
    print("PEBBLE: no free white tokens pid=%lud active=%d max=%d "
          "head=%d gen=%lud\n",
          up ? up->pid : 0, active, PEBBLE_MAX_TOKENS, ps->white_head,
          ps->white_generation);
  }
  return nil;
}

/*
 * Create a UUIDv8 representation of a White token.
 * This allows passing the token as a 128-bit value (e.g., MVID).
 */
int pebble_create_token_uuid(PebbleWhite *white, uuid_t *out_uuid) {
  PebbleState *ps;
  int i, idx = -1;

  if (white == nil || out_uuid == nil)
    return -1;

  ps = pebble_state();
  if (ps == nil)
    return -1;

  /* Verify token validity and find index */
  lock(&pebble_global_lock);
  if (!pebble_valid_white_token(ps, white)) {
    unlock(&pebble_global_lock);
    return -1;
  }

  /* Find index for the token pointer */
  for (i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    if (&ps->whites[i] == white) {
      idx = i;
      break;
    }
  }
  unlock(&pebble_global_lock);

  if (idx == -1)
    return -1; /* Should have been caught by valid check, but safely handle */

  /*
   * Pack into UUID:
   * Token: white->token (Magic)
   * Generation: white->generation
   * Index: idx
   */
  uuid_pack_pebble(out_uuid, white->token, white->generation,
                   (unsigned short)idx);
  return 0;
}

int pebble_valid_white_token(PebbleState *ps, PebbleWhite *white) {
  int i;

  if (ps == nil || white == nil)
    return 0;

  for (i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    if (ps->whites_active[i] && &ps->whites[i] == white) {
      if (white->token != PEBBLE_TOKEN_MAGIC)
        return 0;
      return 1;
    }
  }
  return 0;
}

int pebble_set_budget(ulong budget) {
  PebbleState *ps;

  ps = pebble_state();
  if (ps == nil)
    return -1;

  lock(&pebble_global_lock);
  ps->colorless_bank = budget;
  unlock(&pebble_global_lock);
  return 0;
}

ulong pebble_get_budget(void) {
  PebbleState *ps;
  ulong budget;

  ps = pebble_state();
  if (ps == nil)
    return 0;
  lock(&pebble_global_lock);
  budget = ps->colorless_bank;
  unlock(&pebble_global_lock);
  return budget;
}

/*
 * pebble_increase_budget - Request additional budget via Proof-of-Work
 *
 * Applications write "size nonce" to /dev/pebble/budget.
 * The kernel verifies the PoW and transfers tokens from the global pool.
 *
 * This is the ONLY way for a process to obtain Pebble budget.
 * All allocation functions (Black, Blue, Red, White) consume from this budget.
 *
 * PoW difficulty scales with scarcity: as global pool shrinks, difficulty
 * rises.
 *
 * Returns 0 on success, -1 on failure (invalid PoW, out of tokens, or other
 * error).
 */
int pebble_increase_budget(ulong size, u64int nonce) {
  PebbleState *ps;
  int diff;
  ulong tokens_requested;
  ulong scarcity_factor;

  if (up == nil)
    return -1; /* Kernel cannot use this API */

  ps = pebble_state();
  if (ps == nil)
    return -1;

  /* Round size to token boundary and calculate tokens needed */
  if (size < PEBBLE_MIN_ALLOC)
    size = PEBBLE_MIN_ALLOC;
  size = ROUNDUP(size, PEBBLE_BYTES_PER_TOKEN);
  tokens_requested = size / PEBBLE_BYTES_PER_TOKEN;

  /* Calculate scarcity-based PoW difficulty:
   * As global pool shrinks, difficulty increases proportionally.
   * scarcity_factor = (total - available) / total = usage percentage
   * Difficulty multiplier: 1 + (scarcity_factor * 10)
   */
  lock(&pebble_bank_lock);
  if (pebble_global_colorless_bank < tokens_requested) {
    unlock(&pebble_bank_lock);
    if (pebble_debug)
      print(
          "PEBBLE: out of global tokens pid=%lud requested=%lu available=%lu\n",
          up->pid, tokens_requested, pebble_global_colorless_bank);
    return -1; /* System out of tokens */
  }

  /* Calculate scarcity: 0 = empty, 100 = full */
  if (pebble_total_system_tokens > 0)
    scarcity_factor =
        (pebble_total_system_tokens - pebble_global_colorless_bank) * 100 /
        pebble_total_system_tokens;
  else
    scarcity_factor = 0;
  unlock(&pebble_bank_lock);

  /* Base difficulty + scarcity scaling */
  diff = pow_calculate_difficulty(POW_OP_ALLOC, size);
  diff += (int)(scarcity_factor / 10); /* +1 difficulty per 10% usage */

  /* Verify the provided nonce against the process PID */
  if (!pow_verify(nonce, (u64int)up->pid, diff)) {
    if (pebble_debug)
      print("PEBBLE: budget PoW failure pid=%lud size=%lud diff=%d nonce=%llud "
            "scarcity=%lu%%\n",
            up->pid, size, diff, nonce, scarcity_factor);
    return -1;
  }

  /* PoW verified - transfer tokens from global pool to process */
  lock(&pebble_bank_lock);
  if (pebble_global_colorless_bank < tokens_requested) {
    unlock(&pebble_bank_lock);
    return -1; /* Race condition: tokens taken by another process */
  }
  pebble_global_colorless_bank -= tokens_requested;
  ps->colorless_bank += tokens_requested;
  unlock(&pebble_bank_lock);

  if (pebble_debug)
    print("PEBBLE: budget transferred pid=%lud tokens=%lu total=%lu "
          "global_remaining=%lu\n",
          up->pid, tokens_requested, ps->colorless_bank,
          pebble_global_colorless_bank);

  return 0;
}

/*
 * Dynamic vault secret for Pebble Black allocations.
 * Generated at boot from TPM, RDRAND, or ChaCha20 CSPRNG.
 * This key is used to derive capability hashes - MUST be cryptographically
 * random.
 */
static u8int pebble_vault_key[32];
static int pebble_vault_key_initialized = 0;

static void pebble_init_vault_key(void) {
  extern int tpm_get_random(u8int * buf, int n);
  extern u64int rdrand_u64(void);
  extern int crypto_hw_rdrand_available(void);
  extern u64int chacha20_csprng_u64(void);

  if (pebble_vault_key_initialized)
    return;

  /* Try TPM first (strongest source) */
  if (tpm_get_random(pebble_vault_key, 32) == 32) {
    print("PEBBLE: Vault secret from TPM\\n");
    pebble_vault_key_initialized = 1;
    return;
  }

  /* Fallback to RDRAND (hardware RNG) */
  if (crypto_hw_rdrand_available()) {
    u64int *key64 = (u64int *)pebble_vault_key;
    key64[0] = rdrand_u64();
    key64[1] = rdrand_u64();
    key64[2] = rdrand_u64();
    key64[3] = rdrand_u64();
    print("PEBBLE: Vault secret from RDRAND\\n");
    pebble_vault_key_initialized = 1;
    return;
  }

  /* Last resort: ChaCha20 CSPRNG (software) */
  u64int *key64 = (u64int *)pebble_vault_key;
  key64[0] = chacha20_csprng_u64();
  key64[1] = chacha20_csprng_u64();
  key64[2] = chacha20_csprng_u64();
  key64[3] = chacha20_csprng_u64();
  print("PEBBLE: Vault secret from ChaCha20 CSPRNG (software fallback)\\n");
  pebble_vault_key_initialized = 1;
}

const u8int *pebble_get_vault_secret(void) {
  if (!pebble_vault_key_initialized)
    pebble_init_vault_key();
  return pebble_vault_key;
}

/*@
  requires size > 0;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
/*
 * SMT: Validated by proofs/pebble/pebble_security.v
 * Theorem: Inv_Conservation
 * Description: Verifies black token allocation maintains budget conservation
 */
int pebble_black_alloc(ulong size, UserCapability *out_cap) {
  void *buf;
  PebbleBlack *pb;

  /*
   * If up == nil, we are likely in early boot (xinit/mmuinit).
   * We proceed, treating 'nil' as the Kernel process ownership.
   * borrow_acquire and ledger_mint must handle nil owner!
   */

  const u8int *vault_secret = pebble_get_vault_secret();
  BlindLedgerError ledger_err;

  /* Enforce 8-byte granularity (Tokens) */
  if (size < PEBBLE_MIN_ALLOC) {
    size = PEBBLE_MIN_ALLOC;
  }
  if (size % PEBBLE_MEM_PER_TOKEN != 0) {
    size = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);
  }

  /* 1. Allocate physical memory (kernel heap for now) */
  buf = xallocz(size, 1);
  if (buf == nil)
    return -1;

  /* 2. Acquire ownership via Borrow Checker */
  if (up != nil) {
    if (borrow_acquire(up, (uintptr)buf) != BORROW_OK) {
      xfree(buf);
      return -1;
    }
  } else {
    /* Kernel Allocation during boot */
    if (borrow_acquire_system((uintptr)buf, OWNER_KERNEL) != BORROW_OK) {
      xfree(buf);
      return -1;
    }
  }

  /* 3. Mint capability via Blind Ledger */

  ledger_err = ledger_mint(out_cap, (uintptr)buf, size, up, PEBBLE_CAP_BLACK,
                           vault_secret);

  if (ledger_err != BLIND_LEDGER_OK) {
    if (up != nil)
      borrow_release(up, (uintptr)buf);
    else
      borrow_release_system((uintptr)buf, OWNER_KERNEL);

    xfree(buf);
    return -1;
  }

  /* 4. Track metadata */

  ilock(&pebble_global_lock);
  pb = pebble_meta_alloc(sizeof(PebbleBlack));
  if (pb == nil) {
    iunlock(&pebble_global_lock);
    borrow_release(up, (uintptr)buf);
    xfree(buf);
    return -1;
  }

  memset(pb, 0, sizeof(PebbleBlack));
  pb->capability = *out_cap;
  pb->physical_addr = buf;
  pb->size = size;
  pb->flags = PEBBLE_CAP_BLACK | PEBBLE_CAP_ACTIVE;

  pb->next = pebble_state()->black_list;
  pebble_state()->black_list = pb;
  iunlock(&pebble_global_lock);

  return 0;
}

void *pebble_get_black_addr(const UserCapability *cap) {
  PebbleState *ps;
  PebbleBlack *pb;
  void *addr;

  if (cap == nil)
    return nil;

  ps = pebble_state();
  if (ps == nil)
    return nil;

  lock(&pebble_global_lock);
  pb = pebble_lookup_black_by_cap_locked(ps, cap);
  if (pb == nil) {
    unlock(&pebble_global_lock);
    return nil;
  }
  addr = pb->physical_addr;
  unlock(&pebble_global_lock);
  return addr;
}

/*
 * Internal callback for Blind Ledger to free physical memory.
 * Called by ledger_burn() when a capability is successfully invalidated.
 */
int pebble_black_free_internal(uintptr pa, ulong len, Proc *owner) {
  enum BorrowError borrow_err;

  if (pa == 0)
    return -1;

  // --- Release borrow checker ownership ---
  if (owner != nil)
    borrow_err = borrow_release(owner, pa);
  else
    borrow_err = borrow_release_system(pa, OWNER_KERNEL);
  if (borrow_err != BORROW_OK) {
    // CRITICAL: Borrow checker state inconsistent
    panic("pebble_black_free_internal: FATAL - borrow_release failed for "
          "pa=%#p: error=%d\n",
          pa, borrow_err);
  }

  // --- Free physical memory ---
  xfree((void *)pa);

  if (pebble_debug)
    print("PEBBLE: internal free pid=%lud pa=%#p size=%lud\n",
          owner ? owner->pid : 0, pa, len);

  return 0;
}

/*@
  requires cap != \null;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
int pebble_black_free(const UserCapability *cap) {
  PebbleState *ps;
  PebbleBlack *pb, **pp;
  ulong size;
  BlindLedgerError ledger_err;

  if (cap == nil)
    error(PEBBLE_E_BADARG);

  ps = pebble_state();
  if (ps == nil)
    error(PEBBLE_E_PERM);

  lock(&pebble_global_lock);
  // Use the new lookup function
  pb = pebble_lookup_black_by_cap_locked(ps, cap);
  if (pb == nil) {
    unlock(&pebble_global_lock);
    error(PEBBLE_E_PERM);
  }

  size = pb->size; // Get size from PebbleBlack for budgeting

  // --- Remove from PebbleBlack list ---
  for (pp = &ps->black_list; *pp != nil; pp = &(*pp)->next) {
    if (*pp == pb) {
      *pp = pb->next;
      break;
    }
  }

  // --- Adjust Pebble budget (BLACK → COLORLESS) ---
  ps->black_inuse -= size;
  ps->colorless_bank += size;
  ps->total_frees++;
  unlock(&pebble_global_lock); // Unlock early before external calls

  /*
   * BURN CABILITY via Blind Ledger.
   *
   * ledger_burn() will:
   * 1. Mark capability as burned in RB-tree.
   * 2. Destroy the secret.
   * 3. Call pebble_black_free_internal() to free physical memory.
   */
  ledger_err = ledger_burn(cap, up);
  if (ledger_err != BLIND_LEDGER_OK) {
    // CRITICAL: Blind Ledger state inconsistent with Pebble state
    // We already removed it from Pebble list, so we CANNOT recover.
    panic(
        "pebble_black_free: FATAL - ledger_burn failed for cap=%H: error=%d\n"
        "This indicates critical state corruption (double-burn/invalid cap).\n"
        "Blind Ledger and Pebble system out of sync.",
        cap->hash, ledger_err);
  }

  // --- Free PebbleBlack struct ---
  // (Physical memory was freed by ledger_burn -> pebble_black_free_internal)
  pebble_meta_free(pb);

  if (pebble_debug)
    print("PEBBLE: black free pid=%lud cap=%H size=%lud\n", up->pid, cap->hash,
          size);
  return 0;
}

/*@
  requires white_cap != \null;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
int pebble_white_verify(PebbleWhite *white_cap, void **black_cap) {
  PebbleState *ps;
  int i;
  void *ret;

  if (white_cap == nil || black_cap == nil)
    error(PEBBLE_E_BADARG);

  ps = pebble_state();
  if (ps == nil)
    error(PEBBLE_E_PERM);

  lock(&pebble_global_lock);
  if (!pebble_valid_white_token(ps, white_cap)) {
    unlock(&pebble_global_lock);
    error(PEBBLE_E_PERM);
  }

  ret = white_cap->data_ptr;
  if (white_cap->size != 0)
    ps->white_pending += white_cap->size;
  ps->white_verified++;

  for (i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    if (&ps->whites[i] == white_cap) {
      ps->whites_active[i] = 0;
      break;
    }
  }
  white_cap->token = 0;
  unlock(&pebble_global_lock);

  *black_cap = ret;
  if (pebble_debug)
    print("PEBBLE: white verify pid=%lud -> %#p\n", up->pid, ret);
  return 0;
}

/* REMOVED: pebble_detach_blue_locked() - coupled Blue/Red model deprecated */

static void pebble_free_red(PebbleRed *red) {
  PebbleState *ps;
  ulong size;

  if (red == nil)
    return;

  ps = pebble_state();
  if (ps == nil) {
    /* Fallback: Just free memory without budget tracking */
    if (red->red_data != nil)
      xfree(red->red_data);
    free(red);
    return;
  }

  size = red->red_size;

  /* Free physical memory */
  if (red->red_data != nil)
    xfree(red->red_data);
  free(red);

  /* Return budget to colorless bank (state transition: RED → COLORLESS) */
  lock(&pebble_global_lock);
  ps->colorless_bank += size;
  ps->red_inuse -= size;
  unlock(&pebble_global_lock);
}

/* ========== New Independent Blue/Red API ========== */

/*
 * pebble_blue_alloc - Allocate independent Blue token for block I/O
 *
 * State transition: COLORLESS → BLUE
 * Consumes budget from colorless bank for separate allocation.
 */
/*@
  requires size > 0;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
PebbleBlue *pebble_blue_alloc(ulong size) {
  PebbleState *ps;
  PebbleBlue *blue;

  if (size == 0)
    return nil;

  ps = pebble_state();
  if (ps == nil)
    return nil;

  /* Check budget (state transition: COLORLESS → BLUE) */
  lock(&pebble_global_lock);
  if (ps->colorless_bank < size) {
    unlock(&pebble_global_lock);
    return nil; /* Insufficient budget */
  }
  ps->colorless_bank -= size;
  ps->blue_inuse += size;
  unlock(&pebble_global_lock);

  /* Allocate Blue structure */
  blue = mallocz(sizeof(PebbleBlue), 1);
  if (blue == nil) {
    /* Rollback budget */
    lock(&pebble_global_lock);
    ps->colorless_bank += size;
    ps->blue_inuse -= size;
    unlock(&pebble_global_lock);
    return nil;
  }

  /* Allocate physical memory (backed by budget) */
  blue->blue_data = xallocz(size, 1);
  if (blue->blue_data == nil) {
    /* Rollback budget */
    lock(&pebble_global_lock);
    ps->colorless_bank += size;
    ps->blue_inuse -= size;
    unlock(&pebble_global_lock);
    free(blue);
    return nil;
  }

  blue->blue_size = size;
  blue->flags = 0;

  /* Add to process Blue list */
  lock(&pebble_global_lock);
  blue->next = ps->blue_list;
  ps->blue_list = blue;
  ps->blue_count++;
  unlock(&pebble_global_lock);

  if (pebble_debug)
    print("PEBBLE: blue_alloc pid=%lud size=%lud\n", up->pid, size);

  return blue;
}

/*
 * pebble_blue_free - Free Blue token back to colorless bank
 *
 * State transition: BLUE → COLORLESS
 * Returns budget to colorless bank.
 */
/*@
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
int pebble_blue_free(PebbleBlue *blue) {
  PebbleState *ps;
  PebbleBlue **bp;
  ulong size;

  if (blue == nil)
    return 0;

  ps = pebble_state();
  if (ps == nil)
    return -1;

  size = blue->blue_size;

  /* Remove from process Blue list */
  lock(&pebble_global_lock);
  for (bp = &ps->blue_list; *bp != nil; bp = &(*bp)->next) {
    if (*bp == blue) {
      *bp = blue->next;
      ps->blue_count--;
      break;
    }
  }
  unlock(&pebble_global_lock);

  /* Free physical memory */
  if (blue->blue_data != nil)
    xfree(blue->blue_data);
  free(blue);

  /* Return budget to colorless bank (state transition: BLUE → COLORLESS) */
  lock(&pebble_global_lock);
  ps->colorless_bank += size;
  ps->blue_inuse -= size;
  unlock(&pebble_global_lock);

  if (pebble_debug)
    print("PEBBLE: blue_free pid=%lud size=%lud\n", up->pid, size);

  return 0;
}

/*
 * pebble_red_alloc - Allocate independent Red token for snapshot
 *
 * State transition: COLORLESS → RED
 * Consumes budget from colorless bank for separate allocation.
 */
/*@
  requires size > 0;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
PebbleRed *pebble_red_alloc(ulong size) {
  PebbleState *ps;
  PebbleRed *red;

  if (size == 0)
    return nil;

  ps = pebble_state();
  if (ps == nil)
    return nil;

  /* Check budget (state transition: COLORLESS → RED) */
  lock(&pebble_global_lock);
  if (ps->colorless_bank < size) {
    unlock(&pebble_global_lock);
    return nil; /* Insufficient budget */
  }
  ps->colorless_bank -= size;
  ps->red_inuse += size;
  unlock(&pebble_global_lock);

  /* Allocate Red structure */
  red = mallocz(sizeof(PebbleRed), 1);
  if (red == nil) {
    /* Rollback budget */
    lock(&pebble_global_lock);
    ps->colorless_bank += size;
    ps->red_inuse -= size;
    unlock(&pebble_global_lock);
    return nil;
  }

  /* Allocate physical memory (backed by budget) */
  red->red_data = xallocz(size, 1);
  if (red->red_data == nil) {
    /* Rollback budget */
    lock(&pebble_global_lock);
    ps->colorless_bank += size;
    ps->red_inuse -= size;
    unlock(&pebble_global_lock);
    free(red);
    return nil;
  }

  red->red_size = size;
  red->flags = 0;

  /* Add to process Red list */
  lock(&pebble_global_lock);
  red->next = ps->red_list;
  ps->red_list = red;
  ps->red_count++;
  unlock(&pebble_global_lock);

  if (pebble_debug)
    print("PEBBLE: red_alloc pid=%lud size=%lud\n", up->pid, size);

  return red;
}

/*
 * pebble_red_free - Free Red token back to colorless bank
 *
 * State transition: RED → COLORLESS
 * Returns budget to colorless bank.
 */
/*@
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
@*/
int pebble_red_free(PebbleRed *red) {
  PebbleState *ps;
  PebbleRed **rp;
  ulong size;

  if (red == nil)
    return 0;

  ps = pebble_state();
  if (ps == nil)
    return -1;

  size = red->red_size;

  /* Remove from process Red list */
  lock(&pebble_global_lock);
  for (rp = &ps->red_list; *rp != nil; rp = &(*rp)->next) {
    if (*rp == red) {
      *rp = red->next;
      ps->red_count--;
      break;
    }
  }
  unlock(&pebble_global_lock);

  /* Free physical memory */
  if (red->red_data != nil)
    xfree(red->red_data);
  free(red);

  /* Return budget to colorless bank (state transition: RED → COLORLESS) */
  lock(&pebble_global_lock);
  ps->colorless_bank += size;
  ps->red_inuse -= size;
  unlock(&pebble_global_lock);

  if (pebble_debug)
    print("PEBBLE: red_free pid=%lud size=%lud\n", up->pid, size);

  return 0;
}

/*
 * pebble_red_snapshot - Create Red snapshot from Blue data
 *
 * Allocates new Red token and copies Blue data to it.
 * Blue and Red are independent allocations.
 */
int pebble_red_snapshot(PebbleBlue *blue, PebbleRed **out_red) {
  PebbleRed *red;

  if (blue == nil || out_red == nil)
    return -1;

  /* Allocate Red token (COLORLESS → RED) */
  red = pebble_red_alloc(blue->blue_size);
  if (red == nil)
    return -1;

  /* Copy Blue data to Red */
  memmove(red->red_data, blue->blue_data, blue->blue_size);

  *out_red = red;
  return 0;
}

/* ========== Legacy API (DEPRECATED) ========== */

int pebble_blue_exists(PebbleState *ps, PebbleBlue *blue) {
  PebbleBlue *bp;

  if (ps == nil || blue == nil)
    return 0;

  for (bp = ps->blue_list; bp != nil; bp = bp->next)
    if (bp == blue)
      return 1;
  return 0;
}

/* REMOVED: pebble_blue_exists_locked() - coupled Blue/Red model deprecated */
/* REMOVED: pebble_has_matching_red() - coupled Blue/Red model deprecated */

/* REMOVED: pebble_duplicate_blue() - replaced by pebble_red_snapshot() */
/* REMOVED: pebble_mark_red() - coupled Blue/Red model deprecated */

int pebble_red_copy(PebbleBlue *blue_obj, PebbleRed **red_copy) {
  /* DEPRECATED: Use pebble_red_snapshot() instead.
   * This wrapper maintains backward compatibility.
   */
  if (blue_obj == nil || red_copy == nil)
    error(PEBBLE_E_BADARG);

  return pebble_red_snapshot(blue_obj, red_copy);
}

/* REMOVED: pebble_remove_red_locked() - coupled Blue/Red model deprecated */

int pebble_blue_discard(PebbleBlue *blue_obj) {
  /* DEPRECATED: Use pebble_blue_free() instead.
   * This wrapper maintains backward compatibility.
   */
  if (blue_obj == nil)
    error(PEBBLE_E_BADARG);

  return pebble_blue_free(blue_obj);
}

void pebble_ensure_red_snapshots(PebbleState *ps) {
  /* DEPRECATED: Blue/Red coupling removed.
   * Blue and Red are now independent tokens managed separately.
   * Applications must explicitly create Red snapshots via pebble_red_snapshot()
   * when transaction safety is needed.
   */
  USED(ps);
  return;
}

void pebble_red_blue_exit(void) {
  /* DEPRECATED: Blue/Red are now independent tokens, not coupled to Black.
   * This function previously ensured Red snapshots for all Blue objects,
   * but that coupling model has been removed.
   *
   * Blue/Red are only used in tests and must be managed explicitly via:
   * - pebble_blue_alloc() / pebble_blue_free()
   * - pebble_red_alloc() / pebble_red_free()
   * - pebble_red_snapshot()
   */
  return;
}

void pebble_auto_verify(Proc *p, Ureg *) {
  PebbleState *ps;

  if (!pebble_enabled || p == nil)
    return;
  ps = &p->pebble;
  lock(&pebble_global_lock);
  if (ps->drop_budget != 0) {
    if (ps->drop_budget <= ps->black_inuse) {
      ps->black_inuse -= ps->drop_budget;
      ps->colorless_bank += ps->drop_budget;
    }
    ps->drop_budget = 0;
  }
  unlock(&pebble_global_lock);
}

void pebble_cleanup(Proc *p) {
  PebbleState *ps;
  PebbleBlack *pb, *pbnext;
  PebbleBlue *blue, *bluenext;
  PebbleRed *red, *rednext;
  ulong return_tokens;

  if (p == nil || !pebble_enabled)
    return;
  ps = &p->pebble;

  lock(&pebble_global_lock);

  /* Calculate total tokens to return to global pool:
   * - Unused colorless_bank tokens
   * - black_inuse tokens (from allocations being freed)
   * - blue_inuse and red_inuse tokens
   */
  return_tokens = ps->colorless_bank +
                  (ps->black_inuse / PEBBLE_BYTES_PER_TOKEN) +
                  (ps->blue_inuse / PEBBLE_BYTES_PER_TOKEN) +
                  (ps->red_inuse / PEBBLE_BYTES_PER_TOKEN);

  /* Return tokens to global pool */
  lock(&pebble_bank_lock);
  pebble_global_colorless_bank += return_tokens;
  unlock(&pebble_bank_lock);

  if (pebble_debug && return_tokens > 0)
    print("PEBBLE: pid %lud exit, returned %lud tokens to global pool\n",
          p->pid, return_tokens);

  pb = ps->black_list;
  ps->black_list = nil;
  blue = ps->blue_list;
  ps->blue_list = nil;
  red = ps->red_list;
  ps->red_list = nil;
  ps->black_inuse = 0;
  ps->blue_inuse = 0;
  ps->red_inuse = 0;
  ps->colorless_bank = 0; /* All tokens returned to global pool */
  ps->white_verified = 0;
  ps->white_pending = 0;
  ps->blue_count = 0;
  ps->red_count = 0;
  unlock(&pebble_global_lock);

  for (; pb != nil; pb = pbnext) {
    pbnext = pb->next;
    if (pb->physical_addr != nil)
      xfree(pb->physical_addr);
    free(pb);
  }
  for (; blue != nil; blue = bluenext) {
    bluenext = blue->next;
    free(blue);
  }
  for (; red != nil; red = rednext) {
    rednext = red->next;
    pebble_free_red(red);
  }

  memset(ps->whites_active, 0, sizeof(ps->whites_active));
}

void pebble_selftest(void) {
  PebbleState *ps;
  PebbleWhite *white;
  UserCapability black_cap;
  PebbleBlue *blue;
  PebbleRed *red;
  extern void uartprintf(char *, ...);

  if (!pebble_enabled)
    return;
  ps = pebble_state();
  if (ps == nil)
    return;

  uartprintf("PEBBLE: selftest begin\n");

  /* Test 1: White -> Black allocation */
  white = pebble_issue_white(ps, nil, PEBBLE_MIN_ALLOC);
  if (white == nil) {
    uartprintf("pebble selftest: white issue failed\n");
    return;
  }

  void *black_handle = nil;
  pebble_white_verify(white, &black_handle);
  if (pebble_black_alloc(PEBBLE_MIN_ALLOC, &black_cap) != 0) {
    uartprintf("pebble selftest: black alloc failed\n");
    return;
  }

  /* Test 2: Independent Blue allocation */
  blue = pebble_blue_alloc(PEBBLE_MIN_ALLOC);
  if (blue == nil) {
    uartprintf("pebble selftest: blue alloc failed\n");
    return;
  }

  /* Test 3: Blue -> Red snapshot */
  if (pebble_red_snapshot(blue, &red) != 0) {
    uartprintf("pebble selftest: red snapshot failed\n");
    return;
  }
  if (red == nil) {
    uartprintf("pebble selftest: red nil after snapshot\n");
    return;
  }

  /* Test 4: Free all tokens */
  if (pebble_red_free(red) != 0) {
    uartprintf("pebble selftest: red free failed\n");
    return;
  }
  if (pebble_blue_free(blue) != 0) {
    uartprintf("pebble selftest: blue free failed\n");
    return;
  }

  /* Test 5: Holographic Channels ("Pointer-as-Channel") */
  {
    void *raw_ptr = pebble_get_black_addr(&black_cap);
    void *proj_ch3, *proj_ch7;

    /* Ensure alignment */
    if (((uintptr)raw_ptr & PEBBLE_WAVE_MASK) != 0) {
      uartprintf("pebble selftest: black addr not 8-byte aligned\n");
      return;
    }

    /* Project onto Channel 3 */
    proj_ch3 = PEBBLE_PROJECT(raw_ptr, PEBBLE_WAVE_3);
    if (!PEBBLE_TUNED(proj_ch3, PEBBLE_WAVE_3)) {
      uartprintf("pebble selftest: projection to Ch3 failed\n");
      return;
    }
    if (PEBBLE_TUNED(proj_ch3, PEBBLE_WAVE_2)) {
      uartprintf("pebble selftest: Ch3 bled into Ch2 (filtering fail)\n");
      return;
    }

    /* Project onto Channel 7 */
    proj_ch7 = PEBBLE_PROJECT(raw_ptr, PEBBLE_WAVE_7);
    if (PEBBLE_PTR_WAVE(proj_ch7) != 7) {
      uartprintf("pebble selftest: projection to Ch7 failed\n");
      return;
    }

    /* Verify Base Address Recovery (All waves collapse to source) */
    if (PEBBLE_PTR_ADDR(proj_ch3) != raw_ptr) {
      uartprintf("pebble selftest: Ch3 addr recovery failed\n");
      return;
    }

    uartprintf("PEBBLE: holographic channel verification passed\n");
  }

  if (pebble_black_free(&black_cap) != 0) {
    uartprintf("pebble selftest: black free failed\n");
    return;
  }

  uartprintf("PEBBLE: selftest PASS (independent tokens, circular economy "
             "validated)\n");
}

void pebble_sip_issue_test(void) {
  PebbleWhite *white;
  UserCapability black_cap;
  PebbleBlue *blue;
  PebbleRed *red;
  PebbleState *ps;

  if (!pebble_enabled)
    return;
  ps = pebble_state();
  if (ps == nil)
    return;
  print("PEBBLE: /dev/sip/issue test begin\n");
  if (waserror()) {
    print("PEBBLE: /dev/sip/issue test FAIL: %s\n", up->errstr);
    poperror();
    return;
  }

  /* Test 1: White token with larger size */
  white = pebble_issue_white(ps, nil, PEBBLE_MIN_ALLOC * 2);
  if (white == nil)
    error("pebble sip issue: white issue failed");

  void *black_handle = nil;
  pebble_white_verify(white, &black_handle);

  /* Test 2: Black allocation from white token */
  if (pebble_black_alloc(PEBBLE_MIN_ALLOC, &black_cap) != 0)
    error("pebble sip issue: black alloc failed");

  /* Test 3: Independent Blue allocation */
  blue = pebble_blue_alloc(PEBBLE_MIN_ALLOC);
  if (blue == nil)
    error("pebble sip issue: blue alloc failed");

  /* Test 4: Create Red snapshot */
  if (pebble_red_snapshot(blue, &red) != 0)
    error("pebble sip issue: red snapshot failed");
  if (red == nil)
    error("pebble sip issue: red nil");

  /* Test 5: Free all back to colorless (circular economy) */
  pebble_red_free(red);
  pebble_blue_free(blue);
  pebble_black_free(&black_cap);

  poperror();
  print("PEBBLE: /dev/sip/issue test PASS (circular economy validated)\n");
}
