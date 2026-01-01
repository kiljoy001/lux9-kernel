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

#include "../include/dat.h"
#include "../include/fcall.h"
#include "../include/fns.h"
#include "../include/mem.h"
#include "../include/pebble.h"
#include "../include/portlib.h"
#include "../include/u.h"
#include "../include/pebble_kernel.h"

/* Provide C99 types for wasm3 (kernel uses u8int, u32int, u64int) */
typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s8int int8_t;
typedef s16int int16_t;
typedef s32int int32_t;
typedef s64int int64_t;
typedef usize size_t;
typedef ssize ssize_t;

/* WASM3 headers */
#include "wasi_lux9_shim.h"
#include "wasm_runtime/wasm3/wasm3.h"

#ifndef nil
#define nil ((void *)0)
#endif

/* Global WASM runtime state (per-process environments) */
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

#define WASM_MAX_MODULE_BYTES (16 * 1024 * 1024)
#define WASM_MAX_LINEAR_BYTES (64 * 1024 * 1024)
#define WASM_MAX_FUNC_NAME 255
#define WASM_MAX_ARGS 32
#define WASM_ARG_BYTES 8
#define WASM_LINEAR_GUARD (2 * BY2PG)
#define WASM_LINEAR_SLOTS 16
#define WASM_LINEAR_SLOT_BYTES (WASM_MAX_LINEAR_BYTES + (4 * BY2PG))
#define WASM_HEAP_BYTES (8 * 1024 * 1024)
#define WASM_HEAP_GUARD (2 * BY2PG)

static uintptr wasm_linear_base(Proc *p, u32int map_bytes) {
  uintptr region_size = WASM_LINEAR_SLOTS * WASM_LINEAR_SLOT_BYTES;
  uintptr base = USTKTOP - USTKSIZE - region_size;
  uintptr slot = 0;

  if (p)
    slot = (uintptr)(p->pid % WASM_LINEAR_SLOTS);

  base += slot * WASM_LINEAR_SLOT_BYTES + WASM_LINEAR_GUARD;
  if (base < UTZERO || map_bytes > WASM_MAX_LINEAR_BYTES)
    return 0;
  return base;
}

/* WASM capability permissions - temporarily defined here until moved to kernel
 * headers */
#define PERM_WASM_COMPILE (1UL << 16) /* Can compile WASM modules */
#define PERM_WASM_EXECUTE (1UL << 17) /* Can execute WASM functions */

/* Global isolated runtime (initialized at boot) */
static WasmRuntime wasm_runtime;
static int runtime_initialized = 0;
static uchar wasm_compile_reply[8];
static uchar wasm_execute_reply[8];

static int wasm_heap_init(Proc *p) {
  if (!p)
    return -1;
  if (p->wasm.heap_base != nil)
    return 0;

  uintptr lin_base = wasm_linear_base(p, WASM_MAX_LINEAR_BYTES);
  if (lin_base == 0)
    return -1;
  uintptr heap_base = lin_base - WASM_HEAP_GUARD - WASM_HEAP_BYTES;
  heap_base = PGROUND(heap_base);
  if (heap_base < UTZERO)
    return -1;

  ulong heap_pages = WASM_HEAP_BYTES / BY2PG;
  Segment *h = newseg(SG_BSS | SG_NOEXEC | SG_WASM, heap_base, heap_pages);
  if (h == nil)
    return -1;

  if (p->seg[SEG4]) {
    putseg(p->seg[SEG4]);
    p->seg[SEG4] = nil;
  }
  p->seg[SEG4] = h;
  p->wasm.heap_base = (u8int *)heap_base;
  p->wasm.heap_size = WASM_HEAP_BYTES;
  p->wasm.heap_used = 0;
  return 0;
}

static void wasm_heap_destroy(Proc *p) {
  if (!p)
    return;
  if (p->wasm.heap_used > 0)
    arena_branch_free(&p->wasm.branch, p->wasm.heap_used);
  if (p->seg[SEG4] && (p->seg[SEG4]->type & SG_WASM) != 0) {
    putseg(p->seg[SEG4]);
    p->seg[SEG4] = nil;
  }
  p->wasm.heap_base = nil;
  p->wasm.heap_size = 0;
  p->wasm.heap_used = 0;
}

static int wasm_heap_contains(Proc *p, void *ptr) {
  if (!p || p->wasm.heap_base == nil || ptr == nil)
    return 0;
  return (u8int *)ptr >= p->wasm.heap_base &&
         (u8int *)ptr < p->wasm.heap_base + p->wasm.heap_size;
}

