/*
 * Lux9 9P Router Implementation
 *
 * Routes 9P messages from Exchange Pages to kernel services.
 */

#include <u.h>
#include "portlib.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by
 * kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "9p_router.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "proc_packet.h"

/* Stub definitions for proc FSM (until full FSM is implemented) */
/* EV_* and proc_state are already defined in proc_packet.h */

static void proc_event_stub(Proc *p, int ev) {
  USED(p);
  USED(ev);
  /* TODO: Implement process FSM state transitions */
}
#define proc_event proc_event_stub

static char *proc_state_names_stub[] = {
    "unknown", "ready", "running", "waiting", "stopped", "broken", "dead"};
#define proc_state_names proc_state_names_stub

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

#include "blind_ledger.h"
#include "ghostdag_kernel.h"

/*
 * Session Pebble Management
 */
static void store_session_pebble(Proc *p, PebbleToken *tok) {
  P9Control *ctl;

  if (p->p9page == nil)
    return;

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  memmove(ctl->session_pebble, tok->signature, 16);
  /* Store ledger_id in next 8 bytes */
  PBIT64(ctl->session_pebble + 16, tok->ledger_id);
  /* Store permissions in next byte */
  ctl->session_pebble[24] = tok->permissions;
  /* Store expires in next 8 bytes */
  PBIT64(ctl->session_pebble + 25, tok->expires);
}

static int get_session_pebble(Proc *p, PebbleToken *tok) {
  P9Control *ctl;

  if (p->p9page == nil)
    return -1;

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);

  /* Check if pebble is set (non-zero signature) */
  if (ctl->session_pebble[0] == 0 && ctl->session_pebble[1] == 0)
    return -1;

  /* Reconstruct PebbleToken from session storage */
  memmove(tok->signature, ctl->session_pebble, 16);
  tok->ledger_id = GBIT64(ctl->session_pebble + 16);
  tok->permissions = ctl->session_pebble[24];
  tok->expires = GBIT64(ctl->session_pebble + 25);

  return 0;
}

/*
 * Full Pebble Validation with BlindLedger
 */
static int p9_validate_pebble_full(PebbleToken *tok, char *path, Proc *owner) {
  UserCapability cap;
  BlindLedgerEntry entry;
  BlindLedgerError err;

  /* Basic validation first */
  if (!p9_validate_pebble(tok, path, 0))
    return 0;

  /* Convert PebbleToken to UserCapability for BlindLedger verification */
  memset(&cap, 0, sizeof(cap));
  memmove(cap.hash, tok->signature, 16);
  /* Zero-pad the rest of the hash */
  memset(cap.hash + 16, 0, 16);

  cap.type = CAP_TYPE_DEVICE; /* Device capability */
  cap.perms = 0;
  if (tok->permissions & PEBBLE_PERM_READ)
    cap.perms |= CAP_PERM_READ;
  if (tok->permissions & PEBBLE_PERM_WRITE)
    cap.perms |= CAP_PERM_WRITE;
  if (tok->permissions & PEBBLE_PERM_EXEC)
    cap.perms |= CAP_PERM_EXEC;

  /* Verify with BlindLedger */
  err = ledger_verify(&cap, &entry);
  if (err != BLIND_LEDGER_OK) {
    /* Capability not found or invalid */
    return 0;
  }

  /* Check owner matches */
  if (entry.owner != owner && entry.owner != nil) {
    /* Capability belongs to different process */
    return 0;
  }

  return 1;
}

/*
 * Permission checking helper for device operations
 */
static int check_permission(Proc *p, int required_perm) {
  PebbleToken tok;

  /* Get session pebble */
  if (get_session_pebble(p, &tok) < 0) {
    /* No pebble in session - DENY by default */
    return 0;
  }

  /* Check if required permission is granted */
  return (tok.permissions & required_perm) != 0;
}

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
  if (ghostdag_submit(nil, p, t, path) < 0) {
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
  ghostdag_process_all(nil);

  /*
   * Note: 'r' is populated by p9_dispatch called via ghostdag_process_all ->
   * ghostdag_process_one The hack above ensures 'r' is filled before we return.
   */

  return 0;
}

