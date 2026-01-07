/* qbe_exec.h - Unified QBE Execution Layer
 *
 * Provides a unified interface for executing CIL/Fruity code via:
 *   - Direct CIL interpretation (fastest startup)
 *   - Fruity IR interpretation (Pebble-aware)
 *   - JIT compilation via QBE (best performance)
 *   - AOT compilation (cached native code)
 */

#ifndef QBE_EXEC_H
#define QBE_EXEC_H

#include "../fruity/fruity_ir.h"
#include "../il_parser.h"

/* Execution modes */
typedef enum {
  QBE_EXEC_INTERPRET_CIL,    /* Direct CIL interpretation */
  QBE_EXEC_INTERPRET_FRUITY, /* Fruity IR interpretation */
  QBE_EXEC_JIT,              /* JIT compile via QBE, then execute */
  QBE_EXEC_AOT               /* Use cached AOT-compiled code */
} qbe_exec_mode_t;

/* Execution result */
typedef struct {
  int success;         /* 0 = success, negative = error */
  int64_t int_result;  /* Integer result (if applicable) */
  void *ref_result;    /* Reference result (if applicable) */
  char error_msg[256]; /* Error message (if any) */
} qbe_exec_result_t;

/* Execution context */
typedef struct qbe_exec_ctx qbe_exec_ctx_t;

/*
 * Create execution context
 * Returns NULL on error
 */
qbe_exec_ctx_t *qbe_exec_create(void);

/*
 * Destroy execution context
 */
void qbe_exec_destroy(qbe_exec_ctx_t *ctx);

/*
 * Execute a method from IL assembly
 *
 * Parameters:
 *   ctx      - Execution context
 *   assembly - IL assembly containing the method
 *   method   - Method to execute
 *   mode     - Execution mode
 *   args     - Array of argument values (can be NULL for no-arg methods)
 *   arg_count- Number of arguments
 *   result   - Output result structure
 *
 * Returns:
 *   0 on success, negative on error
 */
int qbe_exec_method(qbe_exec_ctx_t *ctx, il_assembly_t *assembly,
                    il_method_t *method, qbe_exec_mode_t mode, void **args,
                    int arg_count, qbe_exec_result_t *result);

/*
 * Execute a Fruity function directly
 *
 * Parameters:
 *   ctx      - Execution context
 *   func     - Fruity function to execute
 *   mode     - Must be QBE_EXEC_INTERPRET_FRUITY or QBE_EXEC_JIT
 *   args     - Array of argument values
 *   arg_count- Number of arguments
 *   result   - Output result structure
 *
 * Returns:
 *   0 on success, negative on error
 */
int qbe_exec_fruity(qbe_exec_ctx_t *ctx, fruity_function_t *func,
                    qbe_exec_mode_t mode, void **args, int arg_count,
                    qbe_exec_result_t *result);

/*
 * Get recommended execution mode for a method
 * Based on method complexity, hotness, etc.
 */
qbe_exec_mode_t qbe_exec_recommend_mode(il_method_t *method);

/*
 * Check if method has cached AOT code
 */
int qbe_exec_has_aot_cache(il_method_t *method);

/*
 * Precompile method to AOT cache
 */
int qbe_exec_precompile(qbe_exec_ctx_t *ctx, il_assembly_t *assembly,
                        il_method_t *method);

#endif /* QBE_EXEC_H */
