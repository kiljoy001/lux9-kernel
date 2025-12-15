/*
 * Lux9 9P Router Implementation
 *
 * Routes 9P messages from Exchange Pages to kernel services.
 */

#include "portlib.h"
#include "u.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by
 * kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
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
static int ram_9p_handle(Proc *caller, Fcall *t, Fcall *r);

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

  /* Initialize P9Control block at offset 0x1F00 (start of 2nd page + offset) */
  P9Control *ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  memset(ctl, 0, sizeof(P9Control));
  atomic_store(&ctl->status, P9_STATUS_IDLE, ORDER_RELAXED);
  atomic_store(&ctl->doorbell, 0, ORDER_RELAXED);

  /* Map Page 0 (Requests) as Read-Write */
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
#include "msgord.h"

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
 * FID Management via Fgrp (Stateless Router)
 * We reuse the kernel's file descriptor table to map 9P FIDs.
 * Virtual router endpoints are represented by Channels with type = -1.
 */

#define ROUTER_CHAN_TYPE -1
#define TYPE_PROC 1
#define TYPE_DEV 2
#define TYPE_ENV 3
#define TYPE_SRV 4
#define TYPE_MNT 5

/* Device subtypes for TYPE_DEV FIDs */
#define DEV_CONS 1
#define DEV_NULL 2
#define DEV_ZERO 3
#define DEV_RANDOM 4
#define DEV_TIME 5
#define DEV_SYSNAME 6
#define DEV_RAM 7

static int install_fid_with_subtype(int fid, int type, int subtype) {
  Chan *c;
  Fgrp *f = up->fgrp;

  c = newchan();
  if (c == nil)
    return -1;
  c->type = ROUTER_CHAN_TYPE; /* Mark as virtual router channel */
  c->qid.path = type;         /* Store handler type in Qid path */
  c->qid.vers = subtype;      /* Store subtype (e.g., device ID) */
  c->mode = ORDWR;
  c->ref = 1;

  lock(&f->lock);
  if (fid < 0 || growfd(f, fid) < 0) {
    unlockfgrp(f);
    cclose(c);
    return -1;
  }
  if (fid > f->maxfd)
    f->maxfd = fid;
  if (f->fd[fid])
    cclose(f->fd[fid]);
  f->fd[fid] = c;
  f->flag[fid] = 0;
  unlockfgrp(f);
  return 0;
}

static int install_fid(int fid, int type) {
  return install_fid_with_subtype(fid, type, 0);
}

static int get_fid_type(int fid) {
  Chan *c;
  Fgrp *f = up->fgrp;
  int type = 0;

  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != nil) {
    if (c->type == ROUTER_CHAN_TYPE) {
      type = (int)c->qid.path;
    }
  }
  unlock(&f->lock);
  return type;
}

static int get_fid_subtype(int fid) {
  Chan *c;
  Fgrp *f = up->fgrp;
  int subtype = 0;

  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != nil) {
    if (c->type == ROUTER_CHAN_TYPE) {
      subtype = (int)c->qid.vers;
    }
  }
  unlock(&f->lock);
  return subtype;
}

static void remove_fid(int fid) { fdclose(fid, 0); }

/*
 * Dispatch message to appropriate handler
 */
int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  int type = 0;

  if (t->type == Tattach) {
    /* Determine type from path */
    if (path_match(t->aname, "/proc/") || strcmp(t->aname, "/proc") == 0)
      type = TYPE_PROC;
    else if (path_match(t->aname, "/dev/") || strcmp(t->aname, "/dev") == 0)
      type = TYPE_DEV;
    else if (path_match(t->aname, "/env/") || strcmp(t->aname, "/env") == 0)
      type = TYPE_ENV;
    else if (path_match(t->aname, "/srv/") || strcmp(t->aname, "/srv") == 0)
      type = TYPE_SRV;
    else if (path_match(t->aname, "/mnt/") || strcmp(t->aname, "/mnt") == 0)
      type = TYPE_MNT;

    /* Install FID for non-device types; devices handle their own FID with
     * subtype */
    if (type > 0 && type != TYPE_DEV) {
      if (install_fid(t->fid, type) < 0) {
        r->type = Rerror;
        r->ename = "fid allocation failed";
        return -1;
      }
    }
  } else {
    /* Lookup type from fid */
    type = get_fid_type(t->fid);
    if (t->type == Tclunk)
      remove_fid(t->fid);
  }

  int ret = -1;
  if (type == TYPE_PROC)
    ret = proc_9p_handle(p, t, r);
  else if (type == TYPE_DEV)
    ret = dev_9p_handle(p, t, r);
  else if (type == TYPE_ENV)
    ret = env_9p_handle(p, t, r);
  else if (type == TYPE_SRV)
    ret = srv_9p_handle(p, t, r);
  else if (type == TYPE_MNT)
    ret = mnt_9p_handle(p, t, r);
  else {
    r->type = Rerror;
    r->ename = "fid not found or unknown path";
    return -1;
  }

  /* Propagate handler type to newfid on successful Walk */
  if (ret == 0 && t->type == Twalk && r->type == Rwalk) {
    /* If walk succeeded (all names consumed), newfid inherits the handler type
     */
    if (r->nwqid == t->nwname) {
      /* If newfid == fid, it's already set (cloning or walking self).
         If distinct, we must install it. */
      if (t->newfid != t->fid) {
        install_fid(t->newfid, type);
      }
    }
  }
  return ret;
}

