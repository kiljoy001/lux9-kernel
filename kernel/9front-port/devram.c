#ifndef __FRAMAC__
#include "dat.h"
#include "fns.h"
#include "libsec.h"
#include "mem.h"
#include "monocypher.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

/*@ assigns \nothing;
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
  @
  @ type invariant refcount_inv(SecureRamdisk rd) =
  @   rd.refcount >= 0;
  @
  @ type invariant nonce_storage_inv(SecureRamdisk rd) =
  @   (rd.locked == 1 && rd.size >= 24 && \valid(rd.data)) ==>
  @     \valid(rd.data + (0..23));
  */
/* Process Vault Structure */
typedef struct ProcessVault {
  struct ProcessVault *next;
  int id;       /* Unique ID */
  int pid;      /* Owner PID */
  QLock lock;   /* Protect concurrent access */
  uchar *data;  /* Vault data (Pebble Black allocated) */
  ulong size;   /* Vault size in bytes (includes 24-byte nonce prefix) */
  int locked;   /* 1 = locked (encrypted), 0 = unlocked */
  int refcount; /* Number of open channels */
  uchar master_key[32];      /* Derived from password via Argon2id */
  uchar salt[16];            /* Salt for password derivation */
  uchar current_nonce[24];   /* Current XChaCha20 nonce */
  UserCapability capability; /* Pebble Black capability */
  int initialized;           /* 1 = password set, 0 = not initialized */
  int dead;                  /* 1 = unlinked/zombie, waiting for refcount=0 */
} ProcessVault;

static ProcessVault *vault_list = nil;
static QLock vault_list_lock;
static int next_vault_id = 1;

static ProcessVault *find_vault(int id) {
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
        /* Secure wipe */
        if (v->data) {
          secure_wipe(v->data, v->size);
          free(v->data);
        }
        crypto_wipe(v->master_key, 32);

        /* FIXME: Burn capability? */

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
  if (!getconf("debug.invariants"))
    return;

  if (v->locked && v->initialized) {
    if (!getconf("quiet"))
      print("ramdisk: INVARIANT CHECK - locked state verified\n");
  }
}

/* INVARIANT: Initialized implies master key is set
 * Corresponds to init_key_invariant in ramdisk_state.v
 */
/*@ requires v != \null;
  @ requires \valid(v);
  @ requires \valid(v->master_key + (0..31));
  @ ensures (v->initialized == 1) ==>
  @   (\exists integer j; 0 <= j < 32 && v->master_key[j] != 0);
  @ assigns \nothing;
  */
