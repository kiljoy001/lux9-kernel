#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

#include "proc_packet.h"
#include <error.h>

/* FSM Integration */
extern int proc_event(Proc *p, int event);

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void eqlock(QLock *q) {
  Proc *p;
  uintptr pc;

  pc = getcallerpc(&q);

  lock(&q->use);
  if (!q->locked) {
    q->pc = pc;
    q->locked = 1;
    unlock(&q->use);
    return;
  }
  if (up == nil)
    panic("eqlock");
  if (up->notepending) {
    up->notepending = 0;
    unlock(&q->use);
    interrupted();
  }
  p = q->tail;
  if (p == nil)
    q->head = up;
  else
    p->qnext = up;
  q->tail = up;
  up->eql = q;
  up->qnext = nil;
  up->qpc = pc;
  /* up->state = Queueing; -- REPLACED BY FSM */
  proc_event(up, EV_QLOCK);
  unlock(&q->use);
  sched();
  if (up->eql == nil) {
    up->notepending = 0;
    interrupted();
  }
  up->eql = nil;
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void qlock(QLock *q) {
  Proc *p;
  uintptr pc;

  pc = getcallerpc(&q);

  lock(&q->use);
  if (!q->locked) {
    q->pc = pc;
    q->locked = 1;
    unlock(&q->use);
    return;
  }
  if (up == nil)
    panic("qlock");
  p = q->tail;
  if (p == nil)
    q->head = up;
  else
    p->qnext = up;
  q->tail = up;
  up->eql = nil;
  up->qnext = nil;
  up->qpc = pc;
  /* up->state = Queueing; -- REPLACED BY FSM */
  proc_event(up, EV_QLOCK);
  unlock(&q->use);
  sched();
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
int canqlock(QLock *q) {
  if (!canlock(&q->use))
    return 0;
  if (q->locked) {
    unlock(&q->use);
    return 0;
  }
  q->locked = 1;
  q->pc = getcallerpc(&q);
  unlock(&q->use);
  return 1;
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void qunlock(QLock *q) {
  Proc *p;

  lock(&q->use);
  if (!q->locked) {
    unlock(&q->use);
    print("qunlock called with qlock not held, from %#p\n", getcallerpc(&q));
    return;
  }
  p = q->head;
  if (p != nil) {
    if (p->state != Queueing)
      panic("qunlock");
    q->pc = p->qpc;
    q->head = p->qnext;
    if (q->head == nil)
      q->tail = nil;
    unlock(&q->use);
    ready(p);
    return;
  }
  q->locked = 0;
  unlock(&q->use);
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void rlock(RWLock *q) {
  Proc *p;

  lock(&q->use);
  if (q->writer == 0 && q->head == nil) {
    /* no writer, go for it */
    q->readers++;
    unlock(&q->use);
    return;
  }
  p = q->tail;
  if (up == nil)
    panic("rlock");
  if (p == nil)
    q->head = up;
  else
    p->qnext = up;
  q->tail = up;
  up->qnext = nil;
  /* up->state = QueueingR; -- REPLACED BY FSM */
  proc_event(up, EV_QLOCK_R);
  unlock(&q->use);
  sched();
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void runlock(RWLock *q) {
  Proc *p;

  lock(&q->use);
  p = q->head;
  if (--(q->readers) > 0 || p == nil) {
    unlock(&q->use);
    return;
  }

  /* start waiting writer */
  if (p->state != QueueingW)
    panic("runlock");
  q->head = p->qnext;
  if (q->head == nil)
    q->tail = nil;
  q->wpc = p->qpc;
  q->writer = 1;
  unlock(&q->use);
  ready(p);
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void wlock(RWLock *q) {
  Proc *p;
  uintptr pc;

  pc = getcallerpc(&q);

  lock(&q->use);
  if (q->readers == 0 && q->writer == 0) {
    /* noone waiting, go for it */
    q->wpc = pc;
    q->writer = 1;
    unlock(&q->use);
    return;
  }

  /* wait */
  p = q->tail;
  if (up == nil)
    panic("wlock");
  if (p == nil)
    q->head = up;
  else
    p->qnext = up;
  q->tail = up;
  up->qnext = nil;
  up->qpc = pc;
  /* up->state = QueueingW; -- REPLACED BY FSM */
  proc_event(up, EV_QLOCK_W);
  unlock(&q->use);
  sched();
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void wunlock(RWLock *q) {
  Proc *p;

  lock(&q->use);
  p = q->head;
  if (p == nil) {
    q->writer = 0;
    unlock(&q->use);
    return;
  }
  if (p->state == QueueingW) {
    /* start waiting writer */
    q->wpc = p->qpc;
    q->head = p->qnext;
    if (q->head == nil)
      q->tail = nil;
    unlock(&q->use);
    ready(p);
    return;
  }

  if (p->state != QueueingR)
    panic("wunlock");

  /* waken waiting readers */
  while (q->head != nil && q->head->state == QueueingR) {
    p = q->head;
    q->head = p->qnext;
    q->readers++;
    ready(p);
  }
  if (q->head == nil)
    q->tail = nil;
  q->writer = 0;
  unlock(&q->use);
}

/* same as rlock but punts if there are any writers waiting */
/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
int canrlock(RWLock *q) {
  lock(&q->use);
  if (q->writer == 0 && q->head == nil) {
    /* no writer, go for it */
    q->readers++;
    unlock(&q->use);
    return 1;
  }
  unlock(&q->use);
  return 0;
}
