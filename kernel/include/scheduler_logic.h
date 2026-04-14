#ifndef _SCHEDULER_LOGIC_H_
#define _SCHEDULER_LOGIC_H_

/*
 * ACSL Logical Model for Scheduler State Machine.
 * Mirrors proofs/proc/proc_state_dag.v
 */

/*@
  @ type ProcState =
  @     ProcDead | ProcNew | ProcReady | ProcScheding |
  @     ProcRunning | ProcQueueing | ProcWakeme |
  @     ProcBroken | ProcStopped | ProcMoribund;
  @
  @ logic boolean valid_transition(ProcState from, ProcState to) =
  @     (from == ProcDead && to == ProcNew) ||
  @     (from == ProcNew  && to == ProcReady) ||
  @     (from == ProcReady && to == ProcRunning) ||
  @     (from == ProcRunning && to == ProcScheding) ||
  @     (from == ProcRunning && to == ProcWakeme) ||
  @     (from == ProcRunning && to == ProcMoribund) ||
  @     (from == ProcScheding && to == ProcReady) ||
  @     (from == ProcWakeme && to == ProcReady) ||
  @     (from == ProcMoribund && to == ProcDead);
  @*/

#endif /* _SCHEDULER_LOGIC_H_ */
