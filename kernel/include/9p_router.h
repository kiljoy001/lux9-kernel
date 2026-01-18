/*
 * Lux9 9P Router
 *
 * Central routing of 9P messages from Exchange Pages to kernel services.
 */

#ifndef _9P_ROUTER_H_
#define _9P_ROUTER_H_

/* Include base types */
#include "fcall.h"
#include "types_fwd.h"
#include "u.h"

/*
 * 9P Exchange Page Layout - PER-PROCESS VA MODEL
 * ==============================================
 * Each process has ONE 4KB exchange page allocated from the pool and mapped
 * at a per-process virtual address (p->p9uaddr). Ownership flips between
 * process and kernel via borrow checker.
 *
 * Layout:
 *   0x000 - 0xEFF: Message area (3840 bytes) - request OR reply
 *   0xF00 - 0xFFF: Control block (256 bytes)
 */
#define P9_PAGE_SIZE 4096
#define P9_MSG_OFFSET 0x000
#define P9_MSG_SIZE 0xF00 /* 3840 bytes for message */
#define P9_CONTROL_OFFSET 0xF00
#define P9_CONTROL_SIZE 0x100 /* 256 bytes for control */

/* Exchange page ring layout for small messages */
#define P9_RING_SLOT_SIZE 256
#define P9_RING_HEADER_SIZE 8
#define P9_RING_DATA_SIZE (P9_RING_SLOT_SIZE - P9_RING_HEADER_SIZE)
#define P9_RING_SLOTS (P9_MSG_SIZE / P9_RING_SLOT_SIZE)

/* Legacy aliases (for transition) */
#define P9_REQUEST_OFFSET P9_MSG_OFFSET
#define P9_REQUEST_SIZE P9_MSG_SIZE
#define P9_REPLY_OFFSET                                                        \
  P9_MSG_OFFSET /* Same location - ownership-flip model                        \
                 */
#define P9_REPLY_SIZE P9_MSG_SIZE

/* Legacy fixed user VA (deprecated). */
#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL

uintptr p9_user_base(Proc *p);

#include "atomic.h"

/* Control Block (at offset 0xF00) */
typedef struct P9Control {
  uint doorbell; /* Access via atomic_load/store */
  uint status;   /* Access via atomic_load/store */
  volatile uint req_head;
  volatile uint req_tail;
  volatile uint rep_head;
  volatile uint rep_tail;
  volatile uint req_seq;
  volatile uint rep_seq;
  uchar session_pebble[64];
  uchar reserved[160];
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
/* Holographic Channels (Bits 4-6) - Visibility Masks */
#define PEBBLE_HOLOGRAPHIC_MASK 0x70
#define PEBBLE_VISIBILITY_PUBLIC (0x00 << 4)  /* Visible to all servers */
#define PEBBLE_VISIBILITY_GROUP (0x01 << 4)   /* Visible to group/shard */
#define PEBBLE_VISIBILITY_PRIVATE (0x07 << 4) /* Local-only (Intra-server) */
#define PEBBLE_PERM_ADMIN 0x80

/* Custom 9P message types for Lux9 - defined in fcall.h enum */
/* Texec = 128, Rexec = 129 */

/* Router API */
void p9_router_init(void);
int p9_alloc_page(Proc *p);
void p9_free_page(Proc *p);
int p9_handle_doorbell(Proc *p, Ureg *ureg);
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

/* /srv registry helpers */
void srv_init(void);
int srv_post_fd(Proc *caller, const char *name, int fd);
int srv_create_entry(Proc *caller, const char *name);
int srv_remove_entry(Proc *caller, const char *name);
int srv_get_by_index(int index, char *name, int namelen);
int srv_index_of(const char *name);
int srv_get_by_index_for_proc(Proc *caller, int index, char *name, int namelen);
int srv_index_of_for_proc(Proc *caller, const char *name);
Chan *srv_clone_chan(const char *name);

/*
 * Async 9P Operations (Phase 3)
 */

/* Completion callback type */
typedef void (*P9CompletionCallback)(Fcall *reply, void *arg, int status);

/* Async operation tracking */
typedef struct AsyncP9Op {
  uint op_id;                    /* MSGORD message ID */
  Fcall request;                 /* Original request (copied) */
  Fcall reply;                   /* Reply when ready */
  P9CompletionCallback callback; /* Completion callback */
  void *callback_arg;            /* Callback argument */
  uvlong submit_time;            /* When submitted */
  int status;                    /* P9_STATUS_* */
  struct AsyncP9Op *next;        /* Linked list for per-process tracking */
} AsyncP9Op;

/* Async completion status codes */
#define P9_ASYNC_SUCCESS 0
#define P9_ASYNC_PENDING 1
#define P9_ASYNC_TIMEOUT 2
#define P9_ASYNC_ROLLBACK 3
#define P9_ASYNC_ERROR 4

/* Async Router API */
int p9_handle_doorbell_async(Proc *p);
uint p9_submit_async(Proc *p, Fcall *t, char *path, P9CompletionCallback cb,
                     void *arg);
int p9_check_async(Proc *p, uint op_id, Fcall *reply_out);
void p9_cancel_async(Proc *p, uint op_id);

/* Fire all ready async completions for a process */
int p9_fire_completions(Proc *p);

#endif /* _9P_ROUTER_H_ */
