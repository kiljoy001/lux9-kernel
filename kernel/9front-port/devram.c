#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "monocypher.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

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
  Qsecureram,
  Qsecureramctl,
};

static Dirtab ramdir[] = {
    ".",
    {Qdir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "ram",
    {Qram},
    0,
    0666,
    "secureram",
    {Qsecureram},
    0,
    0600,
    "secureram.ctl",
    {Qsecureramctl},
    0,
    0600,
};

/* Standard Ramdisk */
static uchar *ramdisk_data;
static ulong ramdisk_size = 64 * 1024 * 1024; /* 64MB default */

/* Secure Vault Structure */
typedef struct SecureRamdisk {
  uchar *data;               /* Vault data (Pebble Black allocated) */
  ulong size;                /* Vault size in bytes */
  int locked;                /* 1 = locked (encrypted), 0 = unlocked */
  uchar master_key[32];      /* Derived from password via Argon2id */
  uchar salt[16];            /* Salt for password derivation */
  uchar nonce[24];           /* XChaCha20 nonce */
  UserCapability capability; /* Pebble Black capability (not handle) */
  int initialized;           /* 1 = password set, 0 = not initialized */
  int tpm_sealed;            /* 1 if TPM sealed blob is present */
  uchar tpm_blob[512];       /* Sealed blob */
  u16int tpm_blob_len;       /* Length of sealed blob */
} SecureRamdisk;

static SecureRamdisk secure_rd;

/* ========================================================================
 * Secure Wipe Implementation (DoD 5220.22-M)
 * ======================================================================== */

static void secure_wipe(uchar *data, ulong size) {
  ulong i;
  extern void genrandom(uchar * buf, int nbytes);
  extern void (*coherence)(void);

  if (data == nil || size == 0)
    return;

  if (!getconf("quiet"))
    print("ramdisk: wiping vault (7-pass)...\n");

  /* Pass 1: Write 0x00 */
  memset(data, 0x00, size);
  coherence();

  /* Pass 2: Write 0xFF */
  memset(data, 0xFF, size);
  coherence();

  /* Pass 3: Write random */
  for (i = 0; i < size; i += 256) {
    ulong chunk = (size - i) > 256 ? 256 : (size - i);
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
  for (i = 0; i < size; i += 256) {
    ulong chunk = (size - i) > 256 ? 256 : (size - i);
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
 * XChaCha20 Encryption/Decryption
 * ======================================================================== */

static void xchacha20_crypt(uchar *data, ulong size, uchar *key, uchar *nonce) {
  uint64_t ctr = 0;

  if (data == nil || size == 0 || key == nil || nonce == nil)
    return;

  /* XChaCha20 encryption/decryption (same operation) */
  crypto_chacha20_x(data, data, size, key, nonce, ctr);
}

/* ========================================================================
 * Device Reset and Initialization
 * ======================================================================== */

static void ramreset(void) {
  char *conf;

  /* 1. Setup Standard Ramdisk */
  ramdisk_data = xalloc(ramdisk_size);
  if (ramdisk_data == nil)
    panic("ramdisk: cannot allocate memory");

  memset(ramdisk_data, 0, ramdisk_size);
  if (!getconf("quiet"))
    print("ramdisk: %lud MB allocated\n", ramdisk_size / (1024 * 1024));

  /* 2. Setup Secure Vault */
  memset(&secure_rd, 0, sizeof(SecureRamdisk));
  secure_rd.locked = 1;      /* Start locked */
  secure_rd.initialized = 0; /* Not initialized */

  /* Check kernel config for vault size */
  if ((conf = getconf("secure.ramdisk.size")) != nil) {
    secure_rd.size = strtoul(conf, 0, 0);

    /* Parse size suffix (M/G/K) */
    char *p = conf;
    while (*p >= '0' && *p <= '9')
      p++;
    if (*p == 'M' || *p == 'm')
      secure_rd.size *= 1024 * 1024;
    else if (*p == 'G' || *p == 'g')
      secure_rd.size *= 1024 * 1024 * 1024;
    else if (*p == 'K' || *p == 'k')
      secure_rd.size *= 1024;
  } else {
    /* Default to 64MB */
    secure_rd.size = 64 * 1024 * 1024;
  }

  if (secure_rd.size > 0) {
    /* Allocate via Pebble Black for non-swappable backing */
    if (pebble_black_alloc(secure_rd.size, &secure_rd.capability) == 0) {
      /*
       * The actual backing memory address is accessed through the
       * Pebble Black lookup mechanism. The memory is allocated by
       * pebble_black_alloc() and stored in the PebbleBlack structure.
       * For a secure vault, we use xalloc directly as fallback since
       * proper Pebble integration requires white token verification.
       */
      PebbleState *ps = pebble_state();
      PebbleBlack *pb = pebble_lookup_black(ps, &secure_rd.capability);
      if (pb != nil && pb->physical_addr != nil) {
        secure_rd.data = pb->physical_addr;
      } else {
        /* Fallback: use xalloc if Pebble lookup fails */
        if (!getconf("quiet"))
          print("ramdisk: pebble lookup failed, using xalloc fallback\n");
        secure_rd.data = xalloc(secure_rd.size);
      }
    } else {
      /* Fallback: allocate via xalloc if Pebble not available */
      if (!getconf("quiet"))
        print("ramdisk: pebble_black_alloc failed, using xalloc fallback\n");
      secure_rd.data = xalloc(secure_rd.size);
    }

    if (secure_rd.data == nil) {
      if (!getconf("quiet"))
        print("ramdisk: failed to allocate secure vault\n");
      secure_rd.size = 0;
    } else {
      extern void genrandom(uchar * buf, int nbytes);

      /* Generate random salt and nonce */
      genrandom(secure_rd.salt, 16);
      genrandom(secure_rd.nonce, 24);

      /* Zero vault data */
      memset(secure_rd.data, 0, secure_rd.size);

      if (!getconf("quiet"))
        print("ramdisk: secure vault %lud MB allocated\n",
              secure_rd.size / (1024 * 1024));
    }
  }
}

static void raminit(void) { /* Nothing to do */ }

static Chan *ramattach(char *spec) { return devattach('r', spec); }

static Walkqid *ramwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, ramdir, nelem(ramdir), devgen);
}

static int ramstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, ramdir, nelem(ramdir), devgen);
}

