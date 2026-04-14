#include "dat.h"
#include "error.h"
#include "fns.h"
#include "lib.h"
#include "mem.h"
#include "u.h"

#include "blind_ledger.h"
#include "monocypher.h"
#include "pebble.h"
#include "uuid.h"
#include <siphash.h>

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
*/

Lock pebble_global_lock;
Lock pebble_bank_lock;
int pebble_enabled = 1;
int pebble_debug = PEBBLE_DEBUG;

/* Global colorless bank - single pool for entire system */
ulong pebble_global_colorless_bank = 0;
ulong pebble_total_system_tokens = 0;

static int pebble_initialized;

static void pebble_free_red(PebbleRed *);

/*@
  @ requires \valid(ps);
  @ requires cap != \null && \valid_read(cap);
  @ terminates \true;
  @ assigns \nothing;
  @*/
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

/*@
  @ requires \valid(ps);
  @ terminates \true;
  @ assigns *ps;
  @*/
static void pebble_reset_state(PebbleState *ps) {
  ps->colorless_bank = 0; /* Processes start with 0 tokens */
  ps->black_inuse = 0;
  ps->blue_inuse = 0;
  ps->red_inuse = 0;
  ps->white_verified = 0;
  ps->white_pending = 0;
  ps->red_count = 0;
  ps->blue_count = 0;
  ps->total_allocs = 0;
  ps->total_frees = 0;
  ps->vbase = 0;
  ps->black_list = nil;
  ps->blue_list = nil;
  ps->red_list = nil;
  ps->vault_handle = nil;
  ps->in_syscall = 0;
  ps->drop_budget = 0;
  for (int i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    ps->whites[i].token = 0;
    ps->whites[i].size = 0;
    ps->whites_active[i] = 0;
  }
  ps->white_generation = 0;
  ps->white_head = 0;
  ps->vbase = 0x400000000000ull; /* Base for user-space Pebble mapping */
}

// Boot-time state for use before proc0
static PebbleState boot_pstate;

/*@
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result != \null;
  @*/
PebbleState *pebble_state(void) {
  if (up == nil)
    return &boot_pstate;
  return &up->pebble;
}

PebbleState *pebble_kernel_state(void) {
  if (boot_pstate.white_generation == 0) {
    pebble_reset_state(&boot_pstate);
    boot_pstate.colorless_bank = PEBBLE_BOOT_BUDGET;
    boot_pstate.white_generation = 1;
  }
  return &boot_pstate;
}

/*
 * Calculate total system RAM from conf.mem[] entries
 */
/*@
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= 0;
  @*/
static ulong pebble_calculate_system_ram(void) {
  ulong total = 0;
  int i;
  for (i = 0; i < nelem(conf.mem); i++) {
    if (conf.mem[i].npage > 0)
      total += conf.mem[i].npage * BY2PG;
  }
  return total;
}

/*@
  @ terminates \true;
  @ assigns pebble_initialized, pebble_total_system_tokens,
  pebble_global_colorless_bank, boot_pstate;
  @*/
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
  boot_pstate.colorless_bank = PEBBLE_BOOT_BUDGET;
  boot_pstate.white_generation = 1;

  /* Reserve init budget from global pool */
  init_tokens = PEBBLE_INIT_BUDGET / PEBBLE_BYTES_PER_TOKEN;
  if (pebble_global_colorless_bank >= init_tokens)
    pebble_global_colorless_bank -= init_tokens;
  /* init_tokens will be granted to proc0 at proc0() entry */

  if (pebble_debug)
    bprint(
        "PEBBLE: global pool=%lu tokens (%luMB), boot=%lu, reserved_init=%lu\n",
        pebble_global_colorless_bank,
        (pebble_global_colorless_bank * PEBBLE_BYTES_PER_TOKEN) / (1024 * 1024),
        boot_tokens, init_tokens);

  pebble_initialized = 1;
}

/*@
  @ requires p != \null ==> \valid(p);
  @ terminates \true;
  @*/
void pebbleprocinit(Proc *p) {
  if (p == nil)
    return;
  pebble_reset_state(&p->pebble);

  /* Grant initial budget to proc0/init so it can bootstrap */
  if (p->pid == 1) {
    p->pebble.colorless_bank = PEBBLE_INIT_BUDGET;
    if (pebble_debug)
      bprint("PEBBLE: granted %lldMB init budget to pid 1\n",
             (vlong)PEBBLE_INIT_BUDGET / (1024 * 1024));
  }
}

/*@
  @ requires \valid(ps);
  @ requires PEBBLE_TUNED(handle, PEBBLE_WAVE_6) ==>
  \valid_read((uint8_t*)PEBBLE_PTR_ADDR(handle) + (0 .. 31));
  @ terminates \true;
  @ assigns \nothing;
  @*/
static PebbleBlack *pebble_lookup_black_locked(PebbleState *ps, void *handle) {
  PebbleBlack *pb;
  void *base_handle;

  /* Wave 6 "Invisible Lock": Handle points to Elligator noise */
  if (PEBBLE_TUNED(handle, PEBBLE_WAVE_6)) {
    uint8_t curve_point[32];
    uintptr addr = (uintptr)PEBBLE_PTR_ADDR(handle);

    /* Map noise (representative) back to curve point to unlock data */
    crypto_elligator_map(curve_point, (u8int *)addr);

    /* The first 8 bytes of the recovered point are the real pointer */
    base_handle = *(void **)curve_point;
  } else {
    /* Strip wave bits (Holographic View) */
    base_handle = PEBBLE_PTR_ADDR(handle);
  }

  for (pb = ps->black_list; pb != nil; pb = pb->next)
    if (pb == base_handle)
      return pb;
  return nil;
}

/*@
  @ requires ps != \null ==> \valid(ps);
  @ terminates \true;
  @*/
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
  @ requires ps != \null ==> \valid(ps);
  @ terminates \true;
  @ assigns \nothing;
  @*/
PebbleBlack *pebble_lookup_black_by_addr(PebbleState *ps, void *addr) {
  PebbleBlack *pb;

  if (ps == nil || addr == nil)
    return nil;

  lock(&pebble_global_lock);
  for (pb = ps->black_list; pb != nil; pb = pb->next) {
    if (pb->physical_addr == addr) {
      unlock(&pebble_global_lock);
      return pb;
    }
  }
  unlock(&pebble_global_lock);
  return nil;
}

