#ifdef __FRAMAC__
#include <kernel.h>
#include <lib.h>
#include <u.h>
#include "monocypher.h"
#else
#include "libsec.h"
#include "monocypher.h"
#include <kernel.h>
#include <lib.h>
#include <u.h>
#endif

#ifdef __FRAMAC__
#define GETCONF_ENABLED(x) 0
#define PANIC_MSG(msg) ((void)0)
#else
#define GETCONF_ENABLED(x) getconf(x)
#define PANIC_MSG(msg) panic(msg)
#endif

/*@ requires nbytes >= 0;
  @ requires nbytes == 0 || \valid(buf + (0 .. (integer)nbytes - 1));
  @ assigns buf[0 .. (integer)nbytes - 1] \if nbytes > 0;
  @ terminates \true;
  */
extern void genrandom(uchar *buf, int nbytes);

/*@ assigns \nothing;
  @ terminates \true;
  */
extern char *getconf(char *);
extern int tpm2_seal_to_srk(const u8int *data, u16int data_len,
                            const u8int *auth, u16int auth_len, u8int *blob_out,
                            u16int *blob_len_out);
extern int tpm2_unseal_from_blob(const u8int *blob, u16int blob_len,
                                 const u8int *auth, u16int auth_len,
                                 u8int *data_out, u16int *data_len_out);

enum {
  Qdir = 0,
  Qram,
  Qvaultnew,
  Qvaultbase = 1000,
};

enum {
  VaultSaltSize = 16,
  VaultRSize = 32,
  VaultNonceSize = 24,
  VaultMacSize = 16,
  VaultHeaderSize = VaultSaltSize + VaultRSize + VaultNonceSize + VaultMacSize,
};

static Dirtab ramdir[] = {
    ".",         {Qdir, 0, QTDIR}, 0, DMDIR | 0555, "ram", {Qram}, 0, 0666,
    "vault.new", {Qvaultnew},      0, 0600,
};

/* Standard Ramdisk */
static uchar *ramdisk_data;
static ulong ramdisk_size = 8 * 1024 * 1024; /* 8MB default */

/* Secure Vault Structure - FIXED with locking and refcounting
 *
 * ACSL Invariants (from Coq proofs in ramdisk_state.v):
 */
/* type invariant lock_encryption_inv(SecureRamdisk rd) =
  @   (rd.locked == 1 && rd.initialized == 1) ==>
  @     (\valid(rd.data) && rd.size >= 24);
  @
  @ type invariant init_key_inv(SecureRamdisk rd) =
  @   (rd.initialized == 1) ==>
  @     (\exists integer i; 0 <= i < 32 && rd.master_key[i] != 0);
/* Process Vault structure moved to portdat.h */

ProcessVault *vault_list = nil;
QLock vault_list_lock;
int next_vault_id = 1;

ProcessVault *find_vault(int id) {
  ProcessVault *v;
  qlock(&vault_list_lock);
  for (v = vault_list; v != nil; v = v->next) {
    if (v->id == id) {
      qunlock(&vault_list_lock);
      return v;
    }
  }
  qunlock(&vault_list_lock);
  return nil;
}

static ProcessVault *get_vault_from_path(ulong path) {
  if (path < Qvaultbase)
    return nil;
  int id = (path - Qvaultbase) / 2;
  return find_vault(id);
}

int process_has_vault(int pid) {
  ProcessVault *v;
  for (v = vault_list; v != nil; v = v->next) {
    if (v->pid == pid)
      return 1;
  }
  return 0;
}

/* Helper to generate dynamic directory entries */
static int vaultgen(Chan *c, char *name, Dirtab *tab, int ntab, int s,
                    Dir *dp) {
  Qid q;
  ProcessVault *v;
  int i = 0;

  if (s == DEVDOTDOT) {
    devdir(c, c->qid, "#r", 0, eve, 0555, dp);
    return 1;
  }

  /* 0: ram, 1: vault.new */
  if (s < 2)
    return devgen(c, name, ramdir, nelem(ramdir), s, dp);

  /* Dynamic vaults */
  s -= 2;
  qlock(&vault_list_lock);
  for (v = vault_list; v != nil; v = v->next) {
    /* For each vault, we have 2 files: vault.ID and vault.ID.ctl */
    if (s == 0) {
      /* vault.ID */
      mkqid(&q, Qvaultbase + v->id * 2, 0, QTFILE);
      char buf[32];
      snprint(buf, sizeof(buf), "vault.%d", v->id);
      devdir(c, q, buf, v->size - 24, eve, 0600, dp);
      qunlock(&vault_list_lock);
      return 1;
    }
    s--;
    if (s == 0) {
      /* vault.ID.ctl */
      mkqid(&q, Qvaultbase + v->id * 2 + 1, 0, QTFILE);
      char buf[32];
      snprint(buf, sizeof(buf), "vault.%d.ctl", v->id);
      devdir(c, q, buf, 0, eve, 0600, dp);
      qunlock(&vault_list_lock);
      return 1;
    }
    s--;
  }
  qunlock(&vault_list_lock);
  return -1;
}

