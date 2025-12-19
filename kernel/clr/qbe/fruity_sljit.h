/* fruity_sljit.h - Bridge from Fruity IR to sljit JIT compiler
 *
 * Translates Fruity IR opcodes to sljit operations for native code generation.
 * sljit provides a platform-independent API that generates optimized
 * machine code for x86-64, ARM64, RISC-V, etc.
 */

#ifndef FRUITY_SLJIT_H
#define FRUITY_SLJIT_H

#include "../fruity/fruity_ir.h"
#include "../sljit/sljit_src/sljitLir.h"

/* JIT compilation result */
typedef struct {
  void *code;          /* Executable code pointer */
  size_t code_size;    /* Size of generated code */
  int success;         /* 0 = success, negative = error */
  char error_msg[256]; /* Error message if failed */
} fruity_jit_result_t;

/* JIT context (wraps sljit compiler) */
typedef struct fruity_jit_ctx fruity_jit_ctx_t;

/*
 * Create JIT compilation context
 */
fruity_jit_ctx_t *fruity_jit_create(void);

/*
 * Destroy JIT context
 */
void fruity_jit_destroy(fruity_jit_ctx_t *ctx);

/*
 * Compile a Fruity function to native code
 *
 * Parameters:
 *   ctx    - JIT context
 *   func   - Fruity function to compile
 *   result - Output result (code pointer, size, or error)
 *
 * Returns:
 *   0 on success, negative on error
 */
int fruity_jit_compile(fruity_jit_ctx_t *ctx, fruity_function_t *func,
                       fruity_jit_result_t *result);

/*
 * Execute JIT-compiled code
 *
 * Parameters:
 *   result    - Compilation result containing code pointer
 *   args      - Argument values (passed via registers)
 *   arg_count - Number of arguments
 *   ret_val   - Output return value
 *
 * Returns:
 *   0 on success, negative on error
 */
int fruity_jit_execute(fruity_jit_result_t *result, int64_t *args,
                       int arg_count, int64_t *ret_val);

/*
 * Free compiled code
 */
void fruity_jit_free_code(fruity_jit_result_t *result);

#endif /* FRUITY_SLJIT_H */