static Chan *ramopen(Chan *c, int omode) {
  c = devopen(c, omode, ramdir, nelem(ramdir), devgen);
  c->offset = 0;
  return c;
}

static void ramclose(Chan *c) {
  /* Wipe vault on close if locked */
  if ((ulong)c->qid.path == Qsecureram && secure_rd.locked &&
      secure_rd.data != nil) {
    secure_wipe(secure_rd.data, secure_rd.size);
  }
}

/* ========================================================================
 * Read Operations
 * ======================================================================== */

long ramread(Chan *c, void *va, long n, vlong off) {
  char status[256];

  switch ((ulong)c->qid.path) {
  case Qdir:
    return devdirread(c, va, n, ramdir, nelem(ramdir), devgen);

  case Qram:
    /* Standard ramdisk */
    if (off < 0)
      error(Ebadarg);
    if (off >= ramdisk_size)
      return 0;
    if (off + n > ramdisk_size)
      n = ramdisk_size - off;

    memmove(va, ramdisk_data + off, n);
    return n;

  case Qsecureram:
    /* Secure vault data */
    if (secure_rd.data == nil)
      error("vault not initialized");
    if (secure_rd.locked)
      error("vault is locked");
    if (!secure_rd.initialized)
      error("vault not initialized");

    /* Bounds checking */
    if (off < 0)
      error(Ebadarg);
    if (off >= secure_rd.size)
      return 0;
    if (off + n > secure_rd.size)
      n = secure_rd.size - off;

    /* Read decrypted data */
    memmove(va, secure_rd.data + off, n);
    return n;

  case Qsecureramctl:
    /* Status query */
    snprint(status, sizeof(status), "status: %s\nsize: %lud\n",
            secure_rd.locked ? "locked" : "unlocked", secure_rd.size);
    return readstr(off, va, n, status);

  default:
    error(Egreg);
    return 0;
  }
}