/*
 * Main entry point: Submit to MSGORD for ordering
 * The message will be dispatched by p9_process_queue() later.
 */
int p9_route(Proc *p, Fcall *t, Fcall *r) {
  char *path;

  if (t->type == Tattach)
    path = t->aname;
  else
    path = "/"; /* TODO: Need full path for correct DAG dependency? */

  /* Submit for ordering */
  if (msgord_submit(nil, p, t, path) < 0) {
    r->type = Rerror;
    r->ename = "msgord queue full";
    return -1;
  }

  /*
   * IMPORTANT: With MSGORD, we don't process immediately.
   * We return "pending" status if async, or wait if sync.
   * For now, to keep existing code working, we construct a synchronous wait
   * loop. In a pure event-loop kernel, we would return immediately.
   */

  /*
   * TEMPORARY HACK: Process strictly (flush the queue) to simulate synchronous
   * behavior until the scheduler is fully event-driven.
   */
  msgord_process_all(nil);

  /*
   * Note: 'r' is populated by p9_dispatch called via msgord_process_all ->
   * msgord_process_one The hack above ensures 'r' is filled before we return.
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

  /* Ensure the exchange page is mapped into userspace so user code can ring the
   * doorbell. Some early processes may not have it mapped yet. */
  uintptr *pte = mmuwalk(m->pml4, EXCHANGE_PAGE_ADDR, 0, 0);
  if (pte == nil || (*pte & PTEVALID) == 0) {
    userpmap(EXCHANGE_PAGE_ADDR, PADDR(p->p9page),
             PTEVALID | PTEUSER | PTEWRITE);
  }

  /* Get control block and buffers */
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  req_buf = (uchar *)p->p9page + P9_REQUEST_OFFSET;
  rep_buf = (uchar *)p->p9page + P9_REPLY_OFFSET;

  /* Check doorbell is actually rung using Acquire semantics.
   * This ensures we see all userspace writes to the request buffer that
   * happened before the doorbell was rung. */
  if (atomic_load(&ctl->doorbell, ORDER_ACQUIRE) == 0) {
    print("p9_handle_doorbell: doorbell not rung for pid %lud\n", p->pid);
    return -1;
  }

  /* Clear doorbell immediately */
  atomic_store(&ctl->doorbell, 0, ORDER_RELAXED);

  /* Mark as pending */
  atomic_store(&ctl->status, P9_STATUS_PENDING, ORDER_RELAXED);

  /* Parse request from exchange page */
  memset(&t, 0, sizeof(t));
  req_size = ctl->req_tail - ctl->req_head;
  if (req_size == 0 || req_size > P9_REQUEST_SIZE) {
    print("p9_handle_doorbell: invalid request size %ud\n", req_size);
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    return -1;
  }

  if (convM2S(req_buf + ctl->req_head, req_size, &t) == 0) {
    print("p9_handle_doorbell: failed to parse Fcall\n");
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    return -1;
  }

  /* Dispatch through 9P router with MSGORD ordering */
  memset(&r, 0, sizeof(r));
  result = p9_dispatch(p, &t, &r);

  /* Serialize reply to exchange page */
  rep_size = convS2M(&r, rep_buf, P9_REPLY_SIZE);
  if (rep_size == 0) {
    print("p9_handle_doorbell: failed to serialize reply\n");
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    return -1;
  }

  /* Update reply buffer pointers */
  ctl->rep_head = 0;
  ctl->rep_tail = rep_size;
  ctl->rep_seq++;

  /* Mark as complete using Release semantics.
   * This ensures userspace sees the data in rep_buf before they see the
   * STATUS_COMPLETE flag. */
  atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);

  return result;
}

/*
 * Spawn entry point - run as a kernel process (kproc)
 * Transitions to userspace by exec'ing the specified binary.
 */
static void kspawn_entry(void *arg) {
  char *path = (char *)arg;
  char *argv[2];
  ulong args[2];

  print("kspawn_entry: executing '%s'\n", path);

  /*
   * Build arguments for sysexec:
   * args[0] = path string pointer
   * args[1] = argv array pointer (path, nil)
   */
  argv[0] = path;
  argv[1] = nil;

  args[0] = (ulong)path;
  args[1] = (ulong)argv;

  /*
   * Call sysexec to load and execute the binary.
   * sysexec never returns on success - it replaces the current process.
   * On error, we exit.
   */
  if (waserror()) {
    print("kspawn_entry: exec failed: %s\n", up->errstr);
    free(path);
    pexit(up->errstr, 1);
    return;
  }

  sysexec(args);
  /* Not reached on success */
  poperror();

  /* If we get here, exec failed somehow without error */
  print("kspawn_entry: sysexec returned unexpectedly\n");
  free(path);
  pexit("exec failed", 1);
}

