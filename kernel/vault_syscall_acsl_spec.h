/*
 * Process Vault Syscall ACSL Specifications
 *
 * Formal contracts for vault operations via Tsyscall messages.
 * Write specifications FIRST, implement SECOND, verify THIRD.
 *
 * Message Format:
 *   Tsyscall messages with syscall-specific payload in sdata
 */

#ifndef VAULT_SYSCALL_ACSL_SPEC_H
#define VAULT_SYSCALL_ACSL_SPEC_H

#include "fcall.h"
#include "kernel.h"

/* Vault syscall numbers (extend existing Tsyscall space) */
enum {
  SYS_VAULT_CREATE = 190, /* Create and initialize vault */
  SYS_VAULT_LOCK = 191,   /* Lock (encrypt) vault */
  SYS_VAULT_UNLOCK = 192, /* Unlock (decrypt) vault */
  SYS_VAULT_READ = 193,   /* Read from unlocked vault */
  SYS_VAULT_WRITE = 194,  /* Write to unlocked vault */
  SYS_VAULT_WIPE = 195,   /* Secure wipe vault */
  SYS_VAULT_EXPORT = 196, /* Export locked vault to buffer */
  SYS_VAULT_IMPORT = 197, /* Import locked vault from buffer */
};

/* Constants from devram.c */
#define MAX_VAULT_SIZE (512 * 1024) /* 512 KB */
#define MIN_VAULT_SIZE (4 * 1024)   /* 4 KB */
#define VAULT_HEADER_SIZE 88        /* Salt+R+Nonce+MAC */
#define MAX_PASSWORD_LEN 256

/*@ predicate valid_vault_size(ulong size) =
  @   size >= MIN_VAULT_SIZE && size <= MAX_VAULT_SIZE;
  @*/

/*@ predicate valid_password(char *pwd) =
  @   \valid_read(pwd) &&
  @   \valid_read_string(pwd) &&
  @   strlen(pwd) > 0 &&
  @   strlen(pwd) < MAX_PASSWORD_LEN;
  @*/

/*@ predicate vault_exists(int vault_id) =
  @   \exists ProcessVault *v;
  @     v->id == vault_id &&
  @     v->pid == up->pid &&
  @     v->data != \null &&
  @     v->size > 0;
  @*/

/*@ predicate vault_is_locked(int vault_id) =
  @   \exists ProcessVault *v;
  @     v->id == vault_id &&
  @     v->locked == 1 &&
  @     v->has_key == 0;
  @*/

/*@ predicate vault_is_unlocked(int vault_id) =
  @   \exists ProcessVault *v;
  @     v->id == vault_id &&
  @     v->locked == 0 &&
  @     v->has_key == 1;
  @*/

/* ============================================================================
 * SYS_VAULT_CREATE - Create and initialize a new vault
 * ============================================================================
 *
 * Tsyscall payload (sdata):
 *   [size:8]         - Vault size in bytes (ulong)
 *   [pwd_len:4]      - Password length (u32int)
 *   [password:N]     - Password bytes
 *
 * Rsyscall response:
 *   retval: vault_id (>= 0) on success, -1 on error
 */

