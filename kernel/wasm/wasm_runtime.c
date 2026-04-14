/* wasm_runtime.c - Isolated WASM3 Runtime (Layer 1)
 *
 * Provides capability-restricted wasm3 runtime in isolated kernel segment.
 * Accessible ONLY via Tsyscall(SYS_WASM_EXECUTE) - no direct kernel access.
 *
 * SECURITY MODEL:
 * - Runs in isolated segment (cannot access kernel heap)
 * - Memory allocated via exchange pages
 * - Capability checks at Tsyscall boundary
 * - No direct hardware access
 */

/* Kernel headers first - must come before wasm_runtime.h which uses Proc */
#include "../include/dat.h"
#include "../include/error.h"
#include "../include/fcall.h"
#include "../include/fns.h"
#include "../include/mem.h"
#include "../include/pebble.h"

#include "../include/portlib.h"
#include "../include/u.h"

/* WASM runtime headers - after kernel types are defined */
#include "../include/wasm_arena.h"
#include "wasm_capability_bindings.h"
#include "wasm_host_fruity.h"
#include "wasm_host_lux9.h"
#include "wasm_runtime.h"

/* WASM3 headers */
#include "wasi_lux9_shim.h"
#include "wasm_runtime/wasm3/wasm3.h"

/* Global isolated runtime (initialized at boot) */
typedef struct WasmRuntime {
  Lock lock; /* Runtime-wide lock */

  /* Statistics */
  struct {
    u64int total_calls;
    u64int total_modules;
    u64int active_instances;
    u64int errors;
  } stats;
} WasmRuntime;

#define WASM_MAX_MODULE_BYTES (1024 * 1024 * 1024)
#define WASM_MAX_LINEAR_BYTES (4096ULL * 1024 * 1024 - 64 * 1024)
#define WASM_MAX_FUNC_NAME 255
#define WASM_MAX_ARGS 32
#define WASM_ARG_BYTES 8
#define WASM_LINEAR_GUARD (4 * BY2PG)
#define WASM_LINEAR_SLOTS 1024
#define WASM_LINEAR_SLOT_BYTES (8ULL * 1024 * 1024 * 1024)
#define WASM_HEAP_BYTES (2 * 1024 * 1024)
#define WASM_HEAP_GUARD (2 * BY2PG)
#define WASM_HEAP_ALIGN 16UL

static WasmRuntime wasm_runtime;
static int runtime_initialized = 0;
static uchar wasm_compile_reply[8];
static uchar wasm_execute_reply[8];

/* Safe channel cleanup wrapper to prevent cclose panics */
/*@
  @ requires c == \null || \valid(c);
  @ assigns \nothing;
  @*/
static int wasm_safe_channel_close(Chan **c) {
  if (c && *c) {
    /* Check if channel is already being closed */
    if ((*c)->ref <= 0) {
      print("wasm_safe_channel_close: warning - channel %p already freed\n",
            *c);
      *c = nil;
      return 0;
    }
    cclose(*c);
    *c = nil;
    return 1;
  }
  return 0;
}

static int wasm_heap_init(Proc *p);
static int wasm_branch_init(Proc *p);
static int wasm_reserve_linear_slot(Proc *p);
static void wasm_release_linear_slot(Proc *p);
static void wasm_unmap_linear_memory(Proc *p);
void *wasm_heap_alloc(size_t size);
void wasm_heap_free(void *ptr);
int wasm_heap_owned_ptr(void *ptr);
void *wasm_linear_realloc(void *ptr, size_t new_size, size_t old_size);
void wasm_linear_free(void *ptr);
extern void *memcpy(void *dst, const void *src, size_t n);

/*@
  @ requires r == \null || \valid(r);
  @ assigns *r;
  @*/
static void wasm_reply_error(Fcall *r, const char *fmt, ...) {
  va_list arg;

  if (r == nil)
    return;

  r->type = Rerror;
  if (up != nil && fmt != nil) {
    va_start(arg, fmt);
    vsnprint(up->errstr, ERRMAX, fmt, arg);
    va_end(arg);
    r->ename = up->errstr;
  } else {
    r->ename = "wasm error";
  }
}

/*
 * wasm_exec_compile: Compile WASM module for exec() path
 * Called from sysexec() when a WASM binary is detected.
 * Returns: 0 on success, -1 on error
 */
/*@
  @ requires tc == \null || \valid(tc);
  @ requires out_start == \null || \valid(out_start);
  @ assigns \nothing;
  @*/
