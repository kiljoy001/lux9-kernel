/*
 * Userspace Test Harness for Lux9 Process State DAG
 * Designed for Valgrind / Helgrind testing.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* --- Mock Kernel Types & Macros --- */

typedef uint32_t u32int;
typedef uint8_t uchar;
typedef unsigned long ulong;
typedef long long vlong;
typedef void *PreconditionFunc;

#define nil NULL
#define USED(x) (void)(x)

/* Mock Lock using pthread mutex */
typedef struct Lock {
  pthread_mutex_t m;
} Lock;

typedef struct Mach {
  int machno;
} Mach;

typedef struct Rendez {
  Lock lock;
  struct Proc *p;
} Rendez;

typedef struct Proc {
  ulong pid;
  int state;
  Rendez *r;
  Mach *mach;
  Lock rlock;
} Proc;

/* Mock Kernel Functions */
void panic(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "KERNEL PANIC: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  abort();
}

void print(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void coherence(void) { __sync_synchronize(); }

/* Mock Headers for proc_state_dag.c inclusions */
/* We define these empty or minimal to satisfy the includes in proc_state_dag.c
 */

/* We need to trick the includes. Since we are in userspace, we can't fully
 * include kernel headers. We will define the macros/types needed and then
 * #define the header names to nothing so the #includes in proc_state_dag.c are
 * ignored (if we filter them) or we just rely on standard include paths not
 * finding them, but that errors.
 *
 * Better strategy: Create minimal mock headers in this directory.
 */

/* However, writing files is safer. For now, let's include the DAG code directly
   but we'll need to strip the kernel includes or mock them.

   The kernel `proc_state_dag.c` includes:
   #include <u.h>
   #include "portlib.h"
   #include "mem.h"
   #include "dat.h"
   #include "fns.h"
   #include "proc_state_dag.h"
*/

/* --- Forward Declarations for DAG --- */
// Copied from proc_state_dag.h - we need valid enums
enum {
  dag_ProcDead = 0,
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
  dag_ProcMoribund,
  dag_PROC_STATE_COUNT
};

typedef struct {
  int allowed;
  const char *reason;
} ProcStateCheck;

void procstate_dag_init(void);
void procstate_allow_edge(int from, int to);
void procstate_add_precondition(int from, int to,
                                int (*check)(Proc *, const char **));
ProcStateCheck procstate_validate_transition(Proc *p, int from, int to);
void procstate_transition(Proc *p, int from, int to);

// Mock statenmaes for panic
char *statename[dag_PROC_STATE_COUNT] = {
    "Dead",     "New",        "Ready",       "Scheding", "Running",
    "Queueing", "QueueingR",  "QueueingW",   "Wakeme",   "Broken",
    "Stopped",  "Rendezvous", "Waitrelease", "Moribund"};

/* --- The DAG Implementation --- */
/* We will #include the .c file BUT we need to handle the kernel headers first.
   We will treat the .c file as a header for this test file,
   but we need to satisfy its dependencies.
*/

// Mock headers
// We can use the preprocessor to ignore the specific includes if we define
// them? No, #include "file" is processed before macros unless macros change
// "file" (which they can't standardly).

// Best approach: Create dummy empty headers.
// Or copy-paste the logic of proc_state_dag.c here (ignoring the includes).
// Copy-pasting ensures we test the LOGIC.
// BUT we want to test the ACTUAL FILE.

// Let's create dummy headers.

#include "proc_state_dag_impl_mock.c"

/* Test Logic */

Proc test_proc;
Rendez test_rendez;
Mach test_mach;

void *sleeper_thread(void *arg) {
  printf("[Sleeper] Starting...\n");
  // Simulate going to sleep: Running -> Wakeme

  // 1. Lock process
  pthread_mutex_lock(&test_proc.rlock.m);

  // 2. Set checking
  test_proc.r = &test_rendez;
  test_proc.mach = &test_mach; // Still running on mach

  // Transition Running -> Wakeme
  // Note: In kernel, this happens *after* scheduler switch often, but
  // logically:
  printf("[Sleeper] Transitioning Running -> Wakeme\n");
  procstate_transition(&test_proc, dag_ProcRunning, dag_ProcWakeme);

  // Now we are "asleep" (logically waiting for switch).
  pthread_mutex_unlock(&test_proc.rlock.m);

  printf("[Sleeper] Went to sleep. Waiting...\n");
  usleep(100000); // 100ms wait

  return NULL;
}

void *waker_thread(void *arg) {
  usleep(10000); // Wait bit for sleeper to start
  printf("[Waker] Starting...\n");

  // Simulate wakeup
  Lock *rlock = &test_proc.rlock;

  // 1. Lock process
  pthread_mutex_lock(&rlock->m);

  // 2. Check state (kernel checks p->state == Wakeme)
  if (test_proc.state != dag_ProcWakeme) {
    printf("[Waker] Process not asleep yet (%d)\n", test_proc.state);
    pthread_mutex_unlock(&rlock->m);
    return NULL;
  }

  // 3. DO THE FIX: Clear pointers
  printf("[Waker] Clearing pointers...\n");
  test_proc.r = nil;

  // 4. Transition Wakeme -> Ready
  printf("[Waker] Transitioning Wakeme -> Ready\n");
  procstate_transition(&test_proc, dag_ProcWakeme, dag_ProcReady);

  pthread_mutex_unlock(&rlock->m);
  printf("[Waker] Done.\n");
  return NULL;
}

int main() {
  printf("Initializing Userspace DAG Test...\n");

  pthread_mutex_init(&test_proc.rlock.m, NULL);
  test_proc.pid = 123;
  test_proc.state = dag_ProcRunning; // Initial state
  test_proc.mach = &test_mach;
  test_proc.r = nil;

  // Run tests
  procstate_dag_init();

  pthread_t t1, t2;
  pthread_create(&t1, NULL, sleeper_thread, NULL);
  pthread_create(&t2, NULL, waker_thread, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  printf("Test Complete. Final State: %s\n", statename[test_proc.state]);

  assert(test_proc.state == dag_ProcReady);
  assert(test_proc.r == nil);

  return 0;
}
