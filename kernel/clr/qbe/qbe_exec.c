/* qbe_exec.c - Unified QBE Execution Layer Implementation
 *
 * Dispatches execution to appropriate backend:
 *   - CIL interpreter (from CIL-Interpreter/)
 *   - Fruity IR interpreter (new)
 *   - QBE JIT compilation
 */

#ifdef USERSPACE_TEST
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define nil NULL
#define snprint snprintf
typedef unsigned long ulong;
typedef uint32_t u32int;
typedef int64_t s64int;
extern void *xalloc(size_t);
extern void xfree(void *);

/* Stub types for userspace testing (CIL interpreter not linked) */
typedef struct {
  int dummy;
} vm_execution_state_t;
typedef struct {
  unsigned long memory_limit;
} vm_init_t;
typedef enum { VM_TYPE_I8 = 0 } vm_type_t;
typedef struct {
  vm_type_t type;
  union {
    int64_t i8;
  } value;
} vm_value_t;

static vm_execution_state_t vm_create_execution_state(vm_init_t *init) {
  (void)init;
  vm_execution_state_t s = {0};
  return s;
}
static void vm_destroy_execution_state(vm_execution_state_t *s) { (void)s; }
static int vm_stack_push(vm_execution_state_t *s, vm_value_t *v) {
  (void)s;
  (void)v;
  return 1;
}
static int vm_stack_pop(vm_execution_state_t *s, vm_value_t *v) {
  (void)s;
  (void)v;
  return 1;
}
static int vm_execute_method(vm_execution_state_t *s, const uint8_t *c,
                             size_t sz, uint32_t lc) {
  (void)s;
  (void)c;
  (void)sz;
  (void)lc;
  return 1;
}
static const char *vm_get_error(vm_execution_state_t *s) {
  (void)s;
  return "";
}

#else
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/portlib.h"
#include "../../include/u.h"

/* CIL interpreter - only in kernel mode */
#include "../CIL-Interpreter/include/execution_engine.h"
#endif

#include "../fruity/fruity_to_qbe.h"
#include "../il_to_fruity.h"
#include "fruity_sljit.h"
#include "qbe_exec.h"

/* Forward declaration for Fruity interpreter */
extern int fruity_interp_execute(fruity_function_t *func, void **args,
                                 int arg_count, void *result);

/* AOT Cache Entry */
typedef struct aot_cache_entry {
  u32int method_token;          /* ECMA-335 metadata token */
  void *code_ptr;               /* Pointer to compiled machine code */
  ulong code_size;              /* Size of compiled code */
  struct aot_cache_entry *next; /* Hash chain */
} aot_cache_entry_t;

/* AOT Cache hash table size */
#define AOT_CACHE_BUCKETS 64

/* Execution context structure */
struct qbe_exec_ctx {
  /* CIL interpreter state */
  vm_execution_state_t *vm_state;

  /* Method cache for JIT-compiled code */
  struct {
    u32int method_token;
    void *native_code;
  } *jit_cache;
  int jit_cache_count;
  int jit_cache_capacity;

  /* AOT cache (hash table) */
  aot_cache_entry_t *aot_cache[AOT_CACHE_BUCKETS];
  int aot_cache_count;

  /* Statistics */
  ulong interpret_count;
  ulong jit_count;
  ulong aot_count;
};

/* AOT cache hash function */
static u32int aot_cache_hash(u32int token) { return token % AOT_CACHE_BUCKETS; }

/*
 * Create execution context
 */
qbe_exec_ctx_t *qbe_exec_create(void) {
  qbe_exec_ctx_t *ctx = xalloc(sizeof(qbe_exec_ctx_t));
  if (ctx == nil)
    return nil;

  memset(ctx, 0, sizeof(qbe_exec_ctx_t));

  /* Initialize CIL VM state */
  vm_init_t init = {0};
  init.memory_limit = 1024 * 1024; /* 1MB default */
  ctx->vm_state = xalloc(sizeof(vm_execution_state_t));
  if (ctx->vm_state) {
    *ctx->vm_state = vm_create_execution_state(&init);
  }

  return ctx;
}

/*
 * Destroy execution context
 */
