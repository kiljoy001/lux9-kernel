#include "router.h"

int router_dispatch_ipc(Proc *p, Fcall *t, Fcall *r) {
  /*@
    @ requires \valid(p);
    @ requires \valid(t);
    @ requires \valid(r);
    @ ensures \result == 0 || \result == -1;
    @*/
  uchar *ep = t->sdata + t->scount;
  uchar *ptr = t->sdata;

  if (t->type == Tsyscall) {
    switch (t->scallnr) {
    case SYS_PIPE: {
      extern uintptr syspipe(void *list_void);

      /* Pebble: Check/deduct budget for pipe creation (userspace only).
       * TCB processes (kp == 1) are exempt. */
      if (up != nil && up->kp == 0) {
        lock(&pebble_global_lock);
        if (up->pebble.colorless_bank < PEBBLE_PIPE_COST) {
          unlock(&pebble_global_lock);
          r->type = Rerror;
          snprint(r->ename, sizeof(r->ename),
                  "pebble: insufficient budget for pipe");
          return -1;
        }
        up->pebble.colorless_bank -= PEBBLE_PIPE_COST;
        unlock(&pebble_global_lock);
      }

      ptr = tsyscall_skip_argc(ptr, ep, 1);

      int *fd = (int *)((uchar *)p->p9page + P9_MSG_OFFSET + 64);
      int *ufd = (int *)(EXCHANGE_PAGE_ADDR + P9_MSG_OFFSET + 64);
      fd[0] = -1;
      fd[1] = -1;
      ulong args[1] = {(ulong)ufd};

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      syspipe(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 8;
      r->sdata = (uchar *)fd;
      return 0;
    }

    /* Exchange Pool IPC Syscalls */
    case SYS_EXCHANGE_ALLOC: {
      extern uintptr sys_exchange_alloc(void *);
      print("router_ipc: SYS_EXCHANGE_ALLOC\n");

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      uintptr cap = sys_exchange_alloc(nil);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)cap;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_EXCHANGE_FREE: {
      extern uintptr sys_exchange_free(void *);
      print("router_ipc: SYS_EXCHANGE_FREE\n");

      /* Format: [cap_ptr 8] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 8 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      uintptr cap_ptr = (uintptr)GBIT64(ptr);

      ulong args[1] = {cap_ptr};
      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      sys_exchange_free(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_EXCHANGE_PUBLISH: {
      extern uintptr sys_exchange_publish(void *);
      print("router_ipc: SYS_EXCHANGE_PUBLISH\n");

      /* Format: [topic_name s] [data_ptr 8] [len 8] */
      /* But sdata already contains the packed data from userspace */
      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      /* Parse topic from sdata - it's a null-terminated string */
      char *topic = (char *)t->sdata;
      int topic_len = 0;
      while (topic_len < t->scount && topic[topic_len] != '\0')
        topic_len++;

      /* After topic comes data pointer and length */
      uchar *rest = t->sdata + topic_len + 1;
      if (rest + 12 > t->sdata + t->scount) {
        poperror();
        r->type = Rerror;
        r->ename = "exchange_publish: incomplete message";
        return -1;
      }

      uintptr data_ptr = (uintptr)GBIT32(rest);
      if (sizeof(uintptr) > 4) {
        data_ptr |= ((uintptr)GBIT32(rest + 4) << 32);
        rest += 8;
      } else {
        rest += 4;
      }
      ulong len = GBIT32(rest);

      /* Build args for sys_exchange_publish: [topic, data, len] */
      ulong args[3] = {(ulong)topic, (ulong)data_ptr, len};
      uintptr cap = sys_exchange_publish(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)cap;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_EXCHANGE_SUBSCRIBE: {
      extern uintptr sys_exchange_subscribe(void *);
      print("router_ipc: SYS_EXCHANGE_SUBSCRIBE\n");

      /* sdata contains topic name as null-terminated string */
      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      char *topic = (char *)t->sdata;
      ulong args[1] = {(ulong)topic};
      uintptr ret = sys_exchange_subscribe(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_EXCHANGE_UNSUBSCRIBE: {
      extern uintptr sys_exchange_unsubscribe(void *);
      print("router_ipc: SYS_EXCHANGE_UNSUBSCRIBE\n");

      /* sdata contains topic name as null-terminated string */
      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      char *topic = (char *)t->sdata;
      ulong args[1] = {(ulong)topic};
      uintptr ret = sys_exchange_unsubscribe(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_EXCHANGE_RECEIVE: {
      extern uintptr sys_exchange_receive(void *);
      print("router_ipc: SYS_EXCHANGE_RECEIVE\n");

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      uintptr notif = sys_exchange_receive(nil);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)notif;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    default:
      r->type = Rerror;
      r->ename = "IPC syscall not found";
      return -1;
    }
  }

  return -1;
}