void *wasm_heap_alloc(size_t size) {
  Proc *p = up;
  if (p == nil || !p->wasm.initialized)
    return nil;
  if (p->wasm.heap_base == nil)
    return nil;

  size = ROUNDUP(size, 16);
  if (size == 0)
    size = 16;
  if (p->wasm.heap_used + size > p->wasm.heap_size)
    return nil;
  if (arena_branch_alloc(&p->wasm.branch, size) < 0)
    return nil;

  void *ptr = p->wasm.heap_base + p->wasm.heap_used;
  p->wasm.heap_used += (u32int)size;
  memset(ptr, 0, size);
  return ptr;
}

void wasm_heap_free(void *ptr) {
  Proc *p = up;
  if (wasm_heap_contains(p, ptr))
    return;
}

void *wasm_heap_realloc(void *ptr, size_t new_size, size_t old_size) {
  if (M3_UNLIKELY(new_size == old_size))
    return ptr;

  void *new_ptr = wasm_heap_alloc(new_size);
  if (new_ptr == nil)
    return nil;
  if (ptr) {
    size_t copy = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy);
    wasm_heap_free(ptr);
  }
  return new_ptr;
}
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

static void wasm_clear_guard(Proc *p, int segidx) {
  if (!p || !p->seg[segidx])
    return;
  if ((p->seg[segidx]->type & SG_WASM) == 0)
    return;
  putseg(p->seg[segidx]);
  p->seg[segidx] = nil;
}

