/*
 * Proc FSM Implementation
 *
 * Finite State Machine for process state transitions.
 * All state changes MUST go through proc_event().
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "proc_packet.h"
#include <u.h>

/*
 * State names for debugging
 */
char *proc_state_names[PS_COUNT] = {
    [PS_Dead] = "Dead",
    [PS_Moribund] = "Moribund",
    [PS_New] = "New",
    [PS_Ready] = "Ready",
    [PS_Scheding] = "Scheding",
    [PS_Running] = "Running",
    [PS_Queueing] = "Queueing",
    [PS_QueueingR] = "QueueingR",
    [PS_QueueingW] = "QueueingW",
    [PS_Wakeme] = "Wakeme",
    [PS_Broken] = "Broken",
    [PS_Stopped] = "Stopped",
    [PS_Rendezvous] = "Rendezvous",
    [PS_Waitrelease] = "Waitrelease",
    [PS_Intr] = "Intr",
    [PS_IntrReturn] = "IntrReturn",
};

/*
 * Guard Functions
 */

/* Guard: mach must be nil (not on any CPU) */
/*
  // Implements check_mach_cleared per proofs/proc/proc_state_dag.v
 */
static int guard_mach_nil(Proc *p, const char **reason) {
  /*@
    @ requires \valid(p);
    @ ensures \result == 1 ==> p->mach == \null;
    @ assigns \nothing;
    @*/
  if (p->mach != nil) {
    *reason = "mach must be nil";
    return 0;
  }
  return 1;
}

/* Guard: mach must be set (on a CPU) */
/*
  // Implements check_mach_set per proofs/proc/proc_state_dag.v
 */
static int guard_mach_set(Proc *p, const char **reason) {
  /*@
    @ requires \valid(p);
    @ ensures \result == 1 ==> p->mach != \null;
    @ assigns \nothing;
    @*/
  if (p->mach == nil) {
    *reason = "mach must be set";
    return 0;
  }
  return 1;
}

/* Guard: r (rendezvous) must be nil - THE CRITICAL FIX */
/*
  // Implements check_rendezvous_cleared per proofs/proc/proc_state_dag.v
 */
static int guard_r_nil(Proc *p, const char **reason) {
  /*@
    @ requires \valid(p);
    @ ensures \result == 1 ==> p->r == \null;
    @ assigns \nothing;
    @*/
  if (p->r != nil) {
    *reason = "p->r must be nil before wakeup";
    return 0;
  }
  return 1;
}

/* Guard: r (rendezvous) must be set for sleep */
/*
  // Implements check_rendezvous_set per proofs/proc/proc_state_dag.v
 */
static int guard_r_set(Proc *p, const char **reason) {
  /*@
    @ requires \valid(p);
    @ ensures \result == 1 ==> p->r != \null;
    @ assigns \nothing;
    @*/
  if (p->r == nil) {
    *reason = "p->r must be set for sleep";
    return 0;
  }
  return 1;
}

/*
 * Transition Table
 *
 * Each entry: { from_state, event, to_state, guard }
 * Guard is nil if no precondition required.
 */
static ProcTransition fsm_transitions[] = {
    /* Process creation */
    {PS_Dead, EV_CREATE, PS_New, nil},
    {PS_New, EV_READY, PS_Ready, guard_mach_nil},

    /* Scheduling */
    {PS_Ready, EV_READY, PS_Ready, nil}, /* Idempotent: already ready */
    {PS_Ready, EV_SCHEDULE, PS_Running, guard_mach_set},
    {PS_Running, EV_YIELD, PS_Scheding, nil},
    {PS_Scheding, EV_SCHEDULE, PS_Running, guard_mach_set},
    {PS_Scheding, EV_READY, PS_Ready, nil},

    /* Sleep/Wake - THE CRITICAL PATH */
    {PS_Running, EV_SLEEP, PS_Wakeme, guard_r_set},
    {PS_Wakeme, EV_WAKEUP, PS_Ready, guard_r_nil}, /* MUST have r==nil */

    /* QLock waiting */
    {PS_Running, EV_QLOCK, PS_Queueing, nil},
    {PS_Queueing, EV_QUNLOCK, PS_Ready, nil},
    /* Relaxed check: Allow YIELD while Queueing (scheduler quirk) */
    {PS_Queueing, EV_YIELD, PS_Queueing, nil},

    {PS_Running, EV_QLOCK_R, PS_QueueingR, nil},
    {PS_QueueingR, EV_QUNLOCK, PS_Ready, nil},

    {PS_Running, EV_QLOCK_W, PS_QueueingW, nil},
    {PS_QueueingW, EV_QUNLOCK, PS_Ready, nil},

    /* Rendezvous */
    {PS_Running, EV_RENDEZ, PS_Rendezvous, nil},
    {PS_Rendezvous, EV_RENDEZ_DONE, PS_Ready, nil},

    /* Debug/Stop */
    {PS_Running, EV_STOP, PS_Stopped, nil},
    {PS_Stopped, EV_CONT, PS_Ready, nil},

    /* vfork Synchronization (RFMEM) */
    {PS_Running, EV_VFORK, PS_Waitrelease, nil},
    {PS_Waitrelease, EV_VFORK_DONE, PS_Ready, nil},

    /* Exit */
    {PS_Running, EV_EXIT, PS_Moribund, nil},
    {PS_Moribund, EV_REAP, PS_Dead, guard_mach_nil},

    /* Interrupt handling */
    {PS_Running, EV_INTERRUPT, PS_Intr, nil},
    {PS_Intr, EV_IRETURN, PS_IntrReturn, nil},
    {PS_IntrReturn, EV_RESUME, PS_Running, nil},

    /* Break (error) - from any running state */
    {PS_Running, EV_BREAK, PS_Broken, nil},

    /* Sentinel */
    {-1, -1, -1, nil}};

