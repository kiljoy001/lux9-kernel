#ifndef __FRAMAC__
#include "../include/distributed_pebble.h"
#include "../include/msgord.h"
#include "router.h"

/* Forward declarations */
extern uint convM2S(uchar *, uint, Fcall *);
extern uint convS2M(Fcall *, uchar *, uint);

static void zero_bytes(uchar *buf, uint n) {
  if (buf == nil)
    return;
  for (uint i = 0; i < n; i++)
    buf[i] = 0;
}

static int p9_debug_enabled(Proc *p) {
  return p != nil && p->pid == 4;
}

static void dump_ctl_state(const char *label, Proc *p, P9Control *ctl) {
  if (!p9_debug_enabled(p) || ctl == nil)
    return;
  uint status = atomic_load(&ctl->status, ORDER_RELAXED);
  uint doorbell = atomic_load(&ctl->doorbell, ORDER_RELAXED);
  print("%s pid=%lud status=%ud doorbell=%ud req=%ud/%ud rep=%ud/%ud seq=%ud/%ud\n",
        label, p->pid, status, doorbell, ctl->req_head, ctl->req_tail,
        ctl->rep_head, ctl->rep_tail, ctl->req_seq, ctl->rep_seq);
}

/*@
  @ requires \valid(p) && p->p9page != \null;
  @ requires \valid_read(reply + (0..reply_size-1));
  @ requires \valid(saved_ctl);
  @ terminates \true;
  @*/
void scrub_exchange_page(Proc *p, const uchar *reply, uint reply_size,
                         P9Control *saved_ctl, u32int rep_head, u32int rep_tail,
                         int ring_mode) {
  P9Control *ctl;
  uchar *msg_buf;
  uchar reply_copy[P9_MSG_SIZE];

  if (p == nil || p->p9page == nil)
    return;

  zero_bytes(reply_copy, sizeof(reply_copy));
  if (reply != nil && reply_size > 0 && reply_size <= P9_MSG_SIZE)
    memmove(reply_copy, reply, reply_size);

  zero_bytes((uchar *)p->p9page, P9_PAGE_SIZE);

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;

  if (saved_ctl != nil) {
    memmove(ctl->session_pebble, saved_ctl->session_pebble,
            sizeof(ctl->session_pebble));
    ctl->req_seq = saved_ctl->req_seq;
    ctl->rep_seq = saved_ctl->rep_seq;
  }

  if (ring_mode) {
    u32int idx = rep_head;
    while (idx != rep_tail) {
      uchar *src = reply_copy + (idx * P9_RING_SLOT_SIZE);
      uchar *dst = msg_buf + (idx * P9_RING_SLOT_SIZE);
      memmove(dst, src, P9_RING_SLOT_SIZE);
      idx = (idx + 1) % P9_RING_SLOTS;
    }
    ctl->rep_head = rep_head;
    ctl->rep_tail = rep_tail;
  } else if (reply_size > 0) {
    memmove(msg_buf, reply_copy, reply_size);
    ctl->rep_head = 0;
    ctl->rep_tail = reply_size;
  }
}

static /*@
  @ requires \valid_read(buf + (0..n-1));
  @ terminates \true;
  @*/
    void
    dump_bytes(const char *label, const uchar *buf, uint n) {
  uint i;

  if (buf == nil || n == 0)
    return;

  print("%s", label);
  for (i = 0; i < n; i++)
    print(" %02x", buf[i]);
  print("\n");
}

/*
 * Ring-buffer mode: process multiple small messages from exchange page.
 * Layout per slot: [req_size:4][rep_size:4][data...]
 * req_head/req_tail and rep_head/rep_tail are slot indices.
 */
/*@
  @ requires \valid(p) && p->p9page != \null;
  @ requires \valid(ctl);
  @ requires \valid(msg_buf + (0..P9_MSG_SIZE-1));
  @ terminates \true;
  @ assigns *ctl, msg_buf[0..P9_MSG_SIZE-1];
  @*/
