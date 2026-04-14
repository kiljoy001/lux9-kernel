#include "../include/proc_packet.h"
#include "router.h"
#include <error.h>

/*
 * Process control server: /proc/
 * Integrates with our FSM!
 */
enum { PROC9_MAX_FIDS = 1024 };

typedef struct Proc9Fid {
  Proc *owner;
  ulong owner_pid;
  u32int fid;
  char *path;
  Qid qid;
  uchar open_mode;
  uchar inuse;
} Proc9Fid;

static Proc9Fid proc9fids[PROC9_MAX_FIDS];
static Lock proc9fidlk;

static int proc9_raw_attach_match(const char *path) {
  return path != nil && strncmp(path, "#p", 2) == 0;
}

static int proc9_path_match(const char *path) {
  if (path == nil)
    return 0;
  if (strncmp(path, "/proc", 5) != 0)
    return 0;
  return path[5] == 0 || path[5] == '/';
}

static int proc9_bootstrap_attach_match(const char *aname) {
  if (aname == nil || aname[0] == 0)
    return 1;
  if (proc9_path_match(aname))
    return 1;
  if (aname[0] == '/')
    return 0;
  return 1;
}

static char *proc9_dupstr(const char *s) {
  ulong len;
  char *dup;

  if (s == nil)
    return nil;
  len = strlen(s) + 1;
  dup = malloc(len);
  if (dup == nil)
    return nil;
  memmove(dup, s, len);
  return dup;
}

static char *proc9_normalize_attach_path(const char *aname) {
  char *path;
  ulong len;

  if (aname == nil || aname[0] == 0)
    return proc9_dupstr("/proc");

  if (strncmp(aname, "#p", 2) == 0) {
    len = strlen(aname + 2) + strlen("/proc") + 1;
    path = malloc(len);
    if (path == nil)
      return nil;
    snprint(path, len, "/proc%s", aname + 2);
  } else if (proc9_path_match(aname)) {
    path = proc9_dupstr(aname);
  } else if (aname[0] == '/') {
    return nil;
  } else {
    len = strlen(aname) + strlen("/proc/") + 1;
    path = malloc(len);
    if (path == nil)
      return nil;
    snprint(path, len, "/proc/%s", aname);
  }

  if (path != nil)
    cleanname(path);
  if (!proc9_path_match(path)) {
    free(path);
    return nil;
  }
  return path;
}

static char *proc9_walk_path(const char *base, const char *elem) {
  char *path;
  ulong len;

  if (base == nil || elem == nil)
    return nil;
  if (elem[0] == 0)
    return nil;

  len = strlen(base) + 1 + strlen(elem) + 1;
  path = malloc(len);
  if (path == nil)
    return nil;
  snprint(path, len, "%s/%s", base, elem);
  cleanname(path);
  if (!proc9_path_match(path)) {
    free(path);
    return nil;
  }
  return path;
}

static char *proc9_device_path(const char *path) {
  char *devpath;
  ulong len;

  if (!proc9_path_match(path))
    return nil;
  if (strcmp(path, "/proc") == 0)
    return proc9_dupstr("#p");

  len = strlen(path) - strlen("/proc") + strlen("#p") + 1;
  devpath = malloc(len);
  if (devpath == nil)
    return nil;
  snprint(devpath, len, "#p%s", path + strlen("/proc"));
  return devpath;
}

static Proc9Fid *proc9fid_lookup(Proc *owner, u32int fid) {
  for (int i = 0; i < PROC9_MAX_FIDS; i++) {
    if (!proc9fids[i].inuse)
      continue;
    if (proc9fids[i].owner != owner || proc9fids[i].owner_pid != owner->pid)
      continue;
    if (proc9fids[i].fid == fid)
      return &proc9fids[i];
  }
  return nil;
}

static int proc9fid_inuse_locked(Proc *owner, u32int fid) {
  return proc9fid_lookup(owner, fid) != nil;
}

static int proc9fid_set(Proc *owner, u32int fid, const char *path, Qid qid) {
  Proc9Fid *slot = nil;
  char *dup = nil;

  if (owner == nil || path == nil)
    return -1;

  dup = proc9_dupstr(path);
  if (dup == nil)
    return -1;

  lock(&proc9fidlk);
  slot = proc9fid_lookup(owner, fid);
  if (slot == nil) {
    for (int i = 0; i < PROC9_MAX_FIDS; i++) {
      if (!proc9fids[i].inuse) {
        slot = &proc9fids[i];
        break;
      }
    }
  }
  if (slot == nil) {
    unlock(&proc9fidlk);
    free(dup);
    return -1;
  }

  if (slot->path != nil)
    free(slot->path);
  slot->owner = owner;
  slot->owner_pid = owner->pid;
  slot->fid = fid;
  slot->path = dup;
  slot->qid = qid;
  slot->open_mode = 0;
  slot->inuse = 1;
  unlock(&proc9fidlk);
  return 0;
}

static void proc9fid_clunk(Proc *owner, u32int fid) {
  Proc9Fid *slot;

  if (owner == nil)
    return;

  lock(&proc9fidlk);
  slot = proc9fid_lookup(owner, fid);
  if (slot != nil) {
    if (slot->path != nil)
      free(slot->path);
    memset(slot, 0, sizeof(*slot));
  }
  unlock(&proc9fidlk);
}