void qbe_exec_destroy(qbe_exec_ctx_t *ctx) {
  if (ctx == nil)
    return;

  if (ctx->vm_state) {
    vm_destroy_execution_state(ctx->vm_state);
    xfree(ctx->vm_state);
  }

  if (ctx->jit_cache) {
    xfree(ctx->jit_cache);
  }

  xfree(ctx);
}

/*
 * Execute via CIL interpreter
 */
static int exec_interpret_cil(qbe_exec_ctx_t *ctx, il_method_t *method,
                              void **args, int arg_count,
                              qbe_exec_result_t *result) {
  if (ctx->vm_state == nil) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "VM state not initialized");
    result->success = -1;
    return -1;
  }

  /* Push arguments onto VM stack */
  for (int i = 0; i < arg_count; i++) {
    vm_value_t val;
    val.type = VM_TYPE_I8; /* TODO: proper type from metadata */
    val.value.i8 = (int64_t)(uintptr_t)args[i];
    if (!vm_stack_push(ctx->vm_state, &val)) {
      snprint(result->error_msg, sizeof(result->error_msg),
              "Failed to push argument %d", i);
      result->success = -1;
      return -1;
    }
  }

  /* Execute the method */
  /* TODO: decode local_count from method->local_var_sig_token blob */
  if (!vm_execute_method(ctx->vm_state, method->il_code, method->il_code_size,
                         0)) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "Execution failed: %s", vm_get_error(ctx->vm_state));
    result->success = -1;
    return -1;
  }

  /* Pop result */
  vm_value_t ret;
  if (vm_stack_pop(ctx->vm_state, &ret)) {
    result->int_result = ret.value.i8;
    result->ref_result = (void *)(uintptr_t)ret.value.i8;
  }

  ctx->interpret_count++;
  result->success = 0;
  return 0;
}

/*
 * Execute via Fruity IR interpreter
 */
static int exec_interpret_fruity(qbe_exec_ctx_t *ctx, il_assembly_t *assembly,
                                 il_method_t *method, void **args,
                                 int arg_count, qbe_exec_result_t *result) {
  il_to_fruity_error_t err;

  /* Convert IL to Fruity IR */
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  if (func == nil) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "IL to Fruity conversion failed: %s",
            il_to_fruity_error_string(err));
    result->success = -1;
    return -1;
  }

  /* Execute via Fruity interpreter */
  int ret = fruity_interp_execute(func, args, arg_count, &result->int_result);

  fruity_free_function(func);

  if (ret < 0) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "Fruity interpreter failed");
    result->success = -1;
    return -1;
  }

  ctx->interpret_count++;
  result->success = 0;
  return 0;
}

/*
 * Execute via JIT compilation using sljit
 */
static int exec_jit(qbe_exec_ctx_t *ctx, il_assembly_t *assembly,
                    il_method_t *method, void **args, int arg_count,
                    qbe_exec_result_t *result) {
  il_to_fruity_error_t err;
  fruity_jit_ctx_t *jit_ctx = nil;
  fruity_jit_result_t jit_result;
  int ret;

  /* Convert IL to Fruity IR */
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  if (func == nil) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "IL to Fruity conversion failed: %s",
            il_to_fruity_error_string(err));
    result->success = -1;
    return -1;
  }

  /* Create JIT context and compile */
  jit_ctx = fruity_jit_create();
  if (jit_ctx == nil) {
    fruity_free_function(func);
    snprint(result->error_msg, sizeof(result->error_msg),
            "Failed to create JIT context");
    result->success = -1;
    return -1;
  }

  ret = fruity_jit_compile(jit_ctx, func, &jit_result);
  fruity_free_function(func);

  if (ret < 0 || jit_result.success != 0) {
    /* JIT compilation failed - fall back to interpreter */
    fruity_jit_destroy(jit_ctx);

    /* Re-convert and interpret */
    func = il_to_fruity_convert_method(assembly, method, &err);
    if (func == nil) {
      result->success = -1;
      return -1;
    }
    ret = fruity_interp_execute(func, args, arg_count, &result->int_result);
    fruity_free_function(func);

    if (ret < 0) {
      snprint(result->error_msg, sizeof(result->error_msg),
              "JIT failed, interpreter fallback also failed");
      result->success = -1;
      return -1;
    }
    ctx->interpret_count++;
  } else {
    /* Execute JIT-compiled code */
    int64_t *int_args = (int64_t *)args;
    ret = fruity_jit_execute(&jit_result, int_args, arg_count,
                             &result->int_result);
    fruity_jit_free_code(&jit_result);
    fruity_jit_destroy(jit_ctx);

    if (ret < 0) {
      snprint(result->error_msg, sizeof(result->error_msg),
              "JIT execution failed");
      result->success = -1;
      return -1;
    }
    ctx->jit_count++;
  }

  result->success = 0;
  return 0;
}
/*
 * Execute a method from IL assembly
 */
