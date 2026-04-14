/*
 * Process Vault Syscall Handlers
 *
 * Implementation of vault syscalls (210-219) via Tsyscall messages.
 * These handlers satisfy the ACSL contracts in vault_syscall_acsl_spec.h
 */

#include "kernel.h"
#include "libsec.h"
#include "monocypher.h"

static void vault_free_data(ProcessVault *v) {
  if (v == nil || v->data == nil)
    return;
  pebble_black_free(&v->capability);
  v->data = nil;
}

/* Vault helpers from devram.c */
extern ProcessVault *find_vault(int id);
extern int process_has_vault(int pid);
extern void vault_cmd_lock(ProcessVault *v);
extern int vault_cmd_unlock(ProcessVault *v, const char *password);
extern int vault_cmd_init(ProcessVault *v, const char *password);
extern void vault_cmd_wipe(ProcessVault *v);

extern ProcessVault *vault_list;
extern int next_vault_id;
extern QLock vault_list_lock;

#define VAULT_HEADER_SIZE 88

/*
 * SYS_VAULT_CREATE (210) - Create and initialize vault
 *
 * Payload: [size:8][pwd_len:4][password:N]
 * Returns: vault_id in retval
 */
int sys_vault_create_handler(Fcall *tx, Fcall *rx) {
  ulong size;
  u32int pwd_len;
  char *password;
  ProcessVault *v;

  /* Parse payload */
  if (tx->scount < 12) {
    rx->type = Rerror;
    rx->ename = "invalid create payload";
    return -1;
  }

  size = GBIT64(tx->sdata);
  pwd_len = GBIT32(tx->sdata + 8);

  if (pwd_len == 0 || pwd_len >= 256 || tx->scount < 12 + pwd_len) {
    rx->type = Rerror;
    rx->ename = "invalid password length";
    return -1;
  }

  if (size < 4096 || size > 512 * 1024) {
    rx->type = Rerror;
    rx->ename = "invalid vault size";
    return -1;
  }

  /* Check if process already has vault */
  if (process_has_vault(up->pid)) {
    rx->type = Rerror;
    rx->ename = "process already has vault";
    return -1;
  }

  /* Allocate vault structure (using existing code from devram.c) */
  v = xalloc_resident(sizeof(ProcessVault));
  if (v == nil) {
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memset(v, 0, sizeof(ProcessVault));
  v->pid = up->pid;
  v->size = size + VAULT_HEADER_SIZE;

  /* Allocate vault data via Pebble */
  extern int pebble_alloc_with_white(ulong, UserCapability *, void **);
  if (pebble_alloc_with_white(v->size, &v->capability, (void **)&v->data) < 0) {
    xfree_resident(v);
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memset(v->data, 0, v->size);

  /* Copy password to temporary buffer (null-terminated) */
  password = malloc(pwd_len + 1);
  if (password == nil) {
    vault_free_data(v);
    xfree_resident(v);
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memmove(password, tx->sdata + 12, pwd_len);
  password[pwd_len] = '\0';

  /* Initialize vault with password */
  if (vault_cmd_init(v, password) != 0) {
    crypto_wipe((uchar *)password, pwd_len + 1);
    free(password);
    vault_free_data(v);
    xfree_resident(v);
    rx->type = Rerror;
    rx->ename = "vault initialization failed";
    return -1;
  }

  crypto_wipe((uchar *)password, pwd_len + 1);
  free(password);

  /* Add to global vault list */
  qlock(&vault_list_lock);
  v->id = next_vault_id++;
  v->next = vault_list;
  vault_list = v;
  v->refcount = 0; /* Not opened yet */
  v->locked = 1;   /* Created in locked state */
  qunlock(&vault_list_lock);

  /* Success */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = v->id;
  rx->scount = 0;
  rx->sdata = nil;

  return 0;
}

/*
 * SYS_VAULT_LOCK (211) - Lock (encrypt) unlocked vault
 *
 * Payload: [vault_id:4]
 * Returns: 0 on success
 */
int sys_vault_lock_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  ProcessVault *v;

  if (tx->scount < 4) {
    rx->type = Rerror;
    rx->ename = "invalid lock payload";
    return -1;
  }

  vault_id = GBIT32(tx->sdata);
  v = find_vault(vault_id);

  if (v == nil) {
    rx->type = Rerror;
    rx->ename = "vault not found";
    return -1;
  }

  if (v->pid != up->pid) {
    rx->type = Rerror;
    rx->ename = Eperm;
    return -1;
  }

  qlock(&v->lock);

  /* Idempotent: if already locked, success */
  if (v->locked) {
    qunlock(&v->lock);
    rx->type = Rsyscall;
    rx->tag = tx->tag;
    rx->retval = 0;
    return 0;
  }

  /* Lock the vault */
  vault_cmd_lock(v);

  qunlock(&v->lock);

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->scount = 0;
  rx->sdata = nil;

  return 0;
}

/*
 * SYS_VAULT_UNLOCK (212) - Unlock (decrypt) locked vault
 *
 * Payload: [vault_id:4][pwd_len:4][password:N]
 * Returns: 0 on success
 */
int sys_vault_unlock_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  u32int pwd_len;
  char *password;
  ProcessVault *v;
  int result;

  if (tx->scount < 8) {
    rx->type = Rerror;
    rx->ename = "invalid unlock payload";
    return -1;
  }

  vault_id = GBIT32(tx->sdata);
  pwd_len = GBIT32(tx->sdata + 4);

  if (pwd_len == 0 || pwd_len >= 256 || tx->scount < 8 + pwd_len) {
    rx->type = Rerror;
    rx->ename = "invalid password length";
    return -1;
  }

  v = find_vault(vault_id);
  if (v == nil) {
    rx->type = Rerror;
    rx->ename = "vault not found";
    return -1;
  }

  if (v->pid != up->pid) {
    rx->type = Rerror;
    rx->ename = Eperm;
    return -1;
  }

  qlock(&v->lock);

  /* Idempotent: if already unlocked, success */
  if (!v->locked) {
    qunlock(&v->lock);
    rx->type = Rsyscall;
    rx->tag = tx->tag;
    rx->retval = 0;
    return 0;
  }

  /* Copy password to temporary buffer */
  password = malloc(pwd_len + 1);
  if (password == nil) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memmove(password, tx->sdata + 8, pwd_len);
  password[pwd_len] = '\0';

  /* Attempt unlock */
  result = vault_cmd_unlock(v, password);

  crypto_wipe((uchar *)password, pwd_len + 1);
  free(password);

  qunlock(&v->lock);

  if (result != 0) {
    rx->type = Rerror;
    rx->ename = "incorrect password";
    return -1;
  }

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->scount = 0;
  rx->sdata = nil;

  return 0;
}

/*
 * SYS_VAULT_STATUS (218) - Query vault state
 *
 * Payload: [vault_id:4] (-1 = check if process has any vault)
 * Returns: [exists:1][vault_id:4][locked:1][initialized:1][size:8][data_size:8]
 */
int sys_vault_status_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  ProcessVault *v = nil;
  uchar *resp;

  if (tx->scount < 4) {
    rx->type = Rerror;
    rx->ename = "invalid status payload";
    return -1;
  }

  vault_id = (int)GBIT32(tx->sdata);

  /* Allocate response buffer: 1+4+1+1+8+8 = 23 bytes */
  resp = malloc(23);
  if (resp == nil) {
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memset(resp, 0, 23);

  /* vault_id == -1 means "any vault for this process" */
  if (vault_id == -1) {
    qlock(&vault_list_lock);
    for (v = vault_list; v != nil; v = v->next) {
      if (v->pid == up->pid) {
        vault_id = v->id;
        break;
      }
    }
    qunlock(&vault_list_lock);
  } else {
    v = find_vault(vault_id);
    if (v != nil && v->pid != up->pid)
      v = nil; /* Not our vault */
  }

  if (v != nil) {
    qlock(&v->lock);

    /* exists = 1 */
    resp[0] = 1;

    /* vault_id */
    PBIT32(resp + 1, v->id);

    /* locked */
    resp[5] = v->locked ? 1 : 0;

    /* initialized */
    resp[6] = v->initialized ? 1 : 0;

    /* total size */
    PBIT64(resp + 7, v->size);

    /* data size (minus header) */
    PBIT64(resp + 15,
           v->size >= VAULT_HEADER_SIZE ? v->size - VAULT_HEADER_SIZE : 0);

    qunlock(&v->lock);
  } else {
    /* exists = 0, vault_id = -1 */
    resp[0] = 0;
    PBIT32(resp + 1, (u32int)-1);
  }

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->sdata = resp;
  rx->scount = 23;

  return 0;
}

/*
 * SYS_VAULT_READ (213) - Read from unlocked vault
 *
 * Payload: [vault_id:4][offset:8][count:4]
 * Returns: data in sdata, actual bytes read in scount
 */
int sys_vault_read_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  vlong offset;
  u32int count;
  ProcessVault *v;
  uchar *buf;
  ulong data_size, actual_count;

  if (tx->scount < 16) {
    rx->type = Rerror;
    rx->ename = "invalid read payload";
    return -1;
  }

  vault_id = GBIT32(tx->sdata);
  offset = GBIT64(tx->sdata + 4);
  count = GBIT32(tx->sdata + 12);

  v = find_vault(vault_id);
  if (v == nil) {
    rx->type = Rerror;
    rx->ename = "vault not found";
    return -1;
  }

  if (v->pid != up->pid) {
    rx->type = Rerror;
    rx->ename = Eperm;
    return -1;
  }

  qlock(&v->lock);

  if (v->locked) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = "vault is locked";
    return -1;
  }

  data_size = v->size >= VAULT_HEADER_SIZE ? v->size - VAULT_HEADER_SIZE : 0;

  if (offset < 0 || offset >= data_size) {
    qunlock(&v->lock);
    rx->type = Rsyscall;
    rx->tag = tx->tag;
    rx->scount = 0;
    rx->sdata = nil;
    return 0; /* EOF */
  }

  /* Calculate actual bytes to read */
  actual_count = count;
  if (offset + actual_count > data_size)
    actual_count = data_size - offset;

  if (actual_count > 8192) /* Reasonable limit */
    actual_count = 8192;

  buf = malloc(actual_count);
  if (buf == nil) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memmove(buf, v->data + VAULT_HEADER_SIZE + offset, actual_count);

  qunlock(&v->lock);

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->sdata = buf;
  rx->scount = actual_count;

  return 0;
}