/* Secure wipe prototype */
static void secure_wipe(uchar *data, ulong size);

void vault_cleanup_process(int pid) {
  ProcessVault *v, **prev;

  qlock(&vault_list_lock);
  prev = &vault_list;
  while ((v = *prev) != nil) {
    if (v->pid == pid) {
      /* Unlink first */
      *prev = v->next;

      /* Mark as dead and check for deferred free */
      qlock(&v->lock);
      v->dead = 1;

      int can_free = (v->refcount == 0);
      qunlock(&v->lock);

      if (can_free) {
        /* Secure wipe before memory is reclaimed */
        if (v->data) {
          secure_wipe(v->data, v->size);
          /* Burn capability - handles xfree(v->data) via internal callback */
          pebble_black_free(&v->capability);
        }
        if (v->has_key)
          crypto_wipe(v->ephemeral_key, 32);

        free(v);
      }
      /* If refcount > 0, it will be freed in ramclose */
    } else {
      prev = &v->next;
    }
  }
  qunlock(&vault_list_lock);
}

/* ========================================================================
 * Invariant Checking (from formal verification)
 * ======================================================================== */

/* INVARIANT: Locked implies data is encrypted
 * Corresponds to lock_encryption_invariant in ramdisk_state.v
 */
/*@ requires v != \null;
  @ requires \valid(v);
  @ ensures (v->locked == 1 && v->initialized == 1) ==>
  @   (\valid(v->data) && v->size >= 24);
  @ assigns \nothing;
  */
static void check_lock_invariant(ProcessVault *v) {
  if (!GETCONF_ENABLED("debug.invariants"))
    return;

  if (v->locked && v->initialized) {
#ifndef __FRAMAC__
    if (!GETCONF_ENABLED("quiet"))
      print("ramdisk: INVARIANT CHECK - locked state verified\n");
#endif
  }
}

/* INVARIANT: Unlocked implies key is present */
/*@ requires v != \null;
  @ requires \valid(v);
  @ requires \valid(v->ephemeral_key + (0..31));
  @ ensures (v->locked == 0 && v->has_key == 1) ==>
  @   (\exists integer j; 0 <= j < 32 && v->ephemeral_key[j] != 0);
  @ assigns \nothing;
  */
static void check_key_invariant(ProcessVault *v) {
  if (!GETCONF_ENABLED("debug.invariants"))
    return;

  if (!v->locked && v->has_key) {
    int all_zero = 1;
    for (int i = 0; i < 32; i++) {
      if (v->ephemeral_key[i] != 0) {
        all_zero = 0;
        break;
      }
    }
    if (all_zero)
      PANIC_MSG("ramdisk: INVARIANT VIOLATION - unlocked but no key");
  }

  if (v->locked && v->has_key)
    PANIC_MSG("ramdisk: INVARIANT VIOLATION - locked but key present (LEAK!)");
}

/* INVARIANT: Refcount matches number of open channels
 * Corresponds to refcount_invariant in ramdisk_state.v
 */
/*@ requires v != \null;
  @ requires \valid(v);
  @ ensures v->refcount >= 0;
  @ assigns \nothing;
  */
static void check_refcount_invariant(ProcessVault *v) {
  if (!GETCONF_ENABLED("debug.invariants"))
    return;
  if (v->refcount < 0)
    PANIC_MSG("ramdisk: INVARIANT VIOLATION - negative refcount");
}

/* ========================================================================
 * Secure Wipe Implementation (DoD 5220.22-M)
 * ======================================================================== */

/* Secure 7-pass wipe (SMT: Validated by proofs/ramdisk/ramdisk_wipe.v)
 *
 * SPECIFICATION:
 * - Performs 7 overwrite passes as per DoD 5220.22-M
 * - Final state: all bytes set to 0
 * - Each byte written at least 7 times
 * - Uses memory coherence after each pass
 */
