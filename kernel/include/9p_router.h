/*
 * Lux9 9P Router
 *
 * Central routing of 9P messages from Exchange Pages to kernel services.
 */

#ifndef _9P_ROUTER_H_
#define _9P_ROUTER_H_

/* Forward declarations */
typedef struct Proc Proc;
typedef struct Fcall Fcall;

/* 9P Exchange Page Layout */
#define P9_PAGE_SIZE 4096
#define P9_REQUEST_OFFSET 0x000
#define P9_REQUEST_SIZE 0x800
#define P9_REPLY_OFFSET 0x800
#define P9_REPLY_SIZE 0x700
#define P9_CONTROL_OFFSET 0xF00
#define P9_CONTROL_SIZE 0x100

/* Fixed user virtual address for the Exchange Page */
#define EXCHANGE_PAGE_ADDR 0x7FFFFFFF0000ULL

/* Control Block (at offset 0xF00) */
typedef struct P9Control {
  volatile uint doorbell;
  volatile uint status;
  volatile uint req_head;
  volatile uint req_tail;
  volatile uint rep_head;
  volatile uint rep_tail;
  volatile uint req_seq;
  volatile uint rep_seq;
  uchar session_pebble[32];
  uchar reserved[192];
} P9Control;

/* Status codes */
#define P9_STATUS_IDLE 0
#define P9_STATUS_PENDING 1
#define P9_STATUS_COMPLETE 2
#define P9_STATUS_ERROR 3

/* Pebble Token (embedded in Tattach data) */
typedef struct PebbleToken {
  uvlong ledger_id;
  uvlong expires;
  uchar permissions;
  uchar reserved[7];
  uchar signature[16];
} PebbleToken;

#define PEBBLE_MAGIC 0x5045424C /* "PEBL" */
#define PEBBLE_VERSION 1

/* Pebble permission flags */
#define PEBBLE_PERM_READ 0x01
#define PEBBLE_PERM_WRITE 0x02
#define PEBBLE_PERM_EXEC 0x04
#define PEBBLE_PERM_DELETE 0x08
#define PEBBLE_PERM_ADMIN 0x80

/* Router API */
void p9_router_init(void);
int p9_alloc_page(Proc *p);
void p9_free_page(Proc *p);
int p9_handle_doorbell(Proc *p);
int p9_route(Proc *p, Fcall *t, Fcall *r);
int p9_dispatch(Proc *p, Fcall *t, Fcall *r);

/* Pebble validation */
int p9_extract_pebble(uchar *data, ulong len, PebbleToken *out);
int p9_validate_pebble(PebbleToken *tok, char *path, int access);

/* 9P Service handlers */
int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int env_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int mnt_9p_handle(Proc *caller, Fcall *t, Fcall *r);

#endif /* _9P_ROUTER_H_ */