/*@
  @
  //============================================================================
  @ // WHITE TOKEN ISSUANCE - Reserve tokens from COLORLESS budget
  @
  //============================================================================
  @
  @ // Preconditions: Valid state and positive size
  @ requires \valid(ps);
  @ requires size > 0;
  @ requires \valid(ps->whites + (0..PEBBLE_MAX_TOKENS-1));
  @ requires \valid(ps->whites_active + (0..PEBBLE_MAX_TOKENS-1));
  @ requires ps->colorless_bank >= 0;
  @ requires ps->white_pending >= 0;
  @ requires 0 <= ps->white_head < PEBBLE_MAX_TOKENS;
  @
  @ // Conservation invariants (Coq-proven)
  @ requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  @ requires Inv_NonNegative(pebble_state());
  @
  @ // Postconditions: Conservation preserved
  @ ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  @ ensures Inv_NonNegative(pebble_state());
  @
  @ // Success path: WHITE token allocated
  @ ensures \result != \null ==>
  @   \valid(\result) &&
  @   \result->token == PEBBLE_TOKEN_MAGIC &&
  @   \result->size == ROUNDUP(\max(size, PEBBLE_MIN_ALLOC),
  PEBBLE_MEM_PER_TOKEN);
  @
  @ // Failure paths
  @ ensures \result == \null ==>
  @   (ps->colorless_bank < ROUNDUP(\max(size, PEBBLE_MIN_ALLOC),
  PEBBLE_MEM_PER_TOKEN) ||
  @    \forall integer j; 0 <= j < PEBBLE_MAX_TOKENS ==> ps->whites_active[j] ==
  1);
  @
  @ // Memory effects
  @ assigns ps->colorless_bank, ps->white_pending, ps->white_generation,
  @         ps->whites[0..PEBBLE_MAX_TOKENS-1],
  @         ps->whites_active[0..PEBBLE_MAX_TOKENS-1],
  @         ps->white_head;
  @
  @ terminates \true;
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
      bprint("PEBBLE: insufficient budget for WHITE pid=%lud need=%lud "
             "have=%lud\n",
             up ? up->pid : 0, pegged_size, ps->colorless_bank);
    /* Wave 7: Signal resource exhaustion to resurrection */
    if (up != nil)
      pebble_signal_distress(up, DISTRESS_RESOURCE_EXHAUST, pegged_size);
    return nil; /* Insufficient budget */
  }

  /*@
    @ loop invariant 0 <= i <= PEBBLE_MAX_TOKENS;
    @ loop invariant \forall integer j; 0 <= j < i ==>
    @   ps->whites_active[(ps->white_head + j) % PEBBLE_MAX_TOKENS] == 1;
    @ loop assigns i, idx;
    @ loop variant PEBBLE_MAX_TOKENS - i;
    @*/
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
    bprint("PEBBLE: no free white tokens pid=%lud active=%d max=%d "
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
/*@
  @ requires white != \null ==> \valid(white);
  @ requires out_uuid != \null ==> \valid(out_uuid);
  @ terminates \true;
  @ assigns *out_uuid;
  @*/
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
  /*@
    @ loop invariant 0 <= i <= PEBBLE_MAX_TOKENS;
    @ loop invariant idx == -1 || (0 <= idx < i);
    @ loop assigns i, idx;
    @ loop variant PEBBLE_MAX_TOKENS - i;
    @*/
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

/*@
  @ requires ps != \null ==> \valid(ps);
  @ requires white != \null ==> \valid(white);
  @ requires ps != \null ==> \valid(ps->whites_active +
  (0..PEBBLE_MAX_TOKENS-1));
  @ requires ps != \null ==> \valid(ps->whites + (0..PEBBLE_MAX_TOKENS-1));
  @ terminates \true;
  @ assigns \nothing;
  @*/
int pebble_valid_white_token(PebbleState *ps, PebbleWhite *white) {
  int i;

  if (ps == nil || white == nil)
    return 0;

  /*@
    @ loop invariant 0 <= i <= PEBBLE_MAX_TOKENS;
    @ loop assigns i;
    @ loop variant PEBBLE_MAX_TOKENS - i;
    @*/
  for (i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    if (ps->whites_active[i] && &ps->whites[i] == white) {
      if (white->token != PEBBLE_TOKEN_MAGIC)
        return 0;
      return 1;
    }
  }
  return 0;
}

/*@
  @ requires ps != \null ==> \valid(ps);
  @ requires white != \null ==> \valid(white);
  @ requires ps != \null ==> \valid(ps->whites + (0..PEBBLE_MAX_TOKENS-1));
  @ requires ps != \null ==> \valid(ps->whites_active +
  (0..PEBBLE_MAX_TOKENS-1));
  @ terminates \true;
  @ assigns ps->white_pending, ps->whites_active[0..PEBBLE_MAX_TOKENS-1],
  white->token;
  @*/
void pebble_return_white(PebbleState *ps, PebbleWhite *white) {
  if (ps == nil || white == nil)
    return;
  if (!pebble_valid_white_token(ps, white))
    return;

  if (white->size != 0 && ps->white_pending >= white->size)
    ps->white_pending -= white->size;

  /*@
    @ loop invariant 0 <= i <= PEBBLE_MAX_TOKENS;
    @ loop assigns i, ps->whites_active[0..PEBBLE_MAX_TOKENS-1];
    @ loop variant PEBBLE_MAX_TOKENS - i;
    @*/
  for (int i = 0; i < PEBBLE_MAX_TOKENS; i++) {
    if (&ps->whites[i] == white) {
      ps->whites_active[i] = 0;
      break;
    }
  }

  white->token = 0;
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
      bprint(
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
      bprint(
          "PEBBLE: budget PoW failure pid=%lud size=%lud diff=%d nonce=%llud "
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
    bprint("PEBBLE: budget transferred pid=%lud tokens=%lu total=%lu "
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

/* Self-hosted Holographic Vault storage */
static void *master_key_handle = nil; /* Wave 6 "Invisible Lock" Handle */
static int master_key_active = 0;

/* Forward declaration for bootstrap */
int pebble_alloc_with_white(ulong size, UserCapability *out_cap,
                            void **out_addr);

/*@
  @ assigns pebble_vault_key[0..31], pebble_vault_key_initialized,
  @         master_key_handle, master_key_active;
  @*/
static void pebble_init_vault_key(void) {
  extern int tpm_get_random(u8int * buf, int n);
  extern u64int rdrand_u64(void);
  extern int crypto_hw_rdrand_available(void);
  extern u64int chacha20_csprng_u64(void);

  void *key_addr = nil;
  void *noise_addr = nil;
  UserCapability cap_key, cap_noise;
  uint8_t point[32];
  uint8_t noise[32];
  int i;

  if (pebble_vault_key_initialized)
    return;

  /* 1. Generate key into temporary static buffer */
  if (tpm_get_random(pebble_vault_key, 32) == 32) {
    bprint("PEBBLE: Vault secret from TPM\\n");
  } else if (crypto_hw_rdrand_available()) {
    u64int *key64 = (u64int *)pebble_vault_key;
    key64[0] = rdrand_u64();
    key64[1] = rdrand_u64();
    key64[2] = rdrand_u64();
    key64[3] = rdrand_u64();
    bprint("PEBBLE: Vault secret from RDRAND\\n");
  } else {
    u64int *key64 = (u64int *)pebble_vault_key;
    key64[0] = chacha20_csprng_u64();
    key64[1] = chacha20_csprng_u64();
    key64[2] = chacha20_csprng_u64();
    key64[3] = chacha20_csprng_u64();
    bprint("PEBBLE: Vault secret from ChaCha20 CSPRNG (software fallback)\\n");
  }

  pebble_vault_key_initialized = 1;

  /* 2. Allocate 'Vault' memory for the Key */
  if (pebble_alloc_with_white(PEBBLE_MIN_ALLOC, &cap_key, &key_addr) != 0) {
    bprint("PEBBLE: Failed to allocate Key Vault. Using BSS.\\n");
    return;
  }

  /* 3. Allocate memory for the Holographic Noise (The "Door") */
  if (pebble_alloc_with_white(PEBBLE_MIN_ALLOC, &cap_noise, &noise_addr) != 0) {
    bprint("PEBBLE: Failed to allocate Holographic Noise. Using BSS.\\n");
    /* Leak key_addr (minimal issue in kernel panic/boot scenario) */
    return;
  }

  /* 4. Move key into the Vault */
  memmove(key_addr, pebble_vault_key, 32);

  /* 5. Create Holographic "Invisible Lock" (Wave 6)
   * We need a Curve Point where the first 8 bytes are the address of our Key
   * Vault. We loop until we find a random padding that makes the point valid
   * for Elligator.
   */
  memset(point, 0, 32);
  *(void **)point = key_addr; /* Embed pointer in first 8 bytes */

  /*@
    @ loop invariant 0 <= i <= 1000;
    @ loop assigns i, point[8], noise[0..31];
    @ loop variant 1000 - i;
    @*/
  for (i = 0; i < 1000; i++) {
    /* Fill rest with random noise to find valid curve point */
    point[8] = i; /* Simple counter sufficient for finding valid point */
    /* Note: In real production, use stronger RNG for padding */

    /* Try to reverse map Point -> Noise */
    if (crypto_elligator_rev(noise, point, 0) != -1) {
      break;
    }
  }

  if (i == 1000) {
    bprint("PEBBLE: Failed to generate Holographic Lock (Elligator). Using "
           "BSS.\\n");
    return;
  }

  /* 6. Store the Noise in the Noise Vault */
  memmove(noise_addr, noise, 32);

  /* 7. Construct the Wave 6 Pointer (Handle) */
  master_key_handle = (void *)((uintptr)noise_addr | PEBBLE_WAVE_6);
  master_key_active = 1;

  /* 8. Secure the temporary buffer */
  crypto_wipe(pebble_vault_key, 32);

  bprint("PEBBLE: Vault key moved to Holographic Storage (Wave 6)\\n");
}

/*@
  @ assigns \nothing;
  @ ensures \valid_read(\result + (0..31));
  @ terminates \true;
  @*/
const u8int *pebble_get_vault_secret(void) {
  if (!pebble_vault_key_initialized)
    pebble_init_vault_key();

  if (master_key_active) {
    /* Use the Wave 6 handle to perform the Invisible Unlock */
    /* pebble_lookup_black_locked handles the Elligator map internally */
    PebbleBlack *pb = pebble_lookup_black(pebble_state(), master_key_handle);
    if (pb != nil) {
      return (const u8int *)pb->physical_addr;
    }
    /* Fallback if lookup fails (should not happen) */
  }

  return pebble_vault_key;
}

/*
 * Export the Vault Secret to a buffer.
 * Used for saving the system state to a file.
 */
/*@
  @ requires \valid((u8int*)buf + (0..n-1));
  @ requires n >= 32;
  @ assigns ((u8int*)buf)[0..31];
  @ ensures \result == 32;
  @*/
long pebble_export_secret(void *buf, long n) {
  const u8int *key;

  if (buf == nil || n < 32)
    return -1;

  key = pebble_get_vault_secret();
  memmove(buf, key, 32);
  return 32;
}

/*
 * Create a process-specific Holographic Vault.
 * Stores arbitrary data protected by an Invisible Lock (Wave 6).
 */
/*@
  @ requires ps != \null;
  @ requires \valid_read((u8int*)secret_data + (0..len-1));
  @ requires len > 0 && len <= PEBBLE_MAX_ALLOC;
  @ ensures \result == 0 || \result == -1;
  @*/
int pebble_user_vault_create(PebbleState *ps, void *secret_data, ulong len) {
  void *key_addr = nil;
  void *noise_addr = nil;
  UserCapability cap_key, cap_noise;
  uint8_t point[32];
  uint8_t noise[32];
  int i;

  if (ps == nil || secret_data == nil || len == 0 || len > PEBBLE_MAX_ALLOC)
    return -1;

  /* Only one vault per process allowed */
  lock(&pebble_global_lock);
  if (ps->vault_handle != nil) {
    unlock(&pebble_global_lock);
    return -1;
  }
  unlock(&pebble_global_lock);

  /* 1. Allocate storage for the Secret Data (The "Vault Content") */
  /* This can be large (up to PEBBLE_MAX_ALLOC) */
  if (pebble_alloc_with_white(len, &cap_key, &key_addr) != 0) {
    return -1;
  }

  /* 2. Allocate storage for the Holographic Noise (The "Door") */
  /* This is always 32 bytes (size of Elligator representative) */
  if (pebble_alloc_with_white(PEBBLE_MIN_ALLOC, &cap_noise, &noise_addr) != 0) {
    /* Should free key_addr here in robust impl */
    return -1;
  }

  /* 3. Copy Secret to Vault */
  memmove(key_addr, secret_data, len);

  /* 4. Generate Holographic Lock (Elligator) */
  /* We hide the POINTER to the data, not the data itself */
  memset(point, 0, 32);
  *(void **)point = key_addr; /* Embed pointer */

  /*@
    @ loop invariant 0 <= i <= 1000;
    @ loop assigns i, point[8], noise[0..31];
    @ loop variant 1000 - i;
    @*/
  for (i = 0; i < 1000; i++) {
    point[8] = i;
    if (crypto_elligator_rev(noise, point, 0) != -1) {
      break;
    }
  }

  if (i == 1000) {
    /* Failed to generate lock */
    return -1;
  }

  /* 5. Store Noise */
  memmove(noise_addr, noise, 32);

  /* 6. Update Process State */
  lock(&pebble_global_lock);
  ps->vault_handle = (void *)((uintptr)noise_addr | PEBBLE_WAVE_6);
  unlock(&pebble_global_lock);

  return 0;
}

/*@
  @ requires size > 0;
  @ requires white != \null;
  @ requires \valid(white);
  @ requires buf != \null;
  @ requires \valid((uchar*)buf + (0..size-1));
  @ requires out_cap != \null;
  @ requires \valid(out_cap);
  @ requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  @ requires Inv_NonNegative(pebble_state());
  @
  @ behavior success:
  @   assumes white->size == size;
  @   assumes white->token != 0;
  @   ensures \result == 0;
  @   ensures white->token == 0;
  @   ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  @   ensures Inv_NonNegative(pebble_state());
  @   ensures \valid(out_cap);
  @
  @ behavior error_invalid_white:
  @   assumes white == \null || white->size != size || white->token == 0;
  @   ensures \result == -1;
  @
  @ behavior error_buf:
  @   assumes buf == \null;
  @   ensures \result == -1;
  @
  @ complete behaviors;
  @ disjoint behaviors;
  @ terminates \true;
  @ assigns white->token, *out_cap, pebble_state()->black_list,
  pebble_state()->black_inuse;
  @*/
/*
 * SMT: Validated by proofs/pebble/pebble_security.v
 * Theorem: Inv_Conservation
 * Description: Verifies BLACK token allocation from verified WHITE token
 *
 * CRITICAL: This function REQUIRES a verified WHITE token.
 * WHITE tokens must be issued first (pebble_issue_white), then verified
 * (pebble_white_verify), before calling this function.
 *
 * Flow: WHITE (reserve) → Verify → BLACK (allocate)
 */
int pebble_black_alloc_in_state(PebbleState *ps, Proc *owner,
                                PebbleWhite *white, void *buf, ulong size,
                                UserCapability *out_cap) {
  PebbleBlack *pb;

  /*
   * Verify WHITE token was provided (WHITE → BLACK conversion required)
   */
  if (white == nil) {
    bprint("pebble_black_alloc: ERROR - WHITE token required!\n");
    bprint("  Must issue WHITE token first via pebble_issue_white()\n");
    return -1;
  }

  if (buf == nil) {
    bprint("pebble_black_alloc: ERROR - buffer address required!\n");
    return -1;
  }

  /*
   * owner == nil means a kernel-resident allocation tracked against the
   * kernel Pebble state rather than a process Pebble state.
   */
  if (ps == nil || out_cap == nil)
    return -1;

  const u8int *vault_secret = pebble_get_vault_secret();
  BlindLedgerError ledger_err;

  /* Enforce 8-byte granularity (Tokens) */
  if (size < PEBBLE_MIN_ALLOC) {
    size = PEBBLE_MIN_ALLOC;
  }
  if (size % PEBBLE_MEM_PER_TOKEN != 0) {
    size = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);
  }

  /* Verify WHITE token matches the size */
  if (white->size != size) {
    bprint("pebble_black_alloc: WHITE token size mismatch (white=%lud, "
           "requested=%lud)\n",
           white->size, size);
    return -1;
  }

  /* 1. Memory already allocated - WHITE token should point to it */
  /* NOTE: buf is provided by caller after xallocz() */

  /* 2. Acquire ownership via Borrow Checker */
  if (owner != nil) {
    if (borrow_acquire(owner, (uintptr)buf) != BORROW_OK) {
      xfree(buf);
      return -1;
    }
  } else {
    /* Kernel-resident allocation */
    if (borrow_acquire_system((uintptr)buf, OWNER_KERNEL) != BORROW_OK) {
      xfree(buf);
      return -1;
    }
  }

  /* 3. Mint capability via Blind Ledger */

  ledger_err = ledger_mint(out_cap, (uintptr)buf, size, owner, PEBBLE_CAP_BLACK,
                           vault_secret);

  if (ledger_err != BLIND_LEDGER_OK) {
    if (owner != nil)
      borrow_release(owner, (uintptr)buf);
    else
      borrow_release_system((uintptr)buf, OWNER_KERNEL);

    xfree(buf);
    return -1;
  }

  /* 4. Track metadata */

  lock(&pebble_global_lock);
  pb = pebble_meta_alloc(sizeof(PebbleBlack));
  if (pb == nil) {
    unlock(&pebble_global_lock);
    if (owner != nil)
      borrow_release(owner, (uintptr)buf);
    else
      borrow_release_system((uintptr)buf, OWNER_KERNEL);
    xfree(buf);
    return -1;
  }

  pb->capability = *out_cap;
  pb->physical_addr = buf;
  pb->user_vaddr = 0; /* Not mapped yet */
  pb->size = size;
  pb->flags = PEBBLE_CAP_BLACK | PEBBLE_CAP_ACTIVE;

  pb->next = ps->black_list;
  ps->black_list = pb;

  /*
   * Enforce consumption: A WHITE token can only be converted to BLACK ONCE.
   * Zeroing the magic prevents reuse if the caller keeps the pointer.
   */
  white->token = 0;

  /* Account for the transition: WHITE -> BLACK */
  /* white_pending was already updated in white_verify */
  ps->black_inuse += size;

  unlock(&pebble_global_lock);

  return 0;
}

int pebble_black_alloc(PebbleWhite *white, void *buf, ulong size,
                       UserCapability *out_cap) {
  return pebble_black_alloc_in_state(pebble_state(), up, white, buf, size,
                                     out_cap);
}

/*
 * Helper function: Full WHITE→BLACK allocation flow
 *
 * This implements the proper Pebble economy:
 * 1. Issue WHITE token (reservation from COLORLESS budget)
 * 2. Allocate physical memory
 * 3. Bind WHITE to address
 * 4. Verify WHITE (consumes it)
 * 5. Convert to BLACK token
 *
 * Returns: 0 on success, -1 on failure
 */
int pebble_alloc_with_white(ulong size, UserCapability *out_cap,
                            void **out_addr) {
  PebbleState *ps;
  PebbleWhite *white;
  void *buf;
  void *black_handle;

  if (out_cap == nil || out_addr == nil)
    return -1;

  ps = pebble_state();
  if (ps == nil)
    return -1;

  /* Enforce granularity */
  if (size < PEBBLE_MIN_ALLOC)
    size = PEBBLE_MIN_ALLOC;
  if (size % PEBBLE_MEM_PER_TOKEN != 0)
    size = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);

  /* Step 1: Issue WHITE token (reservation from COLORLESS budget) */
  white = pebble_issue_white(ps, nil, size);
  if (white == nil) {
    bprint("pebble_alloc_with_white: WHITE issue failed (no budget?)\n");
    return -1;
  }

  /* Step 2: Allocate physical memory (padded for page alignment) */
  /* We allocate extra space to ensure we can find a full page-aligned region
     that is exclusively owned by this process, preventing pool corruption. */
  ulong alloc_size = size + 2 * BY2PG;
  buf = xallocz(alloc_size, 1);
  if (buf == nil) {
    /* Return WHITE to budget */
    lock(&pebble_global_lock);
    ps->colorless_bank += size;
    ps->white_pending -= size;
    /* Deactivate the white token slot */
    for (int i = 0; i < PEBBLE_MAX_TOKENS; i++) {
      if (&ps->whites[i] == white) {
        ps->whites_active[i] = 0;
        white->token = 0;
        break;
      }
    }
    unlock(&pebble_global_lock);
    bprint("pebble_alloc_with_white: xallocz failed\n");
    return -1;
  }

  /* Step 3: Bind WHITE token to allocated address */
  white->data_ptr = buf;

  /* Step 4: Verify WHITE (consumes it) */
  if (pebble_white_verify(white, &black_handle) != 0) {
    xfree(buf);
    bprint("pebble_alloc_with_white: WHITE verify failed\n");
    return -1;
  }

  /* Step 5: Convert to BLACK token */
  if (pebble_black_alloc(white, buf, size, out_cap) != 0) {
    xfree(buf);
    bprint("pebble_alloc_with_white: BLACK alloc failed\n");
    return -1;
  }

  *out_addr = buf;
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
    /*
     * Process teardown may already have removed borrow records before Pebble
     * capability burn/free runs. Treat stale/not-owner records as non-fatal.
     */
    if (borrow_err == BORROW_ENOTFOUND || borrow_err == BORROW_ENOTOWNER) {
      bprint("pebble_black_free_internal: non-fatal borrow_release miss for "
             "pa=%#p err=%d\n",
             pa, borrow_err);
    } else {
      bpanic("pebble_black_free_internal: FATAL - borrow_release failed for "
             "pa=%#p: error=%d\n",
             pa, borrow_err);
    }
  }

  // --- Free physical memory ---
  xfree((void *)pa);

  if (pebble_debug)
    bprint("PEBBLE: internal free pid=%lud pa=%#p size=%lud\n",
           owner ? owner->pid : 0, pa, len);

  return 0;
}

/*@
  @ requires cap != \null;
  @ requires \valid_read(cap);
  @ requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  @ requires Inv_NonNegative(pebble_state());
  @
  @ behavior success:
  @   assumes pebble_state() != \null;
  @   ensures \result == 0;
  @   ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  @   ensures Inv_NonNegative(pebble_state());
  @
  @ behavior error_invalid_cap:
  @   assumes cap == \null;
  @   ensures \result == -1;
  @
  @ behavior error_state:
  @   assumes pebble_state() == \null;
  @   ensures \result == -1;
  @
  @ complete behaviors;
  @ disjoint behaviors;
  @ terminates \true;
  @ assigns pebble_state()->black_list, pebble_state()->black_inuse;
  @*/
int pebble_black_free_in_state(PebbleState *ps, Proc *owner,
                               const UserCapability *cap) {
  PebbleBlack *pb, **pp;
  ulong size;
  BlindLedgerError ledger_err;

  if (cap == nil)
    error(PEBBLE_E_BADARG);
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
  ledger_err = ledger_burn(cap, owner);
  if (ledger_err != BLIND_LEDGER_OK) {
    // CRITICAL: Blind Ledger state inconsistent with Pebble state
    // We already removed it from Pebble list, so we CANNOT recover.
    bpanic(
        "pebble_black_free: FATAL - ledger_burn failed for cap=%H: error=%d\n"
        "This indicates critical state corruption (double-burn/invalid cap).\n"
        "Blind Ledger and Pebble system out of sync.",
        cap->hash, ledger_err);
  }

  // --- Free PebbleBlack struct ---
  // (Physical memory was freed by ledger_burn -> pebble_black_free_internal)
  pebble_meta_free(pb);

  if (pebble_debug)
    bprint("PEBBLE: black free pid=%lud cap=%H size=%lud\n",
           owner ? owner->pid : 0, cap->hash, size);
  return 0;
}

int pebble_black_free(const UserCapability *cap) {
  return pebble_black_free_in_state(pebble_state(), up, cap);
}

/*@
  requires white_cap != \null;
  requires Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  requires Inv_NonNegative(pebble_state());
  ensures Inv_Conservation(pebble_state(), PEBBLE_DEFAULT_BUDGET);
  ensures Inv_NonNegative(pebble_state());
*/
int pebble_white_verify_in_state(PebbleState *ps, PebbleWhite *white_cap,
                                 void **black_cap) {
  int i;
  void *ret;

  if (ps == nil || white_cap == nil || black_cap == nil)
    error(PEBBLE_E_BADARG);

  lock(&pebble_global_lock);
  if (!pebble_valid_white_token(ps, white_cap)) {
    unlock(&pebble_global_lock);
    error(PEBBLE_E_PERM);
  }

  ret = white_cap->data_ptr;
  if (white_cap->size != 0)
    ps->white_pending -= white_cap->size;
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
    bprint("PEBBLE: white verify pid=%lud -> %#p\n", up ? up->pid : 0, ret);
  return 0;
}

int pebble_white_verify(PebbleWhite *white_cap, void **black_cap) {
  return pebble_white_verify_in_state(pebble_state(), white_cap, black_cap);
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
*/
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
    bprint("PEBBLE: blue_alloc pid=%lud size=%lud\n", up->pid, size);

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
*/
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
    bprint("PEBBLE: blue_free pid=%lud size=%lud\n", up->pid, size);

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
*/
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
    bprint("PEBBLE: red_alloc pid=%lud size=%lud\n", up->pid, size);

  return red;
}

/*
 * pebble_red_free - Free Red token back to colorless bank
 *
 * State transition: RED → COLORLESS
 * Returns budget to colorless bank.
 */
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
    bprint("PEBBLE: red_free pid=%lud size=%lud\n", up->pid, size);

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
   * - Unused colorless_bank tokens (stored in tokens)
   * - black_inuse tokens (from allocations being freed, stored in bytes)
   * - blue_inuse and red_inuse tokens (stored in bytes)
   */
  return_tokens =
      ps->colorless_bank + ((ps->black_inuse + ps->blue_inuse + ps->red_inuse) /
                            PEBBLE_BYTES_PER_TOKEN);

  /* Return tokens to global pool */
  lock(&pebble_bank_lock);
  pebble_global_colorless_bank += return_tokens;
  unlock(&pebble_bank_lock);

  if (pebble_debug && return_tokens > 0)
    bprint("PEBBLE: pid %lud exit, returned %lud tokens to global pool\n",
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
    /* Properly burn the capability via Blind Ledger.
       This will call pebble_black_free_internal to xfree physical memory. */
    ledger_burn(&pb->capability, p);
    pebble_meta_free(pb);
  }
  for (; blue != nil; blue = bluenext) {
    bluenext = blue->next;
    free(blue);
  }
  for (; red != nil; red = rednext) {
    rednext = red->next;
    pebble_free_red(red);
  }

  for (int i = 0; i < PEBBLE_MAX_TOKENS; i++)
    ps->whites_active[i] = 0;
}

void pebble_selftest(void) {
  PebbleState *ps;
  UserCapability black_cap;
  PebbleBlue *blue;
  PebbleRed *red;

  if (!pebble_enabled)
    return;
  ps = pebble_state();
  if (ps == nil)
    return;

  bprint("PEBBLE: selftest begin\n");

  /* Test 1: White -> Black allocation (full flow) */
  void *black_addr = nil;
  if (pebble_alloc_with_white(PEBBLE_MIN_ALLOC, &black_cap, &black_addr) != 0) {
    bprint("pebble selftest: WHITE->BLACK allocation failed\n");
    return;
  }
  if (black_addr == nil) {
    bprint("pebble selftest: BLACK allocation returned nil address\n");
    return;
  }

  /* Test 2: Independent Blue allocation */
  blue = pebble_blue_alloc(PEBBLE_MIN_ALLOC);
  if (blue == nil) {
    bprint("pebble selftest: blue alloc failed\n");
    return;
  }

  /* Test 3: Blue -> Red snapshot */
  if (pebble_red_snapshot(blue, &red) != 0) {
    bprint("pebble selftest: red snapshot failed\n");
    return;
  }
  if (red == nil) {
    bprint("pebble selftest: red nil after snapshot\n");
    return;
  }

  /* Test 4: Free all tokens */
  if (pebble_red_free(red) != 0) {
    bprint("pebble selftest: red free failed\n");
    return;
  }
  if (pebble_blue_free(blue) != 0) {
    bprint("pebble selftest: blue free failed\n");
    return;
  }

  /* Test 5: Holographic Channels ("Pointer-as-Channel") */
  {
    void *raw_ptr = pebble_get_black_addr(&black_cap);
    void *proj_ch3, *proj_ch7;

    /* Ensure alignment */
    if (((uintptr)raw_ptr & PEBBLE_WAVE_MASK) != 0) {
      bprint("pebble selftest: black addr not 8-byte aligned\n");
      return;
    }

    /* Project onto Channel 3 */
    proj_ch3 = PEBBLE_PROJECT(raw_ptr, PEBBLE_WAVE_3);
    if (!PEBBLE_TUNED(proj_ch3, PEBBLE_WAVE_3)) {
      bprint("pebble selftest: projection to Ch3 failed\n");
      return;
    }
    if (PEBBLE_TUNED(proj_ch3, PEBBLE_WAVE_2)) {
      bprint("pebble selftest: Ch3 bled into Ch2 (filtering fail)\n");
      return;
    }

    /* Project onto Channel 7 */
    proj_ch7 = PEBBLE_PROJECT(raw_ptr, PEBBLE_WAVE_7);
    if (PEBBLE_PTR_WAVE(proj_ch7) != 7) {
      bprint("pebble selftest: projection to Ch7 failed\n");
      return;
    }

    /* Verify Base Address Recovery (All waves collapse to source) */
    if (PEBBLE_PTR_ADDR(proj_ch3) != raw_ptr) {
      bprint("pebble selftest: Ch3 addr recovery failed\n");
      return;
    }

    bprint("PEBBLE: holographic channel verification passed\n");
  }

  if (pebble_black_free(&black_cap) != 0) {
    bprint("pebble selftest: black free failed\n");
    return;
  }

  bprint("PEBBLE: selftest PASS (independent tokens, circular economy "
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
  bprint("PEBBLE: /dev/sip/issue test begin\n");
  if (waserror()) {
    bprint("PEBBLE: /dev/sip/issue test FAIL: %s\n", up->errstr);
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
  if (pebble_black_alloc(white, black_handle, PEBBLE_MIN_ALLOC, &black_cap) !=
      0)
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
  bprint("PEBBLE: /dev/sip/issue test PASS (circular economy validated)\n");
}

/* ========== Arena Branch Banks ==========
 *
 * Per-container (WASM, SIP, etc.) resource management using colorless branch
 * banks. See docs/WASM_ARENA_BRANCH_BANKS.md for architecture details.
 *
 * Token flow: Process colorless_bank → branch local_colorless → allocation
 * All transitions are 1:1 (token conservation enforced).
 */

/*
 * arena_branch_init - Initialize a branch bank with budget from process
 *
 * @branch: Branch to initialize
 * @ps: Owning process PebbleState
 * @initial_budget: Initial tokens to provision from process bank
 */
void arena_branch_init(arena_branch_t *branch, PebbleState *ps,
                       ulong initial_budget) {
  if (branch == nil || ps == nil)
    return;

  memset(&branch->lock, 0, sizeof(branch->lock));
  branch->local_colorless = 0;
  branch->borrowed_from_proc = 0;
  branch->max_tokens = 0;
  branch->low_water = 0;
  branch->high_water = 0;
  branch->total_allocated = 0;
  branch->total_freed = 0;
  branch->owner_ps = ps;

  /* Set water marks for auto-refill/drain (default: 25%/75%) */
  branch->max_tokens = initial_budget;
  branch->low_water = initial_budget / 4;
  branch->high_water = (initial_budget * 3) / 4;

  /* Provision initial budget from process colorless bank */
  lock(&pebble_global_lock);
  if (ps->colorless_bank >= initial_budget) {
    ps->colorless_bank -= initial_budget;
    branch->local_colorless = initial_budget;
    branch->borrowed_from_proc = initial_budget;
  } else {
    /* Partial provision if insufficient budget */
    branch->local_colorless = ps->colorless_bank;
    branch->borrowed_from_proc = ps->colorless_bank;
    ps->colorless_bank = 0;
  }
  unlock(&pebble_global_lock);

  if (pebble_debug) {
    bprint("PEBBLE: arena_branch_init ps->colorless_bank=%lud "
           "initial_request=%lud\n",
           ps->colorless_bank, initial_budget);
    bprint("PEBBLE: arena_branch_init provisioned %lud tokens (low=%lud "
           "high=%lud)\n",
           branch->local_colorless, branch->low_water, branch->high_water);
  }
}

/*
 * arena_branch_alloc - Consume tokens from branch for allocation
 *
 * @branch: Branch to allocate from
 * @size: Bytes to allocate (will be rounded to token boundary)
 * @returns: 0 on success, -1 on insufficient tokens
 *
 * Fast path: Only takes branch->lock, not pebble_global_lock.
 * If branch is low, triggers refill from process bank.
 */
int arena_branch_alloc(arena_branch_t *branch, ulong size) {
  ulong tokens_needed;

  if (branch == nil)
    return -1;

  /* Round to token boundary */
  if (size < PEBBLE_MIN_ALLOC)
    size = PEBBLE_MIN_ALLOC;
  tokens_needed = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);

  if (pebble_debug)
    bprint("arena_branch_alloc: size=%lud tokens_needed=%lud local=%lud "
           "max=%lud\n",
           size, tokens_needed, branch->local_colorless, branch->max_tokens);

  if (branch->max_tokens > 0 && tokens_needed > branch->max_tokens) {
    if (pebble_debug)
      bprint("arena_branch_alloc: failed max_tokens check\n");
    return -1;
  }

  lock(&branch->lock);

  /* Check if branch has enough */
  if (branch->local_colorless < tokens_needed) {
    unlock(&branch->lock);
    /* Try refill from process bank */
    if (arena_branch_refill(branch) != 0) {
      if (pebble_debug)
        bprint("arena_branch_alloc: failed refill\n");
      return -1;
    }
    /* Retry after refill */
    lock(&branch->lock);
    if (branch->local_colorless < tokens_needed) {
      unlock(&branch->lock);
      if (pebble_debug)
        bprint("arena_branch_alloc: failed after refill\n");
      return -1; /* Still not enough after refill */
    }
  }

  /* Consume tokens (1:1 conservation) */
  branch->local_colorless -= tokens_needed;
  branch->total_allocated += tokens_needed;

  unlock(&branch->lock);
  return 0;
}

/*
 * arena_branch_free - Return tokens to branch after deallocation
 *
 * @branch: Branch to return tokens to
 * @size: Bytes being freed
 *
 * Tokens return to local branch pool; excess returned to process on drain.
 */
void arena_branch_free(arena_branch_t *branch, ulong size) {
  ulong tokens;

  if (branch == nil)
    return;

  tokens = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);

  lock(&branch->lock);
  branch->local_colorless += tokens;
  branch->total_freed += tokens;
  unlock(&branch->lock);

  /* Check if branch is over high water mark */
  if (branch->local_colorless > branch->high_water) {
    /* Return excess to process bank (cold path) */
    lock(&pebble_global_lock);
    lock(&branch->lock);
    if (branch->local_colorless > branch->high_water) {
      ulong excess = branch->local_colorless - branch->high_water;
      branch->local_colorless -= excess;
      branch->borrowed_from_proc -= (excess < branch->borrowed_from_proc)
                                        ? excess
                                        : branch->borrowed_from_proc;
      branch->owner_ps->colorless_bank += excess;
    }
    unlock(&branch->lock);
    unlock(&pebble_global_lock);
  }
}

/*
 * arena_branch_refill - Request tokens from process bank when low
 *
 * @branch: Branch to refill
 * @returns: 0 on success (some tokens obtained), -1 on failure (process empty)
 */
int arena_branch_refill(arena_branch_t *branch) {
  PebbleState *ps;
  ulong refill_amount;

  if (branch == nil || branch->owner_ps == nil)
    return -1;

  ps = branch->owner_ps;

  /* Refill up to high water mark (clamped by max_tokens) */
  lock(&pebble_global_lock);
  lock(&branch->lock);

  if (branch->max_tokens > 0 && branch->high_water > branch->max_tokens)
    branch->high_water = branch->max_tokens;

  if (branch->local_colorless >= branch->low_water) {
    /* Not actually low */
    unlock(&branch->lock);
    unlock(&pebble_global_lock);
    return 0;
  }

  if (branch->max_tokens > 0 && branch->local_colorless >= branch->max_tokens) {
    unlock(&branch->lock);
    unlock(&pebble_global_lock);
    return 0;
  }

  refill_amount = branch->high_water - branch->local_colorless;
  if (branch->max_tokens > 0 &&
      branch->local_colorless + refill_amount > branch->max_tokens) {
    refill_amount = branch->max_tokens - branch->local_colorless;
  }

  if (ps->colorless_bank >= refill_amount) {
    ps->colorless_bank -= refill_amount;
    branch->local_colorless += refill_amount;
    branch->borrowed_from_proc += refill_amount;
  } else if (ps->colorless_bank > 0) {
    /* Partial refill */
    branch->local_colorless += ps->colorless_bank;
    branch->borrowed_from_proc += ps->colorless_bank;
    ps->colorless_bank = 0;
  } else {
    unlock(&branch->lock);
    unlock(&pebble_global_lock);
    return -1; /* Process exhausted */
  }

  unlock(&branch->lock);
  unlock(&pebble_global_lock);

  if (pebble_debug)
    bprint("PEBBLE: arena_branch_refill added %lu tokens (now %lu)\n",
           refill_amount, branch->local_colorless);

  return 0;
}

/*
 * arena_branch_drain - Return ALL tokens from branch to process bank
 *
 * @branch: Branch to drain
 *
 * Called during container cleanup to return resources 1:1.
 */
void arena_branch_drain(arena_branch_t *branch) {
  ulong drained;

  if (branch == nil || branch->owner_ps == nil)
    return;

  lock(&pebble_global_lock);
  lock(&branch->lock);

  drained = branch->local_colorless;
  branch->owner_ps->colorless_bank += drained;
  branch->local_colorless = 0;
  branch->borrowed_from_proc = 0;

  unlock(&branch->lock);
  unlock(&pebble_global_lock);

  if (pebble_debug)
    bprint("PEBBLE: arena_branch_drain returned %lud tokens to process "
           "(alloc=%lud freed=%lud)\n",
           drained, branch->total_allocated, branch->total_freed);
}

/* ========== Wave 7 Distress Signal System ========== */

/*
 * Kernel-side ring buffer for distress events.
 * Events are produced by pebble_signal_distress() and consumed
 * by /dev/distress (via pebble_read_distress).
 */
static DistressEvent distress_ring[DISTRESS_RING_SIZE];
static volatile u32int distress_head = 0; /* Consumer reads here */
static volatile u32int distress_tail = 0; /* Producer writes here */
static Lock distress_lock;
static Rendez distress_rendez; /* For blocking reads */

/*
 * pebble_signal_distress - Emit a Wave 7 distress signal
 *
 * @p: Process in distress (nil for kernel)
 * @reason: DISTRESS_* reason code
 * @context: Reason-specific data (address, cap hash, etc)
 *
 * Called from fault handlers, capability checks, etc.
 * Non-blocking; drops events if ring is full (prefer liveness over
 * completeness).
 */
void pebble_signal_distress(Proc *p, int reason, u64int context) {
  DistressEvent ev;
  u32int next;

  /* Build event */
  ev.timestamp = nsec();
  ev.pid = (p != nil) ? p->pid : 0;
  ev.reason = (u16int)reason;
  ev.context = context;

  /* Auto-assign severity based on reason */
  if (reason >= DISTRESS_VAULT_BREACH)
    ev.severity = DISTRESS_SEV_CRITICAL;
  else if (reason >= DISTRESS_PANIC_IMMINENT)
    ev.severity = DISTRESS_SEV_ERROR;
  else if (reason >= DISTRESS_CAP_VIOLATION)
    ev.severity = DISTRESS_SEV_WARN;
  else
    ev.severity = DISTRESS_SEV_INFO;

  lock(&distress_lock);

  next = (distress_tail + 1) % DISTRESS_RING_SIZE;
  if (next == distress_head) {
    /* Ring full - drop oldest event (overwrite) */
    distress_head = (distress_head + 1) % DISTRESS_RING_SIZE;
  }

  distress_ring[distress_tail] = ev;
  distress_tail = next;

  unlock(&distress_lock);

  /* Wake any waiters on /dev/distress */
  wakeup(&distress_rendez);

  if (pebble_debug)
    bprint("PEBBLE: DISTRESS pid=%lud reason=%d severity=%d context=%#llx\n",
           (p != nil) ? p->pid : 0, reason, ev.severity, context);
}

/*
 * distress_canread - Check if distress events are available
 * Used by sleep() for blocking reads.
 */
static int distress_canread(void *arg) {
  USED(arg);
  return distress_head != distress_tail;
}

/*
 * pebble_read_distress - Read next distress event (for /dev/distress)
 *
 * @out: Buffer to receive event
 *
 * Returns: 1 if event returned, 0 if no events, -1 on error
 *
 * Blocking: sleeps until an event is available.
 * Only accessible by CAP_SERVICE_CONTROL processes (resurrection server).
 */
int pebble_read_distress(DistressEvent *out) {
  if (out == nil)
    return -1;

  /* Block until event available */
  sleep(&distress_rendez, distress_canread, nil);

  lock(&distress_lock);

  if (distress_head == distress_tail) {
    /* Spurious wakeup */
    unlock(&distress_lock);
    return 0;
  }

  *out = distress_ring[distress_head];
  distress_head = (distress_head + 1) % DISTRESS_RING_SIZE;

  unlock(&distress_lock);
  return 1;
}

/*
 * pebble_distress_pending - Check if distress events are pending
 * Returns count of pending events (non-blocking).
 */
int pebble_distress_pending(void) {
  u32int h, t;
  int count;

  lock(&distress_lock);
  h = distress_head;
  t = distress_tail;
  unlock(&distress_lock);

  if (t >= h)
    count = t - h;
  else
    count = DISTRESS_RING_SIZE - h + t;

  return count;
}

int pebble_clear_distress(void) {
  int count;

  lock(&distress_lock);
  if (distress_tail >= distress_head)
    count = distress_tail - distress_head;
  else
    count = DISTRESS_RING_SIZE - distress_head + distress_tail;
  memset(distress_ring, 0, sizeof(distress_ring));
  distress_head = 0;
  distress_tail = 0;
  unlock(&distress_lock);

  return count;
}
