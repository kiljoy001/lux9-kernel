#include "router.h"

/*
 * Process control server: /proc/
 * Integrates with our FSM!
 */
/* Proc file types for routing */
#define PROC_ROOT 0
#define PROC_CTL 1
#define PROC_WAIT 2
#define PROC_STATUS 3
#define PROC_NS 4
#define PROC_SEGMENT 5

int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  /*@
    @ requires \valid(caller);
    @ requires \valid(t);
    @ requires \valid(r);
    @ ensures \result == 0 || \result == -1;
    @*/
  Proc *target = caller; /* Default to self */
  int type = 0;

  r->tag = t->tag;

  /* Retrieve file type from FID (stored in Qid.vers during Walk) */
  if (t->type != Tattach) {
    /* Need helper to get subtype */
    // type = get_fid_subtype((int)t->fid); // Not available here yet, need to export or reimplement
    // For now stub
    type = PROC_ROOT; 
  }

  /* Stub implementation for verification purposes */
  /* Real implementation needs Fgrp access which is in core/doorbell context */
  
  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = PROC_ROOT;
    r->qid.vers = PROC_ROOT;
    return 0;
    
  default:
    r->type = Rerror;
    r->ename = "not implemented";
    return -1;
  }
}

int router_dispatch_proc(Proc *p, Fcall *t, Fcall *r) {
  /*@
    @ requires \valid(p);
    @ requires \valid(t);
    @ requires \valid(r);
    @ ensures \result == 0 || \result == -1;
    @*/
  uchar *ep = t->sdata + t->scount;
  uchar *ptr = t->sdata;

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
    uintptr upath = EXCHANGE_PAGE_ADDR + path_offset;

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
    uintptr uargv = EXCHANGE_PAGE_ADDR + argv_offset;

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
    
    extern void noteret(void);  /* Assembly label for exec return path */
    if (up->dbgreg != nil && ((void **)up->dbgreg)[-1] == noteret) {
        r->type = Rsysexec;
        r->tag = t->tag;
        /* exec succeeded, we are in new process image, but 9p transaction completes? 
           Usually exec doesn't return. If we are here, something is special about how sysexec returns 
           or we are in the parent context? No, sysexec replaces current proc image.
           The logic in original 9p_router.c handles noteret check.
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

      print("router_proc: SYS_RFORK case entered, kp=%d\n", up ? up->kp : -1);

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

      print("router_proc: Tsyscall SYS_RFORK flags=0x%lx\n", flags);

      ulong args[1] = {flags};
      uintptr ret;
      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        r->tag = t->tag;
        return -1;
      }
      ret = sysrfork(args);
      print("DEBUG: sysrfork returned ret=%#p\n", ret);
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

      print("router_proc: Tsyscall SYS_EXIT '%s'\n", ename ? ename : "");
      pexit(ename ? ename : "", 1);
      return 0;
    }

    case SYS_WAIT: {
      extern ulong pwait(Waitmsg * w);
      Waitmsg w;

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
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
    r->retval = (u32int)ret; /* PID */

    print("router_proc: Tsysfork flags=0x%x -> pid=%d\n", t->flags, (int)ret);
    return 0;
  }

  return -1;
}