int wasm_exec_compile(Chan *tc, IM3Function *out_start) {
  int branch_inited = 0;
  int devindex;
  u8int *module_bytes = nil;
  u32int module_size = 0;
  vlong file_size;
  long wasmerr;
  IM3Module module;
  M3Result result;
  u32int mem_size = 0;
  IM3Function start_func;

  if (!runtime_initialized) {
    print("wasm_exec_compile: runtime not initialized\n");
    return -1;
  }

  devindex = devno(tc->type, 0);
  if (devindex < 0 || devtab[devindex] == nil || devtab[devindex]->read == nil) {
    print("wasm_exec_compile: invalid device for chan type=%d\n", tc->type);
    return -1;
  }

  /* Get file size by seeking to end */
  file_size = devtab[devindex]->read(tc, nil, 0, 0);
  print("wasm_exec_compile: determining file size\n");
  if (file_size <= 0) {
    file_size = 64 * 1024;
  }

  /* If running as proc0/init (pid <= 2), cap size to fit in initial budget */
  if (up->pid <= 2 && file_size > 512 * 1024) {
    file_size = 512 * 1024;
  }
  print("wasm_exec_compile: size=%lld\n", file_size);

  if (file_size > WASM_MAX_MODULE_BYTES) {
    print("wasm_exec_compile: module too large (%lld bytes)\n", file_size);
    return -1;
  }

  module_bytes = malloc(file_size);
  if (module_bytes == nil) {
    print("wasm_exec_compile: failed to allocate module buffer\n");
    return -1;
  }

  /* Read file content */
  wasmerr = devtab[devindex]->read(tc, module_bytes, file_size, 0);
  if (wasmerr < 0) {
    print("wasm_exec_compile: failed to read file\n");
    free(module_bytes);
    return -1;
  }
  module_size = wasmerr;

  if (wasm_branch_init(up) < 0) {
    free(module_bytes);
    return -1;
  }
  branch_inited = 1;

  if (wasm_heap_init(up) < 0) {
    arena_branch_drain(&up->wasm.branch);
    memset(&up->wasm.branch, 0, sizeof(up->wasm.branch));
    free(module_bytes);
    return -1;
  }

  up->wasm.module_bytes = module_bytes;
  up->wasm.module_bytes_len = module_size;

  up->wasm.env = m3_NewEnvironment();
  up->wasm.runtime =
      m3_NewRuntime((IM3Environment)up->wasm.env, 256 * 1024, nil);
  if (up->wasm.runtime == nil) {
    print("wasm_exec_compile: failed to create runtime\n");
    goto fail_compile;
  }

  /* Parse module */
  result = m3_ParseModule((IM3Environment)up->wasm.env, &module, module_bytes,
                          module_size);
  if (result) {
    print("wasm_exec_compile: parse failed: %s\n", result);
    goto fail_compile;
  }
  up->wasm.module = (void *)module;

  /* Load module */
  result = m3_LoadModule((IM3Runtime)up->wasm.runtime, module);
  if (result) {
    print("wasm_exec_compile: load failed: %s\n", result);
    goto fail_compile;
  }

  /* Get linear memory pointer after loading module */
  up->wasm.linear_memory =
      m3_GetMemory((IM3Runtime)up->wasm.runtime, &mem_size, 0);
  up->wasm.memory_size = mem_size;
  if (up->wasm.linear_memory) {
    print("wasm_exec_compile: linear memory at %p, size=%ud bytes\n",
          up->wasm.linear_memory, mem_size);
  } else {
    print("wasm_exec_compile: no linear memory (size=%ud)\n", mem_size);
  }

  /* Link WASM binary with Lux9 kernel APIs */
  /* Initialize WASI context with validation */
  up->wasm.wasi_ctx = wasm_heap_alloc(sizeof(wasi_context_t));
  if (up->wasm.wasi_ctx) {
    print("wasm_exec_compile: initializing WASI context at %p\n",
          up->wasm.wasi_ctx);
    wasi_lux9_init_context((wasi_context_t *)up->wasm.wasi_ctx, up);

    /* Validate WASI context initialization */
    wasi_context_t *ctx = (wasi_context_t *)up->wasm.wasi_ctx;
    if (!ctx) {
      print("wasm_exec_compile: ERROR - WASI context is null after init\n");
      wasm_heap_free(up->wasm.wasi_ctx);
      up->wasm.wasi_ctx = nil;
    } else {
      /* Check that essential FDs are initialized */
      if (!ctx->fds[0].is_open || !ctx->fds[1].is_open ||
          !ctx->fds[2].is_open) {
        print("wasm_exec_compile: warning - stdio FDs not properly "
              "initialized\n");
        /* Don't fail - this might be OK in some contexts */
      }

      /* Link WASI functions with enhanced error handling */
      print("wasm_exec_compile: linking WASI functions with mask 0x%08x\n",
            WASI_ALLOW_DEFAULT);
      result = LinkWasi(module, WASI_ALLOW_DEFAULT);
      if (result) {
        print("wasm_exec_compile: WASI link warning: %s (continuing...)\n",
              result);
        /* Don't fail compilation - WASI link warnings are common */
      } else {
        print("wasm_exec_compile: WASI functions linked successfully\n");
      }

      /* Link Lux9 Host Functions */
      print("wasm_exec_compile: linking Lux9 host functions\n");
      result = LinkLux9(module);
      if (result) {
        print("wasm_exec_compile: Lux9 link warning: %s (continuing...)\n",
              result);
        /* Don't fail - some modules might not need Lux9 functions */
      } else {
        print("wasm_exec_compile: Lux9 host functions linked successfully\n");
      }
    }
  } else {
    print("wasm_exec_compile: WARNING - failed to allocate WASI context\n");
    /* Continue without WASI context - some WASM modules don't need it */
  }

  /* Compile all functions */
  print("wasm_exec_compile: compiling module\n");
  result = m3_CompileModule(module);
  if (result) {
    print("wasm_exec_compile: compile failed: %s\n", result);
    goto fail_compile;
  }

  /* Find entry point (_start) */
  result = m3_FindFunction(&start_func, (IM3Runtime)up->wasm.runtime, "_start");
  if (result) {
    print("wasm_exec_compile: _start not found, trying main\n");
    result = m3_FindFunction(&start_func, (IM3Runtime)up->wasm.runtime, "main");
    if (result) {
      print("wasm_exec_compile: no entry point (_start or main): %s\n", result);
      goto fail_compile;
    }
  }

  print("wasm_exec_compile: success, entry=%p\n", start_func);

  if (out_start)
    *out_start = start_func;

  /* Validate linear memory is accessible */
  if (up->wasm.linear_memory && up->wasm.memory_size > 0) {
    volatile u8int first = up->wasm.linear_memory[0];
    volatile u8int last = up->wasm.linear_memory[up->wasm.memory_size - 1];
    print("wasm_exec_compile: linear memory validated [%p-%p]\n",
          up->wasm.linear_memory,
          up->wasm.linear_memory + up->wasm.memory_size);
  } else {
    print("wasm_exec_compile: WARNING - no linear memory mapped\n");
  }

  return 0;

fail_compile:
  wasm_runtime_cleanup_process(up);
  if (branch_inited) {
    arena_branch_drain(&up->wasm.branch);
    memset(&up->wasm.branch, 0, sizeof(up->wasm.branch));
  }
  up->wasm.initialized = 0;
  return -1;
}

/* Forward declaration */
static const char *wasm_trap_label(M3Result result);

/*
 * wasm_exec_run: Execute _start function and handle WASM process lifecycle
 * Called from sysexec() when start_func is found.
 */
/*@
  @ assigns \nothing;
  @*/
void wasm_exec_run(IM3Function start_func) {
  if (!start_func) {
    print("wasm_exec_run: invalid start_func\n");
    pexit("wasm invalid start", 1);
  }

  print("wasm_exec_run: executing entry point for pid=%lud\n", up->pid);

  /* Execute WASM entry point */
  M3Result result = m3_CallV(start_func);

  /* Check for WASI exit trap */
  if (result && strcmp((char *)result, (char *)m3Err_trapExit) == 0) {
    u32int exit_code = 0;
    if (up->wasm.wasi_ctx) {
      exit_code = ((wasi_context_t *)up->wasm.wasi_ctx)->exit_code;
    }
    print("wasm_exec_run: pid=%lud exited with code %ud\n", up->pid, exit_code);
    if (exit_code == 0) {
      pexit(nil, 0);
    } else {
      char exit_msg[32];
      snprint(exit_msg, sizeof(exit_msg), "exit code %ud", exit_code);
      pexit(exit_msg, 1);
    }
  }

  /* Check for other traps */
  if (result) {
    const char *trap_label = wasm_trap_label(result);
    if (trap_label) {
      print("wasm_exec_run: pid=%lud trapped: %s\n", up->pid, trap_label);
      pexit((char *)trap_label, 1);
    } else {
      print("wasm_exec_run: pid=%lud error: %s\n", up->pid, result);
      pexit((char *)result, 1);
    }
  }

  /* Normal return from entry point */
  print("wasm_exec_run: pid=%lud completed successfully\n", up->pid);
  pexit(nil, 0);
}

#ifndef nil
#define nil ((void *)0)
#endif

typedef struct WasmHeapBlock {
  u32int size; /* payload size */
  u8int free;
  u8int pad[3];
  struct WasmHeapBlock *next;
} WasmHeapBlock;

