/* fruity_interp.h - Fruity IR Interpreter
 *
 * Provides direct interpretation of Fruity IR with full Pebble semantics.
 * This is faster than JIT compilation for small methods and provides
 * a fallback when native code generation is not available.
 */

#ifndef FRUITY_INTERP_H
#define FRUITY_INTERP_H

#include "fruity_ir.h"

/* Maximum stack depth for interpreter */
#define FRUITY_INTERP_MAX_STACK 256

/* Maximum local variables */
#define FRUITY_INTERP_MAX_LOCALS 64

/* Value types for interpreter stack */
typedef enum {
  FVAL_NONE = 0,
  FVAL_I32,      /* 32-bit signed integer */
  FVAL_I64,      /* 64-bit signed integer */
  FVAL_R32,      /* 32-bit float */
  FVAL_R64,      /* 64-bit float */
  FVAL_REF,      /* Object reference (pointer) */
  FVAL_NATIVEINT /* Native int (pointer-sized) */
} fruity_val_type_t;

/* Stack value */
typedef struct {
  fruity_val_type_t type;
  union {
    int32_t i32;
    int64_t i64;
    float r32;
    double r64;
    void *ref;
    intptr_t nint;
  } val;
} fruity_val_t;

/* Interpreter state */
typedef struct {
  /* Evaluation stack */
  fruity_val_t stack[FRUITY_INTERP_MAX_STACK];
  int sp; /* Stack pointer */

  /* Local variables */
  fruity_val_t locals[FRUITY_INTERP_MAX_LOCALS];
  int local_count;

  /* Arguments */
  fruity_val_t *args;
  int arg_count;

  /* Current execution position */
  fruity_basic_block_t *current_block;
  fruity_instruction_t *current_instr;

  /* Function being executed */
  fruity_function_t *func;

  /* Module (for method resolution during CALL) */
  fruity_module_t *module;

  /* Error handling */
  int has_error;
  char error_msg[256];

  /* Execution statistics */
  unsigned long instr_count;
} fruity_interp_state_t;

/*
 * Initialize interpreter state
 */
void fruity_interp_init(fruity_interp_state_t *state);

/*
 * Reset interpreter state for new execution
 */
void fruity_interp_reset(fruity_interp_state_t *state);

/*
 * Execute a Fruity function
 *
 * Parameters:
 *   func      - Function to execute
 *   args      - Array of argument values (can be NULL)
 *   arg_count - Number of arguments
 *   result    - Output result value
 *
 * Returns:
 *   0 on success, negative on error
 */
int fruity_interp_execute(fruity_function_t *func, void **args, int arg_count,
                          void *result);

/*
 * Execute with explicit state (for debugging/stepping)
 */
int fruity_interp_execute_state(fruity_interp_state_t *state,
                                fruity_function_t *func, void **args,
                                int arg_count, fruity_val_t *result);

/*
 * Execute single instruction (for stepping)
 * Returns: 1 = continue, 0 = function returned, -1 = error
 */
int fruity_interp_step(fruity_interp_state_t *state);

/*
 * Get error message from state
 */
const char *fruity_interp_get_error(fruity_interp_state_t *state);

/*
 * Stack manipulation helpers
 */
int fruity_interp_push(fruity_interp_state_t *state, fruity_val_t *val);
int fruity_interp_pop(fruity_interp_state_t *state, fruity_val_t *val);
int fruity_interp_peek(fruity_interp_state_t *state, fruity_val_t *val);

/*
 * Value constructors
 */
fruity_val_t fruity_val_i32(int32_t v);
fruity_val_t fruity_val_i64(int64_t v);
fruity_val_t fruity_val_r32(float v);
fruity_val_t fruity_val_r64(double v);
fruity_val_t fruity_val_ref(void *v);
fruity_val_t fruity_val_null(void);

#endif /* FRUITY_INTERP_H */
