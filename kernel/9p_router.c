/*
 * Lux9 9P Router Implementation
 *
 * Routes 9P messages from Exchange Pages to kernel services.
 */

#include "u.h"
#include "portlib.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by
 * kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"
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
		P9Control *ctl = (P9Control*)((uintptr)p->p9page + P9_CONTROL_OFFSET);
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

static int install_fid(int fid, int type) {
    Chan *c;
    Fgrp *f = up->fgrp;
    
    c = newchan();
    if (c == nil) return -1;
    c->type = ROUTER_CHAN_TYPE; /* Mark as virtual router channel */
    c->qid.path = type;         /* Store handler type in Qid path */
    c->mode = ORDWR;
    c->ref = 1;

    lock(&f->lock);
    if (fid < 0 || growfd(f, fid) < 0) {
        unlockfgrp(f);
        cclose(c);
        return -1;
    }
    if (fid > f->maxfd) f->maxfd = fid;
    if (f->fd[fid]) cclose(f->fd[fid]);
    f->fd[fid] = c;
    f->flag[fid] = 0;
    unlockfgrp(f);
    return 0;
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

static void remove_fid(int fid) {
    fdclose(fid, 0);
}

/*
 * Dispatch message to appropriate handler
 */
int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  int type = 0;

  if (t->type == Tattach) {
      /* Determine type from path */
      if (path_match(t->aname, "/proc/") || strcmp(t->aname, "/proc") == 0) type = TYPE_PROC;
      else if (path_match(t->aname, "/dev/") || strcmp(t->aname, "/dev") == 0) type = TYPE_DEV;
      else if (path_match(t->aname, "/env/") || strcmp(t->aname, "/env") == 0) type = TYPE_ENV;
      else if (path_match(t->aname, "/srv/") || strcmp(t->aname, "/srv") == 0) type = TYPE_SRV;
      else if (path_match(t->aname, "/mnt/") || strcmp(t->aname, "/mnt") == 0) type = TYPE_MNT;
      
      if (type > 0) {
          if (install_fid(t->fid, type) < 0) {
              r->type = Rerror;
              r->ename = "fid allocation failed";
              return -1;
          }
      }
  } else {
      /* Lookup type from fid */
      type = get_fid_type(t->fid);
      if (t->type == Tclunk) remove_fid(t->fid);
  }

  if (type == TYPE_PROC) return proc_9p_handle(p, t, r);
  if (type == TYPE_DEV) return dev_9p_handle(p, t, r);
  if (type == TYPE_ENV) return env_9p_handle(p, t, r);
  if (type == TYPE_SRV) return srv_9p_handle(p, t, r);
  if (type == TYPE_MNT) return mnt_9p_handle(p, t, r);

  r->type = Rerror;
  r->ename = "fid not found or unknown path";
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

  /* Dispatch through 9P router with GHOSTDAG ordering */
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
 * Helper: Handle writes to /proc/self/ctl
 */
static int handle_proc_ctl_write(Proc *p, char *cmd, int len) {
    char buf[256];
    char *args[16];
    int n;
    
    if (len >= sizeof(buf)) return -1;
    memmove(buf, cmd, len);
    buf[len] = 0;
    
    n = tokenize(buf, args, nelem(args));
    if (n < 1) return -1;
    
    if (strcmp(args[0], "dup") == 0) {
        /* dup old [new] */
        if (n < 2) return -1;
        int old = strtoul(args[1], 0, 0);
        int new = (n > 2) ? strtoul(args[2], 0, 0) : -1;
        
        Chan *c = fdtochan(old, -1, 0, 1);
        if (c == nil) return -1;
        
        if (new != -1) {
            Fgrp *f = up->fgrp;
            lock(&f->lock);
            if (new < 0 || growfd(f, new) < 0) {
                unlockfgrp(f);
                cclose(c);
                return -1;
            }
            if (new > f->maxfd) f->maxfd = new;
            
            Chan *oc = f->fd[new];
            f->fd[new] = c;
            f->flag[new] = 0;
            unlockfgrp(f);
            if (oc != nil) cclose(oc);
        } else {
            if (waserror()) { cclose(c); nexterror(); }
            int fd = newfd(c, 0);
            poperror();
            if (fd < 0) return -1;
        }
        return len;
    }

    if (strcmp(args[0], "chdir") == 0) {
        if (n < 2) return -1;
        Chan *c = namec(args[1], Atodir, 0, 0);
        if (waserror()) { cclose(c); return -1; }
        cclose(up->dot);
        up->dot = c;
        poperror();
        return len;
    }

    if (strcmp(args[0], "rendezvous") == 0) {
        /* rendezvous <tag> <val> */
        if (n < 3) return -1;
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
        if (n < 3) return -1;
        long *addr = (long*)strtoul(args[1], 0, 16);
        int block = strtoul(args[2], 0, 0);
        Segment *s = seg(up, (uintptr)addr, 0);
        if (s == nil) return -1;
        semacquire(s, addr, block);
        return len;
    }
    
    if (strcmp(args[0], "semrelease") == 0) {
        if (n < 3) return -1;
        long *addr = (long*)strtoul(args[1], 0, 16);
        long delta = strtoul(args[2], 0, 0);
        Segment *s = seg(up, (uintptr)addr, 0);
        if (s == nil) return -1;
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
        if (ms > 0) tsleep(&up->sleep, return0, 0, ms);
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
        if (n < 2) return -1;
        void *fn = (void*)strtoul(args[1], 0, 16);
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
        if (n < 2) return -1;
        postnote(p, 1, args[1], NUser);
        return len;
    }
    
    /* 
     * "spawn" is the official Phase 6 process creation mechanism.
     * Legacy rfork/exec are deprecated in the 9P path.
     */
    if (strcmp(args[0], "spawn") == 0) {
        /* spawn <path> [args...] */
        if (n < 2) return -1;
        
        if (!check_permission(p, PEBBLE_PERM_EXEC)) {
            /* r->ename will be set by caller on -1? No, we need to set errstr */
            /* error() sets up->errstr which is what we want */
            error("spawn: permission denied");
            return -1;
        }
        
        /* 
         * TODO: Implement actual kspawn(path, argv)
         * This requires:
         * 1. newproc()
         * 2. loading 'path' into new process memory (like sysexec but for other proc)
         * 3. copying args
         * 4. ready(new_proc)
         */
        print("9P SPAWN: %s (simulated)\n", args[1]);
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
            r->wqid[0].type = QTFILE; r->wqid[0].path = 1;
        } else if (strcmp(t->wname[0], "wait") == 0) {
            r->wqid[0].type = QTFILE; r->wqid[0].path = 2;
        } else if (strcmp(t->wname[0], "status") == 0) {
            r->wqid[0].type = QTFILE; r->wqid[0].path = 3;
        } else if (strcmp(t->wname[0], "ns") == 0) {
            r->wqid[0].type = QTFILE; r->wqid[0].path = 4;
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
    if (t->fid == 3 || (t->nwname==0 && 1)) { /* Hack: assume status if checking by logic */
        /* Real impl: check fid->qid.path */
        r->type = Rread;
        r->count = snprint((char *)r->data, 256,
                           "%s %lud %s %lud %lud %lud %lud\n",
                           target->text, target->pid, 
                           proc_state_names[proc_state(target)],
                           target->time[TUser], target->time[TSys],
                           target->time[TReal], 
                           procpagecount(target)*BY2PG);
        return 0;
    }
    
    /* /proc/self/wait */
    /* This blocks! In pure 9P router, we should ideally handle this async/GhostDAG.
       For now, we block, which is allowed but stalls the 9P worker for this proc. */
    if (0 /* path == 2 */) { 
        Waitmsg w;
        if (pwait(&w) == 0) { /* blocks */
             r->type = Rerror;
             r->ename = "wait failed";
             return -1;
        }
        r->type = Rread;
        r->count = snprint((char *)r->data, 256,
                           "%lud %lud %lud %lud %s",
                           w.pid, w.time[TUser], w.time[TSys], w.time[TReal], w.msg);
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
  if (strcmp(dev, "ram") == 0)
    return ram_9p_handle(caller, t, r);

  /* Device not found */
  r->type = Rerror;
  r->ename = "device not found";
  return -1;
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

int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  r->type = Rerror;
  r->ename = "srv: not implemented";
  return -1;
}

static int mnt_ctl_write(Proc *p, char *cmd, int len) {
    char buf[256];
    char *args[5];
    int n;
    
    if (len >= sizeof(buf)) return -1;
    memmove(buf, cmd, len);
    buf[len] = 0;
    
    n = tokenize(buf, args, 5);
    if (n < 3) return -1;
    
    if (strcmp(args[0], "bind") == 0) {
        /* bind new old [flags] */
        Chan *c0, *c1;
        int flag = (n > 3) ? strtoul(args[3], 0, 0) : 0;
        
        if (waserror()) return -1;
        
        c0 = namec(args[1], Abind, 0, 0);
        if (waserror()) { cclose(c0); nexterror(); }
        
        c1 = namec(args[2], Amount, 0, 0);
        if (waserror()) { cclose(c1); nexterror(); }
        
        cmount(c0, c1, flag, nil);
        
        poperror(); cclose(c1);
        poperror(); cclose(c0);
        poperror();
        return len;
    }

    if (strcmp(args[0], "pipe") == 0) {
        /* pipe <mountpoint> */
        /* Convenience command: binds #| to <mountpoint> */
        Chan *c0, *c1;
        
        if (n < 2) return -1;
        
        if (waserror()) return -1;
        
        c0 = namec("#|", Abind, 0, 0);
        if (waserror()) { cclose(c0); nexterror(); }
        
        c1 = namec(args[1], Amount, 0, 0);
        if (waserror()) { cclose(c1); nexterror(); }
        
        cmount(c0, c1, MREPL, nil);
        
        poperror(); cclose(c1);
        poperror(); cclose(c0);
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
 * Integrates with GHOSTDAG consensus depth classification.
 */
#include "consensus_depth.h"
#include "ghostdag_kernel.h"

/* Forward declaration - global rollback registry from consensus_depth.c */
extern RollbackRegistry *global_rollback_registry;

/*
 * Callback wrapper that fires when GHOSTDAG completes message ordering
 */
static void p9_ghostdag_callback(GhostMsg *msg, int status, void *arg) {
  AsyncP9Op *op = (AsyncP9Op *)arg;

  if (op == nil)
    return;

  /* Map GHOSTDAG status to P9 async status */
  if (status == GHOSTDAG_CB_SUCCESS)
    op->status = P9_ASYNC_SUCCESS;
  else if (status == GHOSTDAG_CB_ROLLBACK)
    op->status = P9_ASYNC_ROLLBACK;
  else
    op->status = P9_ASYNC_ERROR;

  /* Fire the user-provided callback if present */
  if (op->callback != nil) {
    op->callback(&op->reply, op->callback_arg, op->status);
  }
}

/*
 * Submit 9P operation asynchronously through GHOSTDAG
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

  /* Submit to GHOSTDAG with callback */
  op_id = ghostdag_submit_async(ghostdag, p, t, path, p9_ghostdag_callback, op);
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
  GhostMsg *msg;

  USED(p);

  if (op_id == 0)
    return P9_ASYNC_ERROR;

  msg = ghostdag_find_by_id(ghostdag, op_id);
  if (msg == nil)
    return P9_ASYNC_SUCCESS; /* Already completed and removed */

  if (msg->gm_state >= GHOSTDAG_STATE_DELIVERED) {
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
  GhostMsg *msg;

  USED(p);

  if (op_id == 0)
    return;

  msg = ghostdag_find_by_id(ghostdag, op_id);
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
  /* Delegate to GHOSTDAG fire_completions */
  return ghostdag_fire_completions(ghostdag);
}
