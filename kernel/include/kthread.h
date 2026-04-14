#ifndef _KTHREAD_H_
#define _KTHREAD_H_

#include "dat.h"
#include "scheduler_logic.h"

/*
 * KThread - Minimal Kernel Execution Unit for "Thin-Lux9 Ultra".
 * This structure is the target for seL4-style refinement proofs.
 * High-level process logic is moved to userspace /srv/pm.
 */

typedef struct KThread KThread;

struct KThread {
  Label sched;    /* Saved context (sp, pc, etc.) - Must be first */
  int state;      /* Execution state (Ready, Running, etc.) */
  Mach *mach;     /* CPU running this thread */
  KThread *rnext; /* Next thread in run queue */

  int priority; /* Scheduling priority */
  int affinity; /* CPU affinity (-1 for any) */

  uchar *kstack; /* Pointer to base of kernel stack */
  int nlocks;    /* Number of spinlocks held (scheduler safety) */

  ulong tid; /* Thread ID */
};

/*@ axiomatic KThreadModel {
  @   logic ProcState KThread_model_state{L}(KThread *p) ;
  @ } */

/* State constants (subset of Proc states) */
enum {
  KDead = 0,
  KSeding,
  KReady,
  KRunning,
  KWakeme,
  KStopped,
};

#endif /* _KTHREAD_H_ */
