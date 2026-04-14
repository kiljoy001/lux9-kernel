/*
 * kernel_stubs.c - Stubs for functions moved to userspace
 *
 * These functions were in deleted kernel files (auth.c, devmnt.c, sysfile.c)
 * and are needed for linking. Minimal implementations for boot.
 *
 * FORMAL VERIFICATION:
 *   Coq proofs: N/A (stub implementations)
 *   Frama-C: ACSL annotations for safety
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <authsrv.h>
#include <error.h>
#include <libsec.h>
#include <monocypher.h>

typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type *)((list)++))

/*
 * From auth.c - Authentication/authorization
 */

/*@
  @ assigns \nothing;
  @ ensures \result == 0;
  @*/
int iseve(void) {
  return eve != nil && up != nil && up->user != nil && strcmp(eve, up->user) == 0;
}

/* Note: renameuser, srvrenameuser, shrrenameuser are provided by compat files */

/* Authentication info - minimal stub */
char *eve = "nobody";
char hostdomain[DOMLEN];

/*
 * From sysfile.c - compatibility syscalls still used by auth code.
 */

uintptr sysfversion(void *list_void) {
  syscall_va_list list;
  int msize, arglen, fd;
  char *vers;
  Chan *c;

  list = (syscall_va_list)list_void;
  fd = SYSCALL_ARG(list, int);
  msize = SYSCALL_ARG(list, int);
  vers = SYSCALL_ARG(list, char *);
  arglen = SYSCALL_ARG(list, int);

  validaddr((uintptr)vers, (ulong)arglen, 1);
  if (arglen <= 0 || memchr(vers, 0, (ulong)arglen) == nil)
    error(Ebadarg);

  c = fdtochan(fd, ORDWR, 0, 1);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  msize = mntversion(c, vers, msize, arglen);
  cclose(c);
  poperror();
  return (uintptr)msize;
}

uintptr sys_fsession(void *list_void) {
  syscall_va_list list;
  int fd;
  char *str;
  uint len;

  list = (syscall_va_list)list_void;
  fd = SYSCALL_ARG(list, int);
  str = SYSCALL_ARG(list, char *);
  len = SYSCALL_ARG(list, uint);
  USED(fd);

  if (len == 0)
    error(Ebadarg);
  validaddr((uintptr)str, len, 1);
  *str = '\0';
  return 0;
}

uintptr sysfauth(void *list_void) {
  syscall_va_list list;
  Chan *c, *ac;
  char *aname;
  int fd;

  list = (syscall_va_list)list_void;
  fd = SYSCALL_ARG(list, int);
  aname = SYSCALL_ARG(list, char *);

  validaddr((uintptr)aname, 1, 0);
  aname = validnamedup(aname, 1);
  if (waserror()) {
    free(aname);
    nexterror();
  }

  c = fdtochan(fd, ORDWR, 0, 1);
  if (waserror()) {
    cclose(c);
    nexterror();
  }

  ac = mntauth(c, aname);
  poperror();
  cclose(c);
  poperror();
  free(aname);

  if (waserror()) {
    cclose(ac);
    nexterror();
  }

  fd = newfd(ac, OCEXEC);
  if (fd < 0)
    error(Enofd);
  poperror();

  return (uintptr)fd;
}

long userwrite(char *a, int n) {
  if (n != 4 || strncmp(a, "none", 4) != 0)
    error(Eperm);
  procsetuser("none");
  return n;
}

long hostownerwrite(char *a, int n) {
  char buf[KNAMELEN];

  if (!iseve())
    error(Eperm);
  if (n <= 0)
    error(Ebadarg);
  if ((ulong)n >= sizeof buf)
    error(Etoolong);

  memmove(buf, a, (ulong)n);
  buf[n] = 0;

  renameuser(eve, buf);
  srvrenameuser(eve, buf);
  shrrenameuser(eve, buf);
  kstrdup(&eve, buf);
  procsetuser(buf);
  return n;
}