#define WASM_HEAP_HDR_SIZE                                                    \
  ((size_t)ROUNDUP((size_t)sizeof(WasmHeapBlock), (size_t)WASM_HEAP_ALIGN))

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static int wasm_branch_init(Proc *p) {
  u64int branch_cap;
  ulong initial_budget;

  if (!p)
    return -1;
  if (p->wasm.branch.owner_ps != nil)
    return 0;

  memset(&p->wasm.branch, 0, sizeof(p->wasm.branch));

  branch_cap = (u64int)WASM_MAX_LINEAR_BYTES + WASM_HEAP_BYTES;
  initial_budget = 1024 * 1024;
  if ((u64int)initial_budget > branch_cap)
    initial_budget = (ulong)branch_cap;

  arena_branch_init(&p->wasm.branch, &p->pebble, initial_budget);
  p->wasm.branch.max_tokens =
      (ulong)ROUNDUP(branch_cap, (u64int)PEBBLE_MEM_PER_TOKEN);
  if (p->wasm.branch.high_water > p->wasm.branch.max_tokens)
    p->wasm.branch.high_water = p->wasm.branch.max_tokens;
  if (p->wasm.branch.low_water > p->wasm.branch.high_water)
    p->wasm.branch.low_water = p->wasm.branch.high_water;

  return 0;
}

static int wasm_reserve_linear_slot(Proc *p) {
  uintptr base;

  if (!p)
    return -1;
  if (p->wasm.linear_slot_base != 0)
    return 0;

  base = wasm_arena_alloc_slot(p);
  if (base == 0 || base < UTZERO)
    return -1;

  p->wasm.linear_slot_base = base;
  return 0;
}

static void wasm_release_linear_slot(Proc *p) {
  if (!p || p->wasm.linear_slot_base == 0)
    return;

  wasm_arena_free_slot(p->wasm.linear_slot_base);
  p->wasm.linear_slot_base = 0;
}

static uintptr wasm_linear_base(Proc *p, u64int map_bytes) {
  if (!p || map_bytes > WASM_MAX_LINEAR_BYTES)
    return 0;
  if (wasm_reserve_linear_slot(p) < 0)
    return 0;
  return p->wasm.linear_slot_base;
}

/* WASM capability permissions - temporarily defined here until moved to kernel
 * headers */
/* Capability permissions defined in wasm_runtime.h */