/*@
  requires tx != \null && rx != \null;
  requires \valid(tx) && \valid(rx);
  requires tx->type == Tsyscall;
  requires tx->sdata != \null;
  requires tx->scount >= 12;  // Minimum: size(8) + pwd_len(4)

  // Parse size from sdata
  requires \let size = GBIT64(tx->sdata);
           valid_vault_size(size);

  // Parse password length
  requires \let pwd_len = GBIT32(tx->sdata + 8);
           pwd_len > 0 && pwd_len < MAX_PASSWORD_LEN;

  // Password data must be readable
  requires tx->scount >= 12 + GBIT32(tx->sdata + 8);
  requires \valid_read(tx->sdata + 12 + (0 .. GBIT32(tx->sdata + 8) - 1));

  // Process must not already have a vault (one per process)
  requires !process_has_vault(up->pid);

  behavior success:
    assumes valid_vault_size(GBIT64(tx->sdata));
    assumes GBIT32(tx->sdata + 8) > 0 && GBIT32(tx->sdata + 8) <
  MAX_PASSWORD_LEN; ensures rx->type == Rsyscall; ensures rx->retval >= 0;
    ensures vault_exists(rx->retval);
    ensures vault_is_locked(rx->retval);
    ensures \exists ProcessVault *v;
            v->id == rx->retval &&
            v->pid == up->pid &&
            v->initialized == 1 &&
            v->locked == 1 &&
            v->has_key == 0 &&
            v->size == GBIT64(tx->sdata) + VAULT_HEADER_SIZE;

  behavior failure:
    assumes !valid_vault_size(GBIT64(tx->sdata)) ||
            process_has_vault(up->pid);
    ensures rx->type == Rerror;
    ensures \valid_read_string(rx->ename);

  complete behaviors;
  disjoint behaviors;

  assigns rx->type, rx->tag, rx->retval, rx->ename;
  assigns vault_list, next_vault_id;
*/
int sys_vault_create_handler(Fcall *tx, Fcall *rx);

/* ============================================================================
 * SYS_VAULT_LOCK - Lock (re-encrypt) an unlocked vault
 * ============================================================================
 *
 * Tsyscall payload (sdata):
 *   [vault_id:4]     - Vault identifier (int)
 *
 * Rsyscall response:
 *   retval: 0 on success, -1 on error
 */

/*@
  requires tx != \null && rx != \null;
  requires \valid(tx) && \valid(rx);
  requires tx->type == Tsyscall;
  requires tx->sdata != \null;
  requires tx->scount >= 4;

  requires \let vid = GBIT32(tx->sdata);
           vault_exists(vid);

  behavior success:
    assumes \let vid = GBIT32(tx->sdata);
            vault_exists(vid) && vault_is_unlocked(vid);
    ensures rx->type == Rsyscall;
    ensures rx->retval == 0;
    ensures vault_is_locked(GBIT32(\old(tx->sdata)));
    ensures \forall ProcessVault *v;
            v->id == GBIT32(\old(tx->sdata)) ==>
              v->locked == 1 && v->has_key == 0;

  behavior already_locked:
    assumes \let vid = GBIT32(tx->sdata);
            vault_exists(vid) && vault_is_locked(vid);
    ensures rx->type == Rsyscall;
    ensures rx->retval == 0;  // Idempotent

  behavior not_found:
    assumes !\exists ProcessVault *v; v->id == GBIT32(tx->sdata);
    ensures rx->type == Rerror;

  complete behaviors;
  disjoint behaviors;

  assigns rx->type, rx->tag, rx->retval, rx->ename;
  assigns \nothing;  // Lock modifies vault state internally
*/
int sys_vault_lock_handler(Fcall *tx, Fcall *rx);

/* ============================================================================
 * SYS_VAULT_UNLOCK - Unlock (decrypt) a locked vault
 * ============================================================================
 *
 * Tsyscall payload (sdata):
 *   [vault_id:4]     - Vault identifier (int)
 *   [pwd_len:4]      - Password length (u32int)
 *   [password:N]     - Password bytes
 *
 * Rsyscall response:
 *   retval: 0 on success, -1 on error
 */

