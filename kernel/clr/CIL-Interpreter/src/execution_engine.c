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
        case CIL_OPCODE_LDC_I4_0: { vm_value_t v = vm_make_i4(0); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_1: { vm_value_t v = vm_make_i4(1); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4_S: { vm_value_t v = vm_make_i4(ip->operand.byte_val); vm_stack_push(state, &v); break; }
        case CIL_OPCODE_LDC_I4:   { vm_value_t v = vm_make_i4(ip->operand.int_val); vm_stack_push(state, &v); break; }
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

        case CIL_OPCODE_RET: state->current_frame = NULL; break;
        
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

// Stubs for linker satisfaction
bool vm_shl(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_shr(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
bool vm_shr_un(vm_value_t* a, vm_value_t* b, vm_value_t* c) { return false; }
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