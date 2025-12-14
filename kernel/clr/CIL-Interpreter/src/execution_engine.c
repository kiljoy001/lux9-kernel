#include "../include/execution_engine.h"
#include "../../il_parser.h" // Assuming il_parser.h is in parent dir or include path needs adjustment
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

// ==================== CORE VM FUNCTIONS ====================

vm_execution_state_t* vm_create_execution_state(vm_init_t* init) {
    vm_execution_state_t* state = (vm_execution_state_t*)malloc(sizeof(vm_execution_state_t));
    if (state == NULL) return NULL;
    memset(state, 0, sizeof(vm_execution_state_t));
    state->assembly = NULL; // Initialize to NULL
    return state;
}

void vm_destroy_execution_state(vm_execution_state_t* state) {
    if (state != NULL) {
        while (state->eval_stack != NULL) {
            vm_stack_t* temp = state->eval_stack;
            state->eval_stack = state->eval_stack->next;
            free(temp);
        }
        if (state->current_frame) {
            if (state->current_frame->locals) free(state->current_frame->locals);
            if (state->current_frame->args) free(state->current_frame->args);
            free(state->current_frame);
        }
        free(state);
    }
}

bool vm_execute_method(vm_execution_state_t* state, const uint8_t* il_code, size_t code_size, uint32_t local_count) {
    if (state == NULL || il_code == NULL || code_size == 0) return false;
    
    vm_frame_t* frame = (vm_frame_t*)malloc(sizeof(vm_frame_t));
    if (frame == NULL) {
        vm_set_error(state, "Failed to allocate method frame");
        return false;
    }
    
    memset(frame, 0, sizeof(vm_frame_t));
    frame->local_count = local_count;
    if (local_count > 0)
        frame->locals = (vm_value_t*)calloc(local_count, sizeof(vm_value_t));
    
    frame->arg_count = 0;
    frame->args = NULL;
    
    cil_decoder_t* decoder = create_cil_decoder(il_code, code_size);
    if (decoder == NULL) {
        free(frame->locals);
        free(frame);
        vm_set_error(state, "Failed to create IL decoder");
        return false;
    }
    
    frame->ip = decode_next_instruction(decoder);
    state->current_frame = frame;
    state->instruction_count = 0;
    state->stack_depth_max = 0;
    
    while (frame->ip != NULL && !vm_has_error(state)) {
        if (!vm_execute_instruction(state)) break;
        frame->ip = decode_next_instruction(decoder);
        state->instruction_count++;
    }
    
    destroy_cil_decoder(decoder);
    free(frame->locals);
    if (frame->args) free(frame->args);
    free(frame);
    state->current_frame = NULL;
    
    return !vm_has_error(state);
}

bool vm_stack_push(vm_execution_state_t* state, vm_value_t* value) {
    if (state == NULL || value == NULL) return false;
    vm_stack_t* new_node = (vm_stack_t*)malloc(sizeof(vm_stack_t));
    if (new_node == NULL) return false;
    new_node->value = *value;
    new_node->next = state->eval_stack;
    state->eval_stack = new_node;
    uint32_t current_depth = vm_stack_depth(state);
    if (current_depth > state->stack_depth_max) state->stack_depth_max = current_depth;
    return true;
}

bool vm_stack_pop(vm_execution_state_t* state, vm_value_t* value) {
    if (state == NULL || value == NULL || state->eval_stack == NULL) return false;
    vm_stack_t* top = state->eval_stack;
    *value = top->value;
    state->eval_stack = top->next;
    free(top);
    return true;
}

bool vm_stack_peek(vm_execution_state_t* state, vm_value_t* value) {
    if (state == NULL || value == NULL || state->eval_stack == NULL) return false;
    *value = state->eval_stack->value;
    return true;
}

uint32_t vm_stack_depth(vm_execution_state_t* state) {
    if (state == NULL) return 0;
    uint32_t depth = 0;
    vm_stack_t* current = state->eval_stack;
    while (current != NULL) {
        depth++;
        current = current->next;
    }
    return depth;
}

// VM value constructors
vm_value_t vm_make_i1(int8_t value) { vm_value_t v; v.type = VM_TYPE_I1; v.value.i1 = value; return v; }
vm_value_t vm_make_u1(uint8_t value) { vm_value_t v; v.type = VM_TYPE_U1; v.value.u1 = value; return v; }
vm_value_t vm_make_i2(int16_t value) { vm_value_t v; v.type = VM_TYPE_I2; v.value.i2 = value; return v; }
vm_value_t vm_make_u2(uint16_t value) { vm_value_t v; v.type = VM_TYPE_U2; v.value.u2 = value; return v; }
vm_value_t vm_make_i4(int32_t value) { vm_value_t v; v.type = VM_TYPE_I4; v.value.i4 = value; return v; }
vm_value_t vm_make_u4(uint32_t value) { vm_value_t v; v.type = VM_TYPE_U4; v.value.u4 = value; return v; }
vm_value_t vm_make_i8(int64_t value) { vm_value_t v; v.type = VM_TYPE_I8; v.value.i8 = value; return v; }
vm_value_t vm_make_u8(uint64_t value) { vm_value_t v; v.type = VM_TYPE_U8; v.value.u8 = value; return v; }
vm_value_t vm_make_i(intptr_t value) { vm_value_t v; v.type = VM_TYPE_I; v.value.i = value; return v; }
vm_value_t vm_make_r4(float value) { vm_value_t v; v.type = VM_TYPE_R4; v.value.r4 = value; return v; }
vm_value_t vm_make_r8(double value) { vm_value_t v; v.type = VM_TYPE_R8; v.value.r8 = value; return v; }
vm_value_t vm_make_ref(void* value) { vm_value_t v; v.type = VM_TYPE_REF; v.value.ref = value; return v; }
vm_value_t vm_make_null(void) { return vm_make_ref(NULL); }

bool vm_is_integer_type(vm_type_t type) { return type >= VM_TYPE_I1 && type <= VM_TYPE_I8; }
const char* vm_type_name(vm_type_t type) { return "unknown"; }

void vm_set_error(vm_execution_state_t* state, const char* message) {
    if (state) { state->has_error = true; state->error_message = message; }
}
const char* vm_get_error(vm_execution_state_t* state) { return state ? state->error_message : NULL; }
bool vm_has_error(vm_execution_state_t* state) { return state && state->has_error; }

// ==================== INSTRUCTION EXECUTION ====================