int qbe_exec_method(qbe_exec_ctx_t *ctx, il_assembly_t *assembly,
                    il_method_t *method, qbe_exec_mode_t mode, void **args,
                    int arg_count, qbe_exec_result_t *result) {
  if (ctx == nil || method == nil || result == nil) {
    if (result)
      snprint(result->error_msg, sizeof(result->error_msg),
              "Invalid parameters");
    return -1;
  }

  memset(result, 0, sizeof(qbe_exec_result_t));

  switch (mode) {
  case QBE_EXEC_INTERPRET_CIL:
    return exec_interpret_cil(ctx, method, args, arg_count, result);

  case QBE_EXEC_INTERPRET_FRUITY:
    return exec_interpret_fruity(ctx, assembly, method, args, arg_count,
                                 result);

  case QBE_EXEC_JIT:
    return exec_jit(ctx, assembly, method, args, arg_count, result);

  case QBE_EXEC_AOT: {
    /* AOT Compilation Mode (Simulated Cache) */
    il_to_fruity_error_t err;
    fruity_jit_ctx_t *jit_ctx = nil;
    fruity_jit_result_t jit_result;
    void *aot_buffer = nil;
    size_t aot_size = 0;
    int ret;

    /* 1. Convert IL to Fruity IR */
    fruity_function_t *func =
        il_to_fruity_convert_method(assembly, method, &err);
    if (func == nil) {
      snprint(result->error_msg, sizeof(result->error_msg),
              "IL to Fruity conversion failed: %s",
              il_to_fruity_error_string(err));
      result->success = -1;
      return -1;
    }

    jit_ctx = fruity_jit_create();
    if (!jit_ctx) {
      fruity_free_function(func);
      snprint(result->error_msg, sizeof(result->error_msg),
              "Failed to create JIT context");
      result->success = -1;
      return -1;
    }

    /* 2. Compile to AOT Buffer (Serialize) */
    ret = fruity_aot_compile(jit_ctx, func, &aot_buffer, &aot_size);
    fruity_free_function(func);
    fruity_jit_destroy(jit_ctx);

    if (ret < 0 || !aot_buffer) {
      /* Fail - real AOT should fallback, but here we report error */
      snprint(result->error_msg, sizeof(result->error_msg),
              "AOT compilation/serialization failed");
      result->success = -1;
      return -1;
    }

    /* 3. Load from AOT Buffer (Deserialize) */
    /* In a real implementation, we would load 'aot_buffer' from disk cache here
     */
    ret = fruity_aot_load(aot_buffer, aot_size, &jit_result);

    /* We can free the serialized buffer now */
    if (aot_buffer) {
#ifdef USERSPACE_TEST
      free(aot_buffer);
#else
      xfree(aot_buffer);
#endif
    }

    if (ret < 0) {
      snprint(result->error_msg, sizeof(result->error_msg), "AOT load failed");
      result->success = -1;
      return -1;
    }

    /* 4. Execute Loaded Code */
    int64_t *int_args = (int64_t *)args;
    ret = fruity_jit_execute(&jit_result, int_args, arg_count,
                             &result->int_result);
    fruity_jit_free_code(&jit_result);

    if (ret < 0) {
      snprint(result->error_msg, sizeof(result->error_msg),
              "AOT execution failed");
      result->success = -1;
      return -1;
    }

    result->success = 0;
    ctx->aot_count++;
    return 0;
  }

  default:
    snprint(result->error_msg, sizeof(result->error_msg),
            "Unknown execution mode: %d", mode);
    result->success = -1;
    return -1;
  }
}

/*
 * Execute a Fruity function directly
 */
