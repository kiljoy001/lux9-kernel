/*
 * CLR Async/Await State Machine Runtime
 *
 * Implements the runtime infrastructure for async/await patterns.
 * The C#/F# compiler generates state machines that call these primitives.
 */

#ifndef CLR_ASYNC_H
#define CLR_ASYNC_H

#include "../include/u.h"
#include "clr-kernel/clr_pebble_integration.h"

/* ========== Async State Machine States ========== */

typedef enum {
  ASYNC_STATE_CREATED = -1,   /* Initial state before first MoveNext */
  ASYNC_STATE_RUNNING = 0,    /* Currently executing */
  ASYNC_STATE_AWAITING = 1,   /* Waiting on an awaiter */
  ASYNC_STATE_COMPLETED = -2, /* Successfully completed */
  ASYNC_STATE_FAULTED = -3,   /* Completed with exception */
} clr_async_state_t;

/* ========== Task Status (matches System.Threading.Tasks.TaskStatus) ==========
 */

typedef enum {
  TASK_STATUS_CREATED = 0,
  TASK_STATUS_WAITING_FOR_ACTIVATION = 1,
  TASK_STATUS_WAITING_TO_RUN = 2,
  TASK_STATUS_RUNNING = 3,
  TASK_STATUS_WAITING_FOR_CHILDREN = 4,
  TASK_STATUS_RAN_TO_COMPLETION = 5,
  TASK_STATUS_CANCELED = 6,
  TASK_STATUS_FAULTED = 7,
} clr_task_status_t;

/* ========== Task Object ========== */

typedef struct clr_task {
  clr_object_t base; /* Inherits from CLR object */

  clr_task_status_t status; /* Current task status */
  s32int id;                /* Unique task ID */

  /* Result storage */
  void *result;                 /* Task<T> result value */
  clr_type_info_t *result_type; /* Result type info */

  /* Exception (if faulted) */
  clr_object_t *exception; /* AggregateException if faulted */

  /* Continuation chain */
  struct clr_continuation *continuations_head;
  struct clr_continuation *continuations_tail;

  /* Parent/children for structured concurrency */
  struct clr_task *parent;
  struct clr_task *children_head;
  struct clr_task *children_tail;
  struct clr_task *next_sibling;

  /* Synchronization */
  Lock lock;

  /* Awaiter notification */
  void (*on_completed)(struct clr_task *task, void *state);
  void *on_completed_state;

} clr_task_t;

/* ========== Continuation ========== */

typedef struct clr_continuation {
  clr_task_t *task;            /* Continuation task */
  void (*action)(void *state); /* Continuation action */
  void *state;                 /* Captured state */

  struct clr_continuation *next;
  struct clr_continuation *prev;
} clr_continuation_t;

/* ========== State Machine Interface ========== */

/*
 * IAsyncStateMachine interface.
 * Compiler-generated state machines implement this.
 */
typedef struct clr_state_machine {
  clr_object_t base; /* Inherits from CLR object */

  s32int state; /* Current state (-1 = not started) */

  /* AsyncTaskMethodBuilder fields */
  clr_task_t *task; /* The task being built */

  /* MoveNext function pointer (compiler-generated) */
  void (*move_next)(struct clr_state_machine *sm);

  /* SetStateMachine function pointer */
  void (*set_state_machine)(struct clr_state_machine *sm,
                            struct clr_state_machine *boxed);

  /* Captured locals (variable-sized, follows this struct) */
  /* void captured_locals[]; */
} clr_state_machine_t;

/* ========== AsyncTaskMethodBuilder ========== */

typedef struct clr_async_builder {
  clr_task_t *task;        /* Task being constructed */
  clr_state_machine_t *sm; /* State machine reference */
  int started;             /* Has Start been called? */
} clr_async_builder_t;

/* ========== Awaiter Interface ========== */

typedef struct clr_awaiter {
  /* IsCompleted property */
  int (*is_completed)(struct clr_awaiter *awaiter);

  /* GetResult method */
  void (*get_result)(struct clr_awaiter *awaiter, void *result);

  /* OnCompleted method */
  void (*on_completed)(struct clr_awaiter *awaiter, void (*cont)(void *),
                       void *state);

  /* Awaited object (usually a Task) */
  void *awaited;
} clr_awaiter_t;

/* ========== API Functions ========== */

/* Task creation and management */
clr_task_t *clr_task_create(clr_heap_t *heap);
clr_task_t *clr_task_create_with_result(clr_heap_t *heap, void *result,
                                        clr_type_info_t *type);
void clr_task_set_result(clr_task_t *task, void *result);
void clr_task_set_exception(clr_task_t *task, clr_object_t *exception);
void clr_task_set_canceled(clr_task_t *task);
int clr_task_is_completed(clr_task_t *task);

/* Continuation management */
void clr_task_add_continuation(clr_task_t *task, clr_continuation_t *cont);
void clr_task_run_continuations(clr_task_t *task);

/* AsyncTaskMethodBuilder operations */
void clr_async_builder_start(clr_async_builder_t *builder,
                             clr_state_machine_t *sm);
void clr_async_builder_await_unsafe(clr_async_builder_t *builder,
                                    clr_awaiter_t *awaiter,
                                    clr_state_machine_t *sm);
void clr_async_builder_set_result(clr_async_builder_t *builder, void *result);
void clr_async_builder_set_exception(clr_async_builder_t *builder,
                                     clr_object_t *exception);
clr_task_t *clr_async_builder_get_task(clr_async_builder_t *builder);

/* Awaiter operations */
clr_awaiter_t *clr_task_get_awaiter(clr_task_t *task);

/* Scheduler integration (runs continuations) */
void clr_async_scheduler_post(void (*action)(void *), void *state);

#endif /* CLR_ASYNC_H */
