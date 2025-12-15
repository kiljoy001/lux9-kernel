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
#define BY2PG 4096

/* Include VM Engine */
#include "CIL-Interpreter/include/execution_engine.h"
#include "CIL-Interpreter/include/il_decoder.h"

#include <borrowchecker.h>
/* Forward decls */

extern Proc *up;

/* Include decoupling headers */
#include "clr_codepage.h"
#include "clr_tasklet.h"

#define USED(x)                                                                \
  if (x) {                                                                     \
  }

extern int print(char *fmt, ...);
extern void *xalloc(ulong size);

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

/*
 * TaskletHeap - 4KB exchange page for execution state
 * Holds execution context, extended locals, and eval stack.
 */
typedef struct TaskletHeap {
  /* Execution Context */
  TaskletExecContext ctx;

  /* Extended storage */

  /* VM Engine State */
  vm_execution_state_t vm_state;
  vm_frame_t vm_frame;

  /* Decoder State */
  cil_decoder_t decoder;

  /* Flat Storage */
  /* 128 stack slots * 16 bytes = 2048 bytes */
  vm_value_t stack_mem[128];
  /* 64 locals * 16 bytes = 1024 bytes */
  vm_value_t locals_mem[64];

  /* Remainder for parsing buffers etc */
} TaskletHeap;

/* ... execute_step implementation ... */

/* ... struct definitions ... */

/* Helper to read integer values from IL stream */
static s32int read_i4(u8int *il, ulong ip) {
  /* TODO: Endianness */
  return *(s32int *)(il + ip);
}

static s8int read_i1(u8int *il, ulong ip) { return *(s8int *)(il + ip); }

ILResult clr_tasklet_execute_step(TaskletSlot *t, int max_ops) {
  int ops = 0;
  TaskletHeap *heap;
  cil_instruction_t inst;

  if (t == nil || t->state != TASKLET_RUNNING || t->overflow_heap == nil)
    return IL_RESULT_ERROR;

  heap = (TaskletHeap *)t->overflow_heap;

  /* Sync decoder to IP */
  heap->decoder.offset = t->ip;

  for (ops = 0; ops < max_ops; ops++) {
    /* Decode Next */
    if (!has_more_instructions(&heap->decoder)) {
      t->state = TASKLET_DONE;
      if (heap->ctx.code_handle)
        clr_codepage_release_read(heap->ctx.code_handle, nil);
      return IL_RESULT_DONE;
    }

    if (!decode_instruction_to_buffer(&heap->decoder, &inst)) {
      print("CLR: Decode error at %d\n", t->ip);
      return IL_RESULT_ERROR;
    }

    /* Handle Flow Control manually (Raw Offsets) */
    if (inst.opcode == CIL_OPCODE_BR || inst.opcode == CIL_OPCODE_BR_S) {
      s32int offset =
          (inst.opcode == CIL_OPCODE_BR)
              ? inst.operand.branch_offset
              : /* wait, decode puts int in int_val? Checking header */
              inst.operand.branch_offset_short;

      /* Decoder already advanced offset past this instr */
      /* Branch target is relative to NEXT instruction */
      /* ECMA-335: offsets are from the start of the instruction following the
       * current one. */
      /* decoder.offset IS pointing to next instr now. */
      t->ip = heap->decoder.offset + offset;
      heap->decoder.offset = t->ip; /* Jump */
      continue;
    }

    /* Handle ALU/Stack via Engine */
    heap->vm_frame.ip = &inst;
    if (!vm_execute_instruction(&heap->vm_state)) {
      print("CLR: VM Error: %s\n", heap->vm_state.error_message
                                       ? heap->vm_state.error_message
                                       : "Unknown");
      return IL_RESULT_ERROR;
    }

    /* Advance IP */
    t->ip = heap->decoder.offset;
  }
  /* Stub for execution engine internal calls */

  return IL_RESULT_YIELD;
}

/*
 * clr_tasklet_bind_method - Bind a method to a tasklet for execution
 *
 * Sets up the execution context from an assembly and method token.
 * Allocates a full 4KB overflow heap page for execution state.
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

  /* Allocate full 4KB page for heap */
  if (t->overflow_heap == nil) {
    t->overflow_heap = xalloc(BY2PG);
    if (t->overflow_heap == nil)
      return -1;
    memset(t->overflow_heap, 0, BY2PG);
  }

  TaskletHeap *heap = (TaskletHeap *)t->overflow_heap;
  TaskletExecContext *ctx = &heap->ctx;

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

  /* Reset tasklet state */
  t->ip = 0;
  t->stack_top = 0;
  t->locals_count = 0;

  /* Initialize locals */
  if (local_count > 0 && local_count <= TASKLET_INLINE_LOCALS) {
    memset(t->locals, 0, local_count * sizeof(ulong));
    t->locals_count = local_count;
  }

  print("CLR: bound method 0x%ux from CodePage to tasklet %ud (heap=4KB)\n",
        method_token, t->id);
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