/*
 * p9_handle_doorbell - Process 9P message from exchange page
 *
 * This is the ONLY entry point for pure 9P architecture.
 * Replaces all traditional syscalls with direct 9P protocol.
 *
 * Flow:
 *   1. Read Fcall from exchange page request buffer
 *   2. Parse using convM2S()
 *   3. Route through p9_dispatch()
 *   4. Serialize reply using convS2M()
 *   5. Set status to COMPLETE
 *
 * Called by: VectorSYSCALL handler (doorbell-only mode)
 */
int p9_handle_doorbell(Proc *p) {
  P9Control *ctl;
  uchar *req_buf, *rep_buf;
  Fcall t, r;
  uint req_size, rep_size;
  int result;

  /* Validate exchange page exists */
  if (p->p9page == nil) {
    print("p9_handle_doorbell: no exchange page for pid %lud\n", p->pid);
    return -1;
  }

  /* Get control block and buffers */
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  req_buf = (uchar *)p->p9page + P9_REQUEST_OFFSET;
  rep_buf = (uchar *)p->p9page + P9_REPLY_OFFSET;

  /* Check doorbell is actually rung */
  if (ctl->doorbell == 0) {
    print("p9_handle_doorbell: doorbell not rung for pid %lud\n", p->pid);
    return -1;
  }

  /* Mark as pending */
  ctl->status = P9_STATUS_PENDING;

  /* Parse request from exchange page */
  memset(&t, 0, sizeof(t));
  req_size = ctl->req_tail - ctl->req_head;
  if (req_size == 0 || req_size > P9_REQUEST_SIZE) {
    print("p9_handle_doorbell: invalid request size %ud\n", req_size);
    ctl->status = P9_STATUS_ERROR;
    ctl->doorbell = 0;
    return -1;
  }

  if (convM2S(req_buf + ctl->req_head, req_size, &t) == 0) {
    print("p9_handle_doorbell: failed to parse Fcall\n");
    ctl->status = P9_STATUS_ERROR;
    ctl->doorbell = 0;
    return -1;
  }

  /* Dispatch through 9P router with GHOSTDAG ordering */
  memset(&r, 0, sizeof(r));
  result = p9_dispatch(p, &t, &r);

  /* Serialize reply to exchange page */
  rep_size = convS2M(&r, rep_buf, P9_REPLY_SIZE);
  if (rep_size == 0) {
    print("p9_handle_doorbell: failed to serialize reply\n");
    ctl->status = P9_STATUS_ERROR;
    ctl->doorbell = 0;
    return -1;
  }

  /* Update reply buffer pointers */
  ctl->rep_head = 0;
  ctl->rep_tail = rep_size;
  ctl->rep_seq++;

  /* Mark as complete and clear doorbell */
  ctl->status = P9_STATUS_COMPLETE;
  ctl->doorbell = 0;

  return result;
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

/*
 * Device path parsing helper
 */
static char *devname_from_path(char *path) {
  if (path == nil)
    return "cons";
  if (strncmp(path, "/dev/", 5) == 0)
    return path + 5;
  return path;
}

/*
 * Common Tattach handler with Pebble validation
 * Returns 0 on success, -1 on permission error
 */
static int handle_tattach_with_pebble(Proc *caller, Fcall *t, Fcall *r,
                                      int required_perms, uchar qid_path) {
  PebbleToken tok;
  Qid q;

  /* Check if attach data contains Pebble */
  if (t->data != nil && t->count >= 8 + sizeof(PebbleToken)) {
    if (p9_extract_pebble((uchar *)t->data, t->count, &tok) == 0) {
      /* Validate pebble for required access */
      if (!p9_validate_pebble(&tok, t->aname, required_perms)) {
        r->type = Rerror;
        r->ename = "permission denied";
        return -1;
      }
      /* Full validation with BlindLedger */
      if (!p9_validate_pebble_full(&tok, t->aname, caller)) {
        r->type = Rerror;
        r->ename = "invalid capability";
        return -1;
      }
      /* Store in session */
      store_session_pebble(caller, &tok);
    }
  }

  /* Successful attach */
  r->type = Rattach;
  q.type = QTFILE;
  q.path = qid_path;
  q.vers = 0;
  memmove(&r->qid, &q, sizeof(Qid));
  r->iounit = 8192;
  return 0;
}

/*
 * Console device handler: /dev/cons
 */
static int cons_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 1);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    putstrn((char *)t->data, t->count);
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Console read not implemented yet */
    r->type = Rread;
    r->count = 0;
    r->data = nil;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    r->type = Rerror;
    r->ename = "stat not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Null device handler: /dev/null
 */
