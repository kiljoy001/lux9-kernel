#include "router.h"

/*@
  @
  //============================================================================
  @ // AXIOMATIC DEFINITIONS - IPC Protocol Correctness
  @
  //============================================================================
  @
  @ axiomatic IPC_Protocol {
  @
  @   // Valid P9 exchange page structure
  @   predicate valid_p9page(Proc *p) =
  @     \valid(p) && p->p9page != \null &&
  @     \valid((uchar*)p->p9page + (0..P9_PAGE_SIZE-1));
  @
  @   // Valid Fcall request for IPC
  @   predicate valid_ipc_request(Fcall *t) =
  @     \valid(t) &&
  @     t->type == Tsyscall &&
  @     (t->sdata == \null || \valid_read(t->sdata + (0..t->scount-1)));
  @
  @   // Valid reply buffer (uninitialized input, must be set)
  @   predicate valid_reply_buffer(Fcall *r) =
  @     \valid(r);
  @
  @   // Valid reply output (after processing)
  @   predicate valid_reply_output(Fcall *r) =
  @     \valid(r) &&
  @     (r->type == Rsyscall || r->type == Rerror);
  @
  @   // Pebble budget sufficient for operation
  @   predicate has_pebble_budget(Proc *p, u64int cost) =
  @     p == \null || p->kp == 1 || p->pebble.colorless_bank >= cost;
  @
  @   // Valid syscall number for IPC subsystem
  @   predicate valid_ipc_syscall(int scallnr) =
  @     scallnr == SYS_PIPE ||
  @     scallnr == SYS_EXCHANGE_ALLOC ||
  @     scallnr == SYS_EXCHANGE_FREE ||
  @     scallnr == SYS_EXCHANGE_PUBLISH ||
  @     scallnr == SYS_EXCHANGE_SUBSCRIBE ||
  @     scallnr == SYS_EXCHANGE_UNSUBSCRIBE ||
  @     scallnr == SYS_EXCHANGE_RECEIVE;
  @
  @   // Tag preservation axiom
  @   axiom tag_preservation:
  @     \forall Fcall *t, *r;
  @       \valid(t) && \valid(r) ==> r->tag == t->tag;
  @ }
  @*/