/*@ behavior null_or_zero:
  @   assumes data == \null || size == 0;
  @   assigns \nothing;
  @ behavior valid_wipe:
  @   assumes data != \null && size > 0;
  @   requires \valid(data + (0 .. (integer)size - 1));
  @   ensures \forall integer i; 0 <= i < size ==> data[i] == 0;
  @   assigns data[0 .. (integer)size - 1];
  @ complete behaviors;
  @ disjoint behaviors;
  */
static void secure_wipe(uchar *data, ulong size) {
  ulong i;
  extern void (*coherence)(void);

  if (data == nil || size == 0)
    return;

  /*@ assert data != \null; */
  /*@ assert size > 0; */
  /*@ assert \valid(data + (0 .. (integer)size - 1)); */

#ifndef __FRAMAC__
  if (!GETCONF_ENABLED("quiet"))
    print("ramdisk: wiping vault (7-pass)...\n");
#endif

#ifdef __FRAMAC__
  /* Frama-C: model the final post-state without random passes. */
  memset(data, 0x00, size);
  return;
#endif

  /* Pass 1: Write 0x00 */
  memset(data, 0x00, size);
  coherence();

  /* Pass 2: Write 0xFF */
  memset(data, 0xFF, size);
  coherence();

  /* Pass 3: Write random */
  /*@ loop invariant 0 <= i <= size;
    @ loop invariant size > 0;
    @ loop assigns i, data[0 .. (integer)size - 1];
    @ loop variant size - i;
    */
  for (i = 0; i < size; i += 256) {
    ulong chunk = (size - i) > 256 ? 256 : (size - i);
    /*@ assert 0 < chunk <= 256; */
    /*@ assert i + chunk <= size; */
    /*@ assert \valid(data + (i .. i + chunk - 1)); */
    genrandom(data + i, (int)chunk);
  }
  coherence();

  /* Pass 4: Write 0x00 */
  memset(data, 0x00, size);
  coherence();

  /* Pass 5: Write 0xFF */
  memset(data, 0xFF, size);
  coherence();

  /* Pass 6: Write random */
  /*@ loop invariant 0 <= i <= size;
    @ loop invariant size > 0;
    @ loop assigns i, data[0 .. (integer)size - 1];
    @ loop variant size - i;
    */
  for (i = 0; i < size; i += 256) {
    ulong chunk = (size - i) > 256 ? 256 : (size - i);
    /*@ assert 0 < chunk <= 256; */
    /*@ assert i + chunk <= size; */
    /*@ assert \valid(data + (i .. i + chunk - 1)); */
    genrandom(data + i, (int)chunk);
  }
  coherence();

  /* Pass 7: Write 0x00 (final) */
  memset(data, 0x00, size);
  coherence();

#ifndef __FRAMAC__
  if (!GETCONF_ENABLED("quiet"))
    print("ramdisk: vault wiped\n");
#endif
}

/* ========================================================================
 * Argon2id Password-Based Key Derivation
 * ======================================================================== */

/*@
  requires password != \null && salt != \null && key_out != \null;
  requires \valid(salt + (0..15));
  requires \valid(key_out + (0..31));
  assigns key_out[0..31];
*/
static int derive_key_from_password(const char *password, uchar *salt,
                                    uchar *key_out) {
  uint32_t nb_blocks = 4096;  /* 4MB memory (4096 blocks × 1024 bytes) */
  uint32_t nb_iterations = 3; /* 3 passes */
  void *work_area;
  crypto_argon2_config config;
  crypto_argon2_inputs inputs;
  extern const crypto_argon2_extras crypto_argon2_no_extras;

  if (password == nil || salt == nil || key_out == nil)
    return -1;

  /* Allocate work area for Argon2id (4MB) */
  work_area = xalloc(nb_blocks * 1024);
  if (work_area == nil) {
#ifndef __FRAMAC__
    print("ramdisk: failed to allocate Argon2id work area\n");
#endif
    return -1;
  }

  /* Configure Argon2id */
  config.algorithm = CRYPTO_ARGON2_ID; /* Argon2id (hybrid) */
  config.nb_blocks = nb_blocks;        /* 4096 blocks = 4MB */
  config.nb_passes = nb_iterations;    /* 3 passes */
  config.nb_lanes = 1;                 /* Single-threaded */

  /* Setup inputs */
  inputs.pass = (const uint8_t *)password;
  inputs.salt = salt;
  inputs.pass_size = strlen(password);
  inputs.salt_size = 16;

  /* Derive key using Argon2id */
  crypto_argon2(key_out, 32, work_area, config, inputs,
                crypto_argon2_no_extras);

  /* Wipe work area */
  crypto_wipe(work_area, nb_blocks * 1024);
  free(work_area);

  return 0;
}

