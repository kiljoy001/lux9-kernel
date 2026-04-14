#include "router.h"

extern uintptr syssleep(void *list_void);

uintptr p9_user_base(Proc *p) {
  if (p && p->p9uaddr)
    return p->p9uaddr;
  return EXCHANGE_PAGE_ADDR;
}

/*@
  @ requires \valid_read(p + (0..3)) && p <= ep;
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == p || \result == p + 4;
  @*/
uchar *tsyscall_skip_argc(uchar *p, uchar *ep, u32int expected) {
  if (p + 4 <= ep) {
    u32int argc = GBIT32(p);
    if (argc == expected)
      return p + 4;
  }
  return p;
}

/*@
  @ requires p == \null || \valid(p);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == 0 || \result == 1;
  @ ensures p != \null && p->p9page != \null ==>
  @   (\result == 1 <==>
  @     (uintptr)ptr >= (uintptr)p->p9page + P9_REQUEST_OFFSET &&
  @     (uintptr)ptr + len <= (uintptr)p->p9page + P9_REQUEST_OFFSET +
  P9_REQUEST_SIZE);
  @*/
int p9_exchange_contains(Proc *p, void *ptr, ulong len) {
  if (!p || !p->p9page || !ptr || len == 0)
    return 0;
  uintptr base = (uintptr)p->p9page + P9_REQUEST_OFFSET;
  uintptr end = base + P9_REQUEST_SIZE;
  uintptr addr = (uintptr)ptr;
  if (addr < base)
    return 0;
  if (addr + len < addr || addr + len > end)
    return 0;
  return 1;
}

/*@
  @
  //============================================================================
  @ // MAIN 9P DISPATCHER - Routes messages to subsystem handlers
  @
  //============================================================================
  @
  @ requires \valid(p) && \valid(t) && \valid(r);
  @ requires p->p9page == \null || \valid((uchar*)p->p9page +
  (0..P9_PAGE_SIZE-1));
  @
  @ // Return value semantics
  @ ensures \result == 0 || \result == -1;
  @ ensures \result == 0 ==> r->type == Rsyscall;
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
int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  if (p->wasm.initialized) {
    if (t->data && t->count > 0 &&
        !p9_exchange_contains(p, t->data, t->count)) {
      r->type = Rerror;
      r->ename = "wasm data must use exchange page";
      return -1;
    }
    if (t->sdata && t->scount > 0 &&
        !p9_exchange_contains(p, t->sdata, t->scount)) {
      r->type = Rerror;
      r->ename = "wasm sdata must use exchange page";
      return -1;
    }
  }

  if (proc_9p_can_handle(p, t))
    return proc_9p_handle(p, t, r);

  /* Handle Generic Tsyscall (130) */
  if (t->type == Tsyscall) {
    /*
     * Legacy raw syscall numbers overlap with split-router SYS_* aliases.
     * Route them before the grouped switch so fs.c can still serve the old ABI.
     */
    if (t->scallnr == BIND || t->scallnr == FD2PATH || t->scallnr == UNMOUNT)
      return router_dispatch_fs(p, t, r);

    /* Dispatch based on syscall number groups */
    switch (t->scallnr) {
    /* File System Operations */
    case SYS_OPEN:
    case SYS_CREATE:
    case SYS_CLOSE:
    case SYS_SEEK:
    case SYS_READ:
    case SYS_PREAD:
    case SYS_WRITE:
    case SYS_PWRITE:
    case SYS_STAT:
    case SYS_WSTAT:
    case SYS_REMOVE:
      return router_dispatch_fs(p, t, r);

    /* Process Control */
    case SYS_FORK:
    case SYS_RFORK:
    case SYS_EXIT:
    case SYS_WAIT:
    case SYS_BRK:
    case SYS_PEBBLE_ALLOC:
    case SYS_PEBBLE_FREE:
    case SYS_SEGATTACH:
      return router_dispatch_proc(p, t, r);

    /* IPC & Exchange */
    case SYS_PIPE:
    case SYS_EXCHANGE_ALLOC:
    case SYS_EXCHANGE_FREE:
    case SYS_EXCHANGE_PUBLISH:
    case SYS_EXCHANGE_SUBSCRIBE:
    case SYS_EXCHANGE_UNSUBSCRIBE:
    case SYS_EXCHANGE_RECEIVE:
    case SYS_EXCHANGE_PREPARE:
    case SYS_EXCHANGE_PREPARE_RANGE:
    case SYS_EXCHANGE_ACCEPT:
    case SYS_EXCHANGE_CANCEL:
    case SYS_EXCHANGE_TRANSFER:
      return router_dispatch_ipc(p, t, r);

    case SYS_NSROOT_PUBLISH:
    case SYS_NSROOT_UNPUBLISH:
      return router_dispatch_fs(p, t, r);

    /* WASM */
    case SYS_WASM_COMPILE:
    case SYS_WASM_EXECUTE:
    case SYS_WASM_DESTROY:
      return router_dispatch_wasm(p, t, r);

    /* Misc */
    case SYS_NSEC:
      /* Moved to router_dispatch_fs or handle here?
         Original 9p_router.c handled it inline.
         Let's put it in fs.c or separate misc?
         For now, let's keep it here but fix the implicit decl. */
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = nsec();
      r->scount = 0;
      r->sdata = nil;
      return 0;

    case SYS_MOUNT:
      /* Mount is special, could be in fs or ipc, putting in fs for now */
      return router_dispatch_fs(p, t, r);

    default:
      r->type = Rerror;
      /* FIX: Don't write to NULL r->ename! Use up->errstr. */
      if (up) {
        snprint(up->errstr, ERRMAX, "unknown syscall %d", t->scallnr);
        r->ename = up->errstr;
      } else {
        r->ename = "unknown syscall (no proc)";
      }
      return -1;
    }
  }

  /* Handle Texec (128) - Direct execution message */
  if (t->type == Texec) {
    return router_dispatch_proc(p, t, r);
  }

  /* Handle Tsys* - Specific syscall message types (132-205) */
  if (t->type >= Tsysopen && t->type <= Tsysremove) {
    return router_dispatch_fs(p, t, r);
  }
  if (t->type == Tsysbind || t->type == Tsysmount || t->type == Tsysunmount ||
      t->type == Tsysfd2path) {
    return router_dispatch_fs(p, t, r);
  }
  if (t->type == Tsyswait) {
    Waitmsg w;
    char *msg;
    ulong msgmax;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    r->pid = pwait(&w);
    poperror();

    msg = (char *)p->p9page + P9_MSG_OFFSET + 64;
    msgmax = P9_MSG_SIZE - 64;
    snprint(msg, msgmax, "%s", w.msg);

    r->type = Rsyswait;
    r->tag = t->tag;
    r->ename = msg;
    return 0;
  }
  if (t->type == Tsysexit || t->type == Tsysbrk || t->type == Tsysfork ||
      t->type == Tsysexec || t->type == Tsysspawn) {
    return router_dispatch_proc(p, t, r);
  }
  if (t->type == Tsyssleep) {
    ulong args[1];

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    args[0] = t->count;
    syssleep(args);
    poperror();

    r->type = Rsyssleep;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysdup) {
    return router_dispatch_fs(p, t, r); /* FD op */
  }
  if (t->type >= Tsysstat && t->type <= Tsysfwstat) {
    return router_dispatch_fs(p, t, r);
  }

  r->type = Rerror;
  if (up != nil) {
    snprint(up->errstr, ERRMAX, "unknown message type %d", t->type);
    r->ename = up->errstr;
  } else {
    r->ename = "unknown message type (no proc)";
  }
  return -1;
}