static void proc9fid_clunk_all(Proc *owner) {
  if (owner == nil)
    return;

  lock(&proc9fidlk);
  for (int i = 0; i < PROC9_MAX_FIDS; i++) {
    if (!proc9fids[i].inuse)
      continue;
    if (proc9fids[i].owner != owner || proc9fids[i].owner_pid != owner->pid)
      continue;
    if (proc9fids[i].path != nil)
      free(proc9fids[i].path);
    memset(&proc9fids[i], 0, sizeof(proc9fids[i]));
  }
  unlock(&proc9fidlk);
}

static int proc9_lookup_qid(const char *path, Qid *qid) {
  Chan *c;
  char *devpath;
  int ok;

  devpath = proc9_device_path(path);
  if (devpath == nil)
    return -1;

  c = nil;
  ok = -1;
  if (waserror()) {
    if (c != nil)
      cclose(c);
    free(devpath);
    return -1;
  }

  c = namec(devpath, Aaccess, 0, 0);
  if (qid != nil)
    *qid = c->qid;
  cclose(c);
  ok = 0;

  poperror();
  free(devpath);
  return ok;
}

static int proc9_open_path(const char *path, int mode, Qid *qid, long *iounit) {
  Chan *c;
  char *devpath;
  int ok;

  devpath = proc9_device_path(path);
  if (devpath == nil)
    return -1;

  c = nil;
  ok = -1;
  if (waserror()) {
    if (c != nil)
      cclose(c);
    free(devpath);
    return -1;
  }

  openmode((ulong)mode);
  c = namec(devpath, Aopen, mode & ~OCEXEC, 0);
  if (qid != nil)
    *qid = c->qid;
  if (iounit != nil)
    *iounit = c->iounit;
  cclose(c);
  ok = 0;

  poperror();
  free(devpath);
  return ok;
}

static long proc9_read_path(const char *path, void *buf, long n, vlong off) {
  Chan *c;
  char *devpath;
  long nr;

  devpath = proc9_device_path(path);
  if (devpath == nil)
    return -1;

  c = nil;
  nr = -1;
  if (waserror()) {
    if (c != nil)
      cclose(c);
    free(devpath);
    return -1;
  }

  c = namec(devpath, Aopen, OREAD, 0);
  nr = devtab[devno(c->type, 0)]->read(c, buf, n, off);
  cclose(c);

  poperror();
  free(devpath);
  return nr;
}

static long proc9_write_path(const char *path, void *buf, long n, vlong off) {
  Chan *c;
  char *devpath;
  long nw;

  devpath = proc9_device_path(path);
  if (devpath == nil)
    return -1;

  c = nil;
  nw = -1;
  if (waserror()) {
    if (c != nil)
      cclose(c);
    free(devpath);
    return -1;
  }

  c = namec(devpath, Aopen, OWRITE, 0);
  nw = devtab[devno(c->type, 0)]->write(c, buf, n, off);
  cclose(c);

  poperror();
  free(devpath);
  return nw;
}

static long proc9_stat_path(const char *path, uchar *buf, long maxn) {
  Chan *c;
  char *devpath;
  long n;

  devpath = proc9_device_path(path);
  if (devpath == nil)
    return -1;

  c = nil;
  n = -1;
  if (waserror()) {
    if (c != nil)
      cclose(c);
    free(devpath);
    return -1;
  }

  c = namec(devpath, Aaccess, 0, 0);
  n = devtab[devno(c->type, 0)]->stat(c, buf, maxn);
  cclose(c);

  poperror();
  free(devpath);
  return n;
}

int proc_9p_can_handle(Proc *caller, Fcall *t) {
  int ok;

  if (caller == nil || t == nil)
    return 0;

  switch (t->type) {
  case Tattach:
    if (proc9_raw_attach_match(t->aname))
      return 1;
    if (p9_ns_root_available("/proc"))
      return 0;
    return proc9_bootstrap_attach_match(t->aname);
  case Twalk:
  case Topen:
  case Tread:
  case Twrite:
  case Tclunk:
  case Tcreate:
  case Tremove:
  case Tstat:
  case Twstat:
  case Tflush:
    lock(&proc9fidlk);
    ok = proc9fid_lookup(caller, t->fid) != nil;
    unlock(&proc9fidlk);
    return ok;
  default:
    return 0;
  }
}

void proc_9p_cleanup(Proc *caller) { proc9fid_clunk_all(caller); }

/*@
  @ requires \valid(caller) && \valid(t) && \valid(r);
  @ terminates \true;
  @ assigns *r;
  @ ensures \result == 0 || \result == -1;
  @*/