static int wasm_map_linear_memory(Proc *p) {
  extern uintptr saved_limine_hhdm_offset;
  if (!p || !p->wasm.linear_memory || p->wasm.memory_size == 0)
    return -1;

  uintptr lin_ptr = (uintptr)p->wasm.linear_memory;
  uintptr offset = lin_ptr & (BY2PG - 1);
  uintptr base_ptr = lin_ptr - offset;
  u32int total_bytes = p->wasm.memory_size + (u32int)offset;
  u32int map_bytes = ROUND(total_bytes, BY2PG);
  ulong map_pages = map_bytes / BY2PG;

  if (map_bytes > WASM_MAX_LINEAR_BYTES)
    return -1;

  if (base_ptr < saved_limine_hhdm_offset ||
      base_ptr + map_bytes >
          saved_limine_hhdm_offset + (256ULL * GiB)) {
    return -1;
  }

  uintptr phys_base = PADDR((void *)base_ptr);
  for (ulong i = 0; i < map_pages; i++) {
    uintptr va = base_ptr + (i * BY2PG);
    if (PADDR((void *)va) != phys_base + (i * BY2PG))
      return -1;
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
  if (base == 0)
    return -1;

  Segment *s = newseg(SG_PHYSICAL | SG_NOEXEC | SG_WASM, base + offset,
                      map_pages);
  if (s == nil)
    return -1;

  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
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
  if (guard_low < UTZERO)
    return -1;
  if (wasm_map_guard(p, SEG2, guard_low, guard_pages) < 0) {
    wasm_unmap_linear_memory(p);
    return -1;
  }
  if (wasm_map_guard(p, SEG3, guard_high, guard_pages) < 0) {
    wasm_unmap_linear_memory(p);
    return -1;
  }
  return 0;
}

static int wasm_refresh_linear_mapping(Proc *p) {
  uint32_t new_size = 0;
  uint8_t *new_mem;

  if (!p || !p->wasm.runtime)
    return -1;

  new_mem = m3_GetMemory((IM3Runtime)p->wasm.runtime, &new_size, 0);
  if (new_mem == nil || new_size == 0) {
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
    p->wasm.linear_memory = new_mem;
    p->wasm.memory_size = new_size;
    p->wasm.memory_pages = new_size / (64 * 1024);
    if (wasm_map_linear_memory(p) != 0)
      return -1;
  }

  return 0;
}

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

void wasm_runtime_cleanup_process(Proc *p) {
  if (!p || !p->wasm.initialized)
    return;

  if (p->wasm.wasi_ctx) {
    wasi_lux9_destroy_context((wasi_context_t *)p->wasm.wasi_ctx);
    wasm_heap_free(p->wasm.wasi_ctx);
    p->wasm.wasi_ctx = nil;
  }

  if (p->wasm.env) {
    m3_FreeEnvironment((IM3Environment)p->wasm.env);
    p->wasm.env = nil;
  }

  wasm_heap_destroy(p);
  wasm_unmap_linear_memory(p);
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

  runtime_initialized = 1;
  print("wasm_runtime: initialization complete (Layer 1 ready)\n");
}

/* ========== Instance Management ========== */

/* NOTE: Instance management functions removed!
 * WASM instances are now processes (Proc.wasm structure).
 * Use up->wasm to access current process's WASM state.
 * No instance_id lookups needed - just check up->wasm.initialized.
 */

/* ========== Tsyscall Handlers (Layer 1 API) ========== */

/* SYS_WASM_COMPILE: Compile WASM module
 * sdata format: [module_size:4] [module_bytes:n]
 * NOTE: No instance_id needed - compiles into current process (up->wasm)
 */
int sys_wasm_compile(Fcall *tx, Fcall *rx) {
  print("WASM: sys_wasm_compile called (scount=%u)\n", tx->scount);
  int branch_inited = 0;
  PebbleState *ps;

  if (!runtime_initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "wasm runtime not initialized");
    return -1;
  }

  /* Check capability: process must have PERM_WASM_COMPILE */
  if (!(up->capabilities & PERM_WASM_COMPILE)) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "no WASM compile permission");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Check if already initialized (can't compile twice) */
  if (up->wasm.initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename),
            "WASM already compiled for this process");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Parse sdata */
  if (!tx->sdata || tx->scount < 4) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid compile payload");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u8int *sdata = (u8int *)tx->sdata;
  u32int module_size = *(u32int *)(sdata + 0);
  if (module_size == 0 || module_size > WASM_MAX_MODULE_BYTES ||
      module_size > tx->scount - 4) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid module size");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u8int *module_bytes = sdata + 4;

  print("wasm_runtime: compile request pid=%lu size=%u\n", up->pid,
        module_size);

  ps = pebble_state();
  if (ps == nil) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "pebble state not initialized");
    wasm_runtime.stats.errors++;
    return -1;
  }

  arena_branch_init(&up->wasm.branch, ps, 1024 * 1024);
  if (wasm_heap_init(up) < 0) {
    snprint(rx->ename, sizeof(rx->ename), "failed to init wasm heap");
    wasm_runtime.stats.errors++;
    return -1;
  }
  up->wasm.initialized = 1;
  branch_inited = 1;

  up->wasm.env = m3_NewEnvironment();
  if (!up->wasm.env) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to create wasm3 environment");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }

  /* Create wasm3 runtime for THIS process (64KB stack) */
  up->wasm.runtime = m3_NewRuntime((IM3Environment)up->wasm.env, 64 * 1024, nil);
  if (!up->wasm.runtime) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to create wasm3 runtime");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }

  /* Parse and load WASM module */
  M3Result result =
      m3_ParseModule((IM3Environment)up->wasm.env,
                     (IM3Module *)&up->wasm.module, module_bytes, module_size);
  if (result) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "module parse failed: %s", result);
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }

  /* Load module into runtime */
  result =
      m3_LoadModule((IM3Runtime)up->wasm.runtime, (IM3Module)up->wasm.module);
  if (result) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "module load failed: %s", result);
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }

  if (wasm_refresh_linear_mapping(up) != 0) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to map linear memory");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }

  /* Initialize WASI Context */
  up->wasm.wasi_ctx = wasm_heap_alloc(sizeof(wasi_context_t));
  if (!up->wasm.wasi_ctx) {
    print("wasm_runtime: failed to allocate WASI context\n");
    // Cleanup?
  } else {
    wasi_lux9_init_context((wasi_context_t *)up->wasm.wasi_ctx);

    /* Link WASI functions */
    M3Result link_res = LinkWasi((IM3Module)up->wasm.module);
    if (link_res) {
      print("wasm_runtime: WASI link warning: %s\n", link_res);
    }
  }

  wasm_runtime.stats.total_modules++;
  wasm_runtime.stats.active_instances++;

  print("wasm_runtime: compiled pid=%lu memory=%u bytes (%u pages)\n", up->pid,
        up->wasm.memory_size, up->wasm.memory_pages);

  /* Send success reply */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->scount = 8;
  rx->sdata = wasm_compile_reply;
  /* Return pid as success confirmation */
  PBIT64(wasm_compile_reply, up->pid);
  return 0;

fail_compile:
  if (up->wasm.wasi_ctx) {
    wasi_lux9_destroy_context((wasi_context_t *)up->wasm.wasi_ctx);
    wasm_heap_free(up->wasm.wasi_ctx);
    up->wasm.wasi_ctx = nil;
  }
  wasm_heap_destroy(up);
  wasm_unmap_linear_memory(up);
  if (up->wasm.runtime) {
    m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
    up->wasm.runtime = nil;
    up->wasm.module = nil;
  }
  if (up->wasm.env) {
    m3_FreeEnvironment((IM3Environment)up->wasm.env);
    up->wasm.env = nil;
  }
  up->wasm.linear_memory = nil;
  up->wasm.memory_size = 0;
  up->wasm.memory_pages = 0;
  if (branch_inited)
    arena_branch_drain(&up->wasm.branch);
  up->wasm.initialized = 0;
  return -1;
}