int qbe_exec_fruity(qbe_exec_ctx_t *ctx, fruity_function_t *func,
                    qbe_exec_mode_t mode, void **args, int arg_count,
                    qbe_exec_result_t *result) {
  if (ctx == nil || func == nil || result == nil) {
    if (result)
      snprint(result->error_msg, sizeof(result->error_msg),
              "Invalid parameters");
    return -1;
  }

  memset(result, 0, sizeof(qbe_exec_result_t));

  if (mode != QBE_EXEC_INTERPRET_FRUITY && mode != QBE_EXEC_JIT) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "Fruity functions only support FRUITY or JIT modes");
    result->success = -1;
    return -1;
  }

  int ret = fruity_interp_execute(func, args, arg_count, &result->int_result);
  if (ret < 0) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "Fruity interpreter failed");
    result->success = -1;
    return -1;
  }

  result->success = 0;
  return 0;
}

/*
 * Get recommended execution mode for a method
 */
qbe_exec_mode_t qbe_exec_recommend_mode(il_method_t *method) {
  if (method == nil)
    return QBE_EXEC_INTERPRET_CIL;

  /* Small methods: interpret directly for minimal startup latency */
  if (method->il_code_size < 64)
    return QBE_EXEC_INTERPRET_CIL;

  /* Medium methods: Fruity interpretation for Pebble-aware execution */
  if (method->il_code_size < 512)
    return QBE_EXEC_INTERPRET_FRUITY;

  /* Large methods: JIT compile for performance */
  return QBE_EXEC_JIT;
}

/*
 * Check if method has cached AOT code
 */
int qbe_exec_has_aot_cache(il_method_t *method) {
  /* Note: This is a simplified version without context.
   * For full implementation, pass ctx and check ctx->aot_cache */
  (void)method;
  return 0; /* Return 0 - use context-aware version below */
}

/*
 * Check if method has cached AOT code (context-aware version)
 */
static aot_cache_entry_t *aot_cache_lookup(qbe_exec_ctx_t *ctx,
                                           u32int method_token) {
  if (ctx == nil)
    return nil;

  u32int bucket = aot_cache_hash(method_token);
  aot_cache_entry_t *entry = ctx->aot_cache[bucket];

  while (entry) {
    if (entry->method_token == method_token) {
      return entry;
    }
    entry = entry->next;
  }
  return nil;
}

/*
 * Add entry to AOT cache
 */
static int aot_cache_insert(qbe_exec_ctx_t *ctx, u32int method_token,
                            void *code_ptr, ulong code_size) {
  if (ctx == nil || code_ptr == nil)
    return -1;

  /* Check if already exists */
  if (aot_cache_lookup(ctx, method_token) != nil)
    return 0;

  /* Allocate new entry */
  aot_cache_entry_t *entry = xalloc(sizeof(aot_cache_entry_t));
  if (entry == nil)
    return -1;

  entry->method_token = method_token;
  entry->code_ptr = code_ptr;
  entry->code_size = code_size;

  /* Insert at head of bucket */
  u32int bucket = aot_cache_hash(method_token);
  entry->next = ctx->aot_cache[bucket];
  ctx->aot_cache[bucket] = entry;
  ctx->aot_cache_count++;

  return 0;
}

/*
 * Precompile method to AOT cache
 */
int qbe_exec_precompile(qbe_exec_ctx_t *ctx, il_assembly_t *assembly,
                        il_method_t *method) {
  if (ctx == nil || method == nil)
    return -1;

  /* Check if already cached */
  if (aot_cache_lookup(ctx, method->method_token) != nil) {
    return 0; /* Already compiled */
  }

  /* Convert IL to Fruity IR */
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  if (func == nil) {
    return -1; /* Conversion failed */
  }

  /* Create JIT context and compile */
  fruity_jit_ctx_t *jit_ctx = fruity_jit_create();
  if (jit_ctx == nil) {
    fruity_free_function(func);
    return -1;
  }

  /* Compile to AOT buffer */
  void *aot_buffer = nil;
  size_t aot_size = 0;
  int ret = fruity_aot_compile(jit_ctx, func, &aot_buffer, &aot_size);

  fruity_free_function(func);
  fruity_jit_destroy(jit_ctx);

  if (ret < 0 || aot_buffer == nil) {
    return -1; /* Compilation failed */
  }

  /* Insert into cache */
  ret = aot_cache_insert(ctx, method->method_token, aot_buffer, aot_size);
  if (ret < 0) {
    xfree(aot_buffer);
    return -1;
  }

  return 0; /* Success */
}