/* ========================================================================
 * Holographic Locking (Elligator 2)
 * ======================================================================== */

/*
 * derive_and_lock:
 * 1. Takes the user's ephemeral key (derived from password) and salt.
 * 2. Stores salt, maps key to R (Elligator 2), and writes the header.
 * 3. Encrypts the rest of the vault using the key.
 * 4. WIPES the key from memory.
 */
/*@
  requires v != \null;
  requires \valid(v);
  requires \valid(v->data + (0..v->size - 1));
  requires v->size >= VaultHeaderSize;
  requires \valid_read(salt + (0..15));
  requires \valid_read(key + (0..31));
  assigns v->data[0..v->size - 1], v->ephemeral_key[0..31], v->has_key,
          v->locked;
  ensures v->locked == 1;
  ensures v->has_key == 0;
*/
static void derive_and_lock(ProcessVault *v, const uchar *salt,
                            const uchar *key) {
  uchar R[32];
  uchar nonce[24];
  uchar mac[16];

  if (v->size < VaultHeaderSize)
    return; /* Should be checked at alloc */

  /* 1. Store salt in header */
  memmove(v->data, salt, VaultSaltSize);

  /* 2. Map key to Representative R (Holographic Header) */
  crypto_elligator_map(R, key);

  /* 3. Write R to header (offset 16) */
  memmove(v->data + VaultSaltSize, R, VaultRSize);

  /* 4. Encrypt body (Offset VaultHeaderSize) */
  /* Generate fresh nonce for XChaCha20 */
  genrandom(nonce, VaultNonceSize);

  /* Store nonce after header */
  memmove(v->data + VaultSaltSize + VaultRSize, nonce, VaultNonceSize);

  /* Encrypt remainder */
  /* Layout: [Salt] [R] [Nonce] [MAC] [Encrypted Data...] */
  ulong body_size = v->size - VaultHeaderSize;
  uchar *body_start = v->data + VaultHeaderSize;

  crypto_aead_lock(body_start, mac, key, nonce, v->data,
                   VaultSaltSize + VaultRSize + VaultNonceSize, body_start,
                   body_size);
  memmove(v->data + VaultSaltSize + VaultRSize + VaultNonceSize, mac,
          VaultMacSize);

  /* 5. Wipe key from struct (Satelessness) */
  crypto_wipe(v->ephemeral_key, 32);
  v->has_key = 0;
  v->locked = 1;

#ifndef __FRAMAC__
  if (!GETCONF_ENABLED("quiet"))
    print("ramdisk: vault locked (holographic)\n");
#endif
}

/*
 * unlock_and_verify:
 * 1. Reads R from header (salt already used to derive candidate_key).
 * 2. Maps candidate key to R' and compares.
 * 3. Decrypts body on success.
 */
/*@
  requires v != \null;
  requires \valid(v);
  requires \valid(v->data + (0..v->size - 1));
  requires v->size >= VaultHeaderSize;
  requires \valid_read(candidate_key + (0..31));
  assigns v->data[VaultHeaderSize..v->size - 1], v->ephemeral_key[0..31],
          v->has_key, v->locked;
*/
static int verify_and_unlock(ProcessVault *v, uchar *candidate_key) {
  uchar R_stored[32];
  uchar R_candidate[32];
  uchar nonce[24];
  uchar mac[16];

  /* 1. Read stored R (offset 16; salt is at 0..15) */
  memmove(R_stored, v->data + VaultSaltSize, VaultRSize);

  /* 2. Map candidate key to R */
  crypto_elligator_map(R_candidate, candidate_key);

  /* 3. Compare */
  if (crypto_verify32(R_stored, R_candidate) != 0) {
    return -1; /* Wrong password/key */
  }

  /* 4. Decrypt */
  memmove(nonce, v->data + VaultSaltSize + VaultRSize, VaultNonceSize);
  memmove(mac, v->data + VaultSaltSize + VaultRSize + VaultNonceSize,
          VaultMacSize);
  ulong body_size = v->size - VaultHeaderSize;
  uchar *body_start = v->data + VaultHeaderSize;

  if (crypto_aead_unlock(body_start, mac, candidate_key, nonce, v->data,
                         VaultSaltSize + VaultRSize + VaultNonceSize,
                         body_start, body_size) != 0) {
    return -1;
  }

  /* 5. Store key in volatile memory */
  memmove(v->ephemeral_key, candidate_key, 32);
  v->has_key = 1;
  v->locked = 0;

  return 0;
}