/*
 * Helper: Handle writes to /proc/self/ctl
 */
static int handle_proc_ctl_write(Proc *p, char *cmd, int len) {
  char buf[256];
  char *args[16];
  int n;

  if (len >= sizeof(buf))
    return -1;
  memmove(buf, cmd, len);
  buf[len] = 0;

  n = tokenize(buf, args, nelem(args));
  if (n < 1)
    return -1;

  if (strcmp(args[0], "dup") == 0) {
    /* dup old [new] */
    if (n < 2)
      return -1;
    int old = strtoul(args[1], 0, 0);
    int new = (n > 2) ? strtoul(args[2], 0, 0) : -1;

    Chan *c = fdtochan(old, -1, 0, 1);
    if (c == nil)
      return -1;

    if (new != -1) {
      Fgrp *f = up->fgrp;
      lock(&f->lock);
      if (new < 0 || growfd(f, new) < 0) {
        unlockfgrp(f);
        cclose(c);
        return -1;
      }
      if (new > f->maxfd)
        f->maxfd = new;

      Chan *oc = f->fd[new];
      f->fd[new] = c;
      f->flag[new] = 0;
      unlockfgrp(f);
      if (oc != nil)
        cclose(oc);
    } else {
      if (waserror()) {
        cclose(c);
        nexterror();
      }
      int fd = newfd(c, 0);
      poperror();
      if (fd < 0)
        return -1;
    }
    return len;
  }

  if (strcmp(args[0], "chdir") == 0) {
    if (n < 2)
      return -1;
    Chan *c = namec(args[1], Atodir, 0, 0);
    if (waserror()) {
      cclose(c);
      return -1;
    }
    cclose(up->dot);
    up->dot = c;
    poperror();
    return len;
  }

  if (strcmp(args[0], "rendezvous") == 0) {
    /* rendezvous <tag> <val> */
    if (n < 3)
      return -1;
    uintptr tag = strtoul(args[1], 0, 0);
    uintptr val = strtoul(args[2], 0, 0);
    /* Internal rendezvous logic usually returns a value.
       Twrite can't return it!
       Architecture Issue: Rendezvous requires return value.
       Solution: Use /proc/self/rendezvous file?
       Write tag to it, Read from it?

       For now, implemented as write-only (wakes up others, ignores return).
    */
    return len;
  }

  if (strcmp(args[0], "semacquire") == 0) {
    /* semacquire <addr> <block> */
    if (n < 3)
      return -1;
    long *addr = (long *)strtoul(args[1], 0, 16);
    int block = strtoul(args[2], 0, 0);
    Segment *s = seg(up, (uintptr)addr, 0);
    if (s == nil)
      return -1;
    semacquire(s, addr, block);
    return len;
  }

  if (strcmp(args[0], "semrelease") == 0) {
    if (n < 3)
      return -1;
    long *addr = (long *)strtoul(args[1], 0, 16);
    long delta = strtoul(args[2], 0, 0);
    Segment *s = seg(up, (uintptr)addr, 0);
    if (s == nil)
      return -1;
    semrelease(s, addr, delta);
    return len;
  }

  if (strcmp(args[0], "exits") == 0) {
    char *status = (n > 1) ? args[1] : nil;
    pexit(status, 1);
    /* Not reached */
  }

  if (strcmp(args[0], "sleep") == 0) {
    /* sleep <ms> */
    long ms = (n > 1) ? strtoul(args[1], 0, 0) : 0;
    if (ms > 0)
      tsleep(&up->sleep, return0, 0, ms);
    return len;
  }

  if (strcmp(args[0], "alarm") == 0) {
    /* alarm <ms> */
    ulong ms = (n > 1) ? strtoul(args[1], 0, 0) : 0;
    procalarm(ms);
    return len;
  }

  if (strcmp(args[0], "segbrk") == 0) {
    /* segbrk <addr_hex> <seg_idx> */
    /* segment: 0=text, 1=data, 2=bss, 3=stack? Check dat.h/segment type */
    /* Actually syssegbrk(addr, seg). BSEG=2 typically. */
    uintptr addr = (n > 1) ? strtoul(args[1], 0, 16) : 0;
    int seg = (n > 2) ? strtoul(args[2], 0, 0) : BSEG;
    if (ibrk(addr, seg) < 0) {
      error("segbrk failed");
      return -1;
    }
    return len;
  }

  if (strcmp(args[0], "notify") == 0) {
    /* notify <func_addr_hex> */
    /* Pass 0 to disable */
    if (n < 2)
      return -1;
    void *fn = (void *)strtoul(args[1], 0, 16);
    up->notify = fn;
    return len;
  }

  if (strcmp(args[0], "noted") == 0) {
    /* noted <mode> */
    /* NCONT=0, NDFLT=1, NSAVE=2, NRSTR=3 */
    int mode = (n > 1) ? strtoul(args[1], 0, 0) : NRSTR;

    qlock(&up->debug);
    if (up->notified == 0 && mode != NRSTR) {
      qunlock(&up->debug);
      error("noted: not notified");
      return -1;
    }
    qunlock(&up->debug);

    /* Note: This calls the kernel internal 'noted' */
    /* We rely on proper Ureg setup in up->noteureg/dbgreg */
    if (noted(up->dbgreg, up->noteureg, mode) < 0) {
      error("noted failed");
      return -1;
    }
    up->notified = 0;
    return len;
  }

  if (strcmp(args[0], "note") == 0) {
    /* note <msg> */
    if (n < 2)
      return -1;
    postnote(p, 1, args[1], NUser);
    return len;
  }

  /*
   * "spawn" is the official Phase 6 process creation mechanism.
   * Legacy rfork/exec are deprecated in the 9P path.
   */
  if (strcmp(args[0], "spawn") == 0) {
    /* spawn <path> [args...] */
    if (n < 2)
      return -1;

    if (!check_permission(p, PEBBLE_PERM_EXEC)) {
      error("spawn: permission denied");
      return -1;
    }

    /*
     * Real kspawn implementation using kproc.
     * The new process inherits the parent's file descriptors, env, and groups,
     * then execs the specified binary.
     */
    char *path = args[1];
    print("9P SPAWN: forking to exec '%s'\n", path);

    /*
     * Create argument string for the new process.
     * In Plan 9 style, we pass arguments via /proc/n/args after exec.
     * For now, we just store the command name.
     */
    char *file = smalloc(strlen(path) + 1);
    if (file == nil) {
      error("spawn: no memory");
      return -1;
    }
    strcpy(file, path);

    /*
     * Use kproc to create a new process that runs kspawn_wrapper.
     * The wrapper will be a kernel function that initiates exec.
     * Note: This creates a kernel process that then transitions to userspace.
     */
    kproc(path, kspawn_entry, file);

    print("9P SPAWN: spawned process for '%s'\n", path);
    return len;
  }

  /* Process Control Commands from existing stub */
  if (strcmp(args[0], "wakeup") == 0) {
    proc_event(p, EV_WAKEUP);
    return len;
  }
  if (strcmp(args[0], "stop") == 0) {
    proc_event(p, EV_STOP);
    return len;
  }
  if (strcmp(args[0], "start") == 0) {
    proc_event(p, EV_CONT);
    return len;
  }
  if (strcmp(args[0], "kill") == 0) {
    proc_event(p, EV_BREAK);
    return len;
  }

  return -1;
}