int p9_handle_ring(Proc *p, P9Control *ctl, uchar *msg_buf) {
  u32int head = ctl->req_head;
  u32int tail = ctl->req_tail;
  u32int rep_head = ctl->rep_head;
  u32int rep_tail = ctl->rep_tail;

  if (head >= P9_RING_SLOTS || tail >= P9_RING_SLOTS ||
      rep_head >= P9_RING_SLOTS || rep_tail >= P9_RING_SLOTS)
    return -1;

  while (head != tail) {
    uchar *slot = msg_buf + (head * P9_RING_SLOT_SIZE);
    u32int req_size = GBIT32(slot);
    if (req_size == 0 || req_size > P9_RING_DATA_SIZE)
      return -1;

    Fcall t, r;
    t = (Fcall){0};
    if (convM2S(slot + P9_RING_HEADER_SIZE, req_size, &t) == 0)
      return -1;

    r = (Fcall){0};
    /* Need to declare p9_dispatch in router.h or extern here */
    extern int p9_dispatch(Proc * p, Fcall * t, Fcall * r);
    if (waserror()) {
      r = (Fcall){.type = Rerror, .tag = t.tag, .ename = up->errstr};
    } else {
      int disp = p9_dispatch(p, &t, &r);
      poperror();
      if (disp < 0 && r.type != Rerror)
        r = (Fcall){.type = Rerror, .tag = t.tag, .ename = "dispatch failed"};
    }

    u32int rep_size =
        convS2M(&r, slot + P9_RING_HEADER_SIZE, P9_RING_DATA_SIZE);
    if (rep_size == 0)
      return -1;
    PBIT32(slot + 4, rep_size);
    /* Zero out the remainder of the slot data for security */
    zero_bytes(slot + P9_RING_HEADER_SIZE + rep_size,
               P9_RING_DATA_SIZE - rep_size);

    u32int next_rep = (rep_tail + 1) % P9_RING_SLOTS;
    if (next_rep == rep_head)
      return -1;
    rep_tail = next_rep;
    head = (head + 1) % P9_RING_SLOTS;
  }

  ctl->req_head = head;
  ctl->rep_tail = rep_tail;
  return 0;
}

/*@
  @ requires \valid(p) && p->p9page != \null;
  @ requires \valid((uchar *)p->p9page + (0..P9_PAGE_SIZE-1));
  @ terminates \true;
  @*/