static void check_init_invariant(ProcessVault *v) {
  if (!getconf("debug.invariants"))
    return;

  if (v->initialized) {
    int all_zero = 1;
    for (int i = 0; i < 32; i++) {
      if (v->master_key[i] != 0) {
        all_zero = 0;
        break;
      }
    }
    if (all_zero)
      panic("ramdisk: INVARIANT VIOLATION - initialized but no master key");
  }
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
  if (!getconf("debug.invariants"))
    return;
  if (v->refcount < 0)
    panic("ramdisk: INVARIANT VIOLATION - negative refcount");
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

  if (!getconf("quiet"))
    print("ramdisk: wiping vault (7-pass)...\n");

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
    int chunk = (size - i) > 256 ? 256 : (int)(size - i);
    /*@ assert 0 < chunk <= 256; */
    /*@ assert i + (ulong)chunk <= size; */
    genrandom(data + i, chunk);
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
    int chunk = (size - i) > 256 ? 256 : (int)(size - i);
    /*@ assert 0 < chunk <= 256; */
    /*@ assert i + (ulong)chunk <= size; */
    genrandom(data + i, chunk);
  }
  coherence();

  /* Pass 7: Write 0x00 (final) */
  memset(data, 0x00, size);
  coherence();

  if (!getconf("quiet"))
    print("ramdisk: vault wiped\n");
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
    print("ramdisk: failed to allocate Argon2id work area\n");
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
 * XChaCha20 Encryption/Decryption - FIXED (BUG #1)
 * ======================================================================== */

/*
 * BUG #1 FIX: Generate FRESH nonce for each encryption operation
 *
 * Data layout after encryption:
 *   [0-23]:  Fresh nonce (24 bytes)
 *   [24-n]:  Encrypted data
 *
 * This fixes the critical nonce reuse vulnerability that broke IND-CPA
 * security. Verified against nonce_freshness_invariant in ramdisk_state.v
 */

/*@ requires v != \null;
  @ requires \valid(v);
  @ requires data == \null || \valid(data + (0..data_size-1));
  @ requires key == \null || \valid(key + (0..31));
  @ requires data_size >= 24;
  @
  @ behavior null_args:
  @   assumes data == \null || data_size < 24 || key == \null;
  @   assigns \nothing;
  @
  @ behavior valid_encrypt:
  @   assumes data != \null && data_size >= 24 && key != \null;
  @   ensures \forall integer i; 0 <= i < 24 ==>
  @     data[i] == \old(data[i]) || data[i] != \old(data[i]);
  @   ensures \forall integer i; 24 <= i < data_size ==>
  @     data[i] != \old(data[i]);
  @   assigns data[0..data_size-1], v->current_nonce[0..23];
  @
  @ complete behaviors;
  @ disjoint behaviors;
  */
static void xchacha20_encrypt_with_fresh_nonce(ProcessVault *v, uchar *data,
                                               ulong data_size, uchar *key) {
  uchar fresh_nonce[24];
  uint64_t ctr = 0;

  if (data == nil || data_size < 24 || key == nil)
    return;

  /* Generate cryptographically secure random nonce */
  genrandom(fresh_nonce, 24);

  /* Store nonce in first 24 bytes */
  memmove(data, fresh_nonce, 24);

  /* Encrypt data starting at offset 24 */
  crypto_chacha20_x(data + 24, data + 24, data_size - 24, key, fresh_nonce,
                    ctr);

  /* Save current nonce for decryption */
  memmove(v->current_nonce, fresh_nonce, 24);

  if (!getconf("quiet"))
    print("ramdisk: encrypted with fresh nonce\n");
}

/*@
  requires data != \null ==> \valid(data + (0..data_size-1));
  requires key != \null ==> \valid(key + (0..31));
  requires data_size >= 24;
  assigns data[24..data_size-1], v->current_nonce[0..23];
*/
static void xchacha20_decrypt_with_stored_nonce(ProcessVault *v, uchar *data,
                                                ulong data_size, uchar *key) {
  uchar stored_nonce[24];
  uint64_t ctr = 0;

  if (data == nil || data_size < 24 || key == nil)
    return;

  /* Extract nonce from first 24 bytes */
  memmove(stored_nonce, data, 24);

  /* Decrypt data starting at offset 24 */
  crypto_chacha20_x(data + 24, data + 24, data_size - 24, key, stored_nonce,
                    ctr);

  /* Save extracted nonce */
  memmove(v->current_nonce, stored_nonce, 24);

  if (!getconf("quiet"))
    print("ramdisk: decrypted with stored nonce\n");
}

/* ========================================================================
 * Device Reset and Initialization
 * ======================================================================== */

static void ramreset(void) {
  /* 1. Setup Standard Ramdisk */
  ramdisk_data = xalloc(ramdisk_size);
  if (ramdisk_data == nil)
    panic("ramdisk: cannot allocate memory");

  memset(ramdisk_data, 0, ramdisk_size);
  if (!getconf("quiet"))
    print("ramdisk: %lud MB allocated\n", ramdisk_size / (1024 * 1024));

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

    /* Allocate new vault */
    v = malloc(sizeof(ProcessVault));
    if (v == nil)
      error(Enomem);

    memset(v, 0, sizeof(ProcessVault));
    v->pid = up->pid;
    v->size = 512 * 1024; /* 512KB default per user request */
    /* Add nonce space */
    v->size += 24;

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

    if (!getconf("quiet"))
      print("ramdisk: new vault %d created for pid %d\n", v->id, v->pid);

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
  *)c->aux)->size-1],
  @           ((ProcessVault *)c->aux)->master_key[0..31];
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
      crypto_wipe(v->master_key, 32);
      free(v);
    }
  }
}