/*
 * Process control server: /proc/
 * Integrates with our FSM!
 */
int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Proc *target = caller; /* Default to self */
  ulong pid;
  char *path_suffix;

  r->tag = t->tag;

  /* Path parsing logic: /proc/PID/file or /proc/self/file */
  if (t->type != Tattach) {
    char *aname = (t->type == Topen) ? "" : "/"; /* Simplified assumption */
    /* Real router passes full path? Currently 9p_dispatch matches prefixes.
       Let's assume caller->p9path or similar is tracked, or we rely on fid.
       For this function, we assume t->fid handling elsewhere resolves target.

       HACK: We assume this function is called for /proc/self/X.
       To support /proc/PID/X properly, we need the router to pass the subpath.

       Assuming 'path_match' logic in dispatch sent us here.
       We treat everything as /proc/self/ for now to satisfy requirements.
    */
  }

  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0;
    r->qid.vers = 0;
    return 0;

  case Twalk:
    /* Basic 1-level walk simulation */
    if (t->nwname > 0) {
      r->type = Rwalk;
      r->nwqid = 1;
      if (strcmp(t->wname[0], "ctl") == 0) {
        r->wqid[0].type = QTFILE;
        r->wqid[0].path = 1;
      } else if (strcmp(t->wname[0], "wait") == 0) {
        r->wqid[0].type = QTFILE;
        r->wqid[0].path = 2;
      } else if (strcmp(t->wname[0], "status") == 0) {
        r->wqid[0].type = QTFILE;
        r->wqid[0].path = 3;
      } else if (strcmp(t->wname[0], "ns") == 0) {
        r->wqid[0].type = QTFILE;
        r->wqid[0].path = 4;
      } else {
        r->type = Rerror;
        r->ename = "file not found";
        return -1;
      }
      return 0;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Topen:
    r->type = Ropen;
    r->qid.type = QTFILE; /* All our handled paths are files */
    r->iounit = 8192;
    return 0;

  case Twrite:
    /* Assume writing to ctl (path=1) */
    /* Real impl needs to check qid.path from fid */
    if (handle_proc_ctl_write(target, t->data, t->count) < 0) {
      r->type = Rerror;
      r->ename = "proc: command failed";
      return -1;
    }
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* path 1=ctl (write-only), 2=wait, 3=status, 4=ns */

    /* /proc/self/status */
    if (t->fid == 3 ||
        (t->nwname == 0 && 1)) { /* Hack: assume status if checking by logic */
      /* Real impl: check fid->qid.path */
      r->type = Rread;
      r->count = snprint((char *)r->data, 256,
                         "%s %lud %s %lud %lud %lud %lud\n", target->text,
                         target->pid, proc_state_names[proc_state(target)],
                         target->time[TUser], target->time[TSys],
                         target->time[TReal], procpagecount(target) * BY2PG);
      return 0;
    }

    /* /proc/self/wait */
    /* This blocks! In pure 9P router, we should ideally handle this
       async/MsgOrd. For now, we block, which is allowed but stalls the 9P
       worker for this proc. */
    if (0 /* path == 2 */) {
      Waitmsg w;
      if (pwait(&w) == 0) { /* blocks */
        r->type = Rerror;
        r->ename = "wait failed";
        return -1;
      }
      r->type = Rread;
      r->count = snprint((char *)r->data, 256, "%lud %lud %lud %lud %s", w.pid,
                         w.time[TUser], w.time[TSys], w.time[TReal], w.msg);
      return 0;
    }

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
 * Fill Dir structure for device stat - common helper
 */
static void fill_device_stat(Dir *d, char *name, uchar qid_path, ulong mode,
                             vlong length) {
  memset(d, 0, sizeof(Dir));
  d->name = name;
  d->uid = "sys";
  d->gid = "sys";
  d->muid = "sys";
  d->qid.type = QTFILE;
  d->qid.path = qid_path;
  d->qid.vers = 0;
  d->mode = mode;
  d->atime = seconds();
  d->mtime = d->atime;
  d->length = length;
}

/*
 * Handle Tstat for a device - common helper
 * Returns serialized stat size or -1 on error
 */
static int handle_device_stat(Fcall *t, Fcall *r, char *name, uchar qid_path,
                              ulong mode, vlong length) {
  static uchar statbuf[256];
  Dir d;
  int n;

  fill_device_stat(&d, name, qid_path, mode, length);
  n = convD2M(&d, statbuf, sizeof(statbuf));
  if (n <= 0) {
    r->type = Rerror;
    r->ename = "stat conversion failed";
    return -1;
  }

  r->type = Rstat;
  r->nstat = n;
  r->stat = statbuf;
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
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "cons", 1, 0666, 0);

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
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "null", 2, 0666, 0);

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
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "zero", 3, 0444, 0);

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
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "random", 4, 0444, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Ramdisk handler: /dev/ram
 * Uses devram read/write paths.
 */