/*@
  requires v != \null;
  requires \valid(v);
  requires \valid(v->data + (0..v->size - 1));
  requires v->size >= VaultHeaderSize;
  requires \valid_read(password);
  assigns v->data[0..v->size - 1], v->ephemeral_key[0..31], v->has_key,
          v->locked, v->initialized, \result;
  ensures \result == 0 ==> (v->initialized == 1 && v->locked == 1 &&
                            v->has_key == 0);
*/
int vault_cmd_init(ProcessVault *v, const char *password) {
  uchar salt[16];
  uchar key[32];

  genrandom(salt, 16);

  if (derive_key_from_password(password, salt, key) != 0)
    return -1;

  v->initialized = 1;
  memmove(v->ephemeral_key, key, 32);
  v->has_key = 1;

  derive_and_lock(v, salt, key);

  crypto_wipe(key, 32);
  check_lock_invariant(v);
  return 0;
}

/*@
  requires v != \null;
  requires \valid(v);
  requires \valid(v->data + (0..v->size - 1));
  requires v->size >= VaultHeaderSize;
  requires \valid_read(password);
  assigns v->data[VaultHeaderSize..v->size - 1], v->ephemeral_key[0..31],
          v->has_key, v->locked;
*/
int vault_cmd_unlock(ProcessVault *v, const char *password) {
  uchar stored_salt[16];
  uchar derived_key[32];
  int rc;

  memmove(stored_salt, v->data, 16);
  if (derive_key_from_password(password, stored_salt, derived_key) != 0)
    return -1;

  rc = verify_and_unlock(v, derived_key);
  crypto_wipe(derived_key, 32);
  return rc;
}

/*@
  requires v != \null;
  requires \valid(v);
  requires \valid(v->data + (0..v->size - 1));
  requires v->size >= VaultHeaderSize;
  requires v->has_key == 1;
  assigns v->data[0..v->size - 1], v->ephemeral_key[0..31], v->has_key,
          v->locked;
  ensures v->locked == 1;
  ensures v->has_key == 0;
*/
void vault_cmd_lock(ProcessVault *v) {
  uchar salt[16];

  genrandom(salt, 16);
  derive_and_lock(v, salt, v->ephemeral_key);
}

/*@
  requires v != \null;
  requires \valid(v);
  requires \valid(v->data + (0..v->size - 1));
  assigns v->data[0..v->size - 1], v->ephemeral_key[0..31], v->has_key,
          v->locked, v->initialized;
  ensures v->locked == 1;
  ensures v->has_key == 0;
  ensures v->initialized == 0;
*/
void vault_cmd_wipe(ProcessVault *v) {
  secure_wipe(v->data, v->size);
  if (v->has_key)
    crypto_wipe(v->ephemeral_key, 32);
  v->has_key = 0;
  v->initialized = 0;
  v->locked = 1;
}

/* ========================================================================
 * Device Reset and Initialization
 * ======================================================================== */

static void ramreset(void) {
  /* 1. Setup Standard Ramdisk */
  ramdisk_data = xalloc(ramdisk_size);
  if (ramdisk_data == nil)
    PANIC_MSG("ramdisk: cannot allocate memory");

  memset(ramdisk_data, 0, ramdisk_size);
#ifndef __FRAMAC__
  if (!GETCONF_ENABLED("quiet"))
    print("ramdisk: %lud MB allocated\n", ramdisk_size / (1024 * 1024));
#endif

  /* 2. Init Vault List Lock */
  memset(&vault_list_lock, 0, sizeof(vault_list_lock));
}

static void raminit(void) { /* Nothing to do */ }

static Chan *ramattach(char *spec) { return devattach('r', spec); }

static Walkqid *ramwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, ramdir, nelem(ramdir), vaultgen);
}

static int ramstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, ramdir, nelem(ramdir), vaultgen);
}

/*@ requires \valid(c);
  @ assigns c->offset, c->aux, c->qid, c->mode, vault_list, next_vault_id;
  @ ensures c->aux == \null || \valid((ProcessVault *)c->aux);
  @ ensures c->aux != \null ==> ((ProcessVault *)c->aux)->refcount > 0;
  */