static const char *wasm_trap_label(M3Result result) {
  if (!result)
    return nil;
  if (strcmp((char *)result, (char *)m3Err_trapOutOfBoundsMemoryAccess) == 0)
    return "out of bounds memory access";
  if (strcmp((char *)result, (char *)m3Err_trapDivisionByZero) == 0)
    return "division by zero";
  if (strcmp((char *)result, (char *)m3Err_trapIntegerOverflow) == 0)
    return "integer overflow";
  if (strcmp((char *)result, (char *)m3Err_trapIntegerConversion) == 0)
    return "invalid integer conversion";
  if (strcmp((char *)result, (char *)m3Err_trapIndirectCallTypeMismatch) == 0)
    return "indirect call type mismatch";
  if (strcmp((char *)result, (char *)m3Err_trapTableIndexOutOfRange) == 0)
    return "table index out of range";
  if (strcmp((char *)result, (char *)m3Err_trapTableElementIsNull) == 0)
    return "null table element";
  if (strcmp((char *)result, (char *)m3Err_trapUnreachable) == 0)
    return "unreachable executed";
  if (strcmp((char *)result, (char *)m3Err_trapStackOverflow) == 0)
    return "stack overflow";
  if (strcmp((char *)result, (char *)m3Err_trapAbort) == 0)
    return "abort";
  return nil;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static int wasm_heap_init(Proc *p) {
  uintptr lin_base;
  uintptr heap_base;
  ulong heap_pages;
  Segment *h;

  if (!p)
    return -1;
  if (p->wasm.heap_base != nil)
    return 0;
  if (wasm_branch_init(p) < 0)
    return -1;

  lin_base = wasm_linear_base(p, (u64int)WASM_MAX_LINEAR_BYTES);
  if (lin_base == 0)
    return -1;
  heap_base =
      lin_base - WASM_LINEAR_GUARD - WASM_HEAP_GUARD - WASM_HEAP_BYTES;
  heap_base = PGROUND(heap_base);
  if (heap_base < UTZERO)
    return -1;

  heap_pages = WASM_HEAP_BYTES / BY2PG;
  h = newseg(SG_BSS | SG_NOEXEC | SG_WASM, heap_base, heap_pages);
  if (h == nil) {
    wasm_release_linear_slot(p);
    return -1;
  }

  if (p->seg[SEG4]) {
    putseg(p->seg[SEG4]);
    p->seg[SEG4] = nil;
  }
  p->seg[SEG4] = h;

  p->wasm.heap_base = (u8int *)heap_base;
  p->wasm.heap_size = WASM_HEAP_BYTES;
  p->wasm.heap_used = 0;
  p->wasm.heap_head = nil;
  p->wasm.heap_live = 0;
  p->wasm.linear_charged = 0;
  p->wasm.initialized = 1;
  return 0;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static void wasm_heap_destroy(Proc *p) {
  if (!p)
    return;
  if (p->wasm.heap_live > 0)
    arena_branch_free(&p->wasm.branch, p->wasm.heap_live);
  if (p->seg[SEG4] && (p->seg[SEG4]->type & SG_WASM) != 0) {
    putseg(p->seg[SEG4]);
    p->seg[SEG4] = nil;
  }
  wasm_release_linear_slot(p);

  p->wasm.heap_base = nil;
  p->wasm.heap_size = 0;
  p->wasm.heap_used = 0;
  p->wasm.heap_head = nil;
  p->wasm.heap_live = 0;
}

/*@
  @ requires p == \null || \valid(p);
  @ requires ptr == \null || \valid(ptr);
  @ assigns \nothing;
  @*/
static int wasm_heap_contains(Proc *p, void *ptr) {
  if (!p || p->wasm.heap_base == nil || ptr == nil)
    return 0;
  return (u8int *)ptr >= p->wasm.heap_base &&
         (u8int *)ptr < p->wasm.heap_base + p->wasm.heap_size;
}

/*@
  @ requires \valid(p);
  @ assigns p->wasm.linear_charged;
  @ behavior growth:
  @   assumes new_size > p->wasm.linear_charged;
  @   ensures \result == 0 ==> p->wasm.linear_charged == new_size;
  @   ensures \result == -1 ==> p->wasm.linear_charged ==
  \old(p->wasm.linear_charged);
  @ behavior no_change:
  @   assumes new_size == p->wasm.linear_charged;
  @   ensures \result == 0;
  @   ensures p->wasm.linear_charged == \old(p->wasm.linear_charged);
  @ complete behaviors growth, no_change;
  @*/
static int wasm_charge_linear(Proc *p, u32int new_size) {
  u32int old_size;

  if (!p)
    return -1;

  old_size = p->wasm.linear_charged;
  if (new_size == old_size)
    return 0;

  if (new_size > old_size) {
    u32int delta = new_size - old_size;
    if (arena_branch_alloc(&p->wasm.branch, delta) != 0) {
      if (pebble_debug)
        print("wasm_charge_linear: branch budget exhausted for delta=%lud\n",
              (ulong)delta);
      return -1;
    }
  } else {
    u32int delta = old_size - new_size;
    arena_branch_free(&p->wasm.branch, delta);
  }

  p->wasm.linear_charged = new_size;
  return 0;
}

/*@
  @ assigns \nothing;
  @*/
int wasm_linear_charge_reserve(uint32_t new_size, uint32_t old_size) {
  Proc *p = up;
  if (!p || !p->wasm.initialized)
    return -1;
  if (wasm_charge_linear(p, new_size) != 0)
    return -1;
  p->wasm.memory_size = new_size;
  p->wasm.memory_pages = new_size / (64 * 1024);
  return 0;
}

/*@
  @ assigns \nothing;
  @*/
void wasm_linear_charge_rollback(uint32_t old_size) {
  Proc *p = up;
  if (!p || !p->wasm.initialized)
    return;
  wasm_charge_linear(p, old_size);
  p->wasm.memory_size = old_size;
  p->wasm.memory_pages = old_size / (64 * 1024);
}

static WasmHeapBlock *wasm_heap_block_from_ptr(Proc *p, void *ptr) {
  if (!wasm_heap_contains(p, ptr))
    return nil;
  return (WasmHeapBlock *)((u8int *)ptr - WASM_HEAP_HDR_SIZE);
}

int wasm_heap_owned_ptr(void *ptr) {
  return wasm_heap_contains(up, ptr);
}

static void *wasm_linear_alloc_aligned(size_t size) {
  uintptr raw;
  uintptr aligned;
  void *base;

  if (size == 0)
    return nil;
  if (size > (size_t)-1 - BY2PG - sizeof(void *))
    return nil;

  base = xalloc((ulong)(size + BY2PG + sizeof(void *)));
  if (base == nil)
    return nil;

  raw = (uintptr)base + sizeof(void *);
  aligned = (raw + BY2PG - 1) & ~((uintptr)BY2PG - 1);
  ((void **)aligned)[-1] = base;
  memset((void *)aligned, 0, size);
  return (void *)aligned;
}

static void wasm_linear_release_aligned(void *ptr) {
  void *base;

  if (ptr == nil)
    return;
  base = ((void **)ptr)[-1];
  if (base != nil)
    xfree(base);
}

void *wasm_linear_realloc(void *ptr, size_t new_size, size_t old_size) {
  void *new_ptr;
  size_t copy_size;

  if (new_size == 0) {
    if (ptr)
      wasm_linear_release_aligned(ptr);
    return nil;
  }

  new_ptr = wasm_linear_alloc_aligned(new_size);
  if (new_ptr == nil)
    return nil;

  copy_size = 0;
  if (ptr && old_size > 0) {
    copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    wasm_linear_release_aligned(ptr);
  }
  if (new_size > copy_size)
    memset((u8int *)new_ptr + copy_size, 0, new_size - copy_size);

  return new_ptr;
}

void wasm_linear_free(void *ptr) {
  wasm_linear_release_aligned(ptr);
}

/*@
  @ requires \valid(up);
  @ assigns up->wasm.heap_used, up->wasm.heap_live;
  @ ensures \result == \null || wasm_heap_contains(up, \result);
  @*/
void *wasm_heap_alloc(size_t size) {
  Proc *p = up;
  WasmHeapBlock *prev;
  WasmHeapBlock *cur;
  u32int used;
  u32int need;
  WasmHeapBlock *blk;
  void *ptr;

  if (p == nil || !p->wasm.initialized)
    return nil;
  if (p->wasm.heap_base == nil)
    return nil;

  size = ROUNDUP(size, WASM_HEAP_ALIGN);
  if (size == 0)
    size = WASM_HEAP_ALIGN;

  prev = nil;
  cur = (WasmHeapBlock *)p->wasm.heap_head;
  while (cur) {
    if (cur->free && cur->size >= size)
      break;
    prev = cur;
    cur = cur->next;
  }

  if (cur) {
    if (arena_branch_alloc(&p->wasm.branch, cur->size) != 0)
      return nil;
    cur->free = 0;
    p->wasm.heap_live += cur->size;
    ptr = (u8int *)cur + WASM_HEAP_HDR_SIZE;
    memset(ptr, 0, cur->size);
    return ptr;
  }

  used = ROUNDUP(p->wasm.heap_used, WASM_HEAP_ALIGN);
  need = WASM_HEAP_HDR_SIZE + (u32int)size;

  if (used + need > p->wasm.heap_size)
    return nil;
  if (arena_branch_alloc(&p->wasm.branch, size) != 0)
    return nil;

  blk = (WasmHeapBlock *)(p->wasm.heap_base + used);
  blk->size = (u32int)size;
  blk->free = 0;
  blk->next = nil;
  if (prev)
    prev->next = blk;
  else
    p->wasm.heap_head = blk;

  p->wasm.heap_used = used + need;
  p->wasm.heap_live += (u32int)size;

  ptr = (u8int *)blk + WASM_HEAP_HDR_SIZE;
  memset(ptr, 0, size);
  return ptr;
}

/*@
  @ requires ptr == \null || \valid(ptr);
  @ assigns \nothing;
  @*/
void wasm_heap_free(void *ptr) {
  Proc *p = up;
  WasmHeapBlock *blk = wasm_heap_block_from_ptr(p, ptr);
  WasmHeapBlock *next;
  WasmHeapBlock *prev;
  WasmHeapBlock *cur;

  if (!blk || blk->free)
    return;

  blk->free = 1;
  if (p->wasm.heap_live >= blk->size)
    p->wasm.heap_live -= blk->size;
  arena_branch_free(&p->wasm.branch, blk->size);

  /* Coalesce with next if adjacent and free */
  next = blk->next;
  if (next && next->free &&
      (u8int *)blk + WASM_HEAP_HDR_SIZE + blk->size == (u8int *)next) {
    blk->size += WASM_HEAP_HDR_SIZE + next->size;
    blk->next = next->next;
  }

  /* Coalesce with previous if adjacent and free */
  prev = nil;
  cur = (WasmHeapBlock *)p->wasm.heap_head;
  while (cur && cur != blk) {
    prev = cur;
    cur = cur->next;
  }
  if (prev && prev->free &&
      (u8int *)prev + WASM_HEAP_HDR_SIZE + prev->size == (u8int *)blk) {
    prev->size += WASM_HEAP_HDR_SIZE + blk->size;
    prev->next = blk->next;
  }
}

void *wasm_heap_realloc(void *ptr, size_t new_size, size_t old_size) {
  Proc *p = up;
  WasmHeapBlock *blk;
  u32int needed;
  u32int delta;
  u8int *blk_end;
  u8int *heap_end;
  void *new_ptr;
  size_t copy;

  if (M3_UNLIKELY(new_size == old_size))
    return ptr;
  if (ptr && !wasm_heap_contains(p, ptr))
    return nil;
  if (ptr) {
    blk = wasm_heap_block_from_ptr(p, ptr);
    if (blk && new_size <= blk->size)
      return ptr;
    if (blk) {
      needed = (u32int)ROUNDUP(new_size, WASM_HEAP_ALIGN);
      if (needed > blk->size) {
        delta = needed - blk->size;
        blk_end = (u8int *)blk + WASM_HEAP_HDR_SIZE + blk->size;
        heap_end = p->wasm.heap_base + p->wasm.heap_used;
        if (!blk->free && blk_end == heap_end &&
            (p->wasm.heap_used + delta) <= p->wasm.heap_size &&
            arena_branch_alloc(&p->wasm.branch, delta) == 0) {
          blk->size = needed;
          p->wasm.heap_used += delta;
          p->wasm.heap_live += delta;
          memset(blk_end, 0, delta);
          return ptr;
        }
      }
    }
  }

  new_ptr = wasm_heap_alloc(new_size);
  if (new_ptr == nil)
    return nil;
  if (ptr) {
    copy = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy);
    wasm_heap_free(ptr);
  }
  return new_ptr;
}
/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static int wasm_map_guard(Proc *p, int segidx, uintptr base, ulong pages) {
  if (!p || pages == 0)
    return 0;
  if (p->seg[segidx]) {
    if ((p->seg[segidx]->type & SG_WASM) == 0)
      return -1;
    putseg(p->seg[segidx]);
    p->seg[segidx] = nil;
  }
  Segment *g = newseg(SG_BSS | SG_FAULT | SG_NOEXEC | SG_WASM, base, pages);
  if (g == nil)
    return -1;
  p->seg[segidx] = g;
  return 0;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static void wasm_clear_guard(Proc *p, int segidx) {
  if (!p || !p->seg[segidx])
    return;
  if ((p->seg[segidx]->type & SG_WASM) == 0)
    return;
  putseg(p->seg[segidx]);
  p->seg[segidx] = nil;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static int wasm_map_linear_memory(Proc *p) {
  extern uintptr saved_limine_hhdm_offset;
  if (!p || !p->wasm.linear_memory || p->wasm.memory_size == 0) {
    print("wasm_map_linear_memory: invalid params p=%p mem=%p size=%ud\n", p,
          p ? p->wasm.linear_memory : 0, p ? p->wasm.memory_size : 0);
    return -1;
  }

  uintptr lin_ptr = (uintptr)p->wasm.linear_memory;
  uintptr offset = lin_ptr & (BY2PG - 1);
  uintptr base_ptr = lin_ptr - offset;
  u64int total_bytes = (u64int)p->wasm.memory_size + (u64int)offset;
  u64int map_bytes = ROUND(total_bytes, BY2PG);
  ulong map_pages = map_bytes / BY2PG;

  if (map_bytes > WASM_MAX_LINEAR_BYTES) {
    print("wasm_map_linear_memory: map_bytes too large %u\n", map_bytes);
    return -1;
  }

  if (base_ptr < saved_limine_hhdm_offset ||
      base_ptr + map_bytes > saved_limine_hhdm_offset + (256ULL * GiB)) {
    print(
        "wasm_map_linear_memory: address out of HHDM range base=%p limit=%p\n",
        base_ptr, saved_limine_hhdm_offset);
    return -1;
  }

  uintptr phys_base = PADDR((void *)base_ptr);
  /*@ loop invariant 0 <= i <= map_pages;
  @ loop assigns i;
  @ loop variant map_pages - i;
  @*/
  for (ulong i = 0; i < map_pages; i++) {
    uintptr va = base_ptr + (i * BY2PG);
    if (PADDR((void *)va) != phys_base + (i * BY2PG)) {
      print("wasm_map_linear_memory: non-contiguous physical memory at i=%lud\n",
            i);
      return -1;
    }
  }

  if (p->seg[LSEG]) {
    if (p->seg[LSEG]->pseg) {
      free(p->seg[LSEG]->pseg);
      p->seg[LSEG]->pseg = nil;
    }
    putseg(p->seg[LSEG]);
    p->seg[LSEG] = nil;
  }
  wasm_clear_guard(p, SEG2);
  wasm_clear_guard(p, SEG3);

  uintptr base = wasm_linear_base(p, map_bytes);
  if (base == 0) {
    print("wasm_map_linear_memory: wasm_linear_base returned 0\n");
    return -1;
  }

  Segment *s =
      newseg(SG_PHYSICAL | SG_NOEXEC | SG_WASM, base + offset, map_pages);
  if (s == nil) {
    print("wasm_map_linear_memory: newseg failed\n");
    return -1;
  }

  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("wasm_map_linear_memory: malloc pseg failed\n");
    putseg(s);
    return -1;
  }

  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "wasmlinear";
  s->pseg->pa = phys_base;
  s->pseg->size = map_bytes;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  p->seg[LSEG] = s;

  uintptr guard_low = (base + offset) - WASM_LINEAR_GUARD;
  uintptr guard_high = (base + offset) + map_pages * BY2PG;
  ulong guard_pages = WASM_LINEAR_GUARD / BY2PG;
  if (guard_low < UTZERO) {
    print("wasm_map_linear_memory: guard_low too low\n");
    return -1;
  }
  if (p->seg[SEG4] &&
      guard_low <
          p->seg[SEG4]
              ->top) { /* SEG4 is ESEG for init? No, init uses ESEG=4? */
    /* ESEG is slot 4. SEG4 is slot 5 via portdat.h definitions usually? need to
     * check */
    /* Assuming standard segments: TSEG=1, DSEG=2, BSEG=3, ESEG=4, SSEG=5? */
    /* userinit uses ESEG=4. SEG4 is usually text/data? */
    /* If conflict with ESEG? */
  }
  if (guard_high + WASM_LINEAR_GUARD > USTKTOP) {
    print("wasm_map_linear_memory: guard_high too high\n");
    return -1;
  }
  if (wasm_map_guard(p, SEG2, guard_low, guard_pages) < 0) {
    print("wasm_map_linear_memory: map guard low failed\n");
    wasm_unmap_linear_memory(p);
    return -1;
  }
  if (wasm_map_guard(p, SEG3, guard_high, guard_pages) < 0) {
    print("wasm_map_linear_memory: map guard high failed\n");
    wasm_unmap_linear_memory(p);
    return -1;
  }
  return 0;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static int wasm_refresh_linear_mapping(Proc *p) {
  uint32_t new_size = 0;
  uint8_t *new_mem;

  if (!p || !p->wasm.runtime)
    return -1;

  new_mem = m3_GetMemory((IM3Runtime)p->wasm.runtime, &new_size, 0);
  if (new_mem == nil || new_size == 0) {
    if (p->wasm.linear_charged > 0)
      wasm_charge_linear(p, 0);
    if (p->wasm.linear_memory != nil)
      wasm_unmap_linear_memory(p);
    p->wasm.linear_memory = nil;
    p->wasm.memory_size = 0;
    p->wasm.memory_pages = 0;
    return 0;
  }

  if (new_size > WASM_MAX_LINEAR_BYTES)
    return -1;

  if (new_mem != p->wasm.linear_memory || new_size != p->wasm.memory_size) {
    u32int old_size = p->wasm.memory_size;
    u32int old_pages = p->wasm.memory_pages;
    u8int *old_mem = p->wasm.linear_memory;
    int charged = 0;

    p->wasm.linear_memory = new_mem;
    p->wasm.memory_size = new_size;
    p->wasm.memory_pages = new_size / (64 * 1024);
    if (!wasm_heap_contains(p, new_mem) && p->wasm.linear_charged != new_size) {
      if (wasm_charge_linear(p, new_size) != 0) {
        p->wasm.linear_memory = old_mem;
        p->wasm.memory_size = old_size;
        p->wasm.memory_pages = old_pages;
        return -1;
      }
      charged = 1;
    }
    if (wasm_map_linear_memory(p) != 0) {
      if (charged)
        wasm_charge_linear(p, old_size);
      p->wasm.linear_memory = old_mem;
      p->wasm.memory_size = old_size;
      p->wasm.memory_pages = old_pages;
      return -1;
    }
  }

  return 0;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static void wasm_unmap_linear_memory(Proc *p) {
  if (!p || !p->seg[LSEG])
    return;
  wasm_clear_guard(p, SEG2);
  wasm_clear_guard(p, SEG3);
  if (p->seg[LSEG]->pseg) {
    free(p->seg[LSEG]->pseg);
    p->seg[LSEG]->pseg = nil;
  }
  putseg(p->seg[LSEG]);
  p->seg[LSEG] = nil;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void wasm_runtime_cleanup_process(Proc *p) {
  if (p->wasm.runtime) {
    /* Should already be freed by sys_wasm_destroy, but safety net */
    m3_FreeRuntime((IM3Runtime)p->wasm.runtime);
  }
  p->wasm.runtime = nil;
  p->wasm.module = nil;
  if (p->wasm.env) {
    m3_FreeEnvironment((IM3Environment)p->wasm.env);
    p->wasm.env = nil;
  }
  if (p->wasm.wasi_ctx) {
    wasi_lux9_destroy_context((wasi_context_t *)p->wasm.wasi_ctx);
    wasm_heap_free(p->wasm.wasi_ctx);
    p->wasm.wasi_ctx = nil;
  }
  if (p->wasm.module_bytes) {
    free(p->wasm.module_bytes);
    p->wasm.module_bytes = nil;
    p->wasm.module_bytes_len = 0;
  }
  if (p->wasm.cap_table) {
    wasm_cap_table_destroy((wasm_cap_table_t *)p->wasm.cap_table);
    xfree(p->wasm.cap_table);
    p->wasm.cap_table = nil;
  }
  wasm_unmap_linear_memory(p);
  if (p->wasm.linear_charged > 0) {
    wasm_charge_linear(p, 0);
  }
  wasm_heap_destroy(p);
  p->wasm.linear_memory = nil;
  p->wasm.memory_size = 0;
  p->wasm.memory_pages = 0;
}

/* NOTE: WASM instances are now stored in Proc.wasm structure
 * See ADR_WASM_AS_PROCESSES.md for architecture rationale.
 * Each WASM program runs as a process (up->wasm.initialized == 1)
 * instead of being an entry in a separate instance table.
 *
 * Benefits:
 * - Leverages up->capabilities for permission checks
 * - Leverages up->seg[LSEG] for linear memory
 * - No artificial limit on WASM processes
 * - WASM processes visible in ps and /proc
 * - Standard process lifecycle (fork, exec, wait, kill)
 */

/* ========== Runtime Initialization ========== */

/*@
  @ assigns \nothing;
  @*/
void wasm_runtime_init(void) {
  if (runtime_initialized) {
    print("wasm_runtime: already initialized\n");
    return;
  }

  print("wasm_runtime: initializing isolated wasm3 runtime (Layer 1)\n");

  /* NOTE: No instance table initialization needed!
   * WASM state is stored in Proc.wasm structure per-process.
   * Each process with wasm.initialized == 1 is a WASM process.
   */

  /* Zero statistics */
  memset(&wasm_runtime.stats, 0, sizeof(wasm_runtime.stats));
  wasm_arena_init();

  runtime_initialized = 1;
  print("wasm_runtime: initialization complete (Layer 1 ready)\n");
}

/* ========== Kernel-Internal API for sysexec() ========== */

/*@
  @ requires tx == \null || \valid(tx);
  @ requires rx == \null || \valid(rx);
  @ assigns \nothing;
  @*/
int sys_wasm_compile(Fcall *tx, Fcall *rx) {
  print("WASM: sys_wasm_compile called (scount=%ud)\n", tx->scount);
  int branch_inited = 0;
  int devindex;
  Chan *c = nil;
  u32int module_size = 0;
  u8int *module_bytes = nil;
  Dir d;
  uchar statbuf[256]; /* Sufficient for most Dir stats */
  int n;
  long rn;

  if (!runtime_initialized) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: runtime not initialized\n");
    wasm_reply_error(rx, "wasm runtime not initialized");
    return -1;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: runtime init ok\n");

  /* Check capability: process must have PERM_WASM_COMPILE */
  if (!(up->capabilities & PERM_WASM_COMPILE)) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: no permission\n");
    wasm_reply_error(rx, "no WASM compile permission");
    wasm_runtime.stats.errors++;
    return -1;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: permission ok\n");

  /* Check if already initialized (can't compile twice) */
  if (up->wasm.initialized) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: already initialized\n");
    wasm_reply_error(rx, "WASM already compiled for this process");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Parse sdata for FD (4 bytes) */
  if (!tx->sdata || tx->scount < 4) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: invalid payload\n");
    wasm_reply_error(rx, "invalid compile payload (expecting FD)");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u32int fd = GBIT32(tx->sdata);
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: fd=%d\n", fd);

  /* Error handling wrapper for file ops */
  if (waserror()) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: error during IO: %s\n", up->errstr);
    wasm_safe_channel_close(&c);
    if (module_bytes)
      free(module_bytes);
    wasm_reply_error(rx, "compile IO failed: %s", up->errstr);
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Resolve FD to Chan */
  c = fdtochan((int)fd, OREAD, 0, 1);
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: fd resolved to chan\n");

  devindex = devno(c->type, 0);
  if (devindex < 0 || devtab[devindex] == nil || devtab[devindex]->stat == nil ||
      devtab[devindex]->read == nil) {
    error("invalid module device");
  }

  /* Get File Size using stat */
  n = devtab[devindex]->stat(c, statbuf, sizeof(statbuf));
  if (n <= 0)
    error("stat failed");

  convM2D(statbuf, (uint)n, &d, nil);
  module_size = (u32int)d.length;
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: stat ok, size=%d\n", module_size);

  if (module_size == 0 || module_size > WASM_MAX_MODULE_BYTES) {
    error("invalid module size");
  }

  print("wasm_runtime: compile request pid=%lud fd=%d size=%ud\n", up->pid, fd,
        module_size);

  /* Allocate buffer for module */
  module_bytes = malloc(module_size);
  if (module_bytes == nil)
    error(Enomem);
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: malloc ok\n");

  /* Read file content */
  rn = devtab[devindex]->read(c, module_bytes, module_size, 0);
  if (rn != module_size) {
    error("short read");
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: read ok\n");

  /* Close channel and clear error stack */
  wasm_safe_channel_close(&c);
  poperror();
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: IO complete\n");

  /* Persist module bytes in process structure */
  up->wasm.module_bytes = module_bytes;
  up->wasm.module_bytes_len = module_size;

  if (wasm_branch_init(up) < 0) {
    wasm_reply_error(rx, "failed to init wasm branch budget");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  branch_inited = 1;
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: arena branch init ok\n");

  if (wasm_heap_init(up) < 0) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: heap init failed\n");
    wasm_reply_error(rx, "failed to init wasm heap");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: heap init ok\n");

  /* Initialize capability table */
  up->wasm.cap_table = xalloc(sizeof(wasm_cap_table_t));
  if (up->wasm.cap_table) {
    wasm_cap_table_init((wasm_cap_table_t *)up->wasm.cap_table);
  } else {
    wasm_reply_error(rx, "failed to alloc cap table");
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: cap table init ok\n");
  up->wasm.initialized = 1;

  up->wasm.env = m3_NewEnvironment();
  if (!up->wasm.env) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: env creat failed\n");
    wasm_reply_error(rx, "failed to create wasm3 environment");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: env created %p\n", up->wasm.env);

  /* Create wasm3 runtime for THIS process (64KB stack) */
  up->wasm.runtime =
      m3_NewRuntime((IM3Environment)up->wasm.env, 64 * 1024, nil);
  if (!up->wasm.runtime) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: runtime creat failed\n");
    wasm_reply_error(rx, "failed to create wasm3 runtime");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: runtime created %p\n", up->wasm.runtime);

  /* Parse and load WASM module - using persistent kernel buffer */
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: parsing module\n");
  M3Result result = m3_ParseModule(
      (IM3Environment)up->wasm.env, (IM3Module *)&up->wasm.module,
      up->wasm.module_bytes, up->wasm.module_bytes_len);
  if (result) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: parse failed: %s\n", result);
    wasm_reply_error(rx, "module parse failed: %s", result);
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: parse ok\n");

  /* Load module into runtime */
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: loading module\n");
  result =
      m3_LoadModule((IM3Runtime)up->wasm.runtime, (IM3Module)up->wasm.module);
  if (result) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: load failed: %s\n", result);
    wasm_reply_error(rx, "module load failed: %s", result);
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: load ok\n");

  if (wasm_refresh_linear_mapping(up) != 0) {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: refresh mapping failed\n");
    wasm_reply_error(rx, "failed to map linear memory");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: refresh mapping ok\n");

  /* Initialize WASI Context */
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: allocating WASI context\n");
  up->wasm.wasi_ctx = wasm_heap_alloc(sizeof(wasi_context_t));
  if (!up->wasm.wasi_ctx) {
    print("wasm_runtime: failed to allocate WASI context\n");
    // Cleanup?
  } else {
    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: initializing WASI context\n");
    wasi_lux9_init_context((wasi_context_t *)up->wasm.wasi_ctx, up);

    /* Link WASI functions */
    u32int allow_mask = WASI_ALLOW_DEFAULT;
    if (up->capabilities & PERM_WASM_NET)
      allow_mask |= (WASI_ALLOW_SOCK | WASI_ALLOW_POLL);

    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: Linking WASI\n");
    M3Result link_res = LinkWasi((IM3Module)up->wasm.module, allow_mask);
    if (link_res) {
      print("wasm_runtime: WASI link warning: %s\n", link_res);
    }

    if (boot_verbose)
      print("DEBUG: sys_wasm_compile: Linking Lux9\n");
    M3Result lux_res = LinkLux9((IM3Module)up->wasm.module);
    if (lux_res) {
      print("wasm_runtime: lux9 link warning: %s\n", lux_res);
    }
  }
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: linking complete\n");

  wasm_runtime.stats.total_modules++;
  wasm_runtime.stats.active_instances++;

  print("wasm_runtime: compiled pid=%lud memory=%ud bytes (%ud pages)\n",
        up->pid,
        up->wasm.memory_size, up->wasm.memory_pages);

  /* Send success reply */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  /* Return pid as success confirmation in header retval */
  rx->retval = up->pid;
  rx->scount = 0;
  rx->sdata = nil;
  if (boot_verbose)
    print("DEBUG: sys_wasm_compile: returning success pid=%lud\n", up->pid);
  return 0;

fail_compile:
  wasm_runtime_cleanup_process(up);
  if (branch_inited) {
    arena_branch_drain(&up->wasm.branch);
    memset(&up->wasm.branch, 0, sizeof(up->wasm.branch));
  }
  up->wasm.initialized = 0;
  return -1;
}

/* SYS_WASM_EXECUTE: Execute WASM function
 * sdata format: [func_name_len:4] [func_name:n] [argc:4] [args:argc*8]
 * NOTE: No instance_id - executes in current process (up->wasm)
 */
/*@
  @ requires tx == \null || \valid(tx);
  @ requires rx == \null || \valid(rx);
  @ assigns \nothing;
  @*/
int sys_wasm_execute(Fcall *tx, Fcall *rx) {
  if (!runtime_initialized) {
    wasm_reply_error(rx, "wasm runtime not initialized");
    return -1;
  }

  /* Check if this is a WASM process */
  if (!up->wasm.initialized) {
    wasm_reply_error(rx, "not a WASM process");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Check capability: process must have PERM_WASM_EXECUTE */
  if (!(up->capabilities & PERM_WASM_EXECUTE)) {
    wasm_reply_error(rx, "no WASM execute permission");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Parse sdata */
  if (!tx->sdata || tx->scount < 4) {
    wasm_reply_error(rx, "invalid execute payload");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u8int *sdata = (u8int *)tx->sdata;
  u32int func_name_len = 0;
  memmove(&func_name_len, sdata, sizeof(func_name_len));
  if (func_name_len == 0 || func_name_len > WASM_MAX_FUNC_NAME ||
      func_name_len > tx->scount - 4) {
    wasm_reply_error(rx, "invalid function name size");
    wasm_runtime.stats.errors++;
    return -1;
  }
  char *func_name = (char *)(sdata + 4);

  /* Null-terminate function name (safe copy) */
  char func_name_buf[256];
  if (func_name_len >= sizeof(func_name_buf)) {
    wasm_reply_error(rx, "function name too long");
    wasm_runtime.stats.errors++;
    return -1;
  }
  memmove(func_name_buf, func_name, func_name_len);
  func_name_buf[func_name_len] = '\0';

  print("wasm_runtime: execute pid=%lud func='%s'\n", up->pid, func_name_buf);
  uint32_t mem_size = 0;
  void *mem_ptr = m3_GetMemory((IM3Runtime)up->wasm.runtime, &mem_size, 0);
  print("wasm_runtime: runtime=%p memory=%p size=%ud\n", up->wasm.runtime,
        mem_ptr, mem_size);

  /* Find function in THIS process's WASM runtime */
  IM3Function func;
  M3Result result =
      m3_FindFunction(&func, (IM3Runtime)up->wasm.runtime, func_name_buf);
  if (result) {
    wasm_reply_error(rx, "function not found: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Parse arguments (optional): [argc:4][args:argc*8] */
  u32int expected_argc = m3_GetArgCount(func);
  u32int argc = 0;
  u8int *argp = sdata + 4 + func_name_len;
  u32int remaining = tx->scount - 4 - func_name_len;

  if (remaining >= 4) {
    memmove(&argc, argp, sizeof(argc));
    argp += 4;
    remaining -= 4;
  }

  if (argc > WASM_MAX_ARGS || remaining < argc * WASM_ARG_BYTES) {
    wasm_reply_error(rx, "invalid args payload");
    wasm_runtime.stats.errors++;
    return -1;
  }

  if (argc != expected_argc) {
    wasm_reply_error(rx, "arg count mismatch");
    wasm_runtime.stats.errors++;
    return -1;
  }

  union ArgValue {
    u32int i32;
    u64int i64;
    float f32;
    double f64;
  } arg_vals[WASM_MAX_ARGS];
  void *arg_ptrs[WASM_MAX_ARGS];

  /*@ loop invariant 0 <= i <= argc;
  @ loop assigns i;
  @ loop variant argc - i;
  @*/
  for (u32int i = 0; i < argc; i++) {
    u64int raw = 0;
    memmove(&raw, argp + (i * WASM_ARG_BYTES), sizeof(raw));
    M3ValueType type = m3_GetArgType(func, i);
    switch (type) {
    case c_m3Type_i32:
      arg_vals[i].i32 = (u32int)raw;
      arg_ptrs[i] = &arg_vals[i].i32;
      break;
    case c_m3Type_i64:
      arg_vals[i].i64 = raw;
      arg_ptrs[i] = &arg_vals[i].i64;
      break;
    case c_m3Type_f32: {
      u32int bits = (u32int)raw;
      memmove(&arg_vals[i].f32, &bits, sizeof(bits));
      arg_ptrs[i] = &arg_vals[i].f32;
      break;
    }
    case c_m3Type_f64:
      memmove(&arg_vals[i].f64, &raw, sizeof(raw));
      arg_ptrs[i] = &arg_vals[i].f64;
      break;
    default:
      wasm_reply_error(rx, "unsupported arg type");
      wasm_runtime.stats.errors++;
      return -1;
    }
  }

  /* Execute function */
  /* NOTE: Process state already tracked by up->state (Running, etc.)
   * No need for separate WASM_INST_RUNNING state */
  result = m3_Call(func, argc, (const void **)arg_ptrs);

  if (result) {
    /* Fixed: m3_Call returns M3Result (const char *). Cast to char * for strcmp
     * and pexit */
    if (strcmp((char *)result, (char *)m3Err_trapExit) == 0) {
      u32int exit_code = 0;
      if (up->wasm.wasi_ctx)
        exit_code = ((wasi_context_t *)up->wasm.wasi_ctx)->exit_code;
      rx->type = Rsyscall;
      rx->tag = tx->tag;
      rx->retval = exit_code;
      rx->scount = 0;
      rx->sdata = nil;
      return 0;
    }
    const char *trap = wasm_trap_label(result);
    if (trap)
      wasm_reply_error(rx, "trap: %s", trap);
    else
      wasm_reply_error(rx, "execution failed: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  if (wasm_refresh_linear_mapping(up) != 0) {
    wasm_reply_error(rx, "linear memory remap failed");
    wasm_runtime.stats.errors++;
    return -1;
  }

  wasm_runtime.stats.total_calls++;

  /* Get return value */
  u32int retc = m3_GetRetCount(func);
  u64int retval = 0;
  if (retc > 1) {
    wasm_reply_error(rx, "multi-value returns unsupported");
    wasm_runtime.stats.errors++;
    return -1;
  }
  if (retc == 1) {
    M3ValueType rtype = m3_GetRetType(func, 0);
    union ArgValue ret_val;
    void *ret_ptrs[1] = {&ret_val};
    result = m3_GetResults(func, 1, (const void **)ret_ptrs);
    if (result) {
      wasm_reply_error(rx, "result fetch failed: %s", result);
      wasm_runtime.stats.errors++;
      return -1;
    }
    switch (rtype) {
    case c_m3Type_i32:
      retval = ret_val.i32;
      break;
    case c_m3Type_i64:
      retval = ret_val.i64;
      break;
    case c_m3Type_f32: {
      u32int bits = 0;
      memmove(&bits, &ret_val.f32, sizeof(bits));
      retval = bits;
      break;
    }
    case c_m3Type_f64:
      memmove(&retval, &ret_val.f64, sizeof(retval));
      break;
    default:
      wasm_reply_error(rx, "unsupported return type");
      wasm_runtime.stats.errors++;
      return -1;
    }
  }

  print("wasm_runtime: executed pid=%lu func='%s' retval=%llu\n", up->pid,
        func_name_buf, retval);

  /* Send success reply */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  /* Use the header retval field for the 64-bit return value */
  rx->retval = retval;
  rx->scount = 0;
  rx->sdata = nil;
  return 0;
}

/* SYS_WASM_DESTROY: Destroy WASM instance
 * sdata format: (none needed - destroys current process's WASM)
 * NOTE: In final architecture, process exit (pexit()) will handle this
 * automatically. This syscall allows explicit cleanup before process
 * termination.
 */
/*@
  @ requires tx == \null || \valid(tx);
  @ requires rx == \null || \valid(rx);
  @ assigns \nothing;
  @*/
int sys_wasm_destroy(Fcall *tx, Fcall *rx) {
  if (!runtime_initialized) {
    wasm_reply_error(rx, "wasm runtime not initialized");
    return -1;
  }

  /* Check if this is a WASM process */
  if (!up->wasm.initialized) {
    wasm_reply_error(rx, "not a WASM process");
    return -1;
  }

  print("wasm_runtime: destroy pid=%lu\n", up->pid);

  /* Free wasm3 runtime (frees module too) */
  if (up->wasm.runtime) {
    m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
    up->wasm.runtime = nil;
    up->wasm.module = nil;
  }

  wasm_runtime_cleanup_process(up);

  /* Drain arena branch back to process colorless bank (1:1 conservation) */
  arena_branch_drain(&up->wasm.branch);
  memset(&up->wasm.branch, 0, sizeof(up->wasm.branch));

  /* Mark WASM as uninitialized */
  up->wasm.initialized = 0;

  wasm_runtime.stats.active_instances--;

  /* Send success reply */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->scount = 0;
  rx->sdata = nil;
  return 0;
}

/* ========== Runtime Statistics ========== */

/*@
  @ assigns \nothing;
  @*/
void wasm_runtime_stats(void) {
  extern Proc *proctab(int i);
  Proc *p;

  print("WASM Runtime Statistics (Layer 1):\n");
  print("  Total calls:       %llu\n", wasm_runtime.stats.total_calls);
  print("  Total modules:     %llu\n", wasm_runtime.stats.total_modules);
  print("  Active instances:  %llu\n", wasm_runtime.stats.active_instances);
  print("  Errors:            %llu\n", wasm_runtime.stats.errors);

  print("  Per-process WASM usage:\n");
  for (int i = 0; (p = proctab(i)) != nil; i++) {
    if (!p->wasm.initialized)
      continue;
    print("    pid=%lud heap_used=%ud heap_live=%ud linear=%ud\n", p->pid,
          p->wasm.heap_used, p->wasm.heap_live, p->wasm.linear_charged);
    print("    branch local=%lud borrowed=%lud max=%lud\n",
          p->wasm.branch.local_colorless, p->wasm.branch.borrowed_from_proc,
          p->wasm.branch.max_tokens);
  }
}