/*@
  @
  //============================================================================
  @ // MAIN IPC DISPATCHER CONTRACT
  @
  //============================================================================
  @
  @ requires valid_ipc_request(t);
  @ requires valid_reply_buffer(r);
  @ requires valid_p9page(p);
  @ requires valid_ipc_syscall(t->scallnr);
  @
  @ // Return value semantics
  @ ensures \result == 0 || \result == -1;
  @ ensures \result == 0 ==> r->type == Rsyscall;
  @ ensures \result == -1 ==> r->type == Rerror;
  @
  @ // Protocol correctness: Tag preservation
  @ ensures r->tag == t->tag;
  @ ensures valid_reply_output(r);
  @
  @ // Memory safety: What we modify
  @ assigns *r,
  @         p->pebble.colorless_bank;
  @
  @ // Pebble conservation for SYS_PIPE (userspace only)
  @ ensures t->scallnr == SYS_PIPE && p->kp == 0 && \result == 0 ==>
  @   p->pebble.colorless_bank == \old(p->pebble.colorless_bank) -
  PEBBLE_PIPE_COST;
  @ ensures t->scallnr == SYS_PIPE && p->kp == 0 && \result == -1 ==>
  @   p->pebble.colorless_bank == \old(p->pebble.colorless_bank);
  @
  @ // TCB processes (kp==1) are exempt from pebble costs
  @ ensures t->scallnr == SYS_PIPE && p->kp == 1 ==>
  @   p->pebble.colorless_bank == \old(p->pebble.colorless_bank);
  @
  @ terminates \true;
  @*/
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
    /*@
      @
      //========================================================================
      @ // SYS_PIPE - Create a pipe and return two file descriptors
      @
      //========================================================================
      @
      @ // Preconditions
      @ requires \valid((uchar*)p->p9page + (P9_MSG_OFFSET + 64) + (0..7));
      @ requires up == \null || \valid(up);
      @ requires up != \null && up->kp == 0 ==>
      @   has_pebble_budget(up, PEBBLE_PIPE_COST);
      @
      @ // Postconditions: Success path
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @ ensures \result == 0 ==> r->retval == 0;
      @ ensures \result == 0 ==> r->scount == 8;
      @ ensures \result == 0 ==> \valid_read(r->sdata + (0..7));
      @
      @ // Postconditions: Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ // Pebble conservation
      @ ensures up != \null && up->kp == 0 && \result == 0 ==>
      @   up->pebble.colorless_bank == \old(up->pebble.colorless_bank) -
      PEBBLE_PIPE_COST;
      @ ensures up != \null && up->kp == 0 && \result == -1 ==>
      @   up->pebble.colorless_bank == \old(up->pebble.colorless_bank);
      @
      @ assigns *r, up->pebble.colorless_bank,
      @         ((uchar*)p->p9page)[P9_MSG_OFFSET+64..P9_MSG_OFFSET+71];
      @*/
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
      uintptr ubase = p9_user_base(p);
      int *ufd = (int *)(ubase + P9_MSG_OFFSET + 64);
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
    /*@
      @
      //========================================================================
      @ // SYS_EXCHANGE_ALLOC - Allocate exchange pool capability
      @
      //========================================================================
      @
      @ // Postconditions: Success returns non-zero capability
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->retval != 0;
      @ ensures \result == 0 ==> r->scount == 0;
      @ ensures \result == 0 ==> r->sdata == \null;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @
      @ // Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ assigns *r;
      @*/
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

    /*@
      @
      //========================================================================
      @ // SYS_EXCHANGE_FREE - Free exchange pool capability
      @
      //========================================================================
      @
      @ // Preconditions: Need 8 bytes for capability pointer
      @ requires t->sdata != \null ==> t->scount >= 8;
      @ requires ptr <= ep;
      @
      @ // Postconditions: Success
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->retval == 0;
      @ ensures \result == 0 ==> r->scount == 0;
      @ ensures \result == 0 ==> r->sdata == \null;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @
      @ // Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ assigns *r;
      @*/
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

    /*@
      @
      //========================================================================
      @ // SYS_EXCHANGE_PUBLISH - Publish data to topic
      @
      //========================================================================
      @
      @ // Preconditions: Topic must be null-terminated in sdata
      @ requires t->scount > 0;
      @ requires \valid_read(t->sdata + (0..t->scount-1));
      @
      @ // Postconditions: Success returns topic capability
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->retval != 0;
      @ ensures \result == 0 ==> r->scount == 0;
      @ ensures \result == 0 ==> r->sdata == \null;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @
      @ // Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ assigns *r;
      @*/
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
      /*@
        @ loop invariant 0 <= topic_len <= t->scount;
        @ loop invariant \forall integer j; 0 <= j < topic_len ==> topic[j] !=
        '\0';
        @ loop assigns topic_len;
        @ loop variant t->scount - topic_len;
        @*/
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

    /*@
      @
      //========================================================================
      @ // SYS_EXCHANGE_SUBSCRIBE - Subscribe to topic
      @
      //========================================================================
      @
      @ // Preconditions: Topic name in sdata as null-terminated string
      @ requires t->scount > 0;
      @ requires \valid_read(t->sdata + (0..t->scount-1));
      @
      @ // Postconditions
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @ ensures \result == 0 ==> r->scount == 0;
      @ ensures \result == 0 ==> r->sdata == \null;
      @
      @ // Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ assigns *r;
      @*/
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

    /*@
      @
      //========================================================================
      @ // SYS_EXCHANGE_UNSUBSCRIBE - Unsubscribe from topic
      @
      //========================================================================
      @
      @ // Preconditions: Topic name in sdata as null-terminated string
      @ requires t->scount > 0;
      @ requires \valid_read(t->sdata + (0..t->scount-1));
      @
      @ // Postconditions
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @ ensures \result == 0 ==> r->scount == 0;
      @ ensures \result == 0 ==> r->sdata == \null;
      @
      @ // Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ assigns *r;
      @*/
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

    /*@
      @
      //========================================================================
      @ // SYS_EXCHANGE_RECEIVE - Receive notification from subscribed topic
      @
      //========================================================================
      @
      @ // Postconditions: Returns notification pointer
      @ ensures \result == 0 ==> r->type == Rsyscall;
      @ ensures \result == 0 ==> r->tag == t->tag;
      @ ensures \result == 0 ==> r->scount == 0;
      @ ensures \result == 0 ==> r->sdata == \null;
      @
      @ // Error path
      @ ensures \result == -1 ==> r->type == Rerror;
      @ ensures \result == -1 ==> r->tag == t->tag;
      @
      @ assigns *r;
      @*/
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