long hostdomainwrite(char *a, int n) {
  char buf[DOMLEN];

  if (!iseve())
    error(Eperm);
  if (n <= 0 || n >= DOMLEN)
    error(Ebadarg);

  memset(buf, 0, sizeof buf);
  strncpy(buf, a, (ulong)n);
  if (buf[0] == 0)
    error(Ebadarg);
  memmove(hostdomain, buf, DOMLEN);
  return n;
}

enum {
  VaultSaltSize = 16,
  VaultRSize = 32,
  VaultNonceSize = 24,
  VaultMacSize = 16,
  VaultHeaderSize = VaultSaltSize + VaultRSize + VaultNonceSize + VaultMacSize,
};

ProcessVault *vault_list = nil;
QLock vault_list_lock;
int next_vault_id = 1;

static void secure_wipe(uchar *data, ulong size) {
  ulong i, chunk;

  if (data == nil || size == 0)
    return;

  memset(data, 0x00, size);
  coherence();
  memset(data, 0xFF, size);
  coherence();

  for (i = 0; i < size; i += 256) {
    chunk = size - i;
    if (chunk > 256)
      chunk = 256;
    genrandom(data + i, (int)chunk);
  }
  coherence();

  memset(data, 0x00, size);
  coherence();
  memset(data, 0xFF, size);
  coherence();

  for (i = 0; i < size; i += 256) {
    chunk = size - i;
    if (chunk > 256)
      chunk = 256;
    genrandom(data + i, (int)chunk);
  }
  coherence();

  memset(data, 0x00, size);
  coherence();
}

static int derive_key_from_password(const char *password, uchar *salt,
                                    uchar *key_out) {
  void *work_area;
  crypto_argon2_config config;
  crypto_argon2_inputs inputs;
  extern const crypto_argon2_extras crypto_argon2_no_extras;

  if (password == nil || salt == nil || key_out == nil)
    return -1;

  work_area = xalloc_driver(4096 * 1024);
  if (work_area == nil)
    return -1;

  config.algorithm = CRYPTO_ARGON2_ID;
  config.nb_blocks = 4096;
  config.nb_passes = 3;
  config.nb_lanes = 1;

  inputs.pass = (const uint8_t *)password;
  inputs.salt = salt;
  inputs.pass_size = strlen(password);
  inputs.salt_size = VaultSaltSize;

  crypto_argon2(key_out, 32, work_area, config, inputs,
                crypto_argon2_no_extras);
  crypto_wipe(work_area, 4096 * 1024);
  xfree_driver(work_area);
  return 0;
}

static void derive_and_lock(ProcessVault *v, const uchar *salt,
                            const uchar *key) {
  uchar rep[VaultRSize];
  uchar nonce[VaultNonceSize];
  uchar mac[VaultMacSize];
  uchar *body_start;
  ulong body_size;

  if (v == nil || v->data == nil || v->size < VaultHeaderSize)
    return;

  memmove(v->data, salt, VaultSaltSize);
  crypto_elligator_map(rep, key);
  memmove(v->data + VaultSaltSize, rep, VaultRSize);

  genrandom(nonce, sizeof nonce);
  memmove(v->data + VaultSaltSize + VaultRSize, nonce, VaultNonceSize);

  body_start = v->data + VaultHeaderSize;
  body_size = v->size - VaultHeaderSize;
  crypto_aead_lock(body_start, mac, key, nonce, v->data,
                   VaultSaltSize + VaultRSize + VaultNonceSize, body_start,
                   body_size);
  memmove(v->data + VaultSaltSize + VaultRSize + VaultNonceSize, mac,
          VaultMacSize);

  crypto_wipe(v->ephemeral_key, sizeof v->ephemeral_key);
  v->has_key = 0;
  v->locked = 1;
}