int p9_handle_doorbell(Proc *p, Ureg *ureg) {
  /*@
    @ requires \valid(p);
    @ requires p->p9page == \null || \valid((uchar*)p->p9page +
    (0..P9_PAGE_SIZE-1));
    @ ensures p->p9page == \null ==> \result == -1;
    @*/
  P9Control *ctl;
  uchar *msg_buf; /* Single buffer for request AND reply */
  Fcall t, r;
  uint msg_size;
  int result;
  uintptr page_pa;
  enum BorrowError berr;
  P9Control ctl_saved;
  u32int req_seq;

  /* CRITICAL: Set up->dbgreg so sysrfork/sysproc can access ureg for fork */
  if (ureg != nil)
    up->dbgreg = ureg;

  /* Validate exchange page exists and is coherent with P9SEG */
  if (p->seg[P9SEG] != nil && p->seg[P9SEG]->pseg != nil &&
      p->seg[P9SEG]->pseg->pa != 0) {
    p->p9page = (void *)kaddr(p->seg[P9SEG]->pseg->pa);
  }
  if (p->p9page == nil) {
    print("p9_handle_doorbell: no exchange page for pid %lud\n", p->pid);
    return -1;
  }

  if (p9_debug_enabled(p)) {
    print("p9_handle_doorbell: entry pid=%lud p9page=%#p p9seg=%#p\n", p->pid,
          p->p9page, p->seg[P9SEG]);
  }

  /* Get physical address of the single exchange page */
  if (p->seg[P9SEG] != nil && p->seg[P9SEG]->pseg != nil &&
      p->seg[P9SEG]->pseg->pa != 0)
    page_pa = p->seg[P9SEG]->pseg->pa;
  else
    page_pa = PADDR(p->p9page);

  if (p9_debug_enabled(p)) {
    print("p9_handle_doorbell: ubase=%#p page_pa=%#p\n",
          (void *)p9_user_base(p), (void *)page_pa);
  }

  /* Ensure exchange page is mapped into userspace */
  uintptr ubase = p9_user_base(p);
  uintptr *pte = mmuwalk(m->pml4, ubase, 0, 0);
  if (pte == nil || (*pte & PTEVALID) == 0) {
    print("p9_handle_doorbell: remapping exchange page for pid %lud\n", p->pid);
    userpmap(ubase, page_pa, PTEVALID | PTEUSER | PTEWRITE);
  } else if (p9_debug_enabled(p)) {
    print("p9_handle_doorbell: ubase pte=%#p pa=%#p\n", *pte, PPN(*pte));
  }

  /*
   * OWNERSHIP TRANSFER: Process -> Kernel
   * =====================================
   * Process has finished writing request and issued syscall.
   * Transfer ownership so kernel has exclusive access.
   */
  int s = splhi(); /* Block interrupts during critical ownership transfer */
  uintptr page_key = (uintptr)kaddr(page_pa);
  if (p9_debug_enabled(p)) {
    print("p9_handle_doorbell: borrow key pid=%lud pa=%#p key=%#p p9page=%#p "
          "p9page_phys=%#llx pseg_pa=%#p\n",
          p ? p->pid : 0, (void *)page_pa, (void *)page_key, p->p9page,
          (unsigned long long)p->p9page_phys,
          p && p->seg[P9SEG] && p->seg[P9SEG]->pseg ?
              (void *)p->seg[P9SEG]->pseg->pa : 0);
  }
  if (!borrow_is_owned(page_key)) {
    if (p9_debug_enabled(p)) {
      print("p9_handle_doorbell: borrow missing, acquiring for pid=%lud "
            "key=%#p\n",
            p ? p->pid : 0, (void *)page_key);
    }
    berr = borrow_acquire(p, page_key);
    if (berr != BORROW_OK && berr != BORROW_EALREADY) {
      print("p9_handle_doorbell: borrow_acquire failed (berr=%d)\n", berr);
    }
  }

  berr = borrow_transfer(p, up, page_key);
  if (berr != BORROW_OK) {
    /* First syscall after boot - process may not have formal ownership yet */
    print("p9_handle_doorbell: borrow_transfer failed (berr=%d), acquiring "
          "directly\n",
          berr);
    berr = borrow_acquire(up, page_key);
    if (berr != BORROW_OK && berr != BORROW_EALREADY) {
      print("p9_handle_doorbell: FATAL - kernel can't acquire page (berr=%d)\n",
            berr);
      splx(s);
      return -1;
    }
  } else if (p9_debug_enabled(p)) {
    print("p9_handle_doorbell: borrow_transfer ok pid=%lud\n", p->pid);
  }

  /* Kernel now has exclusive access to the page */

  /* Get control block and message buffer */
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;
  memmove(&ctl_saved, ctl, sizeof(ctl_saved));

  dump_ctl_state("p9_handle_doorbell: ctl before", p, ctl);

  req_seq = ctl->req_seq;
  if (atomic_load(&ctl->status, ORDER_RELAXED) != P9_STATUS_PENDING ||
      req_seq == ctl->rep_seq) {
    if (p9_debug_enabled(p)) {
      print("p9_handle_doorbell: skip stale req seq=%ud rep=%ud status=%ud "
            "pid=%lud\n",
            req_seq, ctl->rep_seq, atomic_load(&ctl->status, ORDER_RELAXED),
            p ? p->pid : 0);
    }
    result = -1;
    splx(s);
    goto cleanup_ownership;
  }

  if (p9_debug_enabled(p))
    dump_bytes("p9_handle_doorbell: REQUEST msg[0..31]:", msg_buf, 32);

  /* Ring-buffer mode for small messages */
  if (ctl->req_head != ctl->req_tail) {
    if (p9_debug_enabled(p)) {
      print("p9_handle_doorbell: ring mode req_head=%ud req_tail=%ud\n",
            ctl->req_head, ctl->req_tail);
    }
    /* Memory barrier to ensure user writes are visible to kernel */
    __asm__ volatile("mfence" ::: "memory");
    result = p9_handle_ring(p, ctl, msg_buf);
    memmove(&ctl_saved, ctl, sizeof(ctl_saved));
    u32int rep_head = ctl->rep_head;
    u32int rep_tail = ctl->rep_tail;
    u32int req_head = ctl->req_head;
    u32int req_tail = ctl->req_tail;
    scrub_exchange_page(p, msg_buf, P9_MSG_SIZE, &ctl_saved, rep_head, rep_tail,
                        1);
    ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
    ctl->req_head = req_head;
    ctl->req_tail = req_tail;
    ctl->rep_head = rep_head;
    ctl->rep_tail = rep_tail;
    if (result < 0) {
      atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    } else {
      ctl->rep_seq = req_seq;
      atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
    }
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }

  /* Parse request from message buffer */
  t = (Fcall){0};

  /* Memory barrier to ensure user writes are visible to kernel.
   * User writes to EXCHANGE_PAGE_ADDR, kernel reads via HHDM at p->p9page.
   * The mfence ensures cache coherency between different VA mappings. */
  __asm__ volatile("mfence" ::: "memory");

  /* Get message size from 9P header (first 4 bytes) */
  msg_size = GBIT32(msg_buf);
  if (p9_debug_enabled(p))
    print("p9_handle_doorbell: msg_size=%ud first=%02x\n", msg_size, msg_buf[0]);
  if (msg_size < 7 || msg_size > P9_MSG_SIZE) {
    print("p9_handle_doorbell: invalid message size %ud\n", msg_size);
    print("p9_handle_doorbell: ctl req_head=%ud req_tail=%ud rep_head=%ud "
          "rep_tail=%ud\n",
          ctl->req_head, ctl->req_tail, ctl->rep_head, ctl->rep_tail);
    if (p9_debug_enabled(p))
      dump_bytes("p9_handle_doorbell: BAD SIZE msg[0..31]:", msg_buf, 32);
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    result = -1;
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }

  if (convM2S(msg_buf, msg_size, &t) == 0) {
    print("p9_handle_doorbell: failed to parse Fcall (first byte: 0x%02x)\n",
          msg_buf[0]);
    if (p9_debug_enabled(p))
      dump_bytes("p9_handle_doorbell: PARSE FAIL msg[0..31]:", msg_buf, 32);
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }
  splx(s); /* Restore interrupts before long processing. */

  /* Dispatch through 9P router */
  r = (Fcall){0};
  extern int p9_dispatch(Proc * p, Fcall * t, Fcall * r);
  if (waserror()) {
    r = (Fcall){.type = Rerror, .tag = t.tag, .ename = up->errstr};
    result = -1;
  } else {
    result = p9_dispatch(p, &t, &r);
    poperror();
    if (result < 0 && r.type != Rerror)
      r = (Fcall){.type = Rerror, .tag = t.tag, .ename = "dispatch failed"};
  }

  /* Write reply to SAME buffer location (ownership-flip model) */
  uchar reply_copy[P9_MSG_SIZE];
  uint rep_size = convS2M(&r, reply_copy, P9_MSG_SIZE);
  if (rep_size == 0) {
    print("p9_handle_doorbell: failed to serialize reply (r.type=%d)\n",
          r.type);
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    result = -1;
    goto cleanup_ownership;
  }

  if (p9_debug_enabled(p))
    dump_bytes("p9_handle_doorbell: REPLY msg[0..31]:", reply_copy, 32);
  memmove(&ctl_saved, ctl, sizeof(ctl_saved));
  scrub_exchange_page(p, reply_copy, rep_size, &ctl_saved, 0, 0, 0);
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;

  if (p9_debug_enabled(p))
    dump_bytes("p9_handle_doorbell: PAGEDUMP msg[0..31]:", msg_buf, 32);

  /* Set RAX to return value for ABI compatibility and efficient checking */
  /* DO NOT overwrite RAX on successful exec, as sysexec already set it up
   * correctly */
  if (ureg != nil) {
    if (r.type == Rexec || r.type == Rsysexec) {
      print(
          "DOORBELL: detected exec (type=%d), PRESERVING RAX=%#p for pid %ld\n",
          r.type, (void *)ureg->ax, p->pid);
    } else {
      ureg->ax = (ulong)r.retval;
      if (r.type == Rsysfork)
        print("DOORBELL: Rsysfork pid=%ld rax=%#p ureg=%p\n", p->pid,
              (void *)ureg->ax, ureg);
    }
  }

  /* Success! Reply written to buffer.
   * Even if p9_dispatch returned -1 (Rerror), from the perspective of the
   * doorbell mechanism, we successfully processed the message and wrote a
   * reply.
   */
  result = 0;

  /* Update control block */
  ctl->rep_seq = req_seq;

  /* Mark as complete with Release semantics */
  atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);

cleanup_ownership:
  /*
   * OWNERSHIP TRANSFER: Kernel -> Process
   * =====================================
   * Kernel has finished processing. Transfer ownership back so
   * process can read the reply.
   */
  berr = borrow_transfer(up, p, page_key);
  if (berr != BORROW_OK) {
    print(
        "p9_handle_doorbell: WARNING - borrow_transfer back failed (berr=%d)\n",
        berr);
    /* Fall back to release/acquire */
    borrow_release(up, page_key);
    berr = borrow_acquire(p, page_key);
    if (berr != BORROW_OK) {
      print("p9_handle_doorbell: FATAL - can't return page to process "
            "(berr=%d)\n",
            berr);
      panic("p9_handle_doorbell: ownership violation - cannot return page");
    }
  }

  /* Success! Return the reply type so the caller can identify special cases
   * (like exec) */
  return r.type;
}
#endif