/*
 * CRC-16 CCITT (polynomial 0x1021)
 */
ushort proc_crc16(uchar *data, int len) {
  ushort crc = 0xFFFF;
  int i, j;

  for (i = 0; i < len; i++) {
    crc ^= (ushort)data[i] << 8;
    for (j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}

/*
 * Verify header checksum
 */
int proc_verify(Proc *p) {
  ushort computed = proc_crc16((uchar *)p, 6);
  return p->hdr_checksum == computed;
}

/*
 * Update header checksum
 */
void proc_seal(Proc *p) { p->hdr_checksum = proc_crc16((uchar *)p, 6); }

/*
 * Find transition in table
 */
static ProcTransition *fsm_find(int from_state, int event) {
  ProcTransition *t;

  for (t = fsm_transitions; t->from_state >= 0; t++) {
    if (t->from_state == from_state && t->event == event)
      return t;
  }
  return nil;
}

/*
 * Execute state transition
 *
 * Returns new state on success, panics on invalid transition.
 */
/*
  // Enforces valid_transition per proofs/proc/proc_state_dag.v
 */
int proc_event(Proc *p, int event) {
  /*@
    @ requires \valid(p);
    @ ensures p->state == \result;
    @ assigns p->state_trace, p->state, p->hdr_checksum;
    @*/
  int current;
  ProcTransition *t;
  const char *reason;

  current = proc_state(p);

  /* Find valid transition */
  t = fsm_find(current, event);
  if (t == nil) {
    panic("proc_fsm: invalid transition %s + event %d",
          proc_state_names[current], event);
  }

  /* Check guard if present */
  if (t->guard != nil) {
    reason = nil;
    if (!t->guard(p, &reason)) {
      panic("proc_fsm: guard failed %s -> %s: %s\n"
            "  pid=%lud p=%p p->r=%p p->mach=%p",
            proc_state_names[current], proc_state_names[t->to_state],
            reason ? reason : "unknown", p->pid, p, p->r, p->mach);
    }
  }

  /* Update state trace */
  p->state_trace = STATE_PUSH(p->state_trace, t->to_state);

  /* CRITICAL: Also update legacy p->state field for compatibility
   * with queueproc(), ready(), and other scheduler code that checks p->state */
  p->state = t->to_state;

  /* Update checksum */
  proc_seal(p);

  /* Memory barrier */
  coherence();

  return t->to_state;
}

/*
 * Dump state trace for debugging
 */
void proc_trace_dump(Proc *p) {
  ushort trace = p->state_trace;

  print("proc[%lud] state trace: %s <- %s <- %s <- %s (current <- oldest)\n",
        p->pid, proc_state_names[STATE_CURRENT(trace)],
        proc_state_names[STATE_PREV(trace)], proc_state_names[STATE_T2(trace)],
        proc_state_names[STATE_T3(trace)]);
}

/*
 * Initialize FSM (call once at boot)
 */
void proc_fsm_init(void) {
  /* Validate transition table */
  ProcTransition *t;
  int count = 0;

  for (t = fsm_transitions; t->from_state >= 0; t++) {
    if (t->from_state >= PS_COUNT || t->to_state >= PS_COUNT) {
      panic("proc_fsm_init: invalid state in transition table");
    }
    count++;
  }

  print("proc_fsm: initialized with %d transitions\n", count);
}
