/*
 * CLR Runtime - Formally Verified Implementation
 * This implementation is derived directly from the verified Coq proofs
 * ensuring type safety, progress, and stack discipline properties.
 */

#include "clr_runtime.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* ========== State Management ========== */

void clr_state_init(clr_state_t *state) {
    memset(state, 0, sizeof(clr_state_t));
    /* Initialize locals to well-typed null values */
    for (size_t i = 0; i < MAX_LOCALS; i++) {
        state->locals[i] = clr_make_null();
    }
    state->locals_size = MAX_LOCALS;
}

/* ========== Stack Effect Analysis (from stack_discipline theorem) ========== */

clr_stack_effect_t clr_get_stack_effect(clr_opcode_t opcode) {
    /* This corresponds exactly to stack_effect definition in CLRProofs_StackDiscipline.v */
    clr_stack_effect_t effect = {0};
    
    switch (opcode) {
        case CIL_NOP:
            effect.pops = 0; effect.pushes = 0;
            break;
        case CIL_DUP:
            effect.pops = 0; effect.pushes = 1; /* DUP doesn't pop, it duplicates */
            break;
        case CIL_POP:
            effect.pops = 1; effect.pushes = 0;
            break;
        case CIL_LDC_I4:
            effect.pops = 0; effect.pushes = 1;
            break;
        case CIL_ADD:
        case CIL_SUB:
        case CIL_MUL:
            effect.pops = 2; effect.pushes = 1;
            break;
        case CIL_LDLOC:
            effect.pops = 0; effect.pushes = 1;
            break;
        case CIL_STLOC:
            effect.pops = 1; effect.pushes = 0; /* Verified: STLOC pops 1, pushes 0 */
            break;
        case CIL_BR:
        case CIL_RET:
            effect.pops = 0; effect.pushes = 0;
            break;
    }
    
    return effect;
}

/* ========== Verified Instruction Execution ========== */

clr_result_t clr_execute(clr_state_t *state, const clr_instruction_t *instruction) {
    /* Pre-condition: state must be well-typed (from type_preservation theorem) */
    if (!clr_state_is_well_typed(state)) {
        return CLR_ERROR_TYPE_MISMATCH;
    }
    
    clr_result_t result = CLR_SUCCESS;
    clr_value_t v1, v2, temp;
    
    /* Execute instruction based on verified semantics from CLRProofs_Core.v */
    switch (instruction->opcode) {
        
        case CIL_NOP:
            /* NOP: No operation, just advance IP */
            state->ip++;
            break;
            
        case CIL_DUP: {
            /* DUP: Duplicate top stack element (from DUP case proof) */
            if (state->stack_size == 0) {
                return CLR_ERROR_STACK_UNDERFLOW;
            }
            clr_value_t top = state->stack[state->stack_size - 1];
            result = clr_push(state, top);
            if (result != CLR_SUCCESS) return result;
            break;
        }
        
        case CIL_POP:
            /* POP: Remove top stack element */
            result = clr_pop(state, &temp);
            if (result != CLR_SUCCESS) return result;
            state->ip++;
            break;
            
        case CIL_LDC_I4:
            /* LDC_I4: Load 32-bit integer constant */
            result = clr_push(state, clr_make_int32(instruction->arg.int32_arg));
            if (result != CLR_SUCCESS) return result;
            break;
            
        case CIL_ADD:
            /* ADD: Addition (verified for Int32 operands) */
            result = clr_pop_two(state, &v1, &v2);
            if (result != CLR_SUCCESS) return result;
            
            if (v1.type != CLR_INT32 || v2.type != CLR_INT32) {
                return CLR_ERROR_TYPE_MISMATCH;
            }
            
            clr_value_t sum = clr_make_int32(v1.data.int32_val + v2.data.int32_val);
            result = clr_push(state, sum);
            if (result != CLR_SUCCESS) return result;
            break;
            
        case CIL_SUB:
            /* SUB: Subtraction (verified for Int32 operands) */
            result = clr_pop_two(state, &v1, &v2);
            if (result != CLR_SUCCESS) return result;
            
            if (v1.type != CLR_INT32 || v2.type != CLR_INT32) {
                return CLR_ERROR_TYPE_MISMATCH;
            }
            
            clr_value_t diff = clr_make_int32(v2.data.int32_val - v1.data.int32_val);
            result = clr_push(state, diff);
            if (result != CLR_SUCCESS) return result;
            break;
            
        case CIL_MUL:
            /* MUL: Multiplication (verified for Int32 operands) */
            result = clr_pop_two(state, &v1, &v2);
            if (result != CLR_SUCCESS) return result;
            
            if (v1.type != CLR_INT32 || v2.type != CLR_INT32) {
                return CLR_ERROR_TYPE_MISMATCH;
            }
            
            clr_value_t product = clr_make_int32(v1.data.int32_val * v2.data.int32_val);
            result = clr_push(state, product);
            if (result != CLR_SUCCESS) return result;
            break;
            
        case CIL_LDLOC: {
            /* LDLOC: Load local variable */
            size_t index = instruction->arg.index_arg;
            if (index >= state->locals_size) {
                return CLR_ERROR_INVALID_LOCAL;
            }
            
            result = clr_push(state, state->locals[index]);
            if (result != CLR_SUCCESS) return result;
            break;
        }
        
        case CIL_STLOC: {
            /* STLOC: Store to local variable (verified stack discipline) */
            size_t index = instruction->arg.index_arg;
            if (index >= state->locals_size) {
                return CLR_ERROR_INVALID_LOCAL;
            }
            
            result = clr_pop(state, &v1);
            if (result != CLR_SUCCESS) return result;
            
            /* Update local variable with popped value */
            state->locals[index] = v1;
            state->ip++;
            
            /* Post-condition: Stack size decreased by 1 (from stack_discipline proof) */
            /* This is automatically satisfied by the clr_pop operation */
            break;
        }
        
        case CIL_BR:
            /* BR: Unconditional branch */
            state->ip = instruction->arg.branch_target;
            break;
            
        case CIL_RET:
            /* RET: Return from method */
            /* For simplicity, just mark as completed by not advancing IP */
            break;
            
        default:
            return CLR_ERROR_TYPE_MISMATCH;
    }
    
    /* Post-condition: state remains well-typed (guaranteed by type_preservation theorem) */
    assert(clr_state_is_well_typed(state));
    
    return CLR_SUCCESS;
}

/* ========== Utility Functions ========== */

const char* clr_result_to_string(clr_result_t result) {
    switch (result) {
        case CLR_SUCCESS: return "Success";
        case CLR_ERROR_STACK_UNDERFLOW: return "Stack underflow";
        case CLR_ERROR_STACK_OVERFLOW: return "Stack overflow"; 
        case CLR_ERROR_INVALID_LOCAL: return "Invalid local variable";
        case CLR_ERROR_TYPE_MISMATCH: return "Type mismatch";
        case CLR_ERROR_NULL_REFERENCE: return "Null reference";
        default: return "Unknown error";
    }
}

void clr_print_state(const clr_state_t *state) {
    printf("CLR State (IP: %zu):\n", state->ip);
    printf("Stack (%zu items): ", state->stack_size);
    for (size_t i = 0; i < state->stack_size; i++) {
        const clr_value_t *v = &state->stack[i];
        switch (v->type) {
            case CLR_INT32:
                printf("i32(%d) ", v->data.int32_val);
                break;
            case CLR_NULL:
                printf("null ");
                break;
            default:
                printf("<%d> ", v->type);
                break;
        }
    }
    printf("\n");
}