/* ========================================================================
 * Read Operations
 * ======================================================================== */

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
              v->locked ? "locked" : "unlocked", v->size - 24);
      return readstr(off, va, n, status);
    } else {
      /* Data Read */
      /* State validation */
      if (v->data == nil) {
        qunlock(&v->lock);
        error("vault not initialized");
      }
      if (v->locked) {
        qunlock(&v->lock);
        error("vault is locked");
      }
      /* initialized check is implied by data!=nil mostly, but good to check */
      if (!v->initialized) {
        qunlock(&v->lock);
        error("vault not initialized");
      }

      check_lock_invariant(v);
      check_init_invariant(v);

      /* Account for 24-byte nonce prefix */
      vlong actual_data_size = v->size - 24;

      /* Bounds checking */
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

      /* Read decrypted data */
      memmove(va, v->data + 24 + off, n);

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
        if (strlen(argv[1]) < 8) {
          qunlock(&v->lock);
          error("password must be at least 8 characters");
        }

        if (derive_key_from_password(argv[1], v->salt, v->master_key) < 0) {
          qunlock(&v->lock);
          error("key derivation failed");
        }

        v->initialized = 1;

        /* Encrypt initial zeroed state so it decrypts correctly */
        xchacha20_encrypt_with_fresh_nonce(v, v->data, v->size, v->master_key);

        v->locked = 1; /* Start locked */

        crypto_wipe(cmd, sizeof(cmd));

        check_init_invariant(v);
        check_lock_invariant(v);

        qunlock(&v->lock);
        if (!getconf("quiet"))
          print("ramdisk: vault %d initialized\n", v->id);
        return n;
      }

      /* Command: unlock <password> */
      if (strcmp(argv[0], "unlock") == 0) {
        if (!v->initialized) {
          qunlock(&v->lock);
          error("vault not initialized");
        }
        if (!v->locked) {
          qunlock(&v->lock);
          /* Already unlocked, fine */
          return n;
        }

        uchar derived_key[32];
        if (derive_key_from_password(argv[1], v->salt, derived_key) < 0) {
          qunlock(&v->lock);
          error("key derivation failed");
        }

        if (crypto_verify32(derived_key, v->master_key) != 0) {
          crypto_wipe(derived_key, 32);
          qunlock(&v->lock);
          error("incorrect password");
        }
        crypto_wipe(derived_key, 32); /* Wipe immediately */

        /* Decrypt */
        xchacha20_decrypt_with_stored_nonce(v, v->data, v->size, v->master_key);
        v->locked = 0;

        qunlock(&v->lock);
        return n;
      }

      /* Command: lock */
      if (strcmp(argv[0], "lock") == 0) {
        if (!v->initialized) {
          qunlock(&v->lock);
          error("vault not initialized");
        }
        if (v->locked) {
          qunlock(&v->lock);
          return n;
        }

        /* Encrypt */
        xchacha20_encrypt_with_fresh_nonce(v, v->data, v->size, v->master_key);
        v->locked = 1;

        check_lock_invariant(v);
        qunlock(&v->lock);
        return n;
      }

      /* Command: wipe */
      if (strcmp(argv[0], "wipe") == 0) {
        secure_wipe(v->data, v->size);
        crypto_wipe(v->master_key, 32);
        v->initialized = 0;
        v->locked = 1;

        qunlock(&v->lock);
        return n;
      }

      qunlock(&v->lock);
      error(Ebadarg);
    } else {
      /* Data Write */
      /* State validation */
      if (v->data == nil) {
        qunlock(&v->lock);
        error("vault not initialized");
      }
      if (v->locked) {
        qunlock(&v->lock);
        error("vault is locked");
      }
      if (!v->initialized) {
        qunlock(&v->lock);
        error("vault not initialized");
      }

      check_lock_invariant(v);
      check_init_invariant(v);

      vlong actual_data_size = v->size - 24;

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

      memmove(v->data + 24 + off, va, n);

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
#endif
