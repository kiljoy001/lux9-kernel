#include "router.h"

uchar *tsyscall_skip_argc(uchar *p, uchar *ep, u32int expected) {
  if (p + 4 <= ep) {
    u32int argc = GBIT32(p);
    if (argc == expected)
      return p + 4;
  }
  return p;
}

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

int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  /*@
    @ requires \valid(p);
    @ requires \valid(t);
    @ requires \valid(r);
    @ ensures \result == 0 || \result == -1;
    @*/
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

  /* Handle Generic Tsyscall (130) */
  if (t->type == Tsyscall) {
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
      return router_dispatch_proc(p, t, r);

    /* IPC & Exchange */
    case SYS_PIPE:
    case SYS_EXCHANGE_ALLOC:
    case SYS_EXCHANGE_FREE:
    case SYS_EXCHANGE_PUBLISH:
    case SYS_EXCHANGE_SUBSCRIBE:
    case SYS_EXCHANGE_UNSUBSCRIBE:
    case SYS_EXCHANGE_RECEIVE:
      return router_dispatch_ipc(p, t, r);

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
      print("p9_dispatch: SYS_NSEC\n");
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
      snprint(r->ename, sizeof(r->ename), "unknown syscall %d", t->scallnr);
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
  if (t->type == Tsysexit || t->type == Tsysbrk || t->type == Tsysfork ||
      t->type == Tsysexec) {
    return router_dispatch_proc(p, t, r);
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
