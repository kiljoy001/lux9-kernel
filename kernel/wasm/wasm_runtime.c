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
#include "../include/portlib.h"
#include "../include/u.h"

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

/* Isolated WASM3 Runtime State */
typedef struct WasmRuntime {
  IM3Environment env; /* wasm3 environment */
  Segment *seg; /* Isolated segment for wasm3 (TODO: implement properly) */
  Lock lock;    /* Runtime-wide lock */

  /* Statistics */
  struct {
    u64int total_calls;
    u64int total_modules;
    u64int active_instances;
    u64int errors;
  } stats;
} WasmRuntime;

/* Temporary segment type for WASM until proper isolation is implemented */
#define SG_WASM 10
#define WASMSIZE (16 * 1024 * 1024) /* 16MB */

/* WASM capability permissions - temporarily defined here until moved to kernel
 * headers */
#define PERM_WASM_COMPILE (1UL << 16) /* Can compile WASM modules */
#define PERM_WASM_EXECUTE (1UL << 17) /* Can execute WASM functions */

/* Global isolated runtime (initialized at boot) */
static WasmRuntime wasm_runtime;
static int runtime_initialized = 0;
static uchar wasm_compile_reply[8];
static uchar wasm_execute_reply[8];

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

  /* TODO: Allocate isolated segment for wasm3 */
  /* This segment CANNOT access kernel heap - only exchange pages */
  /* Temporarily commented out until proper segment types are defined */
  // wasm_runtime.seg = newseg(SG_WASM, WASMSIZE, 0);
  // if (!wasm_runtime.seg) {
  //     print("wasm_runtime: FATAL: failed to allocate isolated segment\n");
  //     return;
  // }
  // print("wasm_runtime: allocated isolated segment at %p, size %llu\n",
  //       wasm_runtime.seg, WASMSIZE);
  wasm_runtime.seg = nil; /* Temporary: no segment isolation yet */

  /* Initialize wasm3 environment */
  wasm_runtime.env = m3_NewEnvironment();
  if (!wasm_runtime.env) {
    print("wasm_runtime: FATAL: failed to create wasm3 environment\n");
    return;
  }

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
  u8int *sdata = (u8int *)tx->sdata;
  u32int module_size = *(u32int *)(sdata + 0);
  u8int *module_bytes = sdata + 4;

  print("wasm_runtime: compile request pid=%lu size=%u\n", up->pid,
        module_size);

  /* Create wasm3 runtime for THIS process (64KB stack) */
  up->wasm.runtime = m3_NewRuntime(wasm_runtime.env, 64 * 1024, nil);
  if (!up->wasm.runtime) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to create wasm3 runtime");
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Parse and load WASM module */
  M3Result result =
      m3_ParseModule(wasm_runtime.env, (IM3Module *)&up->wasm.module,
                     module_bytes, module_size);
  if (result) {
    m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
    up->wasm.runtime = nil;
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "module parse failed: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Load module into runtime */
  result =
      m3_LoadModule((IM3Runtime)up->wasm.runtime, (IM3Module)up->wasm.module);
  if (result) {
    m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
    up->wasm.runtime = nil;
    up->wasm.module = nil;
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "module load failed: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  /* Get linear memory pointer */
  up->wasm.linear_memory =
      m3_GetMemory((IM3Runtime)up->wasm.runtime, &up->wasm.memory_size, 0);
  up->wasm.memory_pages = up->wasm.memory_size / (64 * 1024);

  /* TODO: Map linear memory to seg[LSEG] segment for proper isolation */

  up->wasm.initialized = 1;

  /* Initialize per-container Pebble branch bank for local allocations
   * Budget: 1MB from process bank for WASM heap/stack allocations
   */
  arena_branch_init(&up->wasm.branch, pebble_state(), 1024 * 1024);

  /* Initialize WASI Context */
  up->wasm.wasi_ctx = malloc(sizeof(wasi_context_t));
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
}

/* SYS_WASM_EXECUTE: Execute WASM function
 * sdata format: [func_name_len:4] [func_name:n] [args:...]
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
  u8int *sdata = (u8int *)tx->sdata;
  u32int func_name_len = *(u32int *)(sdata + 0);
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

  /* Execute function */
  /* NOTE: Process state already tracked by up->state (Running, etc.)
   * No need for separate WASM_INST_RUNNING state */

  /* TODO: Parse args from sdata and pass to m3_Call */
  /* For now, call with no arguments */
  result = m3_CallV(func); /* CallV for variadic args, passing none */

  if (result) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "execution failed: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  wasm_runtime.stats.total_calls++;

  /* Get return value */
  /* TODO: Marshal return value to rx->retval_bytes */
  uint64_t retval = 0;
  m3_GetResultsV(func, &retval);

  print("wasm_runtime: executed pid=%lu func='%s' retval=%llu\n", up->pid,
        func_name_buf, retval);

  /* Send success reply */
  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->scount = 8;
  rx->sdata = wasm_execute_reply;
  PBIT64(wasm_execute_reply, retval);
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

  /* Clear linear memory pointer (actual memory freed by wasm3) */
  up->wasm.linear_memory = nil;
  up->wasm.memory_size = 0;
  up->wasm.memory_pages = 0;

  /* TODO: Unmap seg[LSEG] if we mapped linear memory there */

  /* Drain arena branch back to process colorless bank (1:1 conservation) */
  arena_branch_drain(&up->wasm.branch);

  /* Destroy WASI Context */
  if (up->wasm.wasi_ctx) {
    wasi_lux9_destroy_context((wasi_context_t *)up->wasm.wasi_ctx);
    free(up->wasm.wasi_ctx);
    up->wasm.wasi_ctx = nil;
  }

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
