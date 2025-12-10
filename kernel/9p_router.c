/*
 * Lux9 9P Router Implementation
 *
 * Routes 9P messages from Exchange Pages to kernel services.
 */

#include <u.h>
#include "portlib.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"
#include "proc_packet.h"

/* Forward declarations for handlers */
extern int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r);
extern int dev_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int env_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int srv_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int mnt_9p_handle(Proc *p, Fcall *t, Fcall *r);

/*
 * Path matching for routing
 */
static int path_match(char *path, char *pattern) {
  int plen = strlen(pattern);
  if (pattern[plen - 1] == '*') {
    return strncmp(path, pattern, plen - 1) == 0;
  }
  return strcmp(path, pattern) == 0;
}

/*
 * Initialize router subsystem
 */
void p9_router_init(void) { print("9p_router: initialized\n"); }

/*
 * Allocate 9P Exchange Page for a process
 */
int p9_alloc_page(Proc *p) {
  uintptr page;

  page = (uintptr)xalloc(P9_PAGE_SIZE);
  if (page == 0)
    return -1;

  memset((void *)page, 0, P9_PAGE_SIZE);

  P9Control *ctl = (P9Control *)(page + P9_CONTROL_OFFSET);
  ctl->doorbell = 0;
  ctl->status = P9_STATUS_IDLE;
  ctl->req_head = 0;
  ctl->req_tail = 0;
  ctl->rep_head = 0;
  ctl->rep_tail = 0;
  ctl->req_seq = 0;
  ctl->rep_seq = 0;

  p->p9page = (void *)page;
  return 0;
}

void p9_free_page(Proc *p) {
  if (p->p9page) {
    xfree(p->p9page);
    p->p9page = nil;
  }
}

int p9_extract_pebble(uchar *data, ulong len, PebbleToken *out) {
  uint magic, version;

  if (len < 8 + sizeof(PebbleToken))
    return -1;

  magic = GBIT32(data);
  version = GBIT32(data + 4);

  if (magic != PEBBLE_MAGIC)
    return -1;
  if (version != PEBBLE_VERSION)
    return -1;

  memmove(out, data + 8, sizeof(PebbleToken));
  return 0;
}

int p9_validate_pebble(PebbleToken *tok, char *path, int access) {
  if (tok->expires != 0 && tok->expires < (uvlong)seconds()) {
    return 0;
  }
  if ((access & PEBBLE_PERM_READ) && !(tok->permissions & PEBBLE_PERM_READ))
    return 0;
  if ((access & PEBBLE_PERM_WRITE) && !(tok->permissions & PEBBLE_PERM_WRITE))
    return 0;
  return 1;
}

#include "ghostdag_kernel.h"

/*
 * Dispatch message to appropriate handler
 * This is called by GHOSTDAG after ordering is determined.
 */
int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  char *path;

  if (t->type == Tattach) {
    path = t->aname;
  } else {
    path = "/";
  }

  if (path_match(path, "/proc/") || strcmp(path, "/proc") == 0) {
    return proc_9p_handle(p, t, r);
  }
  if (path_match(path, "/dev/") || strcmp(path, "/dev") == 0) {
    return dev_9p_handle(p, t, r);
  }
  if (path_match(path, "/env/") || strcmp(path, "/env") == 0) {
    return env_9p_handle(p, t, r);
  }
  if (path_match(path, "/srv/") || strcmp(path, "/srv") == 0) {
    return srv_9p_handle(p, t, r);
  }
  if (path_match(path, "/mnt/") || strcmp(path, "/mnt") == 0) {
    return mnt_9p_handle(p, t, r);
  }

  r->type = Rerror;
  r->ename = "no such file or directory";
  return -1;
}

/*
 * Main entry point: Submit to GHOSTDAG for ordering
 * The message will be dispatched by p9_process_queue() later.
 */
int p9_route(Proc *p, Fcall *t, Fcall *r) {
  char *path;

  if (t->type == Tattach)
    path = t->aname;
  else
    path = "/"; /* TODO: Need full path for correct DAG dependency? */

  /* Submit for ordering */
  if (ghostdag_submit(p, t, path) < 0) {
    r->type = Rerror;
    r->ename = "ghostdag queue full";
    return -1;
  }

  /*
   * IMPORTANT: With GHOSTDAG, we don't process immediately.
   * We return "pending" status if async, or wait if sync.
   * For now, to keep existing code working, we construct a synchronous wait
   * loop. In a pure event-loop kernel, we would return immediately.
   */

  /*
   * TEMPORARY HACK: Process strictly (flush the queue) to simulate synchronous
   * behavior until the scheduler is fully event-driven.
   */
  ghostdag_process_all();

  /*
   * Note: 'r' is populated by p9_dispatch called via ghostdag_process_all ->
   * ghostdag_process_one The hack above ensures 'r' is filled before we return.
   */

  return 0;
}

int p9_handle_doorbell(Proc *p) {
  /* TODO: Read from Exchange Page, parse, route, write reply */
  return 0;
}

/*
 * Process control server: /proc/
 * Integrates with our FSM!
 */
int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Proc *target;
  ulong pid;
  char *cmd;
  char *path;
  Qid q;

  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    /* Set qid for directory */
    q.type = QTDIR;
    q.path = 0;
    q.vers = 0;
    /* Use memmove to clear any type ambiguity */
    memmove(&r->qid, &q, sizeof(Qid));
    return 0;

  case Twrite:
    path = "/proc/self/ctl";

    if (strncmp(path, "/proc/self", 10) == 0) {
      target = caller;
    } else {
      pid = strtoul(path + 6, nil, 10);
      target = proctab(pid);
      if (target == nil) {
        r->type = Rerror;
        r->ename = "no such process";
        return -1;
      }
    }

    cmd = (char *)t->data;

    if (strcmp(cmd, "wakeup") == 0) {
      proc_event(target, EV_WAKEUP);
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
    if (strcmp(cmd, "stop") == 0) {
      proc_event(target, EV_STOP);
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
    if (strcmp(cmd, "start") == 0) {
      proc_event(target, EV_CONT);
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
    if (strcmp(cmd, "kill") == 0) {
      proc_event(target, EV_BREAK);
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }

    r->type = Rerror;
    r->ename = "unknown command";
    return -1;

  case Tread:
    target = caller;
    r->type = Rread;
    r->count = snprint((char *)r->data, 256,
                       "state: %s\ntrace: %s <- %s <- %s <- %s\n",
                       proc_state_names[proc_state(target)],
                       proc_state_names[STATE_CURRENT(target->state_trace)],
                       proc_state_names[STATE_PREV(target->state_trace)],
                       proc_state_names[STATE_T2(target->state_trace)],
                       proc_state_names[STATE_T3(target->state_trace)]);
    return 0;

  default:
    r->type = Rerror;
    r->ename = "not implemented";
    return -1;
  }
}

int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  r->type = Rerror;
  r->ename = "not implemented";
  return -1;
}

int env_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  r->type = Rerror;
  r->ename = "not implemented";
  return -1;
}

int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  r->type = Rerror;
  r->ename = "not implemented";
  return -1;
}

int mnt_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  r->type = Rerror;
  r->ename = "not implemented";
  return -1;
}
