/*
 * Process State Transition DAG Governor
 *
 * This module enforces valid process state transitions and checks
 * preconditions. The DAG is allocated on the heap and consulted by processes on
 * the stack before any state transition occurs.
 */

#pragma once

/* Forward check for Proc validation */
struct Proc;

/* Process states - must match statename[] in proc.c */
enum {
  dag_ProcDead = 0,
  dag_ProcMoribund,
  dag_ProcNew,
  dag_ProcReady,
  dag_ProcScheding,
  dag_ProcRunning,
  dag_ProcQueueing,
  dag_ProcQueueingR,
  dag_ProcQueueingW,
  dag_ProcWakeme,
  dag_ProcBroken,
  dag_ProcStopped,
  dag_ProcRendezvous,
  dag_ProcWaitrelease,
  dag_PROC_STATE_COUNT
};

/* Precondition check result */
typedef struct ProcStateCheck ProcStateCheck;
struct ProcStateCheck {
  int allowed;        /* 0 = denied, 1 = allowed */
  const char *reason; /* Reason for denial (if allowed == 0) */
};

/* Initialize the process state DAG governor (call once at boot) */
void procstate_dag_init(void);

/* Validate a state transition and check preconditions */
ProcStateCheck procstate_validate_transition(struct Proc *p, int from_state,
                                             int to_state);

/* Perform a validated state transition (panics if invalid) */
void procstate_transition(struct Proc *p, int from_state, int to_state);

/* Register a valid state transition edge in the DAG */
void procstate_allow_edge(int from_state, int to_state);

/* Register a precondition check for a specific transition */
typedef int (*PreconditionFunc)(struct Proc *p, const char **reason);
void procstate_add_precondition(int from_state, int to_state,
                                PreconditionFunc check);
