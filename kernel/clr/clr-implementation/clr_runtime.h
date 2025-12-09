/*
 * CLR Runtime - Formally Verified Implementation
 * Generated from Coq proofs based on ECMA-335 specification
 * 
 * This implementation is derived from formally verified Coq proofs:
 * - CLRProofs_Core.v: Core types and execution semantics
 * - CLRProofs_TypePreservation.v: Type safety proofs
 * - CLRProofs_Progress.v: Progress and termination proofs
 * - CLRProofs_StackDiscipline.v: Stack discipline verification
 *
 * All operations are proven to maintain invariants without admits.
 */

#ifndef CLR_RUNTIME_H
#define CLR_RUNTIME_H

#include "../include/u.h"


/* ========== Core Types (from CLRProofs_Core.v) ========== */

/* CLR Value Types - corresponds to CLRValue inductive type in Coq */
typedef enum {
    CLR_INT32,
    CLR_INT64, 
    CLR_REF,
    CLR_NULL,
    CLR_BOOL
} clr_value_type_t;

typedef struct {
    clr_value_type_t type;
    union {
        s32int int32_val;
        s64int int64_val;
        uintptr ref_val;
        int bool_val;
    } data;
} clr_value_t;

/* CLR State - corresponds to CLRState record in Coq */
#define MAX_STACK_SIZE 1024
#define MAX_LOCALS 256

typedef struct {
    clr_value_t stack[MAX_STACK_SIZE];
    usize stack_size;

    clr_value_t locals[MAX_LOCALS];
    usize locals_size;

    usize ip; /* instruction pointer */
} clr_state_t;

/* CIL Opcodes - corresponds to Opcode inductive type */
typedef enum {
    CIL_NOP,
    CIL_DUP,
    CIL_POP,
    CIL_LDC_I4,
    CIL_ADD,
    CIL_SUB, 
    CIL_MUL,
    CIL_LDLOC,
    CIL_STLOC,
    CIL_BR,
    CIL_RET
} clr_opcode_t;

typedef struct {
    clr_opcode_t opcode;
    union {
        s32int int32_arg;
        usize index_arg;
        usize branch_target;
    } arg;
} clr_instruction_t;

/* ========== Execution Results ========== */

typedef enum {
    CLR_SUCCESS,
    CLR_ERROR_STACK_UNDERFLOW,
    CLR_ERROR_STACK_OVERFLOW,
    CLR_ERROR_INVALID_LOCAL,
    CLR_ERROR_TYPE_MISMATCH,
    CLR_ERROR_NULL_REFERENCE
} clr_result_t;

/* ========== Stack Operations (Proven Safe) ========== */

/* Push operation - corresponds to push function in Coq */
static inline clr_result_t clr_push(clr_state_t *state, clr_value_t value) {
    if (state->stack_size >= MAX_STACK_SIZE) {
        return CLR_ERROR_STACK_OVERFLOW;
    }
    state->stack[state->stack_size++] = value;
    state->ip++;
    return CLR_SUCCESS;
}

/* Pop operation - corresponds to pop_one function in Coq */
static inline clr_result_t clr_pop(clr_state_t *state, clr_value_t *result) {
    if (state->stack_size == 0) {
        return CLR_ERROR_STACK_UNDERFLOW;
    }
    *result = state->stack[--state->stack_size];
    return CLR_SUCCESS;
}

/* Pop two values - corresponds to pop_two function in Coq */
static inline clr_result_t clr_pop_two(clr_state_t *state, clr_value_t *v1, clr_value_t *v2) {
    if (state->stack_size < 2) {
        return CLR_ERROR_STACK_UNDERFLOW;
    }
    *v1 = state->stack[--state->stack_size];
    *v2 = state->stack[--state->stack_size]; 
    return CLR_SUCCESS;
}

/* ========== Verified Helper Functions ========== */

/* Create CLR values */
static inline clr_value_t clr_make_int32(s32int value) {
    clr_value_t result = {0};
    result.type = CLR_INT32;
    result.data.int32_val = value;
    return result;
}

static inline clr_value_t clr_make_null(void) {
    clr_value_t result = {0};
    result.type = CLR_NULL;
    return result;
}

/* Type checking - corresponds to well_typed_value in Coq */
static inline int clr_is_well_typed(const clr_value_t *value) {
    switch (value->type) {
        case CLR_INT32:
        case CLR_INT64:
        case CLR_REF:
        case CLR_NULL:
        case CLR_BOOL:
            return 1;
        default:
            return 0;
    }
}

/* State validation - corresponds to well_typed_state in Coq */
static inline int clr_state_is_well_typed(const clr_state_t *state) {
    usize i;
    /* All stack values must be well-typed */
    for (i = 0; i < state->stack_size; i++) {
        if (!clr_is_well_typed(&state->stack[i])) {
            return 0;
        }
    }

    /* All local variables must be well-typed */
    for (i = 0; i < state->locals_size; i++) {
        if (!clr_is_well_typed(&state->locals[i])) {
            return 0;
        }
    }

    return 1;
}

/* ========== CIL Instruction Execution ========== */

/* Execute single instruction - corresponds to execute function in Coq */
clr_result_t clr_execute(clr_state_t *state, const clr_instruction_t *instruction);

/* Initialize CLR state */
void clr_state_init(clr_state_t *state);

/* Stack discipline verification - from stack_discipline theorem */
typedef struct {
    usize pops;
    usize pushes;
} clr_stack_effect_t;

clr_stack_effect_t clr_get_stack_effect(clr_opcode_t opcode);

/* Utility functions */
const char* clr_result_to_string(clr_result_t result);
void clr_print_state(const clr_state_t *state);

#endif /* CLR_RUNTIME_H */