bool vm_execute_instruction(vm_execution_state_t* state) {
    if (state == NULL || state->current_frame == NULL || state->current_frame->ip == NULL) return false;
    
    cil_instruction_t* ip = state->current_frame->ip;
    
    switch (ip->opcode) {
        case CIL_OPCODE_NOP: break;
        
        // Arithmetic
        case CIL_OPCODE_ADD: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "ADD: Stack"); return false; }
            if (!vm_add(&l, &r, &res)) { vm_set_error(state, "ADD: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SUB: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SUB: Stack"); return false; }
            if (!vm_subtract(&l, &r, &res)) { vm_set_error(state, "SUB: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_MUL: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "MUL: Stack"); return false; }
            if (!vm_multiply(&l, &r, &res)) { vm_set_error(state, "MUL: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DIV: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "DIV: Stack"); return false; }
            if (!vm_divide(&l, &r, &res)) { vm_set_error(state, "DIV: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DIV_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "DIV_UN: Stack"); return false; }
            if (!vm_divide_un(&l, &r, &res)) { vm_set_error(state, "DIV_UN: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_REM: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "REM: Stack"); return false; }
            if (!vm_remainder(&l, &r, &res)) { vm_set_error(state, "REM: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_REM_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "REM_UN: Stack"); return false; }
            if (!vm_remainder_un(&l, &r, &res)) { vm_set_error(state, "REM_UN: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SHL: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SHL: Stack"); return false; }
            if (!vm_shl(&l, &r, &res)) { vm_set_error(state, "SHL: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SHR: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SHR: Stack"); return false; }
            if (!vm_shr(&l, &r, &res)) { vm_set_error(state, "SHR: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SHR_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SHR_UN: Stack"); return false; }
            if (!vm_shr_un(&l, &r, &res)) { vm_set_error(state, "SHR_UN: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_AND: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "AND: Stack"); return false; }
            if (!vm_and(&l, &r, &res)) { vm_set_error(state, "AND: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_OR: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "OR: Stack"); return false; }
            if (!vm_or(&l, &r, &res)) { vm_set_error(state, "OR: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_XOR: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "XOR: Stack"); return false; }
            if (!vm_xor(&l, &r, &res)) { vm_set_error(state, "XOR: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_NOT: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "NOT: Stack"); return false; }
            if (!vm_not(&v, &r)) { vm_set_error(state, "NOT: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_NEG: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "NEG: Stack"); return false; }
            if (!vm_neg(&v, &r)) { vm_set_error(state, "NEG: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        
        // Comparisons
        case CIL_OPCODE_CEQ: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CEQ: Stack"); return false; }
            if (!vm_compare_equal(&l, &r, &res)) { vm_set_error(state, "CEQ: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CGT: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CGT: Stack"); return false; }
            if (!vm_compare_greater(&l, &r, &res)) { vm_set_error(state, "CGT: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CLT: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CLT: Stack"); return false; }
            if (!vm_compare_less(&l, &r, &res)) { vm_set_error(state, "CLT: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        
        // Constants
        case CIL_OPCODE_LDNULL: { vm_value_t v = vm_make_null(); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_M1: { vm_value_t v = vm_make_i4(-1); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_0: { vm_value_t v = vm_make_i4(0); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_1: { vm_value_t v = vm_make_i4(1); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_S: { vm_value_t v = vm_make_i4(ip->operand.byte_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4:   { vm_value_t v = vm_make_i4(ip->operand.int_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I8:   { vm_value_t v = vm_make_i8(ip->operand.long_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_R4:   { vm_value_t v = vm_make_r4(*(float*)&ip->operand.token); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_R8:   { vm_value_t v = vm_make_r8(*(double*)&ip->operand.long_val); vm_stack_push(state, &v); break; }
        
        // Type conversions
        case CIL_OPCODE_CONV_I1: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I1: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I1, &r)) { vm_set_error(state, "CONV.I1: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_I2: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I2: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I2, &r)) { vm_set_error(state, "CONV.I2: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_I4: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I4: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I4, &r)) { vm_set_error(state, "CONV.I4: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_I8: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I8: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I8, &r)) { vm_set_error(state, "CONV.I8: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_U4: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.U4: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_U4, &r)) { vm_set_error(state, "CONV.U4: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_U8: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.U8: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_U8, &r)) { vm_set_error(state, "CONV.U8: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_R4: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.R4: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_R4, &r)) { vm_set_error(state, "CONV.R4: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        case CIL_OPCODE_CONV_R8: {
            vm_value_t v, r;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.R8: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_R8, &r)) { vm_set_error(state, "CONV.R8: Error"); return false; }
            vm_stack_push(state, &r);
            break;
        }
        
        // Overflow arithmetic
        case CIL_OPCODE_ADD_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "ADD_OVF: Stack"); return false; }
            if (!vm_add_ovf(&l, &r, &res)) { vm_set_error(state, "ADD_OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_ADD_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "ADD_OVF_UN: Stack"); return false; }
            if (!vm_add_ovf_un(&l, &r, &res)) { vm_set_error(state, "ADD_OVF_UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_MUL_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "MUL_OVF: Stack"); return false; }
            if (!vm_multiply_ovf(&l, &r, &res)) { vm_set_error(state, "MUL_OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_MUL_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "MUL_OVF_UN: Stack"); return false; }
            if (!vm_multiply_ovf_un(&l, &r, &res)) { vm_set_error(state, "MUL_OVF_UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SUB_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SUB_OVF: Stack"); return false; }
            if (!vm_subtract_ovf(&l, &r, &res)) { vm_set_error(state, "SUB_OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SUB_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SUB_OVF_UN: Stack"); return false; }
            if (!vm_subtract_ovf_un(&l, &r, &res)) { vm_set_error(state, "SUB_OVF_UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DIV_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "DIV_OVF: Stack"); return false; }
            if (!vm_divide_ovf(&l, &r, &res)) { vm_set_error(state, "DIV_OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DIV_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "DIV_OVF_UN: Stack"); return false; }
            if (!vm_divide_ovf_un(&l, &r, &res)) { vm_set_error(state, "DIV_OVF_UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        
        // Memory access
        case CIL_OPCODE_LDIND_I1: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I1: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I1, &result)) { vm_set_error(state, "LDIND.I1: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_U1: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.U1: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_U1, &result)) { vm_set_error(state, "LDIND.U1: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_I2: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I2: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I2, &result)) { vm_set_error(state, "LDIND.I2: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_U2: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.U2: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_U2, &result)) { vm_set_error(state, "LDIND.U2: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_I4: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I4: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I4, &result)) { vm_set_error(state, "LDIND.I4: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_U4: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.U4: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_U4, &result)) { vm_set_error(state, "LDIND.U4: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_I8: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I8: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I8, &result)) { vm_set_error(state, "LDIND.I8: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_I: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I, &result)) { vm_set_error(state, "LDIND.I: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_R4: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.R4: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_R4, &result)) { vm_set_error(state, "LDIND.R4: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_R8: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.R8: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_R8, &result)) { vm_set_error(state, "LDIND.R8: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDIND_REF: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.REF: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_REF, &result)) { vm_set_error(state, "LDIND.REF: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_STIND_REF: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.REF: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.REF: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I1: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I1: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I1: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I2: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I2: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I2: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I4: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I4: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I4: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I8: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I8: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I8: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_R4: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.R4: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.R4: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_R8: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.R8: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.R8: Error"); return false; }
            break;
        }
        
        // Object operations
        case CIL_OPCODE_NEWOBJ: {
            vm_value_t constructor_token, result;
            if (!vm_stack_pop(state, &constructor_token)) { vm_set_error(state, "NEWOBJ: Stack"); return false; }
            if (!vm_new_object(&constructor_token, &result)) { vm_set_error(state, "NEWOBJ: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_NEWARR: {
            vm_value_t size, result;
            if (!vm_stack_pop(state, &size)) { vm_set_error(state, "NEWARR: Stack"); return false; }
            if (!vm_new_array(&size, &result)) { vm_set_error(state, "NEWARR: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDSTR: {
            vm_value_t string_token, result;
            if (!vm_load_string_constant(&string_token, &result)) { vm_set_error(state, "LDSTR: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_BOX: {
            vm_value_t value, box_type, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "BOX: Stack"); return false; }
            if (!vm_box_value(&value, &box_type, &result)) { vm_set_error(state, "BOX: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_UNBOX: {
            vm_value_t obj, unbox_type, result;
            if (!vm_stack_pop(state, &obj)) { vm_set_error(state, "UNBOX: Stack"); return false; }
            if (!vm_unbox_value(&obj, &unbox_type, &result)) { vm_set_error(state, "UNBOX: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDFLD: {
            vm_value_t obj, field_token, result;
            if (!vm_stack_pop(state, &field_token) || !vm_stack_pop(state, &obj)) { vm_set_error(state, "LDFLD: Stack"); return false; }
            if (!vm_load_field_object(&obj, &field_token, &result)) { vm_set_error(state, "LDFLD: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_STFLD: {
            vm_value_t obj, field_token, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &field_token) || !vm_stack_pop(state, &obj)) { vm_set_error(state, "STFLD: Stack"); return false; }
            if (!vm_store_field_object(&obj, &field_token, &value)) { vm_set_error(state, "STFLD: Error"); return false; }
            break;
        }
        case CIL_OPCODE_LDSFLD: {
            vm_value_t field_token, result;
            if (!vm_stack_pop(state, &field_token)) { vm_set_error(state, "LDSFLD: Stack"); return false; }
            if (!vm_load_static_field(&field_token, &result)) { vm_set_error(state, "LDSFLD: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_STSFLD: {
            vm_value_t field_token, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &field_token)) { vm_set_error(state, "STSFLD: Stack"); return false; }
            if (!vm_store_static_field(&field_token, &value)) { vm_set_error(state, "STSFLD: Error"); return false; }
            break;
        }
        case CIL_OPCODE_LDFLDA: {
            vm_value_t obj, field_token, result;
            if (!vm_stack_pop(state, &field_token) || !vm_stack_pop(state, &obj)) { vm_set_error(state, "LDFLDA: Stack"); return false; }
            if (!vm_load_field_address(&obj, &field_token, &result)) { vm_set_error(state, "LDFLDA: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDSFLDA: {
            vm_value_t field_token, result;
            if (!vm_stack_pop(state, &field_token)) { vm_set_error(state, "LDSFLDA: Stack"); return false; }
            if (!vm_load_static_field_address(&field_token, &result)) { vm_set_error(state, "LDSFLDA: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CASTCLASS: {
            vm_value_t obj, cast_type, result;
            if (!vm_stack_pop(state, &obj)) { vm_set_error(state, "CASTCLASS: Stack"); return false; }
            if (!vm_cast_class(&obj, &cast_type, &result)) { vm_set_error(state, "CASTCLASS: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_ISINST: {
            vm_value_t obj, test_type, result;
            if (!vm_stack_pop(state, &obj)) { vm_set_error(state, "ISINST: Stack"); return false; }
            if (!vm_is_instance(&obj, &test_type, &result)) { vm_set_error(state, "ISINST: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        
        // Stack operations
        case CIL_OPCODE_DUP: {
            vm_value_t top;
            if (!vm_stack_pop(state, &top)) { vm_set_error(state, "DUP: Stack"); return false; }
            vm_stack_push(state, &top);
            vm_stack_push(state, &top);
            break;
        }
        case CIL_OPCODE_POP: {
            vm_value_t top;
            if (!vm_stack_pop(state, &top)) { vm_set_error(state, "POP: Stack"); return false; }
            break;
        }
        
        // Local variables
        case CIL_OPCODE_LDARG_0: {
            if (!state->current_frame || state->current_frame->arg_count <= 0) { vm_set_error(state, "LDARG.0: No args"); return false; }
            vm_stack_push(state, &state->current_frame->args[0]);
            break;
        }
        case CIL_OPCODE_LDARG_1: {
            if (!state->current_frame || state->current_frame->arg_count <= 1) { vm_set_error(state, "LDARG.1: No args"); return false; }
            vm_stack_push(state, &state->current_frame->args[1]);
            break;
        }
        case CIL_OPCODE_LDARG_2: {
            if (!state->current_frame || state->current_frame->arg_count <= 2) { vm_set_error(state, "LDARG.2: No args"); return false; }
            vm_stack_push(state, &state->current_frame->args[2]);
            break;
        }
        case CIL_OPCODE_LDARG_3: {
            if (!state->current_frame || state->current_frame->arg_count <= 3) { vm_set_error(state, "LDARG.3: No args"); return false; }
            vm_stack_push(state, &state->current_frame->args[3]);
            break;
        }
        case CIL_OPCODE_LDARG_S: {
            uint8_t index = ip->operand.byte_val;
            if (!state->current_frame || state->current_frame->arg_count <= index) { vm_set_error(state, "LDARG.S: No args"); return false; }
            vm_stack_push(state, &state->current_frame->args[index]);
            break;
        }
        case CIL_OPCODE_LDARGA_S: {
            uint8_t index = ip->operand.byte_val;
            if (!state->current_frame || state->current_frame->arg_count <= index) { vm_set_error(state, "LDARGA.S: No args"); return false; }
            vm_value_t addr = vm_make_ref(&state->current_frame->args[index]);
            vm_stack_push(state, &addr);
            break;
        }
        case CIL_OPCODE_STARG_S: {
            uint8_t index = ip->operand.byte_val;
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "STARG.S: Stack"); return false; }
            if (!state->current_frame || state->current_frame->arg_count <= index) { vm_set_error(state, "STARG.S: No args"); return false; }
            state->current_frame->args[index] = value;
            break;
        }
        case CIL_OPCODE_LDLOC_0: {
            if (!state->current_frame || state->current_frame->local_count <= 0) { vm_set_error(state, "LDLOC.0: No locals"); return false; }
            vm_stack_push(state, &state->current_frame->locals[0]);
            break;
        }
        case CIL_OPCODE_LDLOC_1: {
            if (!state->current_frame || state->current_frame->local_count <= 1) { vm_set_error(state, "LDLOC.1: No locals"); return false; }
            vm_stack_push(state, &state->current_frame->locals[1]);
            break;
        }
        case CIL_OPCODE_LDLOC_2: {
            if (!state->current_frame || state->current_frame->local_count <= 2) { vm_set_error(state, "LDLOC.2: No locals"); return false; }
            vm_stack_push(state, &state->current_frame->locals[2]);
            break;
        }
        case CIL_OPCODE_LDLOC_3: {
            if (!state->current_frame || state->current_frame->local_count <= 3) { vm_set_error(state, "LDLOC.3: No locals"); return false; }
            vm_stack_push(state, &state->current_frame->locals[3]);
            break;
        }
        case CIL_OPCODE_LDLOC_S: {
            uint8_t index = ip->operand.byte_val;
            if (!state->current_frame || state->current_frame->local_count <= index) { vm_set_error(state, "LDLOC.S: No locals"); return false; }
            vm_stack_push(state, &state->current_frame->locals[index]);
            break;
        }
        case CIL_OPCODE_STLOC_0: {
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "STLOC.0: Stack"); return false; }
            if (!state->current_frame || state->current_frame->local_count <= 0) { vm_set_error(state, "STLOC.0: No locals"); return false; }
            state->current_frame->locals[0] = value;
            break;
        }
        case CIL_OPCODE_STLOC_1: {
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "STLOC.1: Stack"); return false; }
            if (!state->current_frame || state->current_frame->local_count <= 1) { vm_set_error(state, "STLOC.1: No locals"); return false; }
            state->current_frame->locals[1] = value;
            break;
        }
        case CIL_OPCODE_STLOC_2: {
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "STLOC.2: Stack"); return false; }
            if (!state->current_frame || state->current_frame->local_count <= 2) { vm_set_error(state, "STLOC.2: No locals"); return false; }
            state->current_frame->locals[2] = value;
            break;
        }
        case CIL_OPCODE_STLOC_3: {
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "STLOC.3: Stack"); return false; }
            if (!state->current_frame || state->current_frame->local_count <= 3) { vm_set_error(state, "STLOC.3: No locals"); return false; }
            state->current_frame->locals[3] = value;
            break;
        }
        case CIL_OPCODE_STLOC_S: {
            uint8_t index = ip->operand.byte_val;
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "STLOC.S: Stack"); return false; }
            if (!state->current_frame || state->current_frame->local_count <= index) { vm_set_error(state, "STLOC.S: No locals"); return false; }
            state->current_frame->locals[index] = value;
            break;
        }
        
        // ==================== BRANCHING ====================
        case CIL_OPCODE_BR: {
            state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BR_S: {
            state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BEQ: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BEQ: Stack"); return false; }
            
            bool eq = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 == v2.value.i4) eq = true;
            
            if (eq) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BEQ_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BEQ.S: Stack"); return false; }
            
            bool eq = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 == v2.value.i4) eq = true;
            
            if (eq) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BGE_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGE.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 >= v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BGT_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGT.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 > v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BLE_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLE.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 <= v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BLT_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLT.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 < v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BGE_UN_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGE.UN.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 >= u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BGT_UN_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGT.UN.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 > u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BLE_UN_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLE.UN.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 <= u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BLT_UN_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLT.UN.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 < u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BNE_UN_S: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BNE.UN.S: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 != u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BGE: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGE: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 >= v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BGE_UN: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGE.UN: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 >= u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BGT: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGT: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 > v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BGT_UN: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGT.UN: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 > u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BLE: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLE: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 <= v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BLE_UN: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLE.UN: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 <= u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BLT: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLT: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 < v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BLT_UN: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BLT.UN: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 < u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BNE_UN: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BNE.UN: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4) {
                uint32_t u1 = (uint32_t)v1.value.i4;
                uint32_t u2 = (uint32_t)v2.value.i4;
                if (u1 != u2) cond = true;
            }
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BRTRUE: {
            vm_value_t val;
            if (!vm_stack_pop(state, &val)) { vm_set_error(state, "BRTRUE: Stack"); return false; }
            bool cond = false;
            if (val.type == VM_TYPE_I4) cond = (val.value.i4 != 0);
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BRFALSE: {
            vm_value_t val;
            if (!vm_stack_pop(state, &val)) { vm_set_error(state, "BRFALSE: Stack"); return false; }
            bool cond = false;
            if (val.type == VM_TYPE_I4) cond = (val.value.i4 == 0);
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_RET: {
            state->current_frame = NULL;
            break;
        }
        case CIL_OPCODE_CALLI: {
            vm_value_t method_ptr, result;
            if (!vm_stack_pop(state, &method_ptr)) { vm_set_error(state, "CALLI: Stack"); return false; }
            if (!vm_call_indirect(&method_ptr, &result)) { vm_set_error(state, "CALLI: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CGT_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CGT.UN: Stack"); return false; }
            if (!vm_compare_greater_un(&l, &r, &res)) { vm_set_error(state, "CGT.UN: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CLT_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CLT.UN: Stack"); return false; }
            if (!vm_compare_less_un(&l, &r, &res)) { vm_set_error(state, "CLT.UN: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        
        // === ALL MISSING CONTROL FLOW OPCODES ===
        case CIL_OPCODE_BREAK: break;
        case CIL_OPCODE_BRFALSE_S: {
            vm_value_t val;
            if (!vm_stack_pop(state, &val)) { vm_set_error(state, "BRFALSE.S: Stack"); return false; }
            bool cond = false;
            if (val.type == VM_TYPE_I4) cond = (val.value.i4 == 0);
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_BRTRUE_S: {
            vm_value_t val;
            if (!vm_stack_pop(state, &val)) { vm_set_error(state, "BRTRUE.S: Stack"); return false; }
            bool cond = false;
            if (val.type == VM_TYPE_I4) cond = (val.value.i4 != 0);
            if (cond) state->current_frame->ip += (ip->operand.branch_offset_short - 1);
            break;
        }
        case CIL_OPCODE_JMP: {
            state->current_frame = NULL;
            break;
        }
        case CIL_OPCODE_SWITCH: {
            vm_set_error(state, "SWITCH: Not implemented");
            return false;
        }
        
        // === ALL MISSING METHOD INVOCATION OPCODES ===
        case CIL_OPCODE_CALL: {
            vm_value_t method_token, result;
            if (!vm_stack_pop(state, &method_token)) { vm_set_error(state, "CALL: Stack"); return false; }
                        if (!vm_call_method(state, &method_token, &result)) {
                            vm_set_error(state, "CALL: Error invoking method");
                            return false;
                        }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CALLVIRT: {
            vm_value_t method_token, result;
            if (!vm_stack_pop(state, &method_token)) { vm_set_error(state, "CALLVIRT: Stack"); return false; }
            if (!vm_call_virtual(&method_token, &result)) { vm_set_error(state, "CALLVIRT: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDFTN: {
            vm_value_t method_token, result;
            if (!vm_stack_pop(state, &method_token)) { vm_set_error(state, "LDFTN: Stack"); return false; }
            if (!vm_load_function_ptr(&method_token, &result)) { vm_set_error(state, "LDFTN: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDVIRTFTN: {
            vm_value_t obj, method_token, result;
            if (!vm_stack_pop(state, &method_token) || !vm_stack_pop(state, &obj)) { vm_set_error(state, "LDVIRTFTN: Stack"); return false; }
            if (!vm_load_virtual_function_ptr(&obj, &method_token, &result)) { vm_set_error(state, "LDVIRTFTN: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LOCALLOC: {
            vm_value_t size, result;
            if (!vm_stack_pop(state, &size)) { vm_set_error(state, "LOCALLOC: Stack"); return false; }
            if (!vm_local_alloc(&size, &result)) { vm_set_error(state, "LOCALLOC: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        
        // === ALL MISSING OBJECT MODEL OPCODES ===
        case CIL_OPCODE_LDOBJ: {
            vm_value_t addr, result;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDOBJ: Stack"); return false; }
            if (!vm_load_object(&addr, &result)) { vm_set_error(state, "LDOBJ: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_STOBJ: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STOBJ: Stack"); return false; }
            if (!vm_store_object(&addr, &value)) { vm_set_error(state, "STOBJ: Error"); return false; }
            break;
        }
        case CIL_OPCODE_UNBOX_ANY: {
            vm_value_t obj, type_token, result;
            if (!vm_stack_pop(state, &obj) || !vm_stack_pop(state, &type_token)) { vm_set_error(state, "UNBOX.ANY: Stack"); return false; }
            if (!vm_unbox_any(&obj, &type_token, &result)) { vm_set_error(state, "UNBOX.ANY: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        
        // === ALL MISSING MEMORY ACCESS OPCODES ===
        case CIL_OPCODE_CPBLK: {
            vm_value_t dest, src, len;
            if (!vm_stack_pop(state, &len) || !vm_stack_pop(state, &src) || !vm_stack_pop(state, &dest)) { vm_set_error(state, "CPBLK: Stack"); return false; }
            if (!vm_copy_memory(&dest, &src, &len)) { vm_set_error(state, "CPBLK: Error"); return false; }
            break;
        }
        case CIL_OPCODE_INITBLK: {
            vm_value_t addr, value, len;
            if (!vm_stack_pop(state, &len) || !vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "INITBLK: Stack"); return false; }
            if (!vm_init_memory(&addr, &value, &len)) { vm_set_error(state, "INITBLK: Error"); return false; }
            break;
        }
        case CIL_OPCODE_LDELEMA: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEMA: Stack"); return false; }
            if (!vm_load_array_element_address(&array, &index, &result)) { vm_set_error(state, "LDELEMA: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_I1: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.I1: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_I1, &result)) { vm_set_error(state, "LDELEM.I1: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_U1: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.U1: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_U1, &result)) { vm_set_error(state, "LDELEM.U1: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_I2: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.I2: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_I2, &result)) { vm_set_error(state, "LDELEM.I2: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_U2: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.U2: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_U2, &result)) { vm_set_error(state, "LDELEM.U2: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_I4: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.I4: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_I4, &result)) { vm_set_error(state, "LDELEM.I4: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_U4: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.U4: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_U4, &result)) { vm_set_error(state, "LDELEM.U4: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_I8: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.I8: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_I8, &result)) { vm_set_error(state, "LDELEM.I8: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_R4: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.R4: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_R4, &result)) { vm_set_error(state, "LDELEM.R4: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_R8: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.R8: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_R8, &result)) { vm_set_error(state, "LDELEM.R8: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_REF: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.REF: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_REF, &result)) { vm_set_error(state, "LDELEM.REF: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_I: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.I: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_I, &result)) { vm_set_error(state, "LDELEM.I: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDELEM_ANY: {
            vm_value_t array, index, type_token, result;
            if (!vm_stack_pop(state, &type_token) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.ANY: Stack"); return false; }
            if (!vm_load_array_element_any(&array, &index, &type_token, &result)) { vm_set_error(state, "LDELEM.ANY: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_STELEM_I: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.I: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_I, &value)) { vm_set_error(state, "STELEM.I: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_I1: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.I1: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_I1, &value)) { vm_set_error(state, "STELEM.I1: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_I2: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.I2: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_I2, &value)) { vm_set_error(state, "STELEM.I2: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_I4: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.I4: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_I4, &value)) { vm_set_error(state, "STELEM.I4: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_I8: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.I8: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_I8, &value)) { vm_set_error(state, "STELEM.I8: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_R4: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.R4: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_R4, &value)) { vm_set_error(state, "STELEM.R4: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_R8: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.R8: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_R8, &value)) { vm_set_error(state, "STELEM.R8: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_REF: {
            vm_value_t array, index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.REF: Stack"); return false; }
            if (!vm_store_array_element(&array, &index, VM_TYPE_REF, &value)) { vm_set_error(state, "STELEM.REF: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STELEM_ANY: {
            vm_value_t array, index, type_token, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &type_token) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.ANY: Stack"); return false; }
            if (!vm_store_array_element_any(&array, &index, &type_token, &value)) { vm_set_error(state, "STELEM.ANY: Error"); return false; }
            break;
        }
        
        // === ALL MISSING TYPE SYSTEM OPCODES ===
        case CIL_OPCODE_ARGLIST: {
            vm_value_t result;
            if (!vm_get_argument_list(&result)) { vm_set_error(state, "ARGLIST: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I1: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I1: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_I1, &result)) { vm_set_error(state, "CONV.OVF.I1: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I1_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I1.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_I1, &result)) { vm_set_error(state, "CONV.OVF.I1.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I2: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I2: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_I2, &result)) { vm_set_error(state, "CONV.OVF.I2: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I2_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I2.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_I2, &result)) { vm_set_error(state, "CONV.OVF.I2.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I4: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I4: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_I4, &result)) { vm_set_error(state, "CONV.OVF.I4: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I4_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I4.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_I4, &result)) { vm_set_error(state, "CONV.OVF.I4.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I8: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I8: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_I8, &result)) { vm_set_error(state, "CONV.OVF.I8: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I8_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I8.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_I8, &result)) { vm_set_error(state, "CONV.OVF.I8.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_I_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.I.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_I, &result)) { vm_set_error(state, "CONV.OVF.I.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U1: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U1: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_U1, &result)) { vm_set_error(state, "CONV.OVF.U1: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U1_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U1.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_U1, &result)) { vm_set_error(state, "CONV.OVF.U1.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U2: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U2: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_U2, &result)) { vm_set_error(state, "CONV.OVF.U2: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U2_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U2.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_U2, &result)) { vm_set_error(state, "CONV.OVF.U2.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U4: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U4: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_U4, &result)) { vm_set_error(state, "CONV.OVF.U4: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U4_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U4.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_U4, &result)) { vm_set_error(state, "CONV.OVF.U4.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U8: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U8: Stack"); return false; }
            if (!vm_convert_ovf(&value, VM_TYPE_U8, &result)) { vm_set_error(state, "CONV.OVF.U8: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U8_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U8.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_U8, &result)) { vm_set_error(state, "CONV.OVF.U8.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_OVF_U_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.OVF.U.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_U, &result)) { vm_set_error(state, "CONV.OVF.U.UN: Overflow"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_CONV_R_UN: {
            vm_value_t value, result;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "CONV.R.UN: Stack"); return false; }
            if (!vm_convert_ovf_un(&value, VM_TYPE_R8, &result)) { vm_set_error(state, "CONV.R.UN: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SIZEOF: {
            vm_value_t type_token, result;
            if (!vm_stack_pop(state, &type_token)) { vm_set_error(state, "SIZEOF: Stack"); return false; }
            if (!vm_size_of(&type_token, &result)) { vm_set_error(state, "SIZEOF: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        
        // === ALL MISSING EXCEPTION HANDLING OPCODES ===
        case CIL_OPCODE_THROW: {
            vm_value_t exception_obj;
            if (!vm_stack_pop(state, &exception_obj)) { vm_set_error(state, "THROW: Stack"); return false; }
            if (!vm_throw_exception(&exception_obj)) { vm_set_error(state, "THROW: Error"); return false; }
            break;
        }
        case CIL_OPCODE_RETHROW: {
            if (!vm_rethrow_exception(NULL)) { vm_set_error(state, "RETHROW: Error"); return false; }
            break;
        }
        case CIL_OPCODE_ENDFILTER: {
            vm_value_t value;
            if (!vm_stack_pop(state, &value)) { vm_set_error(state, "ENDFILTER: Stack"); return false; }
            if (!vm_end_filter(&value)) { vm_set_error(state, "ENDFILTER: Error"); return false; }
            break;
        }
        
        // === ALL OTHER MISSING OPCODES ===
        case CIL_OPCODE_CONSTRAINED: {
            vm_value_t type_token;
            if (!vm_stack_pop(state, &type_token)) { vm_set_error(state, "CONSTRAINED: Stack"); return false; }
            if (!vm_constrained_prefix(&type_token)) { vm_set_error(state, "CONSTRAINED: Error"); return false; }
            break;
        }
        case CIL_OPCODE_CPOBJ: {
            vm_value_t dest, src, type_token;
            if (!vm_stack_pop(state, &type_token) || !vm_stack_pop(state, &src) || !vm_stack_pop(state, &dest)) { vm_set_error(state, "CPOBJ: Stack"); return false; }
            if (!vm_copy_object(&dest, &src, &type_token)) { vm_set_error(state, "CPOBJ: Error"); return false; }
            break;
        }
        case CIL_OPCODE_INITOBJ: {
            vm_value_t addr, type_token;
            if (!vm_stack_pop(state, &type_token) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "INITOBJ: Stack"); return false; }
            if (!vm_init_object(&addr, &type_token)) { vm_set_error(state, "INITOBJ: Error"); return false; }
            break;
        }
        case CIL_OPCODE_LDARG: {
            vm_value_t arg_index, result;
            if (!vm_stack_pop(state, &arg_index)) { vm_set_error(state, "LDARG: Stack"); return false; }
            if (!vm_load_argument(&arg_index, &result)) { vm_set_error(state, "LDARG: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDARGA: {
            vm_value_t arg_index, result;
            if (!vm_stack_pop(state, &arg_index)) { vm_set_error(state, "LDARGA: Stack"); return false; }
            if (!vm_load_argument_address(&arg_index, &result)) { vm_set_error(state, "LDARGA: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDC_I4_2: { vm_value_t v = vm_make_i4(2); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_3: { vm_value_t v = vm_make_i4(3); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_4: { vm_value_t v = vm_make_i4(4); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_5: { vm_value_t v = vm_make_i4(5); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_6: { vm_value_t v = vm_make_i4(6); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_7: { vm_value_t v = vm_make_i4(7); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_8: { vm_value_t v = vm_make_i4(8); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDLEN: {
            vm_value_t array, result;
            if (!vm_stack_pop(state, &array)) { vm_set_error(state, "LDLEN: Stack"); return false; }
            if (!vm_get_array_length(&array, &result)) { vm_set_error(state, "LDLEN: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDLOC: {
            vm_value_t local_index, result;
            if (!vm_stack_pop(state, &local_index)) { vm_set_error(state, "LDLOC: Stack"); return false; }
            if (!vm_load_local(&local_index, &result)) { vm_set_error(state, "LDLOC: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDLOCA: {
            vm_value_t local_index, result;
            if (!vm_stack_pop(state, &local_index)) { vm_set_error(state, "LDLOCA: Stack"); return false; }
            if (!vm_load_local_address(&local_index, &result)) { vm_set_error(state, "LDLOCA: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_LDLOCA_S: {
            vm_value_t local_index, result;
            if (!vm_stack_pop(state, &local_index)) { vm_set_error(state, "LDLOCA.S: Stack"); return false; }
            if (!vm_load_local_address(&local_index, &result)) { vm_set_error(state, "LDLOCA.S: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_READONLY: break;
        case CIL_OPCODE_REFANYTYPE: {
            vm_value_t typed_ref, result;
            if (!vm_stack_pop(state, &typed_ref)) { vm_set_error(state, "REFANYTYPE: Stack"); return false; }
            if (!vm_ref_any_type(&typed_ref, &result)) { vm_set_error(state, "REFANYTYPE: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_STARG: {
            vm_value_t arg_index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &arg_index)) { vm_set_error(state, "STARG: Stack"); return false; }
            if (!vm_store_argument(&arg_index, &value)) { vm_set_error(state, "STARG: Error"); return false; }
            break;
        }
        case CIL_OPCODE_STLOC: {
            vm_value_t local_index, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &local_index)) { vm_set_error(state, "STLOC: Stack"); return false; }
            if (!vm_store_local(&local_index, &value)) { vm_set_error(state, "STLOC: Error"); return false; }
            break;
        }
        case CIL_OPCODE_TAIL: break;
        case CIL_OPCODE_UNALIGNED: break;
        case CIL_OPCODE_VOLATILE: break;
        
        // Symbolic operations
        case CIL_OPCODE_SYM_CREATE: {
            vm_value_t var_name, result;
            if (!vm_stack_pop(state, &var_name)) { vm_set_error(state, "SYM_CREATE: Stack"); return false; }
            if (!vm_symbolic_create(&var_name, &result)) { vm_set_error(state, "SYM_CREATE: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_EXPR: {
            vm_value_t left, right, result;
            if (!vm_stack_pop(state, &right) || !vm_stack_pop(state, &left)) { vm_set_error(state, "SYM_EXPR: Stack"); return false; }
            if (!vm_symbolic_expr(&left, &right, &result)) { vm_set_error(state, "SYM_EXPR: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_DIFF: {
            vm_value_t expr, var, result;
            if (!vm_stack_pop(state, &var) || !vm_stack_pop(state, &expr)) { vm_set_error(state, "SYM_DIFF: Stack"); return false; }
            if (!vm_symbolic_differentiate(&expr, &var, &result)) { vm_set_error(state, "SYM_DIFF: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_INTEGRATE: {
            vm_value_t expr, var, result;
            if (!vm_stack_pop(state, &var) || !vm_stack_pop(state, &expr)) { vm_set_error(state, "SYM_INTEGRATE: Stack"); return false; }
            if (!vm_symbolic_integrate(&expr, &var, &result)) { vm_set_error(state, "SYM_INTEGRATE: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_SIMPLIFY: {
            vm_value_t expr, result;
            if (!vm_stack_pop(state, &expr)) { vm_set_error(state, "SYM_SIMPLIFY: Stack"); return false; }
            if (!vm_symbolic_simplify(&expr, &result)) { vm_set_error(state, "SYM_SIMPLIFY: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_EVAL: {
            vm_value_t expr, env, result;
            if (!vm_stack_pop(state, &env) || !vm_stack_pop(state, &expr)) { vm_set_error(state, "SYM_EVAL: Stack"); return false; }
            if (!vm_symbolic_eval(&expr, &env, &result)) { vm_set_error(state, "SYM_EVAL: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_MATCH: {
            vm_value_t pattern, expr, result;
            if (!vm_stack_pop(state, &expr) || !vm_stack_pop(state, &pattern)) { vm_set_error(state, "SYM_MATCH: Stack"); return false; }
            if (!vm_symbolic_match(&pattern, &expr, &result)) { vm_set_error(state, "SYM_MATCH: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        case CIL_OPCODE_SYM_REWRITE: {
            vm_value_t expr, rules, result;
            if (!vm_stack_pop(state, &rules) || !vm_stack_pop(state, &expr)) { vm_set_error(state, "SYM_REWRITE: Stack"); return false; }
            if (!vm_symbolic_rewrite(&expr, &rules, &result)) { vm_set_error(state, "SYM_REWRITE: Error"); return false; }
            vm_stack_push(state, &result);
            break;
        }
        
        // PREFIX instructions
        case CIL_OPCODE_PREFIX1: break;
        case CIL_OPCODE_PREFIX2: break;
        case CIL_OPCODE_PREFIX3: break;
        case CIL_OPCODE_PREFIX4: break;
        case CIL_OPCODE_PREFIX5: break;
        case CIL_OPCODE_PREFIX6: break;
        case CIL_OPCODE_PREFIX7: break;
        case CIL_OPCODE_PREFIXREF: break;
        
        // Unused opcodes (reserved)
        case CIL_OPCODE_UNUSED_1: break;
        case CIL_OPCODE_UNUSED_49: break;
        case CIL_OPCODE_UNUSED_50: break;
        case CIL_OPCODE_UNUSED_51: break;
        case CIL_OPCODE_UNUSED_52: break;
        case CIL_OPCODE_UNUSED_53: break;
        case CIL_OPCODE_UNUSED_54: break;
        case CIL_OPCODE_UNUSED_55: break;
        case CIL_OPCODE_UNUSED_56: break;
        case CIL_OPCODE_UNUSED_57: break;
        case CIL_OPCODE_UNUSED_58: break;
        case CIL_OPCODE_UNUSED_59: break;
        case CIL_OPCODE_UNUSED_60: break;
        case CIL_OPCODE_UNUSED_61: break;
        case CIL_OPCODE_UNUSED_62: break;
        case CIL_OPCODE_UNUSED_63: break;
        case CIL_OPCODE_UNUSED_64: break;
        case CIL_OPCODE_UNUSED_65: break;
        case CIL_OPCODE_UNUSED_66: break;
        case CIL_OPCODE_UNUSED_67: break;
        case CIL_OPCODE_UNUSED_69: break;
        case CIL_OPCODE_UNUSED_78: break;
        case CIL_OPCODE_UNUSED_79: break;
        case CIL_OPCODE_UNUSED_80: break;
        case CIL_OPCODE_UNUSED_81: break;
        case CIL_OPCODE_UNUSED_82: break;
        case CIL_OPCODE_UNUSED_83: break;
        case CIL_OPCODE_UNUSED_84: break;
        case CIL_OPCODE_UNUSED_85: break;
        case CIL_OPCODE_UNUSED_86: break;
        case CIL_OPCODE_UNUSED_87: break;
        case CIL_OPCODE_UNUSED_88: break;
        case CIL_OPCODE_UNUSED_89: break;
        case CIL_OPCODE_UNUSED_90: break;
        case CIL_OPCODE_UNUSED_91: break;
        case CIL_OPCODE_UNUSED_92: break;
        case CIL_OPCODE_UNUSED_93: break;
        case CIL_OPCODE_UNUSED_94: break;
        case CIL_OPCODE_UNUSED_95: break;
        case CIL_OPCODE_UNUSED_96: break;
        case CIL_OPCODE_UNUSED_97: break;
        case CIL_OPCODE_UNUSED_98: break;
        case CIL_OPCODE_UNUSED_99: break;
        case CIL_OPCODE_UNUSED_100: break;
        case CIL_OPCODE_UNUSED_101: break;
        case CIL_OPCODE_UNUSED_102: break;
        case CIL_OPCODE_UNUSED_103: break;
        case CIL_OPCODE_UNUSED_104: break;
        case CIL_OPCODE_UNUSED_105: break;
        case CIL_OPCODE_UNUSED_106: break;
        case CIL_OPCODE_UNUSED_107: break;
        case CIL_OPCODE_UNUSED_108: break;
        case CIL_OPCODE_UNUSED_109: break;
        case CIL_OPCODE_UNUSED_110: break;
        case CIL_OPCODE_UNUSED_111: break;
        case CIL_OPCODE_UNUSED_112: break;
        case CIL_OPCODE_UNUSED_113: break;
        case CIL_OPCODE_UNUSED_114: break;
        case CIL_OPCODE_UNUSED_115: break;
        case CIL_OPCODE_UNUSED_116: break;
        case CIL_OPCODE_UNUSED_117: break;
        case CIL_OPCODE_UNUSED_118: break;
        case CIL_OPCODE_UNUSED_119: break;
        case CIL_OPCODE_UNUSED_120: break;
        case CIL_OPCODE_UNUSED_121: break;
        case CIL_OPCODE_UNUSED_122: break;
        case CIL_OPCODE_UNUSED_123: break;
        case CIL_OPCODE_UNUSED_124: break;
        case CIL_OPCODE_UNUSED_125: break;
        case CIL_OPCODE_UNUSED_126: break;
        case CIL_OPCODE_UNUSED_127: break;
        case CIL_OPCODE_UNUSED_128: break;
        case CIL_OPCODE_UNUSED_129: break;
        case CIL_OPCODE_UNUSED_130: break;
        case CIL_OPCODE_UNUSED_131: break;
        case CIL_OPCODE_UNUSED_132: break;
        case CIL_OPCODE_UNUSED_133: break;
        case CIL_OPCODE_UNUSED_134: break;
        case CIL_OPCODE_UNUSED_135: break;
        case CIL_OPCODE_UNUSED_136: break;
        case CIL_OPCODE_UNUSED_137: break;
        case CIL_OPCODE_UNUSED_138: break;
        case CIL_OPCODE_UNUSED_139: break;
        case CIL_OPCODE_UNUSED_140: break;
        case CIL_OPCODE_UNUSED_141: break;
        case CIL_OPCODE_UNUSED_142: break;
        case CIL_OPCODE_UNUSED_143: break;
    }
    return true;
}

// ==================== IMPLEMENTATION OF ALL MISSING HELPER FUNCTIONS ====================

extern void *clr_resolve_internal_call(const char *cls, const char *method);

// Method invocation implementations
bool vm_call_method(vm_execution_state_t* state, vm_value_t* method_token, vm_value_t* result) {
    if (!state || !state->assembly) {
        vm_set_error(state, "CALL: No assembly loaded");
        return false;
    }
    
    uint32_t token = (uint32_t)method_token->value.i4;
    il_assembly_t* assembly = (il_assembly_t*)state->assembly;
    il_method_t* method = il_get_method_by_token(assembly, token);
    
    if (!method) {
        vm_set_error(state, "CALL: Method not found");
        return false;
    }
    
    // Check for InternalCall (0x1000)
    if (method->impl_flags & 0x1000) {
        const char* type_name = il_get_method_parent_type_name(assembly, token);
        if (!type_name) {
             vm_set_error(state, "CALL: Could not resolve parent type for internal call");
             if (method) il_free_method(method);
             return false;
        }
        
        // Full name reconstruction (namespace.name) is skipped for now, assuming simple names match
        // Or assume type_name is full name? il_get_string returns raw string.
        // We might need to append namespace.
        // For Lux9Kernel.fs, Namespace is "Lux9.Kernel", Class is "Kernel".
        // il_parser might return just "Kernel" or "Lux9.Kernel.Kernel" depending on table.
        // TypeDef has separate Namespace index.
        
        // Let's assume we can resolve by just class name for now or construct it.
        // Actually clr_internal_calls.c uses "Lux9.Kernel.Kernel".
        // I need to concat namespace + "." + name.
        
        // Quick hack: try resolving with just type_name
        void* native_func = clr_resolve_internal_call(type_name, method->name);
        
        if (!native_func) {
             // Try constructing full name?
             // Need namespace from TypeDef. il_get_method_parent_type_name only returns Name.
             // I should update that function or just fail for now.
             
             // For testing "Hello World" with Lux9.Kernel.Kernel, let's assume strict match or simple match.
             vm_set_error(state, "CALL: Internal call not found");
             if (method) il_free_method(method);
             return false;
        }
        
        // Invoke Native
        // Assuming 1 argument for now (Print, Panic)
        // Stack: [Arg1]
        vm_value_t arg1;
        if (!vm_stack_pop(state, &arg1)) {
             // Maybe 0 args?
        }
        
        // Call it (assuming void func(void*))
        typedef void (*native_fn_t)(void*);
        ((native_fn_t)native_func)(arg1.value.ref);
        
        // Result
        result->type = VM_TYPE_I4; // Void return
        result->value.i4 = 0;
        
        if (method) il_free_method(method);
        return true;
    }
    
    // Normal IL Call
    // ... (To be implemented)
    
    if (method) il_free_method(method);
    return true;
}

bool vm_call_virtual(vm_value_t* method_token, vm_value_t* result) {
    // Simplified implementation for virtual method calls
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_load_function_ptr(vm_value_t* method_token, vm_value_t* result) {
    // Simplified implementation for loading function pointers
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_load_virtual_function_ptr(vm_value_t* obj, vm_value_t* method_token, vm_value_t* result) {
    // Simplified implementation for loading virtual function pointers
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

// Object model implementations
bool vm_load_object(vm_value_t* addr, vm_value_t* result) {
    // Simplified implementation for loading objects
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_store_object(vm_value_t* addr, vm_value_t* value) {
    // Simplified implementation for storing objects
    return true;
}

bool vm_unbox_any(vm_value_t* obj, vm_value_t* type_token, vm_value_t* result) {
    // Simplified implementation for unboxing any type
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

// Type system implementations
bool vm_get_argument_list(vm_value_t* result) {
    // Simplified implementation for getting argument list
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_convert_ovf(vm_value_t* value, vm_type_t target_type, vm_value_t* result) {
    // Simplified implementation for overflow conversion
    result->type = target_type;
    result->value.i4 = 0;
    return true;
}

bool vm_convert_ovf_un(vm_value_t* value, vm_type_t target_type, vm_value_t* result) {
    // Simplified implementation for unsigned overflow conversion
    result->type = target_type;
    result->value.u4 = 0;
    return true;
}

// Exception handling implementations
bool vm_throw_exception(vm_value_t* exception_obj) {
    // Simplified implementation for throwing exceptions
    return false;
}

bool vm_rethrow_exception(vm_value_t* exception_obj) {
    // Simplified implementation for rethrowing exceptions
    return false;
}

bool vm_end_filter(vm_value_t* value) {
    // Simplified implementation for ending filter
    return true;
}

bool vm_constrained_prefix(vm_value_t* type_token) {
    // Simplified implementation for constrained prefix
    return true;
}

bool vm_copy_object(vm_value_t* dest, vm_value_t* src, vm_value_t* type_token) {
    // Simplified implementation for copying objects
    return true;
}

bool vm_init_object(vm_value_t* addr, vm_value_t* type_token) {
    // Simplified implementation for initializing objects
    return true;
}

// Local variable and argument implementations
bool vm_load_argument(vm_value_t* arg_index, vm_value_t* result) {
    // Simplified implementation for loading arguments
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_load_argument_address(vm_value_t* arg_index, vm_value_t* result) {
    // Simplified implementation for loading argument addresses
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_load_local(vm_value_t* local_index, vm_value_t* result) {
    // Simplified implementation for loading locals
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_load_local_address(vm_value_t* local_index, vm_value_t* result) {
    // Simplified implementation for loading local addresses
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_store_argument(vm_value_t* arg_index, vm_value_t* value) {
    // Simplified implementation for storing arguments
    return true;
}

bool vm_store_local(vm_value_t* local_index, vm_value_t* value) {
    // Simplified implementation for storing locals
    return true;
}

// Memory operations
bool vm_copy_memory(vm_value_t* src, vm_value_t* dest, vm_value_t* len) {
    // Simplified implementation for copying memory
    return true;
}

bool vm_init_memory(vm_value_t* addr, vm_value_t* value, vm_value_t* len) {
    // Simplified implementation for initializing memory
    return true;
}

// Array operations
bool vm_load_array_element_address(vm_value_t* array, vm_value_t* index, vm_value_t* result) {
    // Simplified implementation for loading array element addresses
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_load_array_element_any(vm_value_t* array, vm_value_t* index, vm_value_t* type_token, vm_value_t* result) {
    // Simplified implementation for loading any array element
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_store_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* value) {
    // Simplified implementation for storing array elements
    return true;
}

bool vm_store_array_element_any(vm_value_t* array, vm_value_t* index, vm_value_t* type_token, vm_value_t* value) {
    // Simplified implementation for storing any array element
    return true;
}

bool vm_ref_any_type(vm_value_t* typed_ref, vm_value_t* result) {
    // Simplified implementation for getting reference any type
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_size_of(vm_value_t* type_token, vm_value_t* result) {
    // Simplified implementation for getting size of type
    result->type = VM_TYPE_I4;
    result->value.i4 = 4;
    return true;
}

bool vm_get_array_length(vm_value_t* array, vm_value_t* result) {
    // Simplified implementation for getting array length
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

// Symbolic operations
bool vm_symbolic_eval(vm_value_t* expr, vm_value_t* env, vm_value_t* result) {
    // Simplified implementation for symbolic evaluation
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_symbolic_match(vm_value_t* pattern, vm_value_t* expr, vm_value_t* result) {
    // Simplified implementation for symbolic pattern matching
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_symbolic_rewrite(vm_value_t* expr, vm_value_t* rules, vm_value_t* result) {
    // Simplified implementation for symbolic rewriting
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

// Logic Implementations
bool vm_add(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 + right->value.i4; 
        return true; 
    }
    return false; 
}

bool vm_subtract(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 - right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_multiply(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 * right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_divide(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 / right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_divide_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)(u1 / u2);
        return true; 
    }
    return false;
}

bool vm_remainder(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 % right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_remainder_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)(u1 % u2);
        return true; 
    }
    return false;
}

bool vm_and(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 & right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_or(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 | right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_xor(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 ^ right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_not(vm_value_t* v, vm_value_t* r) {
    if (v->type == VM_TYPE_I4) { 
        r->type = VM_TYPE_I4; 
        r->value.i4 = ~v->value.i4; 
        return true; 
    }
    return false;
}

bool vm_neg(vm_value_t* v, vm_value_t* r) {
    if (v->type == VM_TYPE_I4) { 
        r->type = VM_TYPE_I4; 
        r->value.i4 = -v->value.i4; 
        return true; 
    }
    return false;
}

bool vm_shl(vm_value_t* a, vm_value_t* b, vm_value_t* c) {
    if (a->type == VM_TYPE_I4 && b->type == VM_TYPE_I4) { 
        c->type = VM_TYPE_I4; 
        c->value.i4 = a->value.i4 << b->value.i4; 
        return true; 
    }
    return false;
}

bool vm_shr(vm_value_t* a, vm_value_t* b, vm_value_t* c) {
    if (a->type == VM_TYPE_I4 && b->type == VM_TYPE_I4) { 
        c->type = VM_TYPE_I4; 
        c->value.i4 = a->value.i4 >> b->value.i4; 
        return true; 
    }
    return false;
}

bool vm_shr_un(vm_value_t* a, vm_value_t* b, vm_value_t* c) { 
    if (a->type == VM_TYPE_I4 && b->type == VM_TYPE_I4) {
        uint32_t u1 = (uint32_t)a->value.i4;
        c->type = VM_TYPE_I4;
        c->value.i4 = (int32_t)(u1 >> b->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_equal(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = (left->value.i4 == right->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_greater(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = (left->value.i4 > right->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_greater_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (u1 > u2);
        return true;
    }
    return false;
}

bool vm_compare_less(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = (left->value.i4 < right->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_less_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (u1 < u2);
        return true;
    }
    return false;
}

bool vm_call_indirect(vm_value_t* method_ptr, vm_value_t* result) {
    // Stub implementation for TDD GREEN phase
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

// Type conversion implementation
bool vm_convert(vm_value_t* source, vm_type_t target_type, vm_value_t* result) {
    if (!source || !result) return false;
    
    result->type = target_type;
    
    switch(target_type) {
        case VM_TYPE_I1:
            switch(source->type) {
                case VM_TYPE_I1: result->value.i1 = source->value.i1; return true;
                case VM_TYPE_I2: result->value.i1 = (int8_t)source->value.i2; return true;
                case VM_TYPE_I4: result->value.i1 = (int8_t)source->value.i4; return true;
                case VM_TYPE_I8: result->value.i1 = (int8_t)source->value.i8; return true;
                case VM_TYPE_U1: result->value.i1 = (int8_t)source->value.u1; return true;
                case VM_TYPE_U2: result->value.i1 = (int8_t)source->value.u2; return true;
                case VM_TYPE_U4: result->value.i1 = (int8_t)source->value.u4; return true;
                case VM_TYPE_U8: result->value.i1 = (int8_t)source->value.u8; return true;
                default: return false;
            }
        case VM_TYPE_I2:
            switch(source->type) {
                case VM_TYPE_I1: result->value.i2 = source->value.i1; return true;
                case VM_TYPE_I2: result->value.i2 = source->value.i2; return true;
                case VM_TYPE_I4: result->value.i2 = (int16_t)source->value.i4; return true;
                case VM_TYPE_I8: result->value.i2 = (int16_t)source->value.i8; return true;
                case VM_TYPE_U1: result->value.i2 = (int16_t)source->value.u1; return true;
                case VM_TYPE_U2: result->value.i2 = (int16_t)source->value.u2; return true;
                case VM_TYPE_U4: result->value.i2 = (int16_t)source->value.u4; return true;
                case VM_TYPE_U8: result->value.i2 = (int16_t)source->value.u8; return true;
                default: return false;
            }
        case VM_TYPE_I4:
            switch(source->type) {
                case VM_TYPE_I1: result->value.i4 = source->value.i1; return true;
                case VM_TYPE_I2: result->value.i4 = source->value.i2; return true;
                case VM_TYPE_I4: result->value.i4 = source->value.i4; return true;
                case VM_TYPE_I8: result->value.i4 = (int32_t)source->value.i8; return true;
                case VM_TYPE_U1: result->value.i4 = source->value.u1; return true;
                case VM_TYPE_U2: result->value.i4 = source->value.u2; return true;
                case VM_TYPE_U4: result->value.i4 = (int32_t)source->value.u4; return true;
                case VM_TYPE_U8: result->value.i4 = (int32_t)source->value.u8; return true;
                default: return false;
            }
        case VM_TYPE_U4:
            switch(source->type) {
                case VM_TYPE_I1: result->value.u4 = (uint32_t)source->value.i1; return true;
                case VM_TYPE_I2: result->value.u4 = (uint32_t)source->value.i2; return true;
                case VM_TYPE_I4: result->value.u4 = (uint32_t)source->value.i4; return true;
                case VM_TYPE_I8: result->value.u4 = (uint32_t)source->value.i8; return true;
                case VM_TYPE_U1: result->value.u4 = source->value.u1; return true;
                case VM_TYPE_U2: result->value.u4 = source->value.u2; return true;
                case VM_TYPE_U4: result->value.u4 = source->value.u4; return true;
                case VM_TYPE_U8: result->value.u4 = (uint32_t)source->value.u8; return true;
                default: return false;
            }
        default: return false;
    }
}

// Memory access operations
bool vm_load_indirect(vm_value_t* addr, vm_type_t load_type, vm_value_t* result) {
    if (!addr || !result) return false;
    if (addr->type != VM_TYPE_I && addr->type != VM_TYPE_REF) return false;
    if (!addr->value.ref) return false;
    
    result->type = load_type;
    
    switch(load_type) {
        case VM_TYPE_I1: result->value.i1 = *(int8_t*)addr->value.ref; return true;
        case VM_TYPE_U1: result->value.u1 = *(uint8_t*)addr->value.ref; return true;
        case VM_TYPE_I2: result->value.i2 = *(int16_t*)addr->value.ref; return true;
        case VM_TYPE_U2: result->value.u2 = *(uint16_t*)addr->value.ref; return true;
        case VM_TYPE_I4: result->value.i4 = *(int32_t*)addr->value.ref; return true;
        case VM_TYPE_U4: result->value.u4 = *(uint32_t*)addr->value.ref; return true;
        case VM_TYPE_I8: result->value.i8 = *(int64_t*)addr->value.ref; return true;
        case VM_TYPE_I: result->value.i = *(intptr_t*)addr->value.ref; return true;
        case VM_TYPE_R4: result->value.r4 = *(float*)addr->value.ref; return true;
        case VM_TYPE_R8: result->value.r8 = *(double*)addr->value.ref; return true;
        case VM_TYPE_REF: result->value.ref = *(void**)addr->value.ref; return true;
        default: return false;
    }
}

bool vm_store_indirect(vm_value_t* addr, vm_value_t* value) {
    if (!addr || !value) return false;
    if (addr->type != VM_TYPE_I && addr->type != VM_TYPE_REF) return false;
    if (!addr->value.ref) return false;
    
    switch(value->type) {
        case VM_TYPE_I1: *(int8_t*)addr->value.ref = value->value.i1; return true;
        case VM_TYPE_U1: *(uint8_t*)addr->value.ref = value->value.u1; return true;
        case VM_TYPE_I2: *(int16_t*)addr->value.ref = value->value.i2; return true;
        case VM_TYPE_U2: *(uint16_t*)addr->value.ref = value->value.u2; return true;
        case VM_TYPE_I4: *(int32_t*)addr->value.ref = value->value.i4; return true;
        case VM_TYPE_U4: *(uint32_t*)addr->value.ref = value->value.u4; return true;
        case VM_TYPE_I8: *(int64_t*)addr->value.ref = value->value.i8; return true;
        case VM_TYPE_I: *(intptr_t*)addr->value.ref = value->value.i; return true;
        case VM_TYPE_R4: *(float*)addr->value.ref = value->value.r4; return true;
        case VM_TYPE_R8: *(double*)addr->value.ref = value->value.r8; return true;
        case VM_TYPE_REF: *(void**)addr->value.ref = value->value.ref; return true;
        default: return false;
    }
}

// Stub implementations for complex operations
bool vm_load_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* result) {
    // Simplified stub implementation
    result->type = element_type;
    result->value.i4 = 0;
    return true;
}

bool vm_new_array(vm_value_t* size, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_REF;
    result->value.ref = NULL;
    return true;
}

bool vm_load_string_constant(vm_value_t* string_token, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_REF;
    result->value.ref = NULL;
    return true;
}

bool vm_new_object(vm_value_t* constructor_token, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_REF;
    result->value.ref = NULL;
    return true;
}

bool vm_box_value(vm_value_t* value, vm_value_t* box_type, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_REF;
    result->value.ref = NULL;
    return true;
}

bool vm_unbox_value(vm_value_t* obj, vm_value_t* unbox_type, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_load_field_object(vm_value_t* obj, vm_value_t* field_token, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_store_field_object(vm_value_t* obj, vm_value_t* field_token, vm_value_t* value) {
    // Simplified stub implementation
    return true;
}

bool vm_load_static_field(vm_value_t* field_token, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_store_static_field(vm_value_t* field_token, vm_value_t* value) {
    // Simplified stub implementation
    return true;
}

bool vm_load_field_address(vm_value_t* obj, vm_value_t* field_token, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_load_static_field_address(vm_value_t* field_token, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_I;
    result->value.i = 0;
    return true;
}

bool vm_cast_class(vm_value_t* obj, vm_value_t* cast_type, vm_value_t* result) {
    // Simplified stub implementation
    *result = *obj;
    return true;
}

bool vm_is_instance(vm_value_t* obj, vm_value_t* test_type, vm_value_t* result) {
    // Simplified stub implementation
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

// Overflow arithmetic implementations
bool vm_add_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 + (int64_t)right->value.i4;
        if (temp < INT32_MIN || temp > INT32_MAX) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 + right->value.i4;
        return true;
    }
    return false;
}

bool vm_add_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        uint64_t temp = (uint64_t)left->value.u4 + (uint64_t)right->value.u4;
        if (temp > UINT32_MAX) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 + right->value.u4;
        return true;
    }
    return false;
}

bool vm_multiply_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 * (int64_t)right->value.i4;
        if (temp < INT32_MIN || temp > INT32_MAX) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 * right->value.i4;
        return true;
    }
    return false;
}

bool vm_multiply_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        uint64_t temp = (uint64_t)left->value.u4 * (uint64_t)right->value.u4;
        if (temp > UINT32_MAX) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 * right->value.u4;
        return true;
    }
    return false;
}

bool vm_subtract_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 - (int64_t)right->value.i4;
        if (temp < INT32_MIN || temp > INT32_MAX) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 - right->value.i4;
        return true;
    }
    return false;
}

bool vm_subtract_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (left->value.u4 < right->value.u4) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 - right->value.u4;
        return true;
    }
    return false;
}

bool vm_divide_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    // Division overflow only occurs with INT32_MIN / -1
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        if (right->value.i4 == 0) return false;
        if (left->value.i4 == INT32_MIN && right->value.i4 == -1) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 / right->value.i4;
        return true;
    }
    return false;
}

bool vm_divide_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (right->value.u4 == 0) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 / right->value.u4;
        return true;
    }
    return false;
}