/*
 * SYS_VAULT_WRITE (214) - Write to unlocked vault
 *
 * Payload: [vault_id:4][offset:8][count:4][data:N]
 * Returns: bytes written in retval
 */
int sys_vault_write_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  vlong offset;
  u32int count;
  ProcessVault *v;
  ulong data_size;

  if (tx->scount < 16) {
    rx->type = Rerror;
    rx->ename = "invalid write payload";
    return -1;
  }

  vault_id = GBIT32(tx->sdata);
  offset = GBIT64(tx->sdata + 4);
  count = GBIT32(tx->sdata + 12);

  if (tx->scount < 16 + count) {
    rx->type = Rerror;
    rx->ename = "insufficient data";
    return -1;
  }

  v = find_vault(vault_id);
  if (v == nil) {
    rx->type = Rerror;
    rx->ename = "vault not found";
    return -1;
  }

  if (v->pid != up->pid) {
    rx->type = Rerror;
    rx->ename = Eperm;
    return -1;
  }

  qlock(&v->lock);

  if (v->locked) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = "vault is locked";
    return -1;
  }

  data_size = v->size >= VAULT_HEADER_SIZE ? v->size - VAULT_HEADER_SIZE : 0;

  if (offset < 0 || offset + count > data_size) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = "write out of bounds";
    return -1;
  }

  memmove(v->data + VAULT_HEADER_SIZE + offset, tx->sdata + 16, count);

  qunlock(&v->lock);

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = count;
  rx->scount = 0;
  rx->sdata = nil;

  return 0;
}