/* SYS_WASM_EXECUTE: Execute WASM function
 * sdata format: [func_name_len:4] [func_name:n] [argc:4] [args:argc*8]
 * NOTE: No instance_id - executes in current process (up->wasm)
 */
int sys_wasm_execute(Fcall *tx, Fcall *rx) {
  if (!runtime_initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "wasm runtime not initialized");
    return -1;
  }

  /* Check if this is a WASM process */
  if (!up->wasm.initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "not a WASM process");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Check capability: process must have PERM_WASM_EXECUTE */
  if (!(up->capabilities & PERM_WASM_EXECUTE)) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "no WASM execute permission");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Parse sdata */
  if (!tx->sdata || tx->scount < 4) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid execute payload");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u8int *sdata = (u8int *)tx->sdata;
  u32int func_name_len = 0;
  memmove(&func_name_len, sdata, sizeof(func_name_len));
  if (func_name_len == 0 || func_name_len > WASM_MAX_FUNC_NAME ||
      func_name_len > tx->scount - 4) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid function name size");
    wasm_runtime.stats.errors++;
    return -1;
  }
  char *func_name = (char *)(sdata + 4);

  /* Null-terminate function name (safe copy) */
  char func_name_buf[256];
  if (func_name_len >= sizeof(func_name_buf)) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "function name too long");
    wasm_runtime.stats.errors++;
    return -1;
  }
  memmove(func_name_buf, func_name, func_name_len);
  func_name_buf[func_name_len] = '\0';

  print("wasm_runtime: execute pid=%lu func='%s'\n", up->pid, func_name_buf);

  /* Find function in THIS process's WASM runtime */
  IM3Function func;
  M3Result result =
      m3_FindFunction(&func, (IM3Runtime)up->wasm.runtime, func_name_buf);
  if (result) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "function not found: %s", result);
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
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid args payload");
    wasm_runtime.stats.errors++;
    return -1;
  }

  if (argc != expected_argc) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "arg count mismatch");
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
      rx->type = Rerror;
      snprint(rx->ename, sizeof(rx->ename), "unsupported arg type");
      wasm_runtime.stats.errors++;
      return -1;
    }
  }

  /* Execute function */
  /* NOTE: Process state already tracked by up->state (Running, etc.)
   * No need for separate WASM_INST_RUNNING state */
  result = m3_Call(func, argc, (const void **)arg_ptrs);

  if (result) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "execution failed: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  if (wasm_refresh_linear_mapping(up) != 0) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "linear memory remap failed");
    wasm_runtime.stats.errors++;
    return -1;
  }

  wasm_runtime.stats.total_calls++;

  /* Get return value */
  u32int retc = m3_GetRetCount(func);
  u64int retval = 0;
  if (retc > 1) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "multi-value returns unsupported");
    wasm_runtime.stats.errors++;
    return -1;
  }
  if (retc == 1) {
    M3ValueType rtype = m3_GetRetType(func, 0);
    union ArgValue ret_val;
    void *ret_ptrs[1] = {&ret_val};
    result = m3_GetResults(func, 1, (const void **)ret_ptrs);
    if (result) {
      rx->type = Rerror;
      snprint(rx->ename, sizeof(rx->ename), "result fetch failed: %s",
              result);
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
      rx->type = Rerror;
      snprint(rx->ename, sizeof(rx->ename), "unsupported return type");
      wasm_runtime.stats.errors++;
      return -1;
    }
  }

  print("wasm_runtime: executed pid=%lu func='%s' retval=%llu\n", up->pid,
        func_name_buf, retval);

  /* Send success reply */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  if (retc == 1) {
    rx->scount = 8;
    rx->sdata = wasm_execute_reply;
    PBIT64(wasm_execute_reply, retval);
  } else {
    rx->scount = 0;
    rx->sdata = nil;
  }
  return 0;
}

/* SYS_WASM_DESTROY: Destroy WASM instance
 * sdata format: (none needed - destroys current process's WASM)
 * NOTE: In final architecture, process exit (pexit()) will handle this
 * automatically. This syscall allows explicit cleanup before process
 * termination.
 */
int sys_wasm_destroy(Fcall *tx, Fcall *rx) {
  if (!runtime_initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "wasm runtime not initialized");
    return -1;
  }

  /* Check if this is a WASM process */
  if (!up->wasm.initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "not a WASM process");
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

void wasm_runtime_stats(void) {
  print("WASM Runtime Statistics (Layer 1):\n");
  print("  Total calls:       %llu\n", wasm_runtime.stats.total_calls);
  print("  Total modules:     %llu\n", wasm_runtime.stats.total_modules);
  print("  Active instances:  %llu\n", wasm_runtime.stats.active_instances);
  print("  Errors:            %llu\n", wasm_runtime.stats.errors);
}
