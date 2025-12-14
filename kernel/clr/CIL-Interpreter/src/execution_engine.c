#include "../include/execution_engine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

// ==================== CORE VM FUNCTIONS ====================

vm_execution_state_t* vm_create_execution_state(vm_init_t* init) {
    vm_execution_state_t* state = (vm_execution_state_t*)malloc(sizeof(vm_execution_state_t));
    if (state == NULL) return NULL;
    memset(state, 0, sizeof(vm_execution_state_t));
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
        
        default: vm_set_error(state, "Unsupported opcode"); return false;
    }
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