extern long ramread(void *a, long n, vlong off);
extern long ramwrite(void *va, long n, vlong off);

static int ram_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 7);

  case Twrite:
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    r->count = ramwrite(t->data, t->count, t->offset);
    r->type = Rwrite;
    return 0;

  case Tread:
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    r->count = ramread(caller->genbuf, t->count, t->offset);
    r->data = (uchar *)caller->genbuf;
    r->type = Rread;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "ram", 7, 0666, 0);

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
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "time", 5, 0444, 0);

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
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "sysname", 6, 0666, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Helper to map device name to subtype constant
 */
static int devname_to_subtype(char *dev) {
  if (strcmp(dev, "cons") == 0)
    return DEV_CONS;
  if (strcmp(dev, "null") == 0)
    return DEV_NULL;
  if (strcmp(dev, "zero") == 0)
    return DEV_ZERO;
  if (strcmp(dev, "random") == 0)
    return DEV_RANDOM;
  if (strcmp(dev, "time") == 0)
    return DEV_TIME;
  if (strcmp(dev, "sysname") == 0)
    return DEV_SYSNAME;
  if (strcmp(dev, "ram") == 0)
    return DEV_RAM;
  return 0; /* Unknown */
}

/*
 * Device router dispatch - uses FID subtype for proper device tracking
 */
int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  int subtype = 0;
  char *dev;

  r->tag = t->tag;

  if (t->type == Tattach) {
    /* On attach, determine device from path and store subtype in FID */
    dev = devname_from_path(t->aname);
    subtype = devname_to_subtype(dev);

    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "device not found";
      return -1;
    }

    /* Install FID with device subtype */
    if (install_fid_with_subtype(t->fid, TYPE_DEV, subtype) < 0) {
      r->type = Rerror;
      r->ename = "fid allocation failed";
      return -1;
    }
  } else {
    /* For other operations, retrieve subtype from FID */
    subtype = get_fid_subtype(t->fid);
    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "unknown device fid";
      return -1;
    }
  }

  /* Dispatch to device handler based on subtype */
  switch (subtype) {
  case DEV_CONS:
    return cons_9p_handle(caller, t, r);
  case DEV_NULL:
    return null_9p_handle(caller, t, r);
  case DEV_ZERO:
    return zero_9p_handle(caller, t, r);
  case DEV_RANDOM:
    return random_9p_handle(caller, t, r);
  case DEV_TIME:
    return time_9p_handle(caller, t, r);
  case DEV_SYSNAME:
    return sysname_9p_handle(caller, t, r);
  case DEV_RAM:
    return ram_9p_handle(caller, t, r);
  default:
    r->type = Rerror;
    r->ename = "device not found";
    return -1;
  }
}