/*
 * SYS_VAULT_WIPE (215) - Securely wipe vault
 *
 * Payload: [vault_id:4]
 * Returns: 0 on success
 */
int sys_vault_wipe_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  ProcessVault *v, **prev;

  if (tx->scount < 4) {
    rx->type = Rerror;
    rx->ename = "invalid wipe payload";
    return -1;
  }

  vault_id = GBIT32(tx->sdata);

  qlock(&vault_list_lock);

  prev = &vault_list;
  while ((v = *prev) != nil) {
    if (v->id == vault_id && v->pid == up->pid) {
      /* Unlink from list */
      *prev = v->next;
      qunlock(&vault_list_lock);

      /* Wipe vault */
      qlock(&v->lock);
      vault_cmd_wipe(v);

      /* Free resources */
      vault_free_data(v);
      qunlock(&v->lock);

      xfree_resident(v);

      rx->type = Rsyscall;
      rx->tag = tx->tag;
      rx->retval = 0;
      return 0;
    }
    prev = &v->next;
  }

  qunlock(&vault_list_lock);

  rx->type = Rerror;
  rx->ename = "vault not found";
  return -1;
}

/*
 * SYS_VAULT_EXPORT (216) - Export locked vault as encrypted blob
 *
 * Payload: [vault_id:4]
 * Returns: encrypted vault blob in sdata
 */