/* ========================================================================
 * Write Operations
 * ======================================================================== */

long ramwrite(Chan *c, void *va, long n, vlong off) {
  char cmd[256];
  char *argv[3];
  int argc;

  switch ((ulong)c->qid.path) {
  case Qdir:
    error(Eperm);
    return 0;

  case Qram:
    /* Standard ramdisk */
    if (off < 0)
      error(Ebadarg);
    if (off >= ramdisk_size)
      error(Eio);
    if (off + n > ramdisk_size)
      n = ramdisk_size - off;

    memmove(ramdisk_data + off, va, n);
    return n;

  case Qsecureram:
    /* Secure vault data */
    if (secure_rd.data == nil)
      error("vault not initialized");
    if (secure_rd.locked)
      error("vault is locked");
    if (!secure_rd.initialized)
      error("vault not initialized");

    /* Bounds checking */
    if (off < 0)
      error(Ebadarg);
    if (off >= secure_rd.size)
      error(Eio);
    if (off + n > secure_rd.size)
      n = secure_rd.size - off;

    /* Write to unlocked vault */
    memmove(secure_rd.data + off, va, n);
    return n;

  case Qsecureramctl:
    /* Control interface */
    if (n >= sizeof(cmd))
      n = sizeof(cmd) - 1;
    memmove(cmd, va, n);
    cmd[n] = '\0';

    /* Parse command */
    argc = tokenize(cmd, argv, nelem(argv));
    if (argc == 0)
      error("empty command");

    /* Command: init <password> */
    if (strcmp(argv[0], "init") == 0) {
      if (secure_rd.initialized)
        error("vault already initialized");
      if (argc != 2)
        error("usage: init <password>");
      if (strlen(argv[1]) < 8)
        error("password must be at least 8 characters");

      /* Derive master key from password */
      if (derive_key_from_password(argv[1], secure_rd.salt,
                                   secure_rd.master_key) < 0)
        error("key derivation failed");

      secure_rd.initialized = 1;
      secure_rd.locked = 0; /* Unlock after init */

      /* Wipe command buffer */
      crypto_wipe(cmd, sizeof(cmd));

      if (!getconf("quiet"))
        print("ramdisk: vault initialized and unlocked\n");
      return n;
    }

    /* Command: unlock <password> */
    if (strcmp(argv[0], "unlock") == 0) {
      uchar derived_key[32];

      if (!secure_rd.initialized)
        error("vault not initialized");
      if (!secure_rd.locked)
        error("vault already unlocked");
      if (secure_rd.tpm_sealed)
        error("vault sealed to TPM - use tpmunlock");
      if (argc != 2)
        error("usage: unlock <password>");

      /* Derive key from password */
      if (derive_key_from_password(argv[1], secure_rd.salt, derived_key) < 0) {
        crypto_wipe(cmd, sizeof(cmd));
        error("key derivation failed");
      }

      /* Verify password (constant-time comparison) */
      if (crypto_verify32(secure_rd.master_key, derived_key) != 0) {
        crypto_wipe(derived_key, sizeof(derived_key));
        crypto_wipe(cmd, sizeof(cmd));
        error("incorrect password");
      }

      /* Decrypt vault with XChaCha20 */
      xchacha20_crypt(secure_rd.data, secure_rd.size, secure_rd.master_key,
                      secure_rd.nonce);

      secure_rd.locked = 0;

      /* Wipe temporary key and command buffer */
      crypto_wipe(derived_key, sizeof(derived_key));
      crypto_wipe(cmd, sizeof(cmd));

      if (!getconf("quiet"))
        print("ramdisk: vault unlocked and decrypted\n");
      return n;
    }

    /* Command: lock */
    if (strcmp(argv[0], "lock") == 0) {
      if (!secure_rd.initialized)
        error("vault not initialized");
      if (secure_rd.locked)
        error("vault already locked");

      /* Encrypt vault with XChaCha20 */
      xchacha20_crypt(secure_rd.data, secure_rd.size, secure_rd.master_key,
                      secure_rd.nonce);

      secure_rd.locked = 1;

      crypto_wipe(cmd, sizeof(cmd));
      if (!getconf("quiet"))
        print("ramdisk: vault locked and encrypted\n");
      return n;
    }

    /* Command: tpmseal (seal current or new master key to TPM SRK) */
    if (strcmp(argv[0], "tpmseal") == 0) {
      int rc;
      u16int blob_len = 0;
      if (secure_rd.size == 0 || secure_rd.data == nil)
        error("vault not initialized");
      /* If not initialized, create a random master key and mark initialized */
      if (!secure_rd.initialized) {
        extern void genrandom(uchar * buf, int nbytes);
        genrandom(secure_rd.master_key, sizeof(secure_rd.master_key));
        secure_rd.initialized = 1;
        secure_rd.locked = 0;
      }
      /* Seal master key to TPM SRK (no auth) */
      rc = tpm2_seal_to_srk(secure_rd.master_key, sizeof(secure_rd.master_key),
                            nil, 0, secure_rd.tpm_blob, &blob_len);
      if (rc < 0) {
        error("tpmseal failed");
      }
      secure_rd.tpm_blob_len = blob_len;
      secure_rd.tpm_sealed = 1;
      if (!getconf("quiet"))
        print("ramdisk: master key sealed to TPM (%d bytes)\n", blob_len);
      return n;
    }

    /* Command: tpmunlock (unseal master key and decrypt) */
    if (strcmp(argv[0], "tpmunlock") == 0) {
      u16int key_len = sizeof(secure_rd.master_key);
      int rc;
      if (!secure_rd.tpm_sealed)
        error("no TPM-sealed key present");
      if (!secure_rd.locked)
        error("vault already unlocked");
      rc = tpm2_unseal_from_blob(secure_rd.tpm_blob, secure_rd.tpm_blob_len,
                                 nil, 0, secure_rd.master_key, &key_len);
      if (rc < 0 || key_len != sizeof(secure_rd.master_key)) {
        error("tpmunlock failed");
      }
      /* Decrypt vault */
      xchacha20_crypt(secure_rd.data, secure_rd.size, secure_rd.master_key,
                      secure_rd.nonce);
      secure_rd.locked = 0;
      if (!getconf("quiet"))
        print("ramdisk: vault unlocked via TPM\n");
      return n;
    }

    /* Command: wipe */
    if (strcmp(argv[0], "wipe") == 0) {
      if (secure_rd.data == nil)
        error("vault not initialized");

      /* Secure wipe vault data */
      secure_wipe(secure_rd.data, secure_rd.size);

      /* Wipe master key */
      crypto_wipe(secure_rd.master_key, sizeof(secure_rd.master_key));
      crypto_wipe(secure_rd.salt, sizeof(secure_rd.salt));
      memset(secure_rd.tpm_blob, 0, sizeof(secure_rd.tpm_blob));
      secure_rd.tpm_blob_len = 0;
      secure_rd.tpm_sealed = 0;

      secure_rd.initialized = 0;
      secure_rd.locked = 1;

      crypto_wipe(cmd, sizeof(cmd));
      return n;
    }

    error("unknown command (init, unlock, lock, wipe)");
    return 0;

  default:
    error(Egreg);
    return 0;
  }
}

Dev ramdevtab = {
    'r',      "ram",

    ramreset, devinit,  devshutdown, ramattach, ramwalk,
    ramstat,  ramopen,  devcreate,   ramclose,  ramread,
    devbread, ramwrite, devbwrite,   devremove, devwstat,
};