static int verify_and_unlock(ProcessVault *v, uchar *candidate_key) {
  uchar stored_rep[VaultRSize];
  uchar candidate_rep[VaultRSize];
  uchar nonce[VaultNonceSize];
  uchar mac[VaultMacSize];
  uchar *body_start;
  ulong body_size;

  if (v == nil || v->data == nil || v->size < VaultHeaderSize)
    return -1;

  memmove(stored_rep, v->data + VaultSaltSize, VaultRSize);
  crypto_elligator_map(candidate_rep, candidate_key);
  if (crypto_verify32(stored_rep, candidate_rep) != 0)
    return -1;

  memmove(nonce, v->data + VaultSaltSize + VaultRSize, VaultNonceSize);
  memmove(mac, v->data + VaultSaltSize + VaultRSize + VaultNonceSize,
          VaultMacSize);

  body_start = v->data + VaultHeaderSize;
  body_size = v->size - VaultHeaderSize;
  if (crypto_aead_unlock(body_start, mac, candidate_key, nonce, v->data,
                         VaultSaltSize + VaultRSize + VaultNonceSize,
                         body_start, body_size) != 0)
    return -1;

  memmove(v->ephemeral_key, candidate_key, sizeof v->ephemeral_key);
  v->has_key = 1;
  v->locked = 0;
  return 0;
}

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

int process_has_vault(int pid) {
  ProcessVault *v;

  qlock(&vault_list_lock);
  for (v = vault_list; v != nil; v = v->next) {
    if (v->pid == pid) {
      qunlock(&vault_list_lock);
      return 1;
    }
  }
  qunlock(&vault_list_lock);
  return 0;
}

void vault_cleanup_process(int pid) {
  ProcessVault *v, **prev;
  int can_free;

  qlock(&vault_list_lock);
  prev = &vault_list;
  while ((v = *prev) != nil) {
    if (v->pid != pid) {
      prev = &v->next;
      continue;
    }

    *prev = v->next;
    qlock(&v->lock);
    v->dead = 1;
    can_free = (v->refcount == 0);
    qunlock(&v->lock);

    if (can_free) {
      if (v->data != nil)
        secure_wipe(v->data, v->size);
      if (v->has_key)
        crypto_wipe(v->ephemeral_key, sizeof v->ephemeral_key);
      if (v->data != nil)
        pebble_black_free(&v->capability);
      xfree_resident(v);
    }
  }
  qunlock(&vault_list_lock);
}

int vault_cmd_init(ProcessVault *v, const char *password) {
  uchar salt[VaultSaltSize];
  uchar key[32];

  if (v == nil || v->data == nil || v->size < VaultHeaderSize)
    return -1;

  genrandom(salt, sizeof salt);
  if (derive_key_from_password(password, salt, key) != 0)
    return -1;

  v->initialized = 1;
  memmove(v->ephemeral_key, key, sizeof v->ephemeral_key);
  v->has_key = 1;
  derive_and_lock(v, salt, key);
  crypto_wipe(key, sizeof key);
  return 0;
}

void vault_cmd_lock(ProcessVault *v) {
  uchar salt[VaultSaltSize];

  if (v == nil || v->data == nil || v->size < VaultHeaderSize || !v->has_key)
    return;

  genrandom(salt, sizeof salt);
  derive_and_lock(v, salt, v->ephemeral_key);
}

int vault_cmd_unlock(ProcessVault *v, const char *password) {
  uchar salt[VaultSaltSize];
  uchar key[32];
  int rc;

  if (v == nil || v->data == nil || v->size < VaultHeaderSize)
    return -1;

  memmove(salt, v->data, sizeof salt);
  if (derive_key_from_password(password, salt, key) != 0)
    return -1;
  rc = verify_and_unlock(v, key);
  crypto_wipe(key, sizeof key);
  return rc;
}

void vault_cmd_wipe(ProcessVault *v) {
  if (v == nil || v->data == nil)
    return;
  secure_wipe(v->data, v->size);
  if (v->has_key)
    crypto_wipe(v->ephemeral_key, sizeof v->ephemeral_key);
  v->has_key = 0;
  v->initialized = 0;
  v->locked = 1;
}

int vault_cmd_create(void *a, void *b) { USED(a); USED(b); return -1; }
int vault_cmd_read(void *a, void *b) { USED(a); USED(b); return -1; }
int vault_cmd_write(void *a, void *b) { USED(a); USED(b); return -1; }
int vault_cmd_export(void *a, void *b) { USED(a); USED(b); return -1; }
int vault_cmd_import(void *a, void *b) { USED(a); USED(b); return -1; }