/*@
  requires tx != \null && rx != \null;
  requires \valid(tx) && \valid(rx);
  requires tx->type == Tsyscall;
  requires tx->sdata != \null;
  requires tx->scount >= 8;

  requires \let vid = GBIT32(tx->sdata);
           \let pwd_len = GBIT32(tx->sdata + 4);
           vault_exists(vid) &&
           pwd_len > 0 && pwd_len < MAX_PASSWORD_LEN &&
           tx->scount >= 8 + pwd_len;

  requires \valid_read(tx->sdata + 8 + (0 .. GBIT32(tx->sdata + 4) - 1));

  behavior success:
    assumes \let vid = GBIT32(tx->sdata);
            vault_exists(vid) &&
            vault_is_locked(vid) &&
            password_is_correct(vid, tx->sdata + 8, GBIT32(tx->sdata + 4));
    ensures rx->type == Rsyscall;
    ensures rx->retval == 0;
    ensures vault_is_unlocked(GBIT32(\old(tx->sdata)));
    ensures \forall ProcessVault *v;
            v->id == GBIT32(\old(tx->sdata)) ==>
              v->locked == 0 && v->has_key == 1;

  behavior wrong_password:
    assumes \let vid = GBIT32(tx->sdata);
            vault_exists(vid) &&
            !password_is_correct(vid, tx->sdata + 8, GBIT32(tx->sdata + 4));
    ensures rx->type == Rerror;
    ensures \valid_read_string(rx->ename);
    ensures vault_is_locked(GBIT32(\old(tx->sdata)));  // Stays locked

  behavior already_unlocked:
    assumes \let vid = GBIT32(tx->sdata);
            vault_exists(vid) && vault_is_unlocked(vid);
    ensures rx->type == Rsyscall;
    ensures rx->retval == 0;  // Idempotent

  complete behaviors;
  disjoint behaviors;

  assigns rx->type, rx->tag, rx->retval, rx->ename;
*/
int sys_vault_unlock_handler(Fcall *tx, Fcall *rx);

/* ============================================================================
 * SYS_VAULT_STATUS - Query vault state
 * ============================================================================
 *
 * Tsyscall payload (sdata):
 *   [vault_id:4]     - Vault identifier (int), or -1 to check if process has
 * vault
 *
 * Rsyscall response (sdata):
 *   [exists:1]       - 1 if vault exists, 0 otherwise (uchar)
 *   [vault_id:4]     - Actual vault ID (int)
 *   [locked:1]       - 1 if locked, 0 if unlocked (uchar)
 *   [initialized:1]  - 1 if initialized, 0 otherwise (uchar)
 *   [size:8]         - Total vault size including header (ulong)
 *   [data_size:8]    - User data size (size - VAULT_HEADER_SIZE) (ulong)
 */

/*@
  requires tx != \null && rx != \null;
  requires \valid(tx) && \valid(rx);
  requires tx->type == Tsyscall;
  requires tx->sdata != \null;
  requires tx->scount >= 4;

  behavior vault_exists:
    assumes \let vid = GBIT32(tx->sdata);
            vid >= 0 && vault_exists(vid);
    ensures rx->type == Rsyscall;
    ensures rx->sdata != \null;
    ensures rx->scount == 23;  // 1+4+1+1+8+8
    ensures \valid_read(rx->sdata + (0..22));
    ensures rx->sdata[0] == 1;  // exists = true
    ensures GBIT32(rx->sdata + 1) == GBIT32(\old(tx->sdata));  // vault_id
    ensures \let vid = GBIT32(\old(tx->sdata));
            \exists ProcessVault *v; v->id == vid ==>
              rx->sdata[5] == (v->locked ? 1 : 0) &&
              rx->sdata[6] == (v->initialized ? 1 : 0);

  behavior query_any_vault:
    assumes GBIT32(tx->sdata) == (u32int)-1;  // -1 means "any vault for this
  process" ensures rx->type == Rsyscall; ensures rx->sdata != \null; ensures
  rx->scount == 23; ensures rx->sdata[0] == (process_has_vault(up->pid) ? 1 :
  0);

  behavior vault_not_found:
    assumes \let vid = GBIT32(tx->sdata);
            vid >= 0 && !vault_exists(vid);
    ensures rx->type == Rsyscall;
    ensures rx->sdata != \null;
    ensures rx->scount == 23;
    ensures rx->sdata[0] == 0;  // exists = false
    ensures GBIT32(rx->sdata + 1) == (u32int)-1;  // invalid vault_id

  complete behaviors;
  disjoint behaviors;

  assigns rx->type, rx->tag, rx->sdata, rx->scount;
*/
int sys_vault_status_handler(Fcall *tx, Fcall *rx);

/* Read, Write, Wipe, Export, and Import handlers follow same pattern... */

#endif /* VAULT_SYSCALL_ACSL_SPEC_H */