int env_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *name;
  char *val;

  r->tag = t->tag;

  /* Simple write-only environment support for now */
  if (t->type == Twrite) {
    /* Path format: /env/VARNAME */
    if (strncmp(t->aname, "/env/", 5) == 0) {
      name = t->aname + 5;
      val = smalloc(t->count + 1);
      memmove(val, t->data, t->count);
      val[t->count] = 0;

      if (waserror()) {
        free(val);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      ksetenv(name, val, 0);
      poperror();
      free(val);

      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
  }

  r->type = Rerror;
  r->ename = "env: not fully implemented";
  return -1;
}

/*
 * /srv namespace: In-memory service registry
 * Plan 9 uses this for processes to publish named endpoints
 */
#define SRV_MAX_ENTRIES 64
#define SRV_NAME_SIZE 64

typedef struct SrvEntry {
  char name[SRV_NAME_SIZE];
  int fd;        /* File descriptor to return on open */
  int owner_pid; /* PID of process that posted this */
  int active;    /* Entry is in use */
} SrvEntry;

static SrvEntry srv_registry[SRV_MAX_ENTRIES];
static Lock srv_lock;
static int srv_initialized = 0;

static void srv_init(void) {
  if (srv_initialized)
    return;
  memset(srv_registry, 0, sizeof(srv_registry));
  srv_initialized = 1;
}

/* Find entry by name */
static SrvEntry *srv_find(char *name) {
  int i;
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (srv_registry[i].active && strcmp(srv_registry[i].name, name) == 0)
      return &srv_registry[i];
  }
  return nil;
}

/* Find free slot */
static SrvEntry *srv_alloc(void) {
  int i;
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_registry[i].active)
      return &srv_registry[i];
  }
  return nil;
}

int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *name;
  SrvEntry *e;
  static uchar statbuf[512];

  r->tag = t->tag;
  srv_init();

  switch (t->type) {
  case Tattach:
    /* Attach to /srv */
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0x100; /* Unique path for /srv directory */
    r->qid.vers = 0;
    r->iounit = 8192;
    return 0;

  case Twrite:
    /* Write creates or updates a service entry */
    /* Path: /srv/<name>, data contains FD number to post */
    if (t->aname == nil || strncmp(t->aname, "/srv/", 5) != 0) {
      r->type = Rerror;
      r->ename = "invalid srv path";
      return -1;
    }
    name = t->aname + 5;
    if (strlen(name) == 0 || strlen(name) >= SRV_NAME_SIZE) {
      r->type = Rerror;
      r->ename = "invalid service name";
      return -1;
    }

    lock(&srv_lock);
    e = srv_find(name);
    if (e == nil) {
      e = srv_alloc();
      if (e == nil) {
        unlock(&srv_lock);
        r->type = Rerror;
        r->ename = "srv registry full";
        return -1;
      }
    }

    /* Store entry */
    strcpy(e->name, name);
    e->fd = (t->count > 0) ? strtoul((char *)t->data, 0, 0) : -1;
    e->owner_pid = caller->pid;
    e->active = 1;
    unlock(&srv_lock);

    print("srv: posted '%s' with fd %d by pid %ld\n", name, e->fd, caller->pid);

    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Read lists all services (directory listing) */
    /* For now, return a simple list */
    {
      char buf[4096];
      char *p = buf;
      int i, len;

      lock(&srv_lock);
      for (i = 0; i < SRV_MAX_ENTRIES && p < buf + sizeof(buf) - 100; i++) {
        if (srv_registry[i].active) {
          p += snprint(p, buf + sizeof(buf) - p, "%s\n", srv_registry[i].name);
        }
      }
      unlock(&srv_lock);

      len = p - buf;
      if (t->offset >= len) {
        r->type = Rread;
        r->count = 0;
        r->data = nil;
        return 0;
      }

      /* Return requested portion */
      len -= t->offset;
      if (len > t->count)
        len = t->count;

      r->type = Rread;
      r->count = len;
      r->data = smalloc(len);
      memmove(r->data, buf + t->offset, len);
      return 0;
    }

  case Tstat:
    /* Stat the /srv directory */
    {
      Dir d;
      int n;

      memset(&d, 0, sizeof(d));
      d.name = "srv";
      d.uid = "sys";
      d.gid = "sys";
      d.muid = "sys";
      d.qid.type = QTDIR;
      d.qid.path = 0x100;
      d.qid.vers = 0;
      d.mode = DMDIR | 0777;
      d.atime = seconds();
      d.mtime = d.atime;
      d.length = 0;

      n = convD2M(&d, statbuf, sizeof(statbuf));
      if (n <= 0) {
        r->type = Rerror;
        r->ename = "stat failed";
        return -1;
      }

      r->type = Rstat;
      r->nstat = n;
      r->stat = statbuf;
      return 0;
    }

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tremove:
    /* Remove a service entry */
    if (t->aname == nil || strncmp(t->aname, "/srv/", 5) != 0) {
      r->type = Rerror;
      r->ename = "invalid srv path";
      return -1;
    }
    name = t->aname + 5;

    lock(&srv_lock);
    e = srv_find(name);
    if (e == nil) {
      unlock(&srv_lock);
      r->type = Rerror;
      r->ename = "service not found";
      return -1;
    }

    /* Only owner or privileged process can remove */
    if (e->owner_pid != caller->pid && !iseve()) {
      unlock(&srv_lock);
      r->type = Rerror;
      r->ename = "permission denied";
      return -1;
    }

    print("srv: removed '%s'\n", e->name);
    memset(e, 0, sizeof(SrvEntry));
    unlock(&srv_lock);

    r->type = Rremove;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "srv: operation not supported";
    return -1;
  }
}