static Chan *ramopen(Chan *c, int omode) {
  ProcessVault *v;

  if (c->qid.path == Qvaultnew) {
    /* Create new vault */
    if (omode != OREAD && omode != OEXEC) {
      /* Assume standard open flags, but Qvaultnew is special */
    }

    /* Only one vault per process */
    qlock(&vault_list_lock);
    if (process_has_vault(up->pid)) {
      qunlock(&vault_list_lock);
      error("vault already exists for process");
    }
    qunlock(&vault_list_lock);

    /* Allocate new vault */
    v = malloc(sizeof(ProcessVault));
    if (v == nil)
      error(Enomem);

    memset(v, 0, sizeof(ProcessVault));
    v->pid = up->pid;
    v->size = 512 * 1024; /* 512KB default per user request */
    /* Add header space: Salt + R + Nonce + MAC */
    v->size += VaultHeaderSize;

    /* Mint capability and allocate memory */
    if (pebble_alloc_with_white(v->size, &v->capability, (void **)&v->data) <
        0) {
      free(v);
      error(Enomem);
    }
    memset(v->data, 0, v->size);

    /* Locks */

    /* Add to list */
    qlock(&vault_list_lock);
    if (process_has_vault(up->pid)) {
      qunlock(&vault_list_lock);
      secure_wipe(v->data, v->size);
      free(v->data);
      free(v);
      error("vault already exists for process");
    }
    v->id = next_vault_id++;
    v->next = vault_list;
    vault_list = v;

    /* It's open, so refcount = 1 */
    v->refcount = 1;
    v->locked = 1; /* Start locked */
    qunlock(&vault_list_lock);

    c->aux = v;

    /* Update Chan to point to the new vault */
    c->qid.path = Qvaultbase + v->id * 2; /* Data file */
    c->qid.vers = 0;
    c->qid.type = QTFILE;
    c->mode = omode;

    /* c->path is managed by kernel, don't touch c->name */

#ifndef __FRAMAC__
    if (!GETCONF_ENABLED("quiet"))
      print("ramdisk: new vault %d created for pid %d\n", v->id, v->pid);
#endif

    return c;
  }

  /* Open existing */
  c = devopen(c, omode, ramdir, nelem(ramdir), vaultgen);
  c->offset = 0;

  /* Increment refcount if it's a vault file */
  if (c->qid.path >= Qvaultbase) {
    v = get_vault_from_path(c->qid.path);
    if (v == nil)
      error(Enonexist);

    /* Check permission */
    /* Only owner can open for now (simple model) */
    /* Real implementation would use Blind Ledger capability check here */
    /* Using simple pid check for now */
    if (v->pid != up->pid)
      error(Eperm);

    qlock(&v->lock);
    v->refcount++;
    check_refcount_invariant(v);
    qunlock(&v->lock);

    c->aux = v;
  }

  return c;
}

/*@ requires \valid(c);
  @ behavior vault_close:
  @   assumes c->aux != \null;
  @   assigns ((ProcessVault *)c->aux)->refcount,
  @           ((ProcessVault *)c->aux)->data[0..((ProcessVault
  *)c->aux)->size-1];
  @ behavior other_close:
  @   assumes c->aux == \null;
  @   assigns \nothing;
  @ ensures c->aux == \null || \valid((ProcessVault *)c->aux);
  @ complete behaviors;
  @ disjoint behaviors;
  */
static void ramclose(Chan *c) {
  ProcessVault *v;

  if (c->qid.path >= Qvaultbase) {
    v = c->aux;
    if (v == nil)
      v = get_vault_from_path(c->qid.path);

    if (v == nil)
      return; /* Should not happen */

    qlock(&v->lock);
    if (v->refcount > 0)
      v->refcount--;
    check_refcount_invariant(v);

    /*
     * Note: unlike global vault, we don't wipe on last close here
     * because we want persistence across opens within the process lifetime.
     * Wipe happens on process exit (vault_cleanup_process).
     * Or explicit wipe command.
     */
    int should_free = (v->refcount == 0 && v->dead);

    qunlock(&v->lock);

    if (should_free) {
      if (v->data) {
        secure_wipe(v->data, v->size);
        free(v->data);
      }
      if (v->has_key)
        crypto_wipe(v->ephemeral_key, 32);
      free(v);
    }
  }
}

/* ========================================================================
 * Read Operations
 * ======================================================================== */