int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Proc9Fid *fidstate;
  char *path;
  char *next;
  Qid qid;
  long iounit;
  long n;
  char *data;
  uchar *statbuf;
  int walked;

  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    lock(&proc9fidlk);
    walked = proc9fid_inuse_locked(caller, t->fid);
    unlock(&proc9fidlk);
    if (walked) {
      r->type = Rerror;
      r->ename = Einuse;
      return -1;
    }
    path = proc9_normalize_attach_path(t->aname);
    if (path == nil) {
      r->type = Rerror;
      r->ename = "bad /proc attach path";
      return -1;
    }
    if (proc9_lookup_qid(path, &qid) < 0) {
      free(path);
      r->type = Rerror;
      r->ename = Enonexist;
      return -1;
    }
    if (proc9fid_set(caller, t->fid, path, qid) < 0) {
      free(path);
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    free(path);
    r->type = Rattach;
    r->qid = qid;
    return 0;

  case Twalk:
    lock(&proc9fidlk);
    fidstate = proc9fid_lookup(caller, t->fid);
    if (fidstate == nil) {
      unlock(&proc9fidlk);
      r->type = Rerror;
      r->ename = Ebadarg;
      return -1;
    }
    if (t->newfid != t->fid && proc9fid_inuse_locked(caller, t->newfid)) {
      unlock(&proc9fidlk);
      r->type = Rerror;
      r->ename = Einuse;
      return -1;
    }
    path = proc9_dupstr(fidstate->path);
    qid = fidstate->qid;
    unlock(&proc9fidlk);
    if (path == nil) {
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }

    r->type = Rwalk;
    if (t->nwname == 0) {
      if (proc9fid_set(caller, t->newfid, path, qid) < 0) {
        free(path);
        r->type = Rerror;
        r->ename = Enomem;
        return -1;
      }
      free(path);
      r->nwqid = 0;
      return 0;
    }

    walked = 0;
    for (int i = 0; i < t->nwname; i++) {
      next = proc9_walk_path(path, t->wname[i]);
      if (next == nil)
        break;
      if (proc9_lookup_qid(next, &qid) < 0) {
        free(next);
        break;
      }
      free(path);
      path = next;
      r->wqid[walked++] = qid;
    }

    if (walked == 0) {
      free(path);
      r->type = Rerror;
      r->ename = Enonexist;
      return -1;
    }
    if (proc9fid_set(caller, t->newfid, path, qid) < 0) {
      free(path);
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    free(path);
    r->nwqid = walked;
    return 0;

  case Topen:
    lock(&proc9fidlk);
    fidstate = proc9fid_lookup(caller, t->fid);
    if (fidstate == nil) {
      unlock(&proc9fidlk);
      r->type = Rerror;
      r->ename = Ebadarg;
      return -1;
    }
    path = proc9_dupstr(fidstate->path);
    unlock(&proc9fidlk);
    if (path == nil) {
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    if (proc9_open_path(path, t->mode, &qid, &iounit) < 0) {
      free(path);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    if (proc9fid_set(caller, t->fid, path, qid) < 0) {
      free(path);
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    free(path);
    lock(&proc9fidlk);
    fidstate = proc9fid_lookup(caller, t->fid);
    if (fidstate != nil)
      fidstate->open_mode = t->mode;
    unlock(&proc9fidlk);
    r->type = Ropen;
    r->qid = qid;
    r->iounit = iounit;
    return 0;

  case Tread:
    lock(&proc9fidlk);
    fidstate = proc9fid_lookup(caller, t->fid);
    if (fidstate == nil) {
      unlock(&proc9fidlk);
      r->type = Rerror;
      r->ename = Ebadarg;
      return -1;
    }
    path = proc9_dupstr(fidstate->path);
    unlock(&proc9fidlk);
    if (path == nil) {
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    data = (char *)caller->p9page + P9_MSG_OFFSET + 11;
    if (t->count > P9_REPLY_SIZE - 11)
      t->count = P9_REPLY_SIZE - 11;
    n = proc9_read_path(path, data, (long)t->count, t->offset);
    free(path);
    if (n < 0) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    r->type = Rread;
    r->data = data;
    r->count = n;
    return 0;

  case Twrite:
    lock(&proc9fidlk);
    fidstate = proc9fid_lookup(caller, t->fid);
    if (fidstate == nil) {
      unlock(&proc9fidlk);
      r->type = Rerror;
      r->ename = Ebadarg;
      return -1;
    }
    path = proc9_dupstr(fidstate->path);
    unlock(&proc9fidlk);
    if (path == nil) {
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    n = proc9_write_path(path, t->data, (long)t->count, t->offset);
    free(path);
    if (n < 0) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    r->type = Rwrite;
    r->count = n;
    return 0;

  case Tstat:
    lock(&proc9fidlk);
    fidstate = proc9fid_lookup(caller, t->fid);
    if (fidstate == nil) {
      unlock(&proc9fidlk);
      r->type = Rerror;
      r->ename = Ebadarg;
      return -1;
    }
    path = proc9_dupstr(fidstate->path);
    unlock(&proc9fidlk);
    if (path == nil) {
      r->type = Rerror;
      r->ename = Enomem;
      return -1;
    }
    statbuf = (uchar *)caller->p9page + P9_MSG_OFFSET + 9;
    n = proc9_stat_path(path, statbuf, P9_REPLY_SIZE - 9);
    free(path);
    if (n < 0) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    r->type = Rstat;
    r->nstat = n;
    r->stat = statbuf;
    return 0;

  case Tclunk:
    proc9fid_clunk(caller, t->fid);
    r->type = Rclunk;
    return 0;

  case Tflush:
    r->type = Rflush;
    return 0;

  case Tremove:
  case Tcreate:
  case Twstat:
  default:
    r->type = Rerror;
    r->ename = "operation not supported on /proc";
    return -1;
  }
}

/*@
  @
  //============================================================================
  @ // PROCESS CONTROL DISPATCHER - Routes process syscalls
  @
  //============================================================================
  @
  @ requires \valid(p) && \valid(t) && \valid(r);
  @ requires p->p9page == \null || \valid((uchar*)p->p9page +
  (0..P9_PAGE_SIZE-1));
  @ requires t->sdata == \null || \valid_read(t->sdata + (0..t->scount-1));
  @
  @ // Return value semantics
  @ ensures \result == 0 || \result == -1;
  @ ensures \result == 0 ==> (r->type == Rsyscall || r->type == Rsysexec ||
  @                           r->type == Rsysbrk || r->type == Rsysfork);
  @ ensures \result == -1 ==> r->type == Rerror;
  @
  @ // Protocol correctness: Tag preservation
  @ ensures r->tag == t->tag;
  @
  @ // Memory safety
  @ assigns *r;
  @
  @ terminates \true;
  @*/
int router_dispatch_proc(Proc *p, Fcall *t, Fcall *r) {
  uchar *ep = t->sdata + t->scount;
  uchar *ptr = t->sdata;
  uintptr ubase = p9_user_base(p);

  /* Handle Texec (128) - Direct execution message */
  if (t->type == Texec) {
    char *path;
    ulong args[2];

    /* Extract path from Texec message data */
    /* Format: [2] pathlen + [n] path bytes */
    if (t->count < 2) {
      r->type = Rerror;
      r->ename = "Texec: invalid message format";
      return -1;
    }

    uint pathlen = (uint)t->data[0] | ((uint)t->data[1] << 8);
    if (pathlen == 0 || pathlen > t->count - 2) {
      r->type = Rerror;
      r->ename = "Texec: invalid path length";
      return -1;
    }

    /* Allocate and copy path string for kernel logging/debugging */
    path = xalloc(pathlen + 1);
    if (path == nil) {
      r->type = Rerror;
      r->ename = "Texec: out of memory";
      return -1;
    }
    memmove(path, t->data + 2, pathlen);
    path[pathlen] = '\0';

    print("router_proc: Texec for '%s' (pid %lud)\n", path, p->pid);

    /*
     * Sysexec requires User Virtual Addresses for both path and argv.
     * We must calculate the user address of the path existing in the
     * exchange page, and construct a user-space argv array there as well.
     */

    /* 1. Calculate User Address of the path string */
    /* t->data points into p->p9page. The string starts at data+2 */
    uintptr kpage = (uintptr)p->p9page;
    uintptr kpath = (uintptr)t->data + 2;
    uintptr path_offset = kpath - kpage;
    uintptr upath = ubase + path_offset;

    /* 2. Ensure null-termination in the user buffer */
    /* We can safely write \0 because validaddr/namec expects it.
     * Check bounds to ensure we don't write past valid page. */
    /* P9_PAGE_SIZE assumed known */
    if (path_offset + pathlen < 4096) { /* P9_PAGE_SIZE */
      ((char *)kpath)[pathlen] = 0;
    }

    /* 3. Construct argv array in the Exchange Page */
    /* We need space for 2 pointers: [upath, 0] */
    /* Use the space immediately following the message payload */
    uintptr kargv_start = (uintptr)t->data + t->count;

    /* Align to 8 bytes */
    kargv_start = (kargv_start + 7) & ~7ULL;

    /* Check if we have room in the request buffer */
    if (kargv_start + 2 * sizeof(ulong) > kpage + P9_REQUEST_SIZE) {
      xfree(path);
      r->type = Rerror;
      r->ename = "Texec: message too large, no room for argv";
      return -1;
    }

    /* Write argv to the user page (via kernel mapping) */
    ulong *argv_ptr = (ulong *)kargv_start;
    argv_ptr[0] = (ulong)upath;
    argv_ptr[1] = 0;

    /* Calculate User Address of argv */
    uintptr argv_offset = kargv_start - kpage;
    uintptr uargv = ubase + argv_offset;

    /* Prepare arguments for sysexec */
    /* args[0] = file (user char*) */
    /* args[1] = argv (user char**) */
    args[0] = (ulong)upath;
    args[1] = (ulong)uargv;

    /* Call sysexec - never returns on success */
    if (waserror()) {
      print("router_proc: Texec failed: %s\n", up->errstr);
      xfree(path);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    sysexec(args);
    /* Not reached on success */
    poperror();

    extern void noteret(void); /* Assembly label for exec return path */
    if (up->dbgreg != nil && ((void **)up->dbgreg)[-1] == noteret) {
      r->type = Rsysexec;
      r->tag = t->tag;
      /* exec succeeded, we are in new process image, but 9p transaction
         completes? Usually exec doesn't return. If we are here, something is
         special about how sysexec returns or we are in the parent context? No,
         sysexec replaces current proc image. The logic in original 9p_router.c
         handles noteret check.
      */
      return 0;
    }

    /* If we get here, exec failed somehow or logic flow is different */
    xfree(path);
    r->type = Rerror;
    r->ename = "Texec: exec returned unexpectedly";
    return -1;
  }

  /* Handle Tsyscall variants */
  if (t->type == Tsyscall) {
    switch (t->scallnr) {
    case SYS_FORK: /* RFORK */
    case SYS_RFORK: {
      extern uintptr sysrfork(void *list_void);

      Chan *pm_chan = srv_clone_chan("pm");
      if (pm_chan != nil) {
        cclose(pm_chan);
        print("router_proc: userspace PM present; using kernel rfork fallback\n");
      }

      /* Two-Level Spawn Capability Check (userspace only).
       * Level 1: Namespace (Pgrp) limit - shared by all procs in namespace
       * Level 2: Process limit - individual fork bomb protection
       * TCB processes (kp == 1) are exempt. */
      if (up != nil && up->kp == 0) {
        Pgrp *pg = up->pgrp;

        /* Check spawn capability exists */
        if (uuid_is_null(&up->spawn_cap)) {
          print("router_proc: SYS_RFORK FAILED - spawn_cap is null\n");
          r->type = Rerror;
          snprint(up->errstr, ERRMAX, "no spawn capability");
          r->ename = up->errstr;
          return -1;
        }

        /* Level 1: Namespace limit check */
        if (pg != nil) {
          lock(&pg->spawn_lock);
          if (pg->spawn_count >= pg->spawn_limit) {
            print("router_proc: SYS_RFORK FAILED - namespace limit %d/%d\n",
                  pg->spawn_count, pg->spawn_limit);
            unlock(&pg->spawn_lock);
            r->type = Rerror;
            snprint(up->errstr, ERRMAX, "namespace spawn limit (%d/%d)",
                    pg->spawn_count, pg->spawn_limit);
            r->ename = up->errstr;
            return -1;
          }
          /* Cryptographic binding: verify spawn_cap is bound to this Pgrp. */
          u8int cap_hash[16];
          uuid_get_pa_hash_bits(&up->spawn_cap, cap_hash);
          if (memcmp(cap_hash, pg->identity_hash, 6) != 0) {
            print("router_proc: SYS_RFORK FAILED - spawn cap not bound\n");
            unlock(&pg->spawn_lock);
            r->type = Rerror;
            snprint(up->errstr, ERRMAX, "spawn cap not bound to namespace");
            r->ename = up->errstr;
            return -1;
          }
          unlock(&pg->spawn_lock);
        }

        /* Level 2: Process child limit check */
        if (up->spawn_children >= up->spawn_max_children) {
          print("router_proc: SYS_RFORK FAILED - process limit %d/%d\n",
                up->spawn_children, up->spawn_max_children);
          r->type = Rerror;
          snprint(up->errstr, ERRMAX, "process spawn limit (%d/%d)",
                  up->spawn_children, up->spawn_max_children);
          r->ename = up->errstr;
          return -1;
        }
      }

      /* Format: [flags 4] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      ulong flags = GBIT32(ptr);

      ulong args[1] = {flags};
      uintptr ret;
      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        r->tag = t->tag;
        return -1;
      }
      ret = sysrfork(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret; /* PID is usually returned as u64 in retval */
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_BRK: {
      extern uintptr ibrk(uintptr, int);
      /* Format: [addr 8] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 8 > ep) {
        r->type = Rerror;
        return -1;
      }
      uintptr addr = (uintptr)GBIT64(ptr); // Use 64-bit for addr

      print("router_proc: Tsyscall SYS_BRK addr=0x%p\n", (void *)addr);

      uintptr ret;
      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        r->tag = t->tag;
        return -1;
      }
      ret = ibrk(addr, BSEG);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)ret;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_EXIT: {
      extern void pexit(char *, int);
      static int exit_trace_count;
      /* Format: [status s] ? Or [status 4]?
       * sys_exit(char *msg). So treat as string.
       */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      char *ename = nil;
      if (ptr + 2 <= ep) {
        int len = GBIT16(ptr);
        if (ptr + 2 + len <= ep) {
          ename = smalloc(len + 1);
          memmove(ename, ptr + 2, len);
          ename[len] = 0;
        }
      }

      if (exit_trace_count < 50) {
        char trace_buf[160];
        exit_trace_count++;
        snprint(trace_buf, sizeof(trace_buf),
                "router_proc: SYS_EXIT pid=%d msg='%s'\n",
                up ? up->pid : -1, ename ? ename : "");
        uartputs(trace_buf, (int)strlen(trace_buf));
      }
      pexit(ename ? ename : "", 1);
      return 0;
    }

    case SYS_WAIT: {
      extern ulong pwait(Waitmsg * w);
      Waitmsg w;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      ulong pid = pwait(&w);
      poperror();

      char *msg = (char *)p->p9page + P9_MSG_OFFSET + 64;
      snprint(msg, P9_REPLY_SIZE - 64, "%s", w.msg);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = pid;
      r->scount = strlen(msg) + 1;
      r->sdata = (uchar *)msg;
      return 0;
    }

    case SYS_PEBBLE_ALLOC: {
      extern int pebble_alloc_with_white(ulong, UserCapability *, void **);
      extern void userpmap(uintptr, uintptr, int);
      extern PebbleBlack *pebble_lookup_black(PebbleState *, void *);

      /* Format: [size 8] [userp 8] */
      ptr = tsyscall_skip_argc(ptr, ep, 2);
      if (ptr + 16 > ep) {
        r->type = Rerror;
        return -1;
      }
      ulong size = (ulong)GBIT64(ptr);
      void **userp = (void **)GBIT64(ptr + 8);

      UserCapability cap;
      void *handle = nil;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      if (pebble_alloc_with_white(size, &cap, &handle) != 0)
        error("pebble: no memory");

      /* Establish User-Space Mapping */
      /*
       * Use PGROUND to round UP the physical address to the next page boundary.
       * Combined with the extra allocation padding in pebble.c, this guarantees
       * we map a page that is EXCLUSIVELY owned by this allocation, avoiding
       * accidental clobbering of the pool header in the preceding shared page.
       */
      /*
       * Use architecture paddr() directly here.
       * router.h includes mem.h before fns.h, so PADDR may resolve to the
       * simple KZERO macro variant, which is incorrect for HHDM addresses.
       */
      uintptr pa = paddr(handle);
      uintptr aligned_pa = PGROUND(pa);
      ulong map_size = PGROUND(size);
      uintptr uva = p->pebble.vbase;

      /* Map physically contiguous pages covering the user request */
      for (ulong i = 0; i < map_size; i += BY2PG) {
        userpmap(uva + i, aligned_pa + i, PTEVALID | PTEUSER | PTEWRITE);
      }

      /* Link UVA to Metadata */
      PebbleBlack *pb = pebble_lookup_black(&p->pebble, handle);
      if (pb != nil) {
        pb->user_vaddr = uva;
      }

      /* Advance VBASE for next allocation */
      p->pebble.vbase += map_size;

      /* Return UVA to user space via retval */
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)uva;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_PEBBLE_FREE: {
      extern int pebble_black_free(const UserCapability *);
      /* Format: [addr 8] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 8 > ep) {
        r->type = Rerror;
        return -1;
      }
      uintptr uva = GBIT64(ptr);

      /* Find metadata by User Virtual Address */
      PebbleBlack *pb = nil;
      for (pb = p->pebble.black_list; pb != nil; pb = pb->next) {
        if (pb->user_vaddr == uva)
          break;
      }

      if (pb == nil) {
        r->type = Rerror;
        r->ename = "pebble: invalid address";
        return -1;
      }

      /* Unmap memory from user space to prevent use-after-free corruption */
      ulong map_size = PGROUND(pb->size);
      for (ulong i = 0; i < map_size; i += BY2PG) {
        userpmap(pb->user_vaddr + i, 0, 0);
      }

      /* pebble_black_free cleans up physical memory and metadata */
      pebble_black_free(&pb->capability);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_PEBBLE_INCREASE_BUDGET: {
      extern int pebble_increase_budget(ulong, u64int);
      /* Format: [size 8] [nonce 8] */
      ptr = tsyscall_skip_argc(ptr, ep, 2);
      if (ptr + 16 > ep) {
        r->type = Rerror;
        return -1;
      }
      ulong size = (ulong)GBIT64(ptr);
      u64int nonce = GBIT64(ptr + 8);

      print("router_proc: Tsyscall SYS_PEBBLE_INCREASE_BUDGET size=%lud "
            "nonce=%llud\n",
            size, nonce);

      if (pebble_increase_budget(size, nonce) != 0) {
        r->type = Rerror;
        r->ename =
            "pebble: budget increase failed (invalid PoW or out of tokens)";
        return -1;
      }

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_SEGATTACH: {
      /* Format: [attr 4] [spec ptr 8] [addr ptr 8] [len 8] */
      extern uintptr segattach(int attr, char *name, uintptr va, uintptr len);
      if (ptr + 4 + 8 + 8 + 8 > ep) {
        r->type = Rerror;
        r->ename = "segattach: bad args";
        return -1;
      }
      int attr = GBIT32(ptr);
      char *spec = *(char **)(ptr + 4);
      uintptr va = *(uintptr *)(ptr + 12);
      uintptr len = *(uintptr *)(ptr + 20);

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      char *namedup = validnamedup(spec, 1);
      uintptr ret = segattach(attr, namedup, va, len);
      free(namedup);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    default:
      r->type = Rerror;
      r->ename = "Proc syscall not found";
      return -1;
    }
  }

  /* Handle Tsys* variants */
  if (t->type == Tsysexit) {
    extern void pexit(char *, int);

    print("router_proc: Tsysexit '%s'\n", t->ename ? t->ename : "");

    /* pexit never returns */
    pexit(t->ename ? t->ename : "", 1);

    /* Not reached, but satisfies compiler */
    return 0;
  }

  if (t->type == Tsysbrk) {
    extern uintptr ibrk(uintptr, int);
    uintptr ret;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    /* Call ibrk with address */
    ret = ibrk((uintptr)t->addr, BSEG);
    poperror();

    /* Build Rsysbrk response */
    r->type = Rsysbrk;
    r->tag = t->tag;
    r->addr = ret;

    print("router_proc: Tsysbrk addr=0x%llx -> 0x%llx\n", t->addr, (u64int)ret);
    return 0;
  }

  if (t->type == Tsysfork) {
    extern uintptr sysrfork(void *list_void);
    ulong args[1];
    uintptr ret;

    args[0] = t->flags;
    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    ret = sysrfork(args);
    poperror();

    /* Build Rsysfork response */
    r->type = Rsysfork;
    r->tag = t->tag;
    r->pid = (u32int)ret;    /* PID in message */
    r->retval = (u64int)ret; /* PID in RAX */

    print("router_proc: Tsysfork flags=0x%x -> pid=%d retval=%lld\n", t->flags,
          (int)ret, (long long)r->retval);

    return 0;
  }

  if (t->type == Tsysexec) {
    /* Exec logic for Tsysexec message */
    ulong args[2];
    uintptr kpage = (uintptr)p->p9page;
    uintptr kpath = (uintptr)t->path;
    int i;

    /* Calculate User Address of the path string */
    if (kpath < kpage || kpath >= kpage + P9_PAGE_SIZE) {
      r->type = Rerror;
      r->ename = "Tsysexec: path outside buffer";
      return -1;
    }
    uintptr path_offset = kpath - kpage;
    uintptr upath = ubase + path_offset;

    ulong arg_count = t->argc;

    if (arg_count > MAXWELEM)
      arg_count = MAXWELEM;

    for (i = 0; i < (int)arg_count; i++) {
      if (t->args[i] != nil) {
        print("router_proc: Tsysexec arg[%d]=%s\n", i, t->args[i]);
        print("router_proc: Tsysexec arg[%d] ptr=%p\n", i, t->args[i]);
      }
    }

    uchar *msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;
    uint msg_size = GBIT32(msg_buf);
    print("router_proc: Tsysexec msg_size=%ud argvp_base=%p\n", msg_size,
          msg_buf);
    uintptr argvp = (uintptr)msg_buf + msg_size;
    argvp = (argvp + 7) & ~7ULL;
    if (argvp + (arg_count + 1) * sizeof(uintptr) >
        (uintptr)p->p9page + P9_MSG_OFFSET + P9_MSG_SIZE) {
      r->type = Rerror;
      r->ename = "Tsysexec: argv out of exchange page";
      return -1;
    }
    uintptr *argv_ptr_list = (uintptr *)argvp;

    /*
     * Construct argv in exchange page.
     * Kernel pointers (t->args[i]) must be mapped to user addresses.
     */
    for (i = 0; i < (int)arg_count; i++) {
      uintptr karg = (uintptr)t->args[i];
      if (karg >= kpage && karg < kpage + P9_PAGE_SIZE) {
        uintptr offset = karg - kpage;
        argv_ptr_list[i] = ubase + offset;
      } else {
        argv_ptr_list[i] = 0;
      }
    }
    argv_ptr_list[arg_count] = 0; /* Null terminator */

    uintptr uargv = ubase + (argvp - (uintptr)p->p9page);

    /* Prepare arguments for sysexec: [path, argv] */
    args[0] = (ulong)upath;
    args[1] = (ulong)uargv;

    {
      extern void uartputs(char *, int);
      /*@
        @ requires (n > 0 ==> \valid(s + (0 .. (integer)n-1))) || (n == 0);
        @ requires valid_string(fmt);
        @ assigns s[0 .. (integer)n-1] \if (s != \null && n > 0);
        @ ensures \result >= 0;
        @*/
      extern int snprint(char *s, int n, char *fmt, ...);
      /* strlen is from headers */
      char buf[256];
      snprint(buf, sizeof(buf), "CONSOLE: Tsysexec path hex: ");
      uartputs(buf, (int)strlen(buf));
      unsigned char *cp = (unsigned char *)kpath;
      for (int i = 0; i < (int)strlen((char *)kpath) + 1; i++) {
        snprint(buf, sizeof(buf), "%02x ", cp[i]);
        uartputs(buf, (int)strlen(buf));
      }
      uartputs("\n", 1);

      snprint(buf, sizeof(buf),
              "CONSOLE: Tsysexec up=%p p=%p up->slash=%p up->dot=%p\n", up, p,
              up ? up->slash : 0, up ? up->dot : 0);
      uartputs(buf, (int)strlen(buf));
    }

    print("router_proc: Tsysexec calling sysexec('%s', argv=%#p)\n",
          (char *)kpath, (void *)uargv);
    {
      extern void uartputs(char *, int);
      /*@
        @ requires (n > 0 ==> \valid(s + (0 .. (integer)n-1))) || (n == 0);
        @ requires valid_string(fmt);
        @ assigns s[0 .. (integer)n-1] \if (s != \null && n > 0);
        @ ensures \result >= 0;
        @*/
      extern int snprint(char *s, int n, char *fmt, ...);
      /* strlen is available from portlib.h via includes */
      char buf[256];
      snprint(buf, sizeof(buf), "CONSOLE: Tsysexec calling sysexec('%s')\n",
              (char *)kpath);
      uartputs(buf, (int)strlen(buf));
    }

    if (waserror()) {
      print("router_proc: Tsysexec ERROR: %s\n", up->errstr);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    sysexec(args);
    poperror();

    /* Success! Return Rsysexec flag for trap.c:syscall to handle */
    r->type = Rsysexec;
    r->tag = t->tag;
    return 0;
  }

  if (t->type == Tsysspawn) {
    /* SECURE spawn - atomic fork+exec without stack data leakage */
    ulong args[2];
    uintptr kpage = (uintptr)p->p9page;
    uintptr kpath = (uintptr)t->path;
    int i;

    print("router_proc: Tsysspawn entered for '%s'\n", t->path);

    /* Enforce same two-level spawn policy as SYS_RFORK for userspace tasks. */
    if (up != nil && up->kp == 0) {
      Pgrp *pg = up->pgrp;

      if (uuid_is_null(&up->spawn_cap)) {
        print("router_proc: Tsysspawn FAILED - spawn_cap is null\n");
        r->type = Rerror;
        snprint(up->errstr, ERRMAX, "no spawn capability");
        r->ename = up->errstr;
        return -1;
      }

      if (pg != nil) {
        lock(&pg->spawn_lock);
        if (pg->spawn_count >= pg->spawn_limit) {
          print("router_proc: Tsysspawn FAILED - namespace limit %d/%d\n",
                pg->spawn_count, pg->spawn_limit);
          unlock(&pg->spawn_lock);
          r->type = Rerror;
          snprint(up->errstr, ERRMAX, "namespace spawn limit (%d/%d)",
                  pg->spawn_count, pg->spawn_limit);
          r->ename = up->errstr;
          return -1;
        }
        {
          u8int cap_hash[16];
          uuid_get_pa_hash_bits(&up->spawn_cap, cap_hash);
          if (memcmp(cap_hash, pg->identity_hash, 6) != 0) {
            print("router_proc: Tsysspawn FAILED - spawn cap not bound\n");
            unlock(&pg->spawn_lock);
            r->type = Rerror;
            snprint(up->errstr, ERRMAX, "spawn cap not bound to namespace");
            r->ename = up->errstr;
            return -1;
          }
        }
        unlock(&pg->spawn_lock);
      }

      if (up->spawn_children >= up->spawn_max_children) {
        print("router_proc: Tsysspawn FAILED - process limit %d/%d\n",
              up->spawn_children, up->spawn_max_children);
        r->type = Rerror;
        snprint(up->errstr, ERRMAX, "process spawn limit (%d/%d)",
                up->spawn_children, up->spawn_max_children);
        r->ename = up->errstr;
        return -1;
      }
    }

    /* Validate path is in exchange page */
    if (kpath < kpage || kpath >= kpage + P9_PAGE_SIZE) {
      r->type = Rerror;
      r->ename = "Tsysspawn: path outside buffer";
      return -1;
    }
    uintptr path_offset = kpath - kpage;
    uintptr upath = ubase + path_offset;

    ulong arg_count = t->argc;
    if (arg_count > MAXWELEM)
      arg_count = MAXWELEM;

    /* Build argv array in exchange page */
    uchar *msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;
    uint msg_size = GBIT32(msg_buf);
    uintptr argvp = (uintptr)msg_buf + msg_size;
    argvp = (argvp + 7) & ~7ULL;
    if (argvp + (arg_count + 1) * sizeof(uintptr) >
        (uintptr)p->p9page + P9_MSG_OFFSET + P9_MSG_SIZE) {
      r->type = Rerror;
      r->ename = "Tsysspawn: argv out of exchange page";
      return -1;
    }

    char **kargvp = (char **)argvp;
    for (i = 0; i < (int)arg_count; i++) {
      if (t->args[i] != nil) {
        uintptr karg = (uintptr)t->args[i];
        if (karg < kpage || karg >= kpage + P9_PAGE_SIZE) {
          r->type = Rerror;
          r->ename = "Tsysspawn: arg outside buffer";
          return -1;
        }
        uintptr arg_offset = karg - kpage;
        kargvp[i] = (char *)(ubase + arg_offset);
      } else {
        kargvp[i] = nil;
      }
    }
    kargvp[arg_count] = nil;

    uintptr uargvp = ubase + P9_MSG_OFFSET + msg_size;
    uargvp = (uargvp + 7) & ~7ULL;

    args[0] = (ulong)upath;
    args[1] = (ulong)uargvp;

    print("router_proc: Tsysspawn calling sysspawn('%s', argv=%#p)\n",
          (char *)upath, (void *)uargvp);

    /* Call secure spawn - creates child with NO parent stack exposure */
    extern uintptr sysspawn(void *);
    if (waserror()) {
      print("router_proc: Tsysspawn ERROR: %s\n", up->errstr);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    uintptr child_pid = sysspawn((void *)args);
    poperror();

    print("router_proc: Tsysspawn SUCCESS child_pid=%lld\n", child_pid);

    /* Return child PID to parent */
    r->type = Rsysspawn;
    r->tag = t->tag;
    r->pid = (u32int)child_pid;
    return 0;
  }

  return -1;
}
