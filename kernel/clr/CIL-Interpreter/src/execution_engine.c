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
    while (current != NULL) { depth++; current = current->next; }
    return depth;
}

vm_value_t vm_make_i1(int8_t value) { vm_value_t v; v.type = VM_TYPE_I1; v.value.i1 = value; return v; }
vm_value_t vm_make_i2(int16_t value) { vm_value_t v; v.type = VM_TYPE_I2; v.value.i2 = value; return v; }
vm_value_t vm_make_i4(int32_t value) { vm_value_t v; v.type = VM_TYPE_I4; v.value.i4 = value; return v; }
vm_value_t vm_make_i8(int64_t value) { vm_value_t v; v.type = VM_TYPE_I8; v.value.i8 = value; return v; }
vm_value_t vm_make_u1(uint8_t value) { vm_value_t v; v.type = VM_TYPE_U1; v.value.u1 = value; return v; }
vm_value_t vm_make_u2(uint16_t value) { vm_value_t v; v.type = VM_TYPE_U2; v.value.u2 = value; return v; }
vm_value_t vm_make_u4(uint32_t value) { vm_value_t v; v.type = VM_TYPE_U4; v.value.u4 = value; return v; }
vm_value_t vm_make_u8(uint64_t value) { vm_value_t v; v.type = VM_TYPE_U8; v.value.u8 = value; return v; }
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

        // Bitwise
        case CIL_OPCODE_AND: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "AND: Stack"); return false; }
            if (!vm_and(&l, &r, &res)) { vm_set_error(state, "AND: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_OR: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "OR: Stack"); return false; }
            if (!vm_or(&l, &r, &res)) { vm_set_error(state, "OR: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_XOR: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "XOR: Stack"); return false; }
            if (!vm_xor(&l, &r, &res)) { vm_set_error(state, "XOR: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_NOT: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "NOT: Stack"); return false; }
            if (!vm_not(&v, &res)) { vm_set_error(state, "NOT: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_NEG: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "NEG: Stack"); return false; }
            if (!vm_neg(&v, &res)) { vm_set_error(state, "NEG: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }

        // Comparison
        case CIL_OPCODE_CEQ: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CEQ: Stack"); return false; }
            if (!vm_compare_equal(&l, &r, &res)) { vm_set_error(state, "CEQ: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CGT: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CGT: Stack"); return false; }
            if (!vm_compare_greater(&l, &r, &res)) { vm_set_error(state, "CGT: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CLT: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "CLT: Stack"); return false; }
            if (!vm_compare_less(&l, &r, &res)) { vm_set_error(state, "CLT: Type"); return false; }
            vm_stack_push(state, &res);
            break;
        }

        // Constants
        case CIL_OPCODE_LDNULL: { vm_value_t v = vm_make_null(); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_M1: { vm_value_t v = vm_make_i4(-1); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_0: { vm_value_t v = vm_make_i4(0); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_1: { vm_value_t v = vm_make_i4(1); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_2: { vm_value_t v = vm_make_i4(2); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_3: { vm_value_t v = vm_make_i4(3); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_4: { vm_value_t v = vm_make_i4(4); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_5: { vm_value_t v = vm_make_i4(5); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_6: { vm_value_t v = vm_make_i4(6); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_7: { vm_value_t v = vm_make_i4(7); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_8: { vm_value_t v = vm_make_i4(8); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_S: { vm_value_t v = vm_make_i4(ip->operand.byte_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4:   { vm_value_t v = vm_make_i4(ip->operand.int_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I8:   { vm_value_t v = vm_make_i8(ip->operand.long_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_R4:   { vm_value_t v = vm_make_r4(*(float*)&ip->operand.token); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_R8:   { vm_value_t v = vm_make_r8(*(double*)&ip->operand.long_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_CONV_I1: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I1: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I1, &res)) { vm_set_error(state, "CONV.I1: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_I2: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I2: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I2, &res)) { vm_set_error(state, "CONV.I2: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_I4: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I4: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I4, &res)) { vm_set_error(state, "CONV.I4: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_I8: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.I8: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_I8, &res)) { vm_set_error(state, "CONV.I8: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_U4: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.U4: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_U4, &res)) { vm_set_error(state, "CONV.U4: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_U8: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.U8: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_U8, &res)) { vm_set_error(state, "CONV.U8: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_R4: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.R4: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_R4, &res)) { vm_set_error(state, "CONV.R4: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CONV_R8: {
            vm_value_t v, res;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "CONV.R8: Stack"); return false; }
            if (!vm_convert(&v, VM_TYPE_R8, &res)) { vm_set_error(state, "CONV.R8: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_ADD_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "ADD.OVF: Stack"); return false; }
            if (!vm_add_ovf(&l, &r, &res)) { vm_set_error(state, "ADD.OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_ADD_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "ADD.OVF.UN: Stack"); return false; }
            if (!vm_add_ovf_un(&l, &r, &res)) { vm_set_error(state, "ADD.OVF.UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_MUL_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "MUL.OVF: Stack"); return false; }
            if (!vm_multiply_ovf(&l, &r, &res)) { vm_set_error(state, "MUL.OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_MUL_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "MUL.OVF.UN: Stack"); return false; }
            if (!vm_multiply_ovf_un(&l, &r, &res)) { vm_set_error(state, "MUL.OVF.UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SUB_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SUB.OVF: Stack"); return false; }
            if (!vm_subtract_ovf(&l, &r, &res)) { vm_set_error(state, "SUB.OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_SUB_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "SUB.OVF.UN: Stack"); return false; }
            if (!vm_subtract_ovf_un(&l, &r, &res)) { vm_set_error(state, "SUB.OVF.UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DIV_OVF: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "DIV.OVF: Stack"); return false; }
            if (!vm_divide_ovf(&l, &r, &res)) { vm_set_error(state, "DIV.OVF: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DIV_OVF_UN: {
            vm_value_t r, l, res;
            if (!vm_stack_pop(state, &r) || !vm_stack_pop(state, &l)) { vm_set_error(state, "DIV.OVF.UN: Stack"); return false; }
            if (!vm_divide_ovf_un(&l, &r, &res)) { vm_set_error(state, "DIV.OVF.UN: Overflow"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_I1: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I1: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I1, &res)) { vm_set_error(state, "LDIND.I1: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_U1: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.U1: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_U1, &res)) { vm_set_error(state, "LDIND.U1: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_I2: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I2: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I2, &res)) { vm_set_error(state, "LDIND.I2: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_U2: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.U2: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_U2, &res)) { vm_set_error(state, "LDIND.U2: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_I4: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I4: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I4, &res)) { vm_set_error(state, "LDIND.I4: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_U4: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.U4: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_U4, &res)) { vm_set_error(state, "LDIND.U4: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_I8: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I8: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I8, &res)) { vm_set_error(state, "LDIND.I8: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_I: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.I: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_I, &res)) { vm_set_error(state, "LDIND.I: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_R4: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.R4: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_R4, &res)) { vm_set_error(state, "LDIND.R4: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_R8: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.R8: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_R8, &res)) { vm_set_error(state, "LDIND.R8: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDIND_REF: {
            vm_value_t addr, res;
            if (!vm_stack_pop(state, &addr)) { vm_set_error(state, "LDIND.REF: Stack"); return false; }
            if (!vm_load_indirect(&addr, VM_TYPE_REF, &res)) { vm_set_error(state, "LDIND.REF: Memory"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_STIND_REF: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.REF: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.REF: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I1: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I1: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I1: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I2: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I2: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I2: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I4: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I4: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I4: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_I8: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.I8: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.I8: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_R4: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.R4: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.R4: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_STIND_R8: {
            vm_value_t addr, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &addr)) { vm_set_error(state, "STIND.R8: Stack"); return false; }
            if (!vm_store_indirect(&addr, &value)) { vm_set_error(state, "STIND.R8: Memory"); return false; }
            break;
        }
        case CIL_OPCODE_NEWOBJ: {
            // TODO: Implement NEWOBJ - requires method resolution and object allocation
            vm_set_error(state, "NEWOBJ: Not yet implemented");
            return false;
        }
        case CIL_OPCODE_NEWARR: {
            vm_value_t size, res;
            if (!vm_stack_pop(state, &size)) { vm_set_error(state, "NEWARR: Stack"); return false; }
            if (!vm_new_array(&size, &res)) { vm_set_error(state, "NEWARR: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDSTR: {
            // TODO: Implement LDSTR - requires string pool management
            vm_set_error(state, "LDSTR: Not yet implemented");
            return false;
        }
        case CIL_OPCODE_BOX: {
            vm_value_t value, box_type, res;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &box_type)) { vm_set_error(state, "BOX: Stack"); return false; }
            if (!vm_box_value(&value, &box_type, &res)) { vm_set_error(state, "BOX: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_UNBOX: {
            vm_value_t obj, unbox_type, res;
            if (!vm_stack_pop(state, &obj) || !vm_stack_pop(state, &unbox_type)) { vm_set_error(state, "UNBOX: Stack"); return false; }
            if (!vm_unbox_value(&obj, &unbox_type, &res)) { vm_set_error(state, "UNBOX: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDFLD: {
            vm_value_t obj, field_token, res;
            if (!vm_stack_pop(state, &obj) || !vm_stack_pop(state, &field_token)) { vm_set_error(state, "LDFLD: Stack"); return false; }
            if (!vm_load_field_object(&obj, &field_token, &res)) { vm_set_error(state, "LDFLD: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_STFLD: {
            vm_value_t obj, field_token, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &field_token) || !vm_stack_pop(state, &obj)) { vm_set_error(state, "STFLD: Stack"); return false; }
            if (!vm_store_field_object(&obj, &field_token, &value)) { vm_set_error(state, "STFLD: Error"); return false; }
            break;
        }
        case CIL_OPCODE_LDSFLD: {
            vm_value_t field_token, res;
            if (!vm_stack_pop(state, &field_token)) { vm_set_error(state, "LDSFLD: Stack"); return false; }
            if (!vm_load_static_field(&field_token, &res)) { vm_set_error(state, "LDSFLD: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_STSFLD: {
            vm_value_t field_token, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &field_token)) { vm_set_error(state, "STSFLD: Stack"); return false; }
            if (!vm_store_static_field(&field_token, &value)) { vm_set_error(state, "STSFLD: Error"); return false; }
            break;
        }
        case CIL_OPCODE_LDFLDA: {
            vm_value_t obj, field_token, res;
            if (!vm_stack_pop(state, &obj) || !vm_stack_pop(state, &field_token)) { vm_set_error(state, "LDFLDA: Stack"); return false; }
            if (!vm_load_field_address(&obj, &field_token, &res)) { vm_set_error(state, "LDFLDA: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_LDSFLDA: {
            vm_value_t field_token, res;
            if (!vm_stack_pop(state, &field_token)) { vm_set_error(state, "LDSFLDA: Stack"); return false; }
            if (!vm_load_static_field_address(&field_token, &res)) { vm_set_error(state, "LDSFLDA: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_CASTCLASS: {
            vm_value_t obj, cast_type, res;
            if (!vm_stack_pop(state, &obj) || !vm_stack_pop(state, &cast_type)) { vm_set_error(state, "CASTCLASS: Stack"); return false; }
            if (!vm_cast_class(&obj, &cast_type, &res)) { vm_set_error(state, "CASTCLASS: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_ISINST: {
            vm_value_t obj, test_type, res;
            if (!vm_stack_pop(state, &obj) || !vm_stack_pop(state, &test_type)) { vm_set_error(state, "ISINST: Stack"); return false; }
            if (!vm_is_instance(&obj, &test_type, &res)) { vm_set_error(state, "ISINST: Error"); return false; }
            vm_stack_push(state, &res);
            break;
        }
        case CIL_OPCODE_DUP: {
            vm_value_t v;
            if (!vm_stack_peek(state, &v)) { vm_set_error(state, "DUP: Stack"); return false; }
            vm_stack_push(state, &v);
            break;
        }
        case CIL_OPCODE_POP: {
            vm_value_t v;
            if (!vm_stack_pop(state, &v)) { vm_set_error(state, "POP: Stack"); return false; }
            break;
        }

        // Arguments
        case CIL_OPCODE_LDARG_0: {
            if (state->current_frame->arg_count < 1) { vm_set_error(state, "LDARG.0: Index"); return false; }
            vm_stack_push(state, &state->current_frame->args[0]); break;
        }
        case CIL_OPCODE_LDARG_1: {
            if (state->current_frame->arg_count < 2) { vm_set_error(state, "LDARG.1: Index"); return false; }
            vm_stack_push(state, &state->current_frame->args[1]); break;
        }
        case CIL_OPCODE_LDARG_2: {
            if (state->current_frame->arg_count < 3) { vm_set_error(state, "LDARG.2: Index"); return false; }
            vm_stack_push(state, &state->current_frame->args[2]); break;
        }
        case CIL_OPCODE_LDARG_3: {
            if (state->current_frame->arg_count < 4) { vm_set_error(state, "LDARG.3: Index"); return false; }
            vm_stack_push(state, &state->current_frame->args[3]); break;
        }
        case CIL_OPCODE_LDARG_S: {
            if (ip->operand.byte_val >= state->current_frame->arg_count) { vm_set_error(state, "LDARG.S: Index"); return false; }
            vm_stack_push(state, &state->current_frame->args[ip->operand.byte_val]); break;
        }
        case CIL_OPCODE_LDARGA_S: {
            if (ip->operand.byte_val >= state->current_frame->arg_count) { vm_set_error(state, "LDARGA.S: Index"); return false; }
            vm_value_t addr;
            addr.type = VM_TYPE_I;
            addr.value.i = (intptr_t)&state->current_frame->args[ip->operand.byte_val];
            vm_stack_push(state, &addr);
            break;
        }
        case CIL_OPCODE_STARG_S: {
            if (ip->operand.byte_val >= state->current_frame->arg_count) { vm_set_error(state, "STARG.S: Index"); return false; }
            if (!vm_stack_pop(state, &state->current_frame->args[ip->operand.byte_val])) return false;
            break;
        }

        // Locals
        case CIL_OPCODE_LDLOC_0: {
            if (state->current_frame->local_count < 1) { vm_set_error(state, "LDLOC.0: Index"); return false; }
            vm_stack_push(state, &state->current_frame->locals[0]); break;
        }
        case CIL_OPCODE_LDLOC_1: {
            if (state->current_frame->local_count < 2) { vm_set_error(state, "LDLOC.1: Index"); return false; }
            vm_stack_push(state, &state->current_frame->locals[1]); break;
        }
        case CIL_OPCODE_LDLOC_2: {
            if (state->current_frame->local_count < 3) { vm_set_error(state, "LDLOC.2: Index"); return false; }
            vm_stack_push(state, &state->current_frame->locals[2]); break;
        }
        case CIL_OPCODE_LDLOC_3: {
            if (state->current_frame->local_count < 4) { vm_set_error(state, "LDLOC.3: Index"); return false; }
            vm_stack_push(state, &state->current_frame->locals[3]); break;
        }
        case CIL_OPCODE_LDLOC_S: {
            if (ip->operand.byte_val >= state->current_frame->local_count) { vm_set_error(state, "LDLOC.S: Index"); return false; }
            vm_stack_push(state, &state->current_frame->locals[ip->operand.byte_val]); break;
        }
        case CIL_OPCODE_STLOC_0: {
            if (state->current_frame->local_count < 1) { vm_set_error(state, "STLOC.0: Index"); return false; }
            if (!vm_stack_pop(state, &state->current_frame->locals[0])) return false; break;
        }
        case CIL_OPCODE_STLOC_1: {
            if (state->current_frame->local_count < 2) { vm_set_error(state, "STLOC.1: Index"); return false; }
            if (!vm_stack_pop(state, &state->current_frame->locals[1])) return false; break;
        }
        case CIL_OPCODE_STLOC_2: {
            if (state->current_frame->local_count < 3) { vm_set_error(state, "STLOC.2: Index"); return false; }
            if (!vm_stack_pop(state, &state->current_frame->locals[2])) return false; break;
        }
        case CIL_OPCODE_STLOC_3: {
            if (state->current_frame->local_count < 4) { vm_set_error(state, "STLOC.3: Index"); return false; }
            if (!vm_stack_pop(state, &state->current_frame->locals[3])) return false; break;
        }
        case CIL_OPCODE_STLOC_S: {
            if (ip->operand.byte_val >= state->current_frame->local_count) { vm_set_error(state, "STLOC.S: Index"); return false; }
            if (!vm_stack_pop(state, &state->current_frame->locals[ip->operand.byte_val])) return false; break;
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
        case CIL_OPCODE_BGE: {
            vm_value_t v2, v1;
            if (!vm_stack_pop(state, &v2) || !vm_stack_pop(state, &v1)) { vm_set_error(state, "BGE: Stack"); return false; }
            
            bool cond = false;
            if (v1.type == VM_TYPE_I4 && v2.type == VM_TYPE_I4 && v1.value.i4 >= v2.value.i4) cond = true;
            
            if (cond) state->current_frame->ip += (ip->operand.branch_offset - 1);
            break;
        }
        case CIL_OPCODE_BRTRUE: {
            vm_value_t val;
            if (!vm_stack_pop(state, &val)) { vm_set_error(state, "BRTRUE: Stack"); return false; }
            bool cond = false;
            if (val.type == VM_TYPE_I4) cond = (val.value.i4 != 0);
            /* Simplified for test */
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
        case CIL_OPCODE_LDELEM_I4: {
            vm_value_t array, index, result;
            if (!vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "LDELEM.I4: Stack"); return false; }
            if (!vm_load_array_element(&array, &index, VM_TYPE_I4, &result)) { vm_set_error(state, "LDELEM.I4: Error"); return false; }
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
        case CIL_OPCODE_STELEM_ANY: {
            vm_value_t array, index, type_token, value;
            if (!vm_stack_pop(state, &value) || !vm_stack_pop(state, &type_token) || !vm_stack_pop(state, &index) || !vm_stack_pop(state, &array)) { vm_set_error(state, "STELEM.ANY: Stack"); return false; }
            if (!vm_store_array_element_any(&array, &index, &type_token, &value)) { vm_set_error(state, "STELEM.ANY: Error"); return false; }
            break;
        }
        
        default: vm_set_error(state, "Unsupported opcode"); return false;
    }
    return true;
}

// Logic Implementations
bool vm_add(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { result->type = VM_TYPE_I4; result->value.i4 = left->value.i4 + right->value.i4; return true; }
    return false; /* Simplified for restore */
}
bool vm_subtract(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { result->type = VM_TYPE_I4; result->value.i4 = left->value.i4 - right->value.i4; return true; }
    return false;
}
bool vm_multiply(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { result->type = VM_TYPE_I4; result->value.i4 = left->value.i4 * right->value.i4; return true; }
    return false;
}
bool vm_divide(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        result->type = VM_TYPE_I4; result->value.i4 = left->value.i4 / right->value.i4; return true; 
    }
    return false;
}
bool vm_divide_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) { 
        if (right->value.u4 == 0) return false;
        result->type = VM_TYPE_U4; result->value.u4 = left->value.u4 / right->value.u4; return true; 
    }
    return false;
}
bool vm_remainder(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        result->type = VM_TYPE_I4; result->value.i4 = left->value.i4 % right->value.i4; return true; 
    }
    return false;
}
bool vm_remainder_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) { 
        if (right->value.u4 == 0) return false;
        result->type = VM_TYPE_U4; result->value.u4 = left->value.u4 % right->value.u4; return true; 
    }
    return false;
}

// Bitwise Logic
bool vm_and(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type != right->type) return false;
    result->type = left->type;
    switch(left->type) {
        case VM_TYPE_I4: result->value.i4 = left->value.i4 & right->value.i4; return true;
        default: return false;
    }
}
bool vm_or(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type != right->type) return false;
    result->type = left->type;
    switch(left->type) {
        case VM_TYPE_I4: result->value.i4 = left->value.i4 | right->value.i4; return true;
        default: return false;
    }
}
bool vm_xor(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type != right->type) return false;
    result->type = left->type;
    switch(left->type) {
        case VM_TYPE_I4: result->value.i4 = left->value.i4 ^ right->value.i4; return true;
        default: return false;
    }
}
bool vm_not(vm_value_t* v, vm_value_t* r) {
    r->type = v->type;
    switch(v->type) {
        case VM_TYPE_I4: r->value.i4 = ~v->value.i4; return true;
        default: return false;
    }
}
bool vm_neg(vm_value_t* v, vm_value_t* r) {
    r->type = v->type;
    switch(v->type) {
        case VM_TYPE_I4: r->value.i4 = -v->value.i4; return true;
        default: return false;
    }
}

// Comparison Logic
bool vm_compare_equal(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type != right->type) return false;
    result->type = VM_TYPE_I4;
    switch(left->type) {
        case VM_TYPE_I4: result->value.i4 = (left->value.i4 == right->value.i4); return true;
        default: return false;
    }
}
bool vm_compare_greater(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type != right->type) return false;
    result->type = VM_TYPE_I4;
    switch(left->type) {
        case VM_TYPE_I4: result->value.i4 = (left->value.i4 > right->value.i4); return true;
        default: return false;
    }
}
bool vm_compare_less(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type != right->type) return false;
    result->type = VM_TYPE_I4;
    switch(left->type) {
        case VM_TYPE_I4: result->value.i4 = (left->value.i4 < right->value.i4); return true;
        default: return false;
    }
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
                case VM_TYPE_U1: result->value.i2 = source->value.u1; return true;
                case VM_TYPE_U2: result->value.i2 = source->value.u2; return true;
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
                case VM_TYPE_R4: result->value.i4 = (int32_t)source->value.r4; return true;
                case VM_TYPE_R8: result->value.i4 = (int32_t)source->value.r8; return true;
                default: return false;
            }
            
        case VM_TYPE_I8:
            switch(source->type) {
                case VM_TYPE_I1: result->value.i8 = source->value.i1; return true;
                case VM_TYPE_I2: result->value.i8 = source->value.i2; return true;
                case VM_TYPE_I4: result->value.i8 = source->value.i4; return true;
                case VM_TYPE_I8: result->value.i8 = source->value.i8; return true;
                case VM_TYPE_U1: result->value.i8 = source->value.u1; return true;
                case VM_TYPE_U2: result->value.i8 = source->value.u2; return true;
                case VM_TYPE_U4: result->value.i8 = source->value.u4; return true;
                case VM_TYPE_U8: result->value.i8 = (int64_t)source->value.u8; return true;
                case VM_TYPE_R4: result->value.i8 = (int64_t)source->value.r4; return true;
                case VM_TYPE_R8: result->value.i8 = (int64_t)source->value.r8; return true;
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
                case VM_TYPE_R4: result->value.u4 = (uint32_t)source->value.r4; return true;
                case VM_TYPE_R8: result->value.u4 = (uint32_t)source->value.r8; return true;
                default: return false;
            }
            
        case VM_TYPE_U8:
            switch(source->type) {
                case VM_TYPE_I1: result->value.u8 = (uint64_t)source->value.i1; return true;
                case VM_TYPE_I2: result->value.u8 = (uint64_t)source->value.i2; return true;
                case VM_TYPE_I4: result->value.u8 = (uint64_t)source->value.i4; return true;
                case VM_TYPE_I8: result->value.u8 = (uint64_t)source->value.i8; return true;
                case VM_TYPE_U1: result->value.u8 = source->value.u1; return true;
                case VM_TYPE_U2: result->value.u8 = source->value.u2; return true;
                case VM_TYPE_U4: result->value.u8 = source->value.u4; return true;
                case VM_TYPE_U8: result->value.u8 = source->value.u8; return true;
                case VM_TYPE_R4: result->value.u8 = (uint64_t)source->value.r4; return true;
                case VM_TYPE_R8: result->value.u8 = (uint64_t)source->value.r8; return true;
                default: return false;
            }
            
        case VM_TYPE_R4:
            switch(source->type) {
                case VM_TYPE_I1: result->value.r4 = (float)source->value.i1; return true;
                case VM_TYPE_I2: result->value.r4 = (float)source->value.i2; return true;
                case VM_TYPE_I4: result->value.r4 = (float)source->value.i4; return true;
                case VM_TYPE_I8: result->value.r4 = (float)source->value.i8; return true;
                case VM_TYPE_U1: result->value.r4 = (float)source->value.u1; return true;
                case VM_TYPE_U2: result->value.r4 = (float)source->value.u2; return true;
                case VM_TYPE_U4: result->value.r4 = (float)source->value.u4; return true;
                case VM_TYPE_U8: result->value.r4 = (float)source->value.u8; return true;
                case VM_TYPE_R4: result->value.r4 = source->value.r4; return true;
                case VM_TYPE_R8: result->value.r4 = (float)source->value.r8; return true;
                default: return false;
            }
            
        case VM_TYPE_R8:
            switch(source->type) {
                case VM_TYPE_I1: result->value.r8 = (double)source->value.i1; return true;
                case VM_TYPE_I2: result->value.r8 = (double)source->value.i2; return true;
                case VM_TYPE_I4: result->value.r8 = (double)source->value.i4; return true;
                case VM_TYPE_I8: result->value.r8 = (double)source->value.i8; return true;
                case VM_TYPE_U1: result->value.r8 = (double)source->value.u1; return true;
                case VM_TYPE_U2: result->value.r8 = (double)source->value.u2; return true;
                case VM_TYPE_U4: result->value.r8 = (double)source->value.u4; return true;
                case VM_TYPE_U8: result->value.r8 = (double)source->value.u8; return true;
                case VM_TYPE_R4: result->value.r8 = (double)source->value.r4; return true;
                case VM_TYPE_R8: result->value.r8 = source->value.r8; return true;
                default: return false;
            }
            
        default:
    return false;
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

// Object management operations
bool vm_new_array(vm_value_t* size, vm_value_t* result) {
    if (!size || !result) return false;
    if (size->type != VM_TYPE_I4) return false;
    if (size->value.i4 < 0) return false;
    
    // Simple array allocation - allocate memory for elements
    uint32_t element_count = size->value.i4;
    void* array_data = malloc(element_count * sizeof(void*)); // Array of pointers
    
    result->type = VM_TYPE_REF;
    result->value.ref = array_data;
    return true;
}

bool vm_load_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* result) {
    if (!array || !index || !result) return false;
    if (array->type != VM_TYPE_REF || !array->value.ref) return false;
    if (index->type != VM_TYPE_I4) return false;
    
    int32_t idx = index->value.i4;
    if (idx < 0) return false; // Simple bounds check
    
    // For now, treat all arrays as arrays of void* for simplicity
    void** array_ptr = (void**)array->value.ref;
    if (idx >= 0) { // Simplified bounds check
        void* element = array_ptr[idx];
        if (!element) {
            // Return null reference
            result->type = VM_TYPE_REF;
            result->value.ref = NULL;
            return true;
        }
        
        // For now, return the element as-is (simplified)
        result->type = element_type;
        result->value.ref = element;
        return true;
    }
    
    return false;
}

bool vm_store_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* value) {
    if (!array || !index || !value) return false;
    if (array->type != VM_TYPE_REF || !array->value.ref) return false;
    if (index->type != VM_TYPE_I4) return false;
    
    int32_t idx = index->value.i4;
    if (idx < 0) return false; // Simple bounds check
    
    // For now, treat all arrays as arrays of void* for simplicity
    void** array_ptr = (void**)array->value.ref;
    if (idx >= 0) { // Simplified bounds check
        // Store the value as a reference
        array_ptr[idx] = value->value.ref;
        return true;
    }
    
    return false;
}

bool vm_get_array_length(vm_value_t* array, vm_value_t* result) {
    if (!array || !result) return false;
    if (array->type != VM_TYPE_REF || !array->value.ref) return false;
    
    // For now, return a default length of 10 (simplified implementation)
    result->type = VM_TYPE_I4;
    result->value.i4 = 10;
    return true;
}

bool vm_load_array_element_address(vm_value_t* array, vm_value_t* index, vm_value_t* result) {
    if (!array || !index || !result) return false;
    if (array->type != VM_TYPE_REF || !array->value.ref) return false;
    if (index->type != VM_TYPE_I4) return false;
    
    int32_t idx = index->value.i4;
    if (idx < 0) return false; // Simple bounds check
    
    // For now, treat all arrays as arrays of void* for simplicity
    void** array_ptr = (void**)array->value.ref;
    if (idx >= 0) { // Simplified bounds check
        // Return address of the element
        result->type = VM_TYPE_BYREF;
        result->value.ref = &array_ptr[idx];
        return true;
    }
    
    return false;
}

// Stubs for missing field/object operations
bool vm_load_field_object(vm_value_t* obj, vm_value_t* field_token, vm_value_t* result) {
    // TODO: Implement field loading with proper token resolution
    return false;
}

bool vm_store_field_object(vm_value_t* obj, vm_value_t* field_token, vm_value_t* value) {
    // TODO: Implement field storing with proper token resolution
    return false;
}

bool vm_load_static_field(vm_value_t* field_token, vm_value_t* result) {
    // TODO: Implement static field loading with proper token resolution
    return false;
}

bool vm_store_static_field(vm_value_t* field_token, vm_value_t* value) {
    // TODO: Implement static field storing with proper token resolution
    return false;
}

bool vm_load_field_address(vm_value_t* obj, vm_value_t* field_token, vm_value_t* result) {
    // TODO: Implement field address loading with proper token resolution
    return false;
}

bool vm_load_static_field_address(vm_value_t* field_token, vm_value_t* result) {
    // TODO: Implement static field address loading with proper token resolution
    return false;
}

bool vm_cast_class(vm_value_t* obj, vm_value_t* cast_type, vm_value_t* result) {
    // TODO: Implement class casting with proper type checking
    return false;
}

bool vm_is_instance(vm_value_t* obj, vm_value_t* test_type, vm_value_t* result) {
    // TODO: Implement instance checking with proper type checking
    return false;
}

// Memory management operations
bool vm_convert_ovf_un(vm_value_t* value, vm_type_t target_type, vm_value_t* result) {
    // TODO: Implement overflow-checked unsigned conversion
    if (!value || !result) return false;
    
    // Simplified implementation - basic type conversion
    switch (target_type) {
        case VM_TYPE_I1:
            result->type = VM_TYPE_I1;
            result->value.i1 = (int8_t)value->value.u8; // Simplified
            return true;
        case VM_TYPE_I2:
            result->type = VM_TYPE_I2;
            result->value.i2 = (int16_t)value->value.u8; // Simplified
            return true;
        case VM_TYPE_I4:
            result->type = VM_TYPE_I4;
            result->value.i4 = (int32_t)value->value.u8; // Simplified
            return true;
        case VM_TYPE_I8:
            result->type = VM_TYPE_I8;
            result->value.i8 = (int64_t)value->value.u8; // Simplified
            return true;
        case VM_TYPE_U1:
            result->type = VM_TYPE_U1;
            result->value.u1 = (uint8_t)value->value.u8; // Simplified
            return true;
        default:
            return false;
    }
}

bool vm_local_alloc(vm_value_t* size, vm_value_t* result) {
    // TODO: Implement local memory allocation
    if (!size || !result) return false;
    if (size->type != VM_TYPE_I4) return false;
    
    uint32_t alloc_size = size->value.i4;
    void* mem = malloc(alloc_size);
    if (!mem) return false;
    
    result->type = VM_TYPE_REF;
    result->value.ref = mem;
    return true;
}

bool vm_init_object(vm_value_t* addr, vm_value_t* type_token) {
    // TODO: Implement object initialization
    if (!addr || !type_token) return false;
    if (!addr->value.ref) return false;
    
    // Simplified initialization - zero memory
    memset(addr->value.ref, 0, 8); // Zero first 8 bytes
    return true;
}

bool vm_size_of(vm_value_t* type_token, vm_value_t* result) {
    // TODO: Implement sizeof operation with proper type resolution
    if (!type_token || !result) return false;
    
    // Simplified implementation - return default size
    result->type = VM_TYPE_I4;
    result->value.i4 = 4; // Default size
    return true;
}

bool vm_ref_any_type(vm_value_t* typed_ref, vm_value_t* result) {
    // TODO: Implement reference any type extraction
    if (!typed_ref || !result) return false;
    
    // Simplified implementation - return I4 as type token
    result->type = VM_TYPE_I4;
    result->value.i4 = 0x12345678; // Placeholder type token
    return true;
}

bool vm_copy_memory(vm_value_t* src, vm_value_t* dest, vm_value_t* len) {
    // TODO: Implement memory copy operation
    if (!src || !dest || !len) return false;
    if (!src->value.ref || !dest->value.ref) return false;
    if (len->type != VM_TYPE_I4) return false;
    
    // Simplified memory copy
    memcpy(dest->value.ref, src->value.ref, len->value.i4);
    return true;
}

bool vm_init_memory(vm_value_t* addr, vm_value_t* value, vm_value_t* len) {
    // TODO: Implement memory initialization operation
    if (!addr || !value || !len) return false;
    if (!addr->value.ref) return false;
    if (len->type != VM_TYPE_I4) return false;
    
    // Simplified memory initialization
    memset(addr->value.ref, value->value.i1, len->value.i4);
    return true;
}

bool vm_symbolic_create(vm_value_t* var_name, vm_value_t* result) {
    // TODO: Implement symbolic variable creation
    if (!var_name || !result) return false;
    
    // Simplified implementation - create placeholder symbolic variable
    result->type = VM_TYPE_REF;
    result->value.ref = malloc(sizeof(int)); // Placeholder
    if (result->value.ref) {
        *(int*)result->value.ref = 12345; // Symbolic variable ID
        return true;
    }
    return false;
}

bool vm_symbolic_expr(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    // TODO: Implement symbolic expression building
    if (!left || !right || !result) return false;
    
    // Simplified implementation - return left operand
    *result = *left;
    return true;
}

bool vm_symbolic_differentiate(vm_value_t* expr, vm_value_t* var, vm_value_t* result) {
    // TODO: Implement symbolic differentiation
    if (!expr || !var || !result) return false;
    
    // Simplified implementation - return zero derivative
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_symbolic_integrate(vm_value_t* expr, vm_value_t* var, vm_value_t* result) {
    // TODO: Implement symbolic integration
    if (!expr || !var || !result) return false;
    
    // Simplified implementation - return zero integral
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_symbolic_simplify(vm_value_t* expr, vm_value_t* result) {
    // TODO: Implement symbolic simplification
    if (!expr || !result) return false;
    
    // Simplified implementation - return expression as-is
    *result = *expr;
    return true;
}

bool vm_symbolic_eval(vm_value_t* expr, vm_value_t* env, vm_value_t* result) {
    // TODO: Implement symbolic evaluation
    if (!expr || !env || !result) return false;
    
    // Simplified implementation - return zero
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_symbolic_match(vm_value_t* pattern, vm_value_t* expr, vm_value_t* result) {
    // TODO: Implement symbolic pattern matching
    if (!pattern || !expr || !result) return false;
    
    // Simplified implementation - return false (no match)
    result->type = VM_TYPE_I4;
    result->value.i4 = 0;
    return true;
}

bool vm_box_value(vm_value_t* value, vm_value_t* box_type, vm_value_t* result) {
    if (!value || !result) return false;
    
    // Simple boxing - allocate memory for the value
    size_t value_size = 0;
    switch(value->type) {
        case VM_TYPE_I1: case VM_TYPE_U1: value_size = sizeof(int8_t); break;
        case VM_TYPE_I2: case VM_TYPE_U2: value_size = sizeof(int16_t); break;
        case VM_TYPE_I4: case VM_TYPE_U4: value_size = sizeof(int32_t); break;
        case VM_TYPE_I8: case VM_TYPE_U8: value_size = sizeof(int64_t); break;
        case VM_TYPE_R4: value_size = sizeof(float); break;
        case VM_TYPE_R8: value_size = sizeof(double); break;
        default: return false;
    }
    
    void* boxed_data = malloc(value_size);
    if (!boxed_data) return false;
    
    // Copy the value
    memcpy(boxed_data, &value->value, value_size);
    
    result->type = VM_TYPE_REF;
    result->value.ref = boxed_data;
    return true;
}

bool vm_unbox_value(vm_value_t* obj, vm_value_t* unbox_type, vm_value_t* result) {
    if (!obj || !result) return false;
    if (obj->type != VM_TYPE_REF || !obj->value.ref) return false;
    
    // Simple unboxing - assume the type is correct
    result->type = VM_TYPE_I4; // Default to int32 for simplicity
    result->value.i4 = *(int32_t*)obj->value.ref;
    return true;
}

// Stubs for linker satisfaction
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
    // TODO: Implement unsigned right shift
    return false; 
}
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
bool vm_add_ovf(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_sub_ovf(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_mul_ovf(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_div_ovf(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_convert_ovf_i1(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_u1(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_i2(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_u2(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_i4(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_u4(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_i8(vm_value_t* a, vm_value_t* b) { return false; }
bool vm_convert_ovf_u8(vm_value_t* a, vm_value_t* b) { return false; }
// Symbolic Implementation

static sym_expr_t* alloc_sym() {
    sym_expr_t* e = malloc(sizeof(sym_expr_t));
    memset(e, 0, sizeof(sym_expr_t));
    e->ref_count = 1;
    return e;
}

bool vm_symbolic_create(vm_value_t* var_name, vm_value_t* result) {
    if (var_name->type != VM_TYPE_REF) return false;
    sym_expr_t* e = alloc_sym();
    e->type = SYM_VAR;
    e->data.name = strdup((char*)var_name->value.ref);
    result->type = VM_TYPE_REF;
    result->value.ref = e;
    return true;
}

bool vm_symbolic_add(vm_value_t* l, vm_value_t* r, vm_value_t* res) {
    sym_expr_t* e = alloc_sym();
    e->type = SYM_ADD;
    e->data.binary.left = (sym_expr_t*)l->value.ref;
    e->data.binary.right = (sym_expr_t*)r->value.ref;
    res->type = VM_TYPE_REF;
    res->value.ref = e;
    return true;
}

bool vm_symbolic_mul(vm_value_t* l, vm_value_t* r, vm_value_t* res) {
    sym_expr_t* e = alloc_sym();
    e->type = SYM_MUL;
    e->data.binary.left = (sym_expr_t*)l->value.ref;
    e->data.binary.right = (sym_expr_t*)r->value.ref;
    res->type = VM_TYPE_REF;
    res->value.ref = e;
    return true;
}

static sym_expr_t* diff_node(sym_expr_t* expr, char* var) {
    if (expr->type == SYM_CONST) {
        sym_expr_t* zero = alloc_sym();
        zero->type = SYM_CONST;
        zero->data.value = 0;
        return zero;
    }
    if (expr->type == SYM_VAR) {
        sym_expr_t* res = alloc_sym();
        res->type = SYM_CONST;
        res->data.value = (strcmp(expr->data.name, var) == 0) ? 1 : 0;
        return res;
    }
    if (expr->type == SYM_ADD) {
        sym_expr_t* d = alloc_sym();
        d->type = SYM_ADD;
        d->data.binary.left = diff_node(expr->data.binary.left, var);
        d->data.binary.right = diff_node(expr->data.binary.right, var);
        return d;
    }
    if (expr->type == SYM_MUL) {
        sym_expr_t* u = expr->data.binary.left;
        sym_expr_t* v = expr->data.binary.right;
        
        sym_expr_t* term1 = alloc_sym();
        term1->type = SYM_MUL;
        term1->data.binary.left = u; 
        term1->data.binary.right = diff_node(v, var);
        
        sym_expr_t* term2 = alloc_sym();
        term2->type = SYM_MUL;
        term2->data.binary.left = v; 
        term2->data.binary.right = diff_node(u, var);
        
        sym_expr_t* sum = alloc_sym();
        sum->type = SYM_ADD;
        sum->data.binary.left = term1;
        sum->data.binary.right = term2;
        return sum;
    }
    return NULL;
}

bool vm_symbolic_differentiate(vm_value_t* expr, vm_value_t* var, vm_value_t* res) {
    sym_expr_t* e = (sym_expr_t*)expr->value.ref;
    sym_expr_t* v = (sym_expr_t*)var->value.ref;
    if (!e || !v || v->type != SYM_VAR) return false;
    
    sym_expr_t* d = diff_node(e, v->data.name);
    if (!d) return false;
    
    res->type = VM_TYPE_REF;
    res->value.ref = d;
    return true;
}

// Stubs
bool vm_symbolic_expr(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_symbolic_integrate(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_symbolic_simplify(vm_value_t* a, vm_value_t* b) { return false; }