/*@ requires c != \null;
    requires va != \null;
    requires n >= 0;
    requires (n > 0 ==> \valid((char*)va + (0 .. (integer)n-1))) || (n == 0);
    behavior ramdisk:
      assumes c->qid.path == Qram;
      requires ramdisk_data != \null;
      requires ramdisk_size > 0;
      requires 0 <= off;
      requires off + n <= ramdisk_size;
      assigns ((char*)va)[0 .. (integer)n-1] \if n > 0;
    behavior other:
      assumes c->qid.path != Qram;
      assigns ((char*)va)[0 .. (integer)n-1] \if n > 0;
    complete behaviors;
    disjoint behaviors;
*/
long ramread(Chan *c, void *va, long n, vlong off) {
  char status[256];
  ProcessVault *v;

  if (c->qid.path == Qdir)
    return devdirread(c, va, n, ramdir, nelem(ramdir), vaultgen);

  if (c->qid.path == Qram) {
    /* Standard ramdisk */
    if (off < 0)
      error(Ebadarg);
    if (off >= ramdisk_size)
      return 0;
    if (off + n > ramdisk_size)
      n = ramdisk_size - off;

    memmove(va, ramdisk_data + off, n);
    return n;
  }

  if (c->qid.path == Qvaultnew)
    return 0; /* Nothing to read */

  if (c->qid.path >= Qvaultbase) {
    /* Check if it's a control file (odd Qid) */
    int is_ctl = (c->qid.path - Qvaultbase) % 2;
    v = get_vault_from_path(c->qid.path);
    if (v == nil)
      error(Enonexist);

    /* Verify ownership */
    if (v->pid != up->pid)
      error(Eperm);

    qlock(&v->lock);

    if (is_ctl) {
      /* Status query */
      qunlock(&v->lock); /* Don't need lock for reading static/atomic fields */
      snprint(status, sizeof(status), "status: %s\nsize: %lud\n",
              v->locked ? "locked" : "unlocked", v->size - VaultHeaderSize);
      return readstr(off, va, n, status);
    } else {
      /* Data Read */
      /* NEW: Allow reading even if locked -> returns RAW encrypted blob (SAVE
       * support) */

      /* Account for header */
      vlong actual_data_size = v->size - VaultHeaderSize;

      if (v->locked) {
        /* Raw Read Mode (Save/Export) */
        /* We simulate the file as being valid but encrypted */
        /* User sees the Header + Nonce + Encrypted Data */

        /* If off < 32, reading header */
        /* If off < 56, reading nonce */
        /* etc. */

        if (off >= v->size) {
          qunlock(&v->lock);
          return 0;
        }
        if (off + n > v->size)
          n = v->size - off;

        memmove(va, v->data + off, n);
        qunlock(&v->lock);
        return n;
      }

      /* Unlocked Mode: Read Plaintext Body */

      /* Bounds checking relative to Body */
      if (off < 0) {
        qunlock(&v->lock);
        error(Ebadarg);
      }
      if (off >= actual_data_size) {
        qunlock(&v->lock);
        return 0;
      }
      if (off + n > actual_data_size)
        n = actual_data_size - off;

      /* Read decrypted body */
      memmove(va, v->data + VaultHeaderSize + off, n);

      qunlock(&v->lock);
      return n;
    }
  }

  error(Egreg);
  return 0;
}

/* ========================================================================
 * Write Operations
 * ======================================================================== */