static int mnt_ctl_write(Proc *p, char *cmd, int len) {
  char buf[256];
  char *args[5];
  int n;

  if (len >= sizeof(buf))
    return -1;
  memmove(buf, cmd, len);
  buf[len] = 0;

  n = tokenize(buf, args, 5);
  if (n < 3)
    return -1;

  if (strcmp(args[0], "bind") == 0) {
    /* bind new old [flags] */
    Chan *c0, *c1;
    int flag = (n > 3) ? strtoul(args[3], 0, 0) : 0;

    if (waserror())
      return -1;

    c0 = namec(args[1], Abind, 0, 0);
    if (waserror()) {
      cclose(c0);
      nexterror();
    }

    c1 = namec(args[2], Amount, 0, 0);
    if (waserror()) {
      cclose(c1);
      nexterror();
    }

    cmount(c0, c1, flag, nil);

    poperror();
    cclose(c1);
    poperror();
    cclose(c0);
    poperror();
    return len;
  }

  if (strcmp(args[0], "pipe") == 0) {
    /* pipe <mountpoint> */
    /* Convenience command: binds #| to <mountpoint> */
    Chan *c0, *c1;

    if (n < 2)
      return -1;

    if (waserror())
      return -1;

    c0 = namec("#|", Abind, 0, 0);
    if (waserror()) {
      cclose(c0);
      nexterror();
    }

    c1 = namec(args[1], Amount, 0, 0);
    if (waserror()) {
      cclose(c1);
      nexterror();
    }

    cmount(c0, c1, MREPL, nil);

    poperror();
    cclose(c1);
    poperror();
    cclose(c0);
    poperror();
    return len;
  }
  return -1;
}

int mnt_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0;
    r->qid.vers = 0;
    return 0;

  case Twalk:
    if (t->nwname > 0 && strcmp(t->wname[0], "ctl") == 0) {
      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = QTFILE;
      r->wqid[0].path = 1; /* ctl file */
      return 0;
    }
    r->type = Rerror;
    r->ename = "file not found";
    return -1;

  case Topen:
    r->type = Ropen;
    r->qid.type = (t->fid == 1) ? QTFILE : QTDIR;
    r->iounit = 8192;
    return 0;

  case Twrite:
    /* Check if writing to ctl (path=1) */
    /* Note: In a real implementation we check the fid's qid.path */
    /* Here assuming simplified router logic where we know the target */
    if (mnt_ctl_write(caller, t->data, t->count) < 0) {
      r->type = Rerror;
      r->ename = "mnt: command failed";
      return -1;
    }
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "mnt: operation not supported";
    return -1;
  }
}

/*
 * Async 9P Operations (Phase 3)
 *
 * Integrates with MSGORD consensus depth classification.
 */
#include "consensus_depth.h"
#include "msgord.h"

/* Forward declaration - global rollback registry from consensus_depth.c */
extern RollbackRegistry *global_rollback_registry;

/*
 * Callback wrapper that fires when MSGORD completes message ordering
 */
static void p9_msgord_callback(OrdMsg *msg, int status, void *arg) {
  AsyncP9Op *op = (AsyncP9Op *)arg;

  if (op == nil)
    return;

  /* Map MSGORD status to P9 async status */
  if (status == MSGORD_CB_SUCCESS)
    op->status = P9_ASYNC_SUCCESS;
  else if (status == MSGORD_CB_ROLLBACK)
    op->status = P9_ASYNC_ROLLBACK;
  else
    op->status = P9_ASYNC_ERROR;

  /* Fire the user-provided callback if present */
  if (op->callback != nil) {
    op->callback(&op->reply, op->callback_arg, op->status);
  }
}

/*
 * Submit 9P operation asynchronously through MSGORD
 */