static int null_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 2);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    /* Accept all writes, discard data */
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return empty */
    r->type = Rread;
    r->count = 0;
    r->data = nil;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    r->type = Rerror;
    r->ename = "stat not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Zero device handler: /dev/zero
 */
static int zero_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static uchar zerobuf[8192];
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, PEBBLE_PERM_READ, 3);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    /* Accept all writes, discard data */
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return zeros */
    if (t->count > sizeof(zerobuf))
      t->count = sizeof(zerobuf);
    memset(zerobuf, 0, t->count);
    r->type = Rread;
    r->count = t->count;
    r->data = zerobuf;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    r->type = Rerror;
    r->ename = "stat not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Random device handler: /dev/random
 */
static int random_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static uchar randbuf[8192];
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, PEBBLE_PERM_READ, 4);

  case Twrite:
    /* Random is read-only */
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return random bytes */
    if (t->count > sizeof(randbuf))
      t->count = sizeof(randbuf);
    randomread(randbuf, t->count);
    r->type = Rread;
    r->count = t->count;
    r->data = randbuf;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    r->type = Rerror;
    r->ename = "stat not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Time device handler: /dev/time
 */
static int time_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static char timebuf[64];
  int n;
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, PEBBLE_PERM_READ, 5);

  case Twrite:
    /* Time is read-only */
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return current time in seconds */
    n = snprint(timebuf, sizeof(timebuf), "%ld\n", seconds());
    if (t->offset >= n) {
      r->type = Rread;
      r->count = 0;
      r->data = nil;
      return 0;
    }
    if (t->offset + t->count > n)
      t->count = n - t->offset;
    r->type = Rread;
    r->count = t->count;
    r->data = (uchar *)(timebuf + t->offset);
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    r->type = Rerror;
    r->ename = "stat not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Sysname device handler: /dev/sysname
 */
static int sysname_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static char namebuf[128];
  int n, len;
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 6);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    /* Update sysname */
    if (t->count >= sizeof(namebuf)) {
      r->type = Rerror;
      r->ename = "name too long";
      return -1;
    }
    memmove(namebuf, t->data, t->count);
    namebuf[t->count] = '\0';
    /* Remove trailing newline if present */
    if (t->count > 0 && namebuf[t->count - 1] == '\n')
      namebuf[t->count - 1] = '\0';
    kstrdup(&sysname, namebuf);
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return current sysname */
    if (sysname == nil)
      sysname = "lux9";
    len = strlen(sysname);
    n = snprint(namebuf, sizeof(namebuf), "%s\n", sysname);
    if (t->offset >= n) {
      r->type = Rread;
      r->count = 0;
      r->data = nil;
      return 0;
    }
    if (t->offset + t->count > n)
      t->count = n - t->offset;
    r->type = Rread;
    r->count = t->count;
    r->data = (uchar *)(namebuf + t->offset);
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    r->type = Rerror;
    r->ename = "stat not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Device router dispatch
 */
int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *path;
  char *dev;

  r->tag = t->tag;

  /* Extract device name from attach path */
  if (t->type == Tattach)
    path = t->aname;
  else
    path = "/dev/cons"; /* Default for non-attach operations */

  dev = devname_from_path(path);

  /* Dispatch to device handler */
  if (strcmp(dev, "cons") == 0)
    return cons_9p_handle(caller, t, r);
  if (strcmp(dev, "null") == 0)
    return null_9p_handle(caller, t, r);
  if (strcmp(dev, "zero") == 0)
    return zero_9p_handle(caller, t, r);
  if (strcmp(dev, "random") == 0)
    return random_9p_handle(caller, t, r);
  if (strcmp(dev, "time") == 0)
    return time_9p_handle(caller, t, r);
  if (strcmp(dev, "sysname") == 0)
    return sysname_9p_handle(caller, t, r);

  /* Device not found */
  r->type = Rerror;
  r->ename = "device not found";
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
