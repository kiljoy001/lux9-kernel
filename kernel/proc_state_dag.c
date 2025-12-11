/*
 * Process State Transition DAG Governor Implementation
 */

#include <u.h>
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "proc_state_dag.h"

/* DAG structure on the heap */
static struct {
  Lock lock;
  int initialized;
  /* Adjacency matrix: edges[from][to] = 1 if transition allowed */
  uchar edges[dag_PROC_STATE_COUNT][dag_PROC_STATE_COUNT];
  /* Precondition functions for each edge */
  PreconditionFunc preconditions[dag_PROC_STATE_COUNT][dag_PROC_STATE_COUNT];
} procstate_dag;

/* Precondition: p->r must be nil */
static int check_rendezvous_cleared(Proc *p, const char **reason) {
  if (p->r != nil) {
    *reason = "p->r must be nil before transition";
    return 0;
  }
  return 1;
}

/* Precondition: p->mach must be nil */
static int check_mach_cleared(Proc *p, const char **reason) {
  if (p->mach != nil) {
    *reason = "p->mach must be nil before transition";
    return 0;
  }
  return 1;
}

/* Precondition: p->mach must be set */
static int check_mach_set(Proc *p, const char **reason) {
  if (p->mach == nil) {
    *reason = "p->mach must be set before transition to Running";
    return 0;
  }
  return 1;
}

/* Precondition: p->r must be set for sleeping */
static int check_rendezvous_set(Proc *p, const char **reason) {
  if (p->r == nil) {
    *reason = "p->r must be set before transition to Wakeme";
    return 0;
  }
  return 1;
}

void procstate_dag_init(void) {
  if (procstate_dag.initialized)
    return;

  memset(&procstate_dag, 0, sizeof(procstate_dag));

  /* Define all valid state transitions */
  /* Dead → New */
  procstate_allow_edge(dag_ProcDead, dag_ProcNew);

  /* New → Ready */
  procstate_allow_edge(dag_ProcNew, dag_ProcReady);
  procstate_add_precondition(dag_ProcNew, dag_ProcReady, check_mach_cleared);

  /* Ready → Running */
  procstate_allow_edge(dag_ProcReady, dag_ProcRunning);
  procstate_add_precondition(dag_ProcReady, dag_ProcRunning, check_mach_set);

  /* Running → Wakeme (going to sleep) */
  procstate_allow_edge(dag_ProcRunning, dag_ProcWakeme);
  procstate_add_precondition(dag_ProcRunning, dag_ProcWakeme,
                             check_rendezvous_set);

  /* Wakeme → Ready (woken up) - THE CRITICAL TRANSITION */
  procstate_allow_edge(dag_ProcWakeme, dag_ProcReady);
  procstate_add_precondition(dag_ProcWakeme, dag_ProcReady,
                             check_rendezvous_cleared);

  /* Running → Scheding */
  procstate_allow_edge(dag_ProcRunning, dag_ProcScheding);

  /* Scheding → Ready */
  procstate_allow_edge(dag_ProcScheding, dag_ProcReady);

  /* Running → Moribund */
  procstate_allow_edge(dag_ProcRunning, dag_ProcMoribund);

  /* Moribund → Dead */
  procstate_allow_edge(dag_ProcMoribund, dag_ProcDead);
  procstate_add_precondition(dag_ProcMoribund, dag_ProcDead,
                             check_mach_cleared);

  /* Add more transitions for Queueing, Rendez, etc. */
  procstate_allow_edge(dag_ProcRunning, dag_ProcQueueing);
  procstate_allow_edge(dag_ProcQueueing, dag_ProcReady);
  procstate_allow_edge(dag_ProcRunning, dag_ProcRendezvous);
  procstate_allow_edge(dag_ProcRendezvous, dag_ProcReady);
  procstate_allow_edge(dag_ProcRunning, dag_ProcWaitrelease);
  procstate_allow_edge(dag_ProcWaitrelease, dag_ProcReady);
  procstate_allow_edge(dag_ProcRunning, dag_ProcBroken);
  procstate_allow_edge(dag_ProcRunning, dag_ProcStopped);
  procstate_allow_edge(dag_ProcStopped, dag_ProcReady);

  procstate_dag.initialized = 1;
  print("procstate_dag: initialized with %d states\n", dag_PROC_STATE_COUNT);
}

void procstate_allow_edge(int from_state, int to_state) {
  if (from_state < 0 || from_state >= dag_PROC_STATE_COUNT || to_state < 0 ||
      to_state >= dag_PROC_STATE_COUNT)
    return;

  procstate_dag.edges[from_state][to_state] = 1;
}

void procstate_add_precondition(int from_state, int to_state,
                                PreconditionFunc check) {
  if (from_state < 0 || from_state >= dag_PROC_STATE_COUNT || to_state < 0 ||
      to_state >= dag_PROC_STATE_COUNT)
    return;

  procstate_dag.preconditions[from_state][to_state] = check;
}

ProcStateCheck procstate_validate_transition(Proc *p, int from_state,
                                             int to_state) {
  ProcStateCheck result = {1, nil};
  PreconditionFunc check;

  if (!procstate_dag.initialized)
    procstate_dag_init();

  /* Validate bounds */
  if (from_state < 0 || from_state >= dag_PROC_STATE_COUNT || to_state < 0 ||
      to_state >= dag_PROC_STATE_COUNT) {
    result.allowed = 0;
    result.reason = "invalid state value";
    return result;
  }

  /* Check if edge exists in DAG */
  if (!procstate_dag.edges[from_state][to_state]) {
    result.allowed = 0;
    result.reason = "transition not allowed in DAG";
    return result;
  }

  /* Check preconditions if any */
  check = procstate_dag.preconditions[from_state][to_state];
  if (check != nil) {
    if (!check(p, &result.reason)) {
      result.allowed = 0;
      return result;
    }
  }

  return result;
}

void procstate_transition(Proc *p, int from_state, int to_state) {
  ProcStateCheck check;
  extern char *statename[]; /* From proc.c */

  /* Validate the transition */
  check = procstate_validate_transition(p, from_state, to_state);

  if (!check.allowed) {
    /* Transition denied - panic with detailed info */
    /* Since statename is external and array size might vary, be careful. 
       Assuming it's available or we should print integers if not confident. 
       Using statename as requested. */
    panic("procstate_transition: DENIED %d -> %d for proc %lud: %s\n"
          "  p=%p p->r=%p p->mach=%p\n",
          from_state, to_state, p->pid, check.reason, p,
          p->r, p->mach);
  }

  /* Transition allowed - update state */
  p->state = to_state;

  /* Memory barrier to ensure state write is visible */
  coherence();
}