int sys_vault_export_handler(Fcall *tx, Fcall *rx) {
  int vault_id;
  ProcessVault *v;
  uchar *blob;

  if (tx->scount < 4) {
    rx->type = Rerror;
    rx->ename = "invalid export payload";
    return -1;
  }

  vault_id = GBIT32(tx->sdata);
  v = find_vault(vault_id);

  if (v == nil) {
    rx->type = Rerror;
    rx->ename = "vault not found";
    return -1;
  }

  if (v->pid != up->pid) {
    rx->type = Rerror;
    rx->ename = Eperm;
    return -1;
  }

  qlock(&v->lock);

  if (!v->locked) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = "vault must be locked to export";
    return -1;
  }

  /* Allocate blob */
  blob = malloc(v->size);
  if (blob == nil) {
    qunlock(&v->lock);
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  /* Copy entire vault (header + encrypted data) */
  memmove(blob, v->data, v->size);

  qunlock(&v->lock);

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->sdata = blob;
  rx->scount = v->size;

  return 0;
}

/*
 * SYS_VAULT_IMPORT (217) - Import encrypted vault from blob
 *
 * Payload: [size:4][data:N]
 * Returns: new vault_id in retval
 */
int sys_vault_import_handler(Fcall *tx, Fcall *rx) {
  u32int size;
  ProcessVault *v;

  if (tx->scount < 4) {
    rx->type = Rerror;
    rx->ename = "invalid import payload";
    return -1;
  }

  size = GBIT32(tx->sdata);

  if (size < VAULT_HEADER_SIZE + 4096 ||
      size > VAULT_HEADER_SIZE + 512 * 1024) {
    rx->type = Rerror;
    rx->ename = "invalid vault size";
    return -1;
  }

  if (tx->scount < 4 + size) {
    rx->type = Rerror;
    rx->ename = "insufficient data";
    return -1;
  }

  if (process_has_vault(up->pid)) {
    rx->type = Rerror;
    rx->ename = "process already has vault";
    return -1;
  }

  /* Allocate vault structure */
  v = xalloc_resident(sizeof(ProcessVault));
  if (v == nil) {
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  memset(v, 0, sizeof(ProcessVault));
  v->pid = up->pid;
  v->size = size;

  /* Allocate vault data */
  if (pebble_alloc_with_white(v->size, &v->capability, (void **)&v->data) < 0) {
    xfree_resident(v);
    rx->type = Rerror;
    rx->ename = Enomem;
    return -1;
  }

  /* Copy imported data */
  memmove(v->data, tx->sdata + 4, size);

  /* Vault is imported in locked state */
  v->locked = 1;
  v->initialized = 1;
  v->has_key = 0;

  /* Add to global vault list */
  qlock(&vault_list_lock);
  v->id = next_vault_id++;
  v->next = vault_list;
  vault_list = v;
  v->refcount = 0;
  qunlock(&vault_list_lock);

  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = v->id;
  rx->scount = 0;
  rx->sdata = nil;

  return 0;
}
