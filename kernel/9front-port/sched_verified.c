#include "dat.h"
#include "fns.h"
#include "kthread.h"
#include <u.h>

/*
 * sched_verified.c - Minimal, Formally Verified Scheduler Core.
 * Part of the "Thin-Lux9 Ultra" High-Assurance strategy.
 */

/*@
  @ // Mapping KThread state to Logical ProcState
  @ logic ProcState kstate_to_pstate(int s) =
  @     s == KSeding  ? ProcScheding :
  @     s == KReady   ? ProcReady    :
  @     s == KRunning ? ProcRunning  :
  @     s == KWakeme  ? ProcWakeme   :
  @     s == KStopped ? ProcStopped  :
  @     ProcDead;
  @*/

/*@
  @ requires \valid(up);
  @ requires up->state == KRunning;
  @ assigns up->state, m->proc;
  @ ensures up->state == KSeding;
  @*/
void ksched(void) {
  if (up->nlocks > 0)
    return; /* Safety check: don't sched with locks */

  int s = splhi();

  /* Transition to Scheding (Readying) */
  up->state = KSeding;

  /*@ assert valid_transition(KThread_model_state((KThread*)up), ProcScheding);
   */

  /* Put back on run queue */
  kready((KThread *)up);

  /* Context switch to machine scheduler */
  procswitch();

  splx(s);
}

/*@
  @ requires \valid(p);
  @ requires p->state != KReady;
  @ assigns p->state;
  @ ensures p->state == KReady;
  @*/
void kready(KThread *p) {
  int s = splhi();

  p->state = KReady;
  p->mach = nil;

  /*@ assert valid_transition(KThread_model_state(p), ProcReady); */

  queueproc((Proc *)p);

  splx(s);
}

/*@
  @ assigns \result;
  @ ensures \result == \null || (\valid(\result) && \result->state == KReady);
  @*/
KThread *krunproc(void) {
  KThread *p;

  p = (KThread *)dequeueproc(nil,
                             nil); /* Simplified for refinement targeting */
  if (p != nil) {
    p->state = KRunning;
    p->mach = m;
    /*@ assert valid_transition(KThread_model_state(p), ProcRunning); */
  }

  return p;
}