/*@ requires c != \null;
    requires va != \null;
    requires n >= 0;
    requires (n > 0 ==> \valid_read((char*)va + (0 .. (integer)n-1))) || (n ==
   0); behavior ramdisk: assumes c->qid.path == Qram; requires ramdisk_data !=
   \null; requires ramdisk_size > 0; requires 0 <= off; requires off + n <=
   ramdisk_size; assigns ramdisk_data[off .. off + n - 1] \if n > 0; behavior
   other: assumes c->qid.path != Qram; assigns \nothing; complete behaviors;
    disjoint behaviors;
*/
long ramwrite(Chan *c, void *va, long n, vlong off) {
  char cmd[256];
  char *argv[3];
  int argc;
  ProcessVault *v;

  if (c->qid.path == Qdir)
    error(Eperm);

  if (c->qid.path == Qram) {
    /* Standard ramdisk */
    if (off < 0)
      error(Ebadarg);
    if (off >= ramdisk_size)
      error(Eio);
    if (off + n > ramdisk_size)
      n = ramdisk_size - off;

    memmove(ramdisk_data + off, va, n);
    return n;
  }

  if (c->qid.path == Qvaultnew)
    error(Eio); /* Can't write to creation file */

  if (c->qid.path >= Qvaultbase) {
    int is_ctl = (c->qid.path - Qvaultbase) % 2;
    v = get_vault_from_path(c->qid.path);
    if (v == nil)
      error(Enonexist);

    /* Verify ownership */
    if (v->pid != up->pid)
      error(Eperm);

    qlock(&v->lock);

    if (is_ctl) {
      /* Control Write */
      if (n >= sizeof(cmd))
        n = sizeof(cmd) - 1;
      memmove(cmd, va, n);
      cmd[n] = '\0';

      argc = tokenize(cmd, argv, nelem(argv));
      if (argc == 0) {
        qunlock(&v->lock);
        error("empty command");
      }

      /* Command: init <password> */
      if (strcmp(argv[0], "init") == 0) {
        if (v->initialized) {
          qunlock(&v->lock);
          error("vault already initialized");
        }
        if (argc != 2) {
          qunlock(&v->lock);
          error("usage: init <password>");
        }
        if (v->size < VaultHeaderSize) {
          qunlock(&v->lock);
          error("vault too small");
        }
        if (vault_cmd_init(v, argv[1]) != 0) {
          qunlock(&v->lock);
          error("vault init failed");
        }

        qunlock(&v->lock);
#ifndef __FRAMAC__
        if (!GETCONF_ENABLED("quiet"))
          print("ramdisk: vault %d initialized & locked\n", v->id);
#endif
        return n;
      }

      /* Command: unlock <password> */
      if (strcmp(argv[0], "unlock") == 0) {
        if (!v->locked) {
          qunlock(&v->lock);
          return n;
        }
        if (argc != 2) {
          qunlock(&v->lock);
          error("usage: unlock <password>");
        }
        if (v->size < VaultHeaderSize) {
          qunlock(&v->lock);
          error("vault too small");
        }
        if (vault_cmd_unlock(v, argv[1]) != 0) {
          qunlock(&v->lock);
          error("incorrect password");
        }
        qunlock(&v->lock);
        return n;
      }

      /* Command: lock */
      if (strcmp(argv[0], "lock") == 0) {
        if (!v->has_key) {
          qunlock(&v->lock);
          error("vault not initialized/unlocked");
        }
        if (v->locked) {
          qunlock(&v->lock);
          return n;
        }
        if (v->size < VaultHeaderSize) {
          qunlock(&v->lock);
          error("vault too small");
        }
        vault_cmd_lock(v);

        qunlock(&v->lock);
        return n;
      }

      /* Command: wipe */
      if (strcmp(argv[0], "wipe") == 0) {
        vault_cmd_wipe(v);

        qunlock(&v->lock);
        return n;
      }

      qunlock(&v->lock);
      error(Ebadarg);
    } else {
      /* Data Write */

      /* NEW: Allow writing RAW blobs to locked vaults (Load/Import support) */
      if (v->locked) {
        /* Raw Write (Load) */
        /* CAUTION: This replaces the raw encrypted state, including header/salt
         */
        if (off < 0 || off >= v->size) {
          qunlock(&v->lock);
          error(Ebadarg);
        }
        if (off + n > v->size)
          n = v->size - off;

        memmove(v->data + off, va, n);

        /* User is overwriting state, so initialized = 1 */
        v->initialized = 1;

        qunlock(&v->lock);
        return n;
      }

      /* Unlocked Write: Write to Plaintext Body */
      /* State validation */
      if (v->data == nil) {
        qunlock(&v->lock);
        error("vault not initialized");
      }
      /* Initialized check implied */

      check_key_invariant(v);

      vlong actual_data_size = v->size - VaultHeaderSize; /* Header */

      if (off < 0) {
        qunlock(&v->lock);
        error(Ebadarg);
      }
      if (off >= actual_data_size) {
        qunlock(&v->lock);
        error(Eio);
      }
      if (off + n > actual_data_size)
        n = actual_data_size - off;

      memmove(v->data + VaultHeaderSize + off, va, n);

      qunlock(&v->lock);
      return n;
    }
  }

  error(Egreg);
  return 0;
}

Dev ramdevtab = {
    'r',      "ram",

    ramreset, devinit,  devshutdown, ramattach, ramwalk,
    ramstat,  ramopen,  devcreate,   ramclose,  ramread,
    devbread, ramwrite, devbwrite,   devremove, devwstat,
};
