#include "router.h"
#include "../include/distributed_pebble.h"
#include "../include/msgord.h"

/* Forward declarations */
static int p9_handle_ring(Proc *p, P9Control *ctl, uchar *msg_buf);
extern uint convM2S(uchar *, uint, Fcall *);
extern uint convS2M(Fcall *, uchar *, uint);

static void scrub_exchange_page(Proc *p, const uchar *reply, uint reply_size,
                                P9Control *saved_ctl, u32int rep_head,
                                u32int rep_tail, int ring_mode) {
  P9Control *ctl;
  uchar *msg_buf;
  uchar reply_copy[P9_MSG_SIZE];

  if (p == nil || p->p9page == nil)
    return;

  memset(reply_copy, 0, sizeof(reply_copy));
  if (reply != nil && reply_size > 0 && reply_size <= P9_MSG_SIZE)
    memmove(reply_copy, reply, reply_size);

  memset(p->p9page, 0, P9_PAGE_SIZE);

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

static void dump_bytes(const char *label, const uchar *buf, uint n) {
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
static int p9_handle_ring(Proc *p, P9Control *ctl, uchar *msg_buf) {
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
    memset(&t, 0, sizeof(t));
    if (convM2S(slot + P9_RING_HEADER_SIZE, req_size, &t) == 0)
      return -1;

    memset(&r, 0, sizeof(r));
    /* Need to declare p9_dispatch in router.h or extern here */
    extern int p9_dispatch(Proc *p, Fcall *t, Fcall *r);
    int disp = p9_dispatch(p, &t, &r);
    if (disp < 0)
      r = (Fcall){.type = Rerror, .tag = t.tag, .ename = "dispatch failed"};

    u32int rep_size =
        convS2M(&r, slot + P9_RING_HEADER_SIZE, P9_RING_DATA_SIZE);
    if (rep_size == 0)
      return -1;
    PBIT32(slot + 4, rep_size);

    u32int next_rep = (rep_tail + 1) % P9_RING_SLOTS;
    if (next_rep == rep_head)
      return -1;
    rep_tail = next_rep;
    head = (head + 1) % P9_RING_SLOTS;
    ctl->rep_seq++;
  }

  ctl->req_head = head;
  ctl->rep_tail = rep_tail;
  return 0;
}

int p9_handle_doorbell(Proc *p, Ureg *ureg) {
  /*@
    @ requires \valid(p);
    @ requires p->p9page == \null || \valid((uchar*)p->p9page + (0..P9_PAGE_SIZE-1));
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

  /* Validate exchange page exists and is coherent with P9SEG */
  if (p->seg[P9SEG] != nil && p->seg[P9SEG]->pseg != nil &&
      p->seg[P9SEG]->pseg->pa != 0) {
    p->p9page = (void *)kaddr(p->seg[P9SEG]->pseg->pa);
  }
  if (p->p9page == nil) {
    print("p9_handle_doorbell: no exchange page for pid %lud\n", p->pid);
    return -1;
  }

  /* Get physical address of the single exchange page */
  if (p->seg[P9SEG] != nil && p->seg[P9SEG]->pseg != nil &&
      p->seg[P9SEG]->pseg->pa != 0)
    page_pa = p->seg[P9SEG]->pseg->pa;
  else
    page_pa = PADDR(p->p9page);

  /* Ensure exchange page is mapped into userspace */
  uintptr *pte = mmuwalk(m->pml4, EXCHANGE_PAGE_ADDR, 0, 0);
  if (pte == nil || (*pte & PTEVALID) == 0) {
    print("p9_handle_doorbell: remapping exchange page for pid %lud\n", p->pid);
    userpmap(EXCHANGE_PAGE_ADDR, page_pa, PTEVALID | PTEUSER | PTEWRITE);
  }

  /*
   * OWNERSHIP TRANSFER: Process -> Kernel
   * =====================================
   * Process has finished writing request and issued syscall.
   * Transfer ownership so kernel has exclusive access.
   */
  int s = splhi(); /* Block interrupts during critical ownership transfer */
  berr = borrow_transfer(p, up, page_pa);
  if (berr != BORROW_OK) {
    /* First syscall after boot - process may not have formal ownership yet */
    print("p9_handle_doorbell: borrow_transfer failed (berr=%d), acquiring "
          "directly\n",
          berr);
    berr = borrow_acquire(up, page_pa);
    if (berr != BORROW_OK && berr != BORROW_EALREADY) {
      print("p9_handle_doorbell: FATAL - kernel can't acquire page (berr=%d)\n",
            berr);
      splx(s);
      return -1;
    }
  }

  /* Kernel now has exclusive access to the page */

  /* Get control block and message buffer */
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;
  memmove(&ctl_saved, ctl, sizeof(ctl_saved));

  /* Mark as pending */
  atomic_store(&ctl->status, P9_STATUS_PENDING, ORDER_RELAXED);

  /* Ring-buffer mode for small messages */
  if (ctl->req_head != ctl->req_tail) {
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
    if (result < 0)
      atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    else
      atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }

  /* Parse request from message buffer */
  memset(&t, 0, sizeof(t));

  /* Memory barrier to ensure user writes are visible to kernel.
   * User writes to EXCHANGE_PAGE_ADDR, kernel reads via HHDM at p->p9page.
   * The mfence ensures cache coherency between different VA mappings. */
  __asm__ volatile("mfence" ::: "memory");

  /* Get message size from 9P header (first 4 bytes) */
  msg_size = GBIT32(msg_buf);
  if (msg_size < 7 || msg_size > P9_MSG_SIZE) {
    print("p9_handle_doorbell: invalid message size %ud\n", msg_size);
    print("p9_handle_doorbell: ctl req_head=%ud req_tail=%ud rep_head=%ud "
          "rep_tail=%ud\n",
          ctl->req_head, ctl->req_tail, ctl->rep_head, ctl->rep_tail);
    dump_bytes("p9_handle_doorbell: msg[0..31]:", msg_buf, 32);
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    result = -1;
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }

  if (convM2S(msg_buf, msg_size, &t) == 0) {
    print("p9_handle_doorbell: failed to parse Fcall (first byte: 0x%02x)\n",
          msg_buf[0]);
    print("p9_handle_doorbell: msg_size=%ud ctl req_head=%ud req_tail=%ud "
          "rep_head=%ud rep_tail=%ud\n",
          msg_size, ctl->req_head, ctl->req_tail, ctl->rep_head, ctl->rep_tail);
    dump_bytes("p9_handle_doorbell: msg[0..31]:", msg_buf, 32);
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }
  splx(s); /* Restore interrupts before long processing. */

  /* Dispatch through 9P router */
  memset(&r, 0, sizeof(r));
  extern int p9_dispatch(Proc *p, Fcall *t, Fcall *r);
  result = p9_dispatch(p, &t, &r);

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
  memmove(&ctl_saved, ctl, sizeof(ctl_saved));
  scrub_exchange_page(p, reply_copy, rep_size, &ctl_saved, 0, 0, 0);
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;

  /* Set RAX to return value for ABI compatibility and efficient checking */
  if (ureg != nil) {
    ureg->ax = (ulong)r.retval;
  }

  /* Success! Reply written to buffer.
   * Even if p9_dispatch returned -1 (Rerror), from the perspective of the
   * doorbell mechanism, we successfully processed the message and wrote a
   * reply.
   */
  result = 0;

  /* Update control block */
  ctl->rep_seq++;

  /* Mark as complete with Release semantics */
  atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);

cleanup_ownership:
  /*
   * OWNERSHIP TRANSFER: Kernel -> Process
   * =====================================
   * Kernel has finished processing. Transfer ownership back so
   * process can read the reply.
   */
  berr = borrow_transfer(up, p, page_pa);
  if (berr != BORROW_OK) {
    print(
        "p9_handle_doorbell: WARNING - borrow_transfer back failed (berr=%d)\n",
        berr);
    /* Fall back to release/acquire */
    borrow_release(up, page_pa);
    berr = borrow_acquire(p, page_pa);
    if (berr != BORROW_OK) {
      print("p9_handle_doorbell: FATAL - can't return page to process "
            "(berr=%d)\n",
            berr);
      panic("p9_handle_doorbell: ownership violation - cannot return page");
    }
  }

  return result;
}
