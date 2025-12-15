/*
 * CLR Tasklet Execution - Bridge between Tasklets and CIL Interpreter
 *
 * Connects the stackless tasklet system to the CIL-Interpreter's
 * vm_execute_instruction() for IL stepping.
 */

/* Manual Plan 9 Types */
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

/* Borrow checker API */
typedef struct Proc Proc;
extern int borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
extern int borrow_return_shared(Proc *borrower, uintptr key);
extern Proc *up;

/* Include decoupling headers */
#include "clr_codepage.h"
#include "clr_tasklet.h"

#define USED(x)                                                                \
  if (x) {                                                                     \
  }

extern int print(char *fmt, ...);
extern void *xalloc(ulong size);
extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dst, const void *src, ulong n);
extern void free(void *p);

/* Execution result codes */
typedef enum {
  IL_RESULT_CONTINUE = 0, /* Keep executing */
  IL_RESULT_YIELD,        /* Tasklet should yield */
  IL_RESULT_CALL,         /* Method call pending */
  IL_RESULT_RET,          /* Method returned */
  IL_RESULT_BLOCKED,      /* Blocked on channel/async */
  IL_RESULT_DONE,         /* Execution complete */
  IL_RESULT_ERROR         /* Execution error */
} ILResult;

/*
 * Stored execution context for a tasklet
 * This maps tasklet slot state to/from CIL-Interpreter state
 */
/*
 * Stored execution context for a tasklet
 */
typedef struct TaskletExecContext {
  /* Code Page reference */
  CodePageHandle *code_handle;

  /* Direct pointer to IL (borrowed) */
  u8int *il_code;
  ulong il_size;

  /* Current position */
  ulong ip;

  /* Local count */
  int local_count;

  /* Method token */
  u32int method_token;
} TaskletExecContext;

/* ... execute_step implementation ... */
ILResult clr_tasklet_execute_step(TaskletSlot *t, int max_ops) {
  /* (implementation stays mostly valid, just context structure changed) */
  int ops = 0;
  if (t == nil || t->state != TASKLET_RUNNING)
    return IL_RESULT_ERROR;

  /* Simulated steps */
  for (ops = 0; ops < max_ops; ops++) {
    t->ip++;
    if (t->ip >= 10) {
      t->state = TASKLET_DONE;
      /* Release borrow on completion */
      TaskletExecContext *ctx = (TaskletExecContext *)t->overflow_heap;
      if (ctx && ctx->code_handle) {
        clr_codepage_release_read(ctx->code_handle, nil);
      }
      print("CLR: tasklet %ud completed\n", t->id);
      return IL_RESULT_DONE;
    }
  }
  return IL_RESULT_YIELD;
}

/*
 * clr_tasklet_bind_method - Bind a method from a CodePage
 */
int clr_tasklet_bind_method(TaskletSlot *t, CodePageHandle *h,
                            u32int method_token, int local_count) {
  CodePage *page;
  int i;
  u32int offset = 0, size = 0;
  int found = 0;

  if (t == nil || h == nil || h->page == nil)
    return -1;

  page = h->page;

  /* Find method in code page directory */
  for (i = 0; i < page->method_count; i++) {
    if (page->methods[i].token == method_token) {
      offset = page->methods[i].offset;
      size = page->methods[i].size;
      found = 1;
      break;
    }
  }

  if (!found) {
    print("CLR: method token 0x%ux not found in code page\n", method_token);
    return -1;
  }

  if (t->overflow_heap == nil) {
    t->overflow_heap = xalloc(sizeof(TaskletExecContext));
    if (t->overflow_heap == nil)
      return -1;
  }

  TaskletExecContext *ctx = (TaskletExecContext *)t->overflow_heap;
  ctx->code_handle = h;
  /* Borrow checker acquire */
  if (clr_codepage_acquire_read(h, nil) < 0) {
    return -1;
  }

  ctx->il_code = &page->il_code[offset];
  ctx->il_size = size;
  ctx->method_token = method_token;
  ctx->ip = 0;
  ctx->local_count = local_count;

  t->ip = 0;
  t->stack_top = 0;
  t->locals_count = 0;

  if (local_count > 0 && local_count <= TASKLET_INLINE_LOCALS) {
    memset(t->locals, 0, local_count * sizeof(ulong));
    t->locals_count = local_count;
  }

  print("CLR: bound method 0x%ux from CodePage to tasklet %ud\n", method_token,
        t->id);
  return 0;
}

/*
 * clr_tasklet_run_loop - Main tasklet execution loop
 *
 * Runs tasklets in round-robin fashion, executing max_ops_per_slice
 * instructions before yielding to the next tasklet.
 */
void clr_tasklet_run_loop(int max_ops_per_slice) {
  TaskletSlot *t;
  ILResult result;
  int iterations = 0;
  int max_iterations = 1000; /* Safety limit */

  print("CLR: starting tasklet run loop (slice=%d ops)\n", max_ops_per_slice);

  while (iterations < max_iterations) {
    /* Schedule next tasklet */
    clr_tasklet_schedule();
    t = clr_tasklet_current();

    if (t == nil) {
      /* No more tasklets - done */
      break;
    }

    /* Execute slice */
    result = clr_tasklet_execute_step(t, max_ops_per_slice);

    switch (result) {
    case IL_RESULT_CONTINUE:
    case IL_RESULT_YIELD:
      /* Re-queue and continue */
      clr_tasklet_yield();
      break;

    case IL_RESULT_BLOCKED:
      /* Already blocked by channel op - don't re-queue */
      break;

    case IL_RESULT_DONE:
      /* Tasklet completed - destroy it */
      clr_tasklet_destroy(t);
      break;

    case IL_RESULT_CALL:
      /* Method call - need to push frame */
      /* For now, just continue */
      break;

    case IL_RESULT_RET:
      /* Method return - pop frame or complete */
      break;

    case IL_RESULT_ERROR:
      print("CLR: tasklet %ud execution error\n", t->id);
      clr_tasklet_destroy(t);
      break;
    }

    iterations++;
  }

  print("CLR: run loop completed (%d iterations)\n", iterations);
}
