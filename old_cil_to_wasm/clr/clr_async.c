/*
 * CLR Async/Await State Machine Runtime Implementation
 */

/* Manual Plan 9 Type Definitions */
#define _U_H_
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;

#include "../9front-pc64/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../port/lib.h"

#include "clr_async.h"

static s32int task_id_counter = 0;

/* ========== Task Creation ========== */

clr_task_t *clr_task_create(clr_heap_t *heap) {
  USED(heap);
  clr_task_t *task = xalloc(sizeof(clr_task_t));
  if (!task)
    return nil;

  memset(task, 0, sizeof(clr_task_t));
  task->status = TASK_STATUS_CREATED;
  task->id = ++task_id_counter;

  return task;
}

clr_task_t *clr_task_create_with_result(clr_heap_t *heap, void *result,
                                        clr_type_info_t *type) {
  clr_task_t *task = clr_task_create(heap);
  if (!task)
    return nil;

  task->result = result;
  task->result_type = type;
  task->status = TASK_STATUS_RAN_TO_COMPLETION;

  return task;
}

/* ========== Task Completion ========== */

void clr_task_set_result(clr_task_t *task, void *result) {
  if (!task)
    return;

  lock(&task->lock);
  task->result = result;
  task->status = TASK_STATUS_RAN_TO_COMPLETION;
  unlock(&task->lock);

  clr_task_run_continuations(task);
}

void clr_task_set_exception(clr_task_t *task, clr_object_t *exception) {
  if (!task)
    return;

  lock(&task->lock);
  task->exception = exception;
  task->status = TASK_STATUS_FAULTED;
  unlock(&task->lock);

  clr_task_run_continuations(task);
}

void clr_task_set_canceled(clr_task_t *task) {
  if (!task)
    return;

  lock(&task->lock);
  task->status = TASK_STATUS_CANCELED;
  unlock(&task->lock);

  clr_task_run_continuations(task);
}

int clr_task_is_completed(clr_task_t *task) {
  if (!task)
    return 0;
  return task->status == TASK_STATUS_RAN_TO_COMPLETION ||
         task->status == TASK_STATUS_FAULTED ||
         task->status == TASK_STATUS_CANCELED;
}

/* ========== Continuations ========== */

void clr_task_add_continuation(clr_task_t *task, clr_continuation_t *cont) {
  if (!task || !cont)
    return;

  lock(&task->lock);

  /* If already completed, run immediately */
  if (clr_task_is_completed(task)) {
    unlock(&task->lock);
    if (cont->action) {
      cont->action(cont->state);
    }
    return;
  }

  /* Add to continuation list */
  cont->next = nil;
  cont->prev = task->continuations_tail;

  if (task->continuations_tail) {
    task->continuations_tail->next = cont;
  } else {
    task->continuations_head = cont;
  }
  task->continuations_tail = cont;

  unlock(&task->lock);
}

void clr_task_run_continuations(clr_task_t *task) {
  if (!task)
    return;

  lock(&task->lock);

  clr_continuation_t *cont = task->continuations_head;
  task->continuations_head = nil;
  task->continuations_tail = nil;

  unlock(&task->lock);

  /* Run all continuations */
  while (cont) {
    clr_continuation_t *next = cont->next;
    if (cont->action) {
      cont->action(cont->state);
    }
    cont = next;
  }

  /* Notify awaiter callback */
  if (task->on_completed) {
    task->on_completed(task, task->on_completed_state);
  }
}

/* ========== AsyncTaskMethodBuilder ========== */

void clr_async_builder_start(clr_async_builder_t *builder,
                             clr_state_machine_t *sm) {
  if (!builder || !sm)
    return;

  builder->sm = sm;
  builder->started = 1;

  /* Invoke MoveNext to start the state machine */
  if (sm->move_next) {
    sm->move_next(sm);
  }
}

void clr_async_builder_await_unsafe(clr_async_builder_t *builder,
                                    clr_awaiter_t *awaiter,
                                    clr_state_machine_t *sm) {
  if (!builder || !awaiter || !sm)
    return;

  /* Check if awaiter is already completed */
  if (awaiter->is_completed && awaiter->is_completed(awaiter)) {
    /* No need to suspend, continue inline */
    return;
  }

  /* Register continuation to resume state machine when awaiter completes */
  if (awaiter->on_completed) {
    awaiter->on_completed(awaiter, (void (*)(void *))sm->move_next, sm);
  }
}

void clr_async_builder_set_result(clr_async_builder_t *builder, void *result) {
  if (!builder)
    return;

  /* Create task if needed */
  if (!builder->task) {
    builder->task = clr_task_create(nil);
  }

  if (builder->task) {
    clr_task_set_result(builder->task, result);
  }
}

void clr_async_builder_set_exception(clr_async_builder_t *builder,
                                     clr_object_t *exception) {
  if (!builder)
    return;

  if (!builder->task) {
    builder->task = clr_task_create(nil);
  }

  if (builder->task) {
    clr_task_set_exception(builder->task, exception);
  }
}

clr_task_t *clr_async_builder_get_task(clr_async_builder_t *builder) {
  if (!builder)
    return nil;

  if (!builder->task) {
    builder->task = clr_task_create(nil);
  }

  return builder->task;
}

/* ========== Task Awaiter ========== */

static int task_awaiter_is_completed(clr_awaiter_t *awaiter) {
  if (!awaiter || !awaiter->awaited)
    return 1;
  return clr_task_is_completed((clr_task_t *)awaiter->awaited);
}

static void task_awaiter_get_result(clr_awaiter_t *awaiter, void *result) {
  if (!awaiter || !awaiter->awaited)
    return;

  clr_task_t *task = (clr_task_t *)awaiter->awaited;

  /* If faulted, would throw - for now just copy result */
  if (result && task->result) {
    /* Shallow copy - real implementation would handle types */
    *(void **)result = task->result;
  }
}

static void task_awaiter_on_completed(clr_awaiter_t *awaiter,
                                      void (*cont)(void *), void *state) {
  if (!awaiter || !awaiter->awaited)
    return;

  clr_task_t *task = (clr_task_t *)awaiter->awaited;

  /* Register continuation */
  clr_continuation_t *continuation = xalloc(sizeof(clr_continuation_t));
  if (!continuation)
    return;

  memset(continuation, 0, sizeof(clr_continuation_t));
  continuation->action = cont;
  continuation->state = state;

  clr_task_add_continuation(task, continuation);
}

static clr_awaiter_t task_awaiter_vtable = {
    .is_completed = task_awaiter_is_completed,
    .get_result = task_awaiter_get_result,
    .on_completed = task_awaiter_on_completed,
    .awaited = nil,
};

clr_awaiter_t *clr_task_get_awaiter(clr_task_t *task) {
  if (!task)
    return nil;

  clr_awaiter_t *awaiter = xalloc(sizeof(clr_awaiter_t));
  if (!awaiter)
    return nil;

  *awaiter = task_awaiter_vtable;
  awaiter->awaited = task;

  return awaiter;
}

/* ========== Scheduler ========== */

/* Simple inline scheduler - real implementation would queue to thread pool */
void clr_async_scheduler_post(void (*action)(void *), void *state) {
  if (action) {
    action(state);
  }
}