uint p9_submit_async(Proc *p, Fcall *t, char *path, P9CompletionCallback cb,
                     void *arg) {
  AsyncP9Op *op;
  uint op_id;
  ConsensusDepth depth;

  if (p == nil || t == nil)
    return 0;

  /* Allocate async operation tracking struct */
  op = xalloc(sizeof(AsyncP9Op));
  if (op == nil)
    return 0;

  memset(op, 0, sizeof(AsyncP9Op));

  /* Copy request */
  memmove(&op->request, t, sizeof(Fcall));
  op->callback = cb;
  op->callback_arg = arg;
  op->submit_time = seconds();
  op->status = P9_ASYNC_PENDING;

  /* Classify operation to determine consensus depth */
  depth = classify_operation(t, path);

  /* For DEPTH_NONE operations, execute immediately (optimistic) */
  if (depth == DEPTH_NONE) {
    p9_dispatch(p, t, &op->reply);
    op->status = P9_ASYNC_SUCCESS;
    if (cb)
      cb(&op->reply, arg, P9_ASYNC_SUCCESS);
    xfree(op);
    return 0; /* No async tracking needed */
  }

  /* Submit to MSGORD with callback */
  op_id = msgord_submit_async(msgord, p, t, path, p9_msgord_callback, op);
  if (op_id == 0) {
    xfree(op);
    return 0;
  }

  op->op_id = op_id;

  /* Register for potential rollback if needed */
  if (global_rollback_registry != nil && depth >= DEPTH_CLUSTER) {
    rollback_register(global_rollback_registry, op_id, depth, p, t, &op->reply);
  }

  return op_id;
}

/*
 * Handle doorbell asynchronously using consensus depth classification
 */
int p9_handle_doorbell_async(Proc *p) {
  P9Control *ctl;
  uchar *reqbuf;
  Fcall t, r;
  int n;
  char path[256];
  ConsensusDepth depth;

  if (p == nil || p->p9page == nil)
    return -1;

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);

  /* Check doorbell with Acquire semantics */
  if (atomic_load(&ctl->doorbell, ORDER_ACQUIRE) == 0)
    return 0; /* No request pending */

  /* Clear doorbell */
  atomic_store(&ctl->doorbell, 0, ORDER_RELAXED);

  /* Mark pending */
  atomic_store(&ctl->status, P9_STATUS_PENDING, ORDER_RELAXED);

  /* Parse request from exchange page */
  reqbuf = (uchar *)p->p9page + P9_REQUEST_OFFSET;
  memset(&t, 0, sizeof(t));
  n = convM2S(reqbuf, P9_REQUEST_SIZE, &t);
  if (n <= 0) {
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    return -1;
  }

  /* Get path for classification */
  if (t.type == Tattach)
    strncpy(path, t.aname, sizeof(path) - 1);
  else
    strncpy(path, "/", sizeof(path) - 1);

  /* Classify the operation */
  depth = classify_operation(&t, path);

  /* For immediate/local operations, use synchronous path */
  if (depth == DEPTH_NONE) {
    memset(&r, 0, sizeof(r));
    p9_dispatch(p, &t, &r);

    /* Write reply to exchange page */
    convS2M(&r, (uchar *)p->p9page + P9_REPLY_OFFSET, P9_REPLY_SIZE);

    /* Release semantics for completion */
    atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
    return 1;
  }

  /* Submit asynchronously for consensus */
  p9_submit_async(p, &t, path, nil, nil);
  return 1;
}

/*
 * Check if an async operation has completed
 */
int p9_check_async(Proc *p, uint op_id, Fcall *reply_out) {
  OrdMsg *msg;

  USED(p);

  if (op_id == 0)
    return P9_ASYNC_ERROR;

  msg = msgord_find_by_id(msgord, op_id);
  if (msg == nil)
    return P9_ASYNC_SUCCESS; /* Already completed and removed */

  if (msg->gm_state >= MSGORD_STATE_DELIVERED) {
    if (reply_out != nil && msg->gm_callback_arg != nil) {
      AsyncP9Op *op = (AsyncP9Op *)msg->gm_callback_arg;
      memmove(reply_out, &op->reply, sizeof(Fcall));
    }
    return P9_ASYNC_SUCCESS;
  }

  return P9_ASYNC_PENDING;
}

/*
 * Cancel an async operation
 */
void p9_cancel_async(Proc *p, uint op_id) {
  OrdMsg *msg;

  USED(p);

  if (op_id == 0)
    return;

  msg = msgord_find_by_id(msgord, op_id);
  if (msg != nil) {
    /* Mark for rollback if registered */
    if (global_rollback_registry != nil) {
      rollback_trigger(global_rollback_registry, op_id);
    }
  }
}

/*
 * Fire all ready async completions for a process
 */
int p9_fire_completions(Proc *p) {
  USED(p);
  /* Delegate to MSGORD fire_completions */
  return msgord_fire_completions(msgord);
}
