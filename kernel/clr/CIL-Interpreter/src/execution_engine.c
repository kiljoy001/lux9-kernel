#include "../include/execution_engine.h"
#include "../../../overflow.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

// Create a new execution state
vm_execution_state_t* vm_create_execution_state(vm_init_t* init) {
    vm_execution_state_t* state = (vm_execution_state_t*)malloc(sizeof(vm_execution_state_t));
    if (state == NULL) {
        return NULL;
    }
    
    // Initialize the state
    memset(state, 0, sizeof(vm_execution_state_t));
    
    return state;
}

// Destroy an execution state
void vm_destroy_execution_state(vm_execution_state_t* state) {
    if (state != NULL) {
        // Clean up stacks and frames
        while (state->eval_stack != NULL) {
            vm_stack_t* temp = state->eval_stack;
            state->eval_stack = state->eval_stack->next;
            free(temp);
        }
        
        free(state);
    }
}

// Execute a method with IL bytecode
bool vm_execute_method(vm_execution_state_t* state, const uint8_t* il_code, size_t code_size, uint32_t local_count) {
    if (state == NULL || il_code == NULL || code_size == 0) {
        return false;
    }
    
    // Create a new frame for this method
    vm_frame_t* frame = (vm_frame_t*)malloc(sizeof(vm_frame_t));
    if (frame == NULL) {
        vm_set_error(state, "Failed to allocate method frame");
        return false;
    }
    
    // Initialize the frame
    memset(frame, 0, sizeof(vm_frame_t));
    frame->local_count = local_count;
    frame->locals = (vm_value_t*)calloc(local_count, sizeof(vm_value_t));
    
    // Create a decoder for this IL code
    cil_decoder_t* decoder = create_cil_decoder(il_code, code_size);
    if (decoder == NULL) {
        free(frame);
        vm_set_error(state, "Failed to create IL decoder");
        return false;
    }
    
    // Set up the frame
    frame->ip = decode_next_instruction(decoder);
    state->current_frame = frame;
    state->instruction_count = 0;
    state->stack_depth_max = 0;
    
    // Execute instructions
    while (frame->ip != NULL && !vm_has_error(state)) {
        if (!vm_execute_instruction(state)) {
            break;
        }
        
        // Move to next instruction
        frame->ip = decode_next_instruction(decoder);
        state->instruction_count++;
    }
    
    // Clean up
    destroy_cil_decoder(decoder);
    free(frame->locals);
    free(frame);
    
    return !vm_has_error(state);
}

// Push a value onto the stack
bool vm_stack_push(vm_execution_state_t* state, vm_value_t* value) {
    if (state == NULL || value == NULL) {
        return false;
    }
    
    vm_stack_t* new_node = (vm_stack_t*)malloc(sizeof(vm_stack_t));
    if (new_node == NULL) {
        return false;
    }
    
    new_node->value = *value;
    new_node->next = state->eval_stack;
    state->eval_stack = new_node;
    
    // Update max depth
    uint32_t current_depth = vm_stack_depth(state);
    if (current_depth > state->stack_depth_max) {
        state->stack_depth_max = current_depth;
    }
    
    return true;
}

// Pop a value from the stack
bool vm_stack_pop(vm_execution_state_t* state, vm_value_t* value) {
    if (state == NULL || value == NULL || state->eval_stack == NULL) {
        return false;
    }
    
    vm_stack_t* top = state->eval_stack;
    *value = top->value;
    state->eval_stack = top->next;
    free(top);
    
    return true;
}

// Peek at the top of the stack
bool vm_stack_peek(vm_execution_state_t* state, vm_value_t* value) {
    if (state == NULL || value == NULL || state->eval_stack == NULL) {
        return false;
    }
    
    *value = state->eval_stack->value;
    return true;
}

// Get the current stack depth
uint32_t vm_stack_depth(vm_execution_state_t* state) {
    if (state == NULL) {
        return 0;
    }
    
    uint32_t depth = 0;
    vm_stack_t* current = state->eval_stack;
    while (current != NULL) {
        depth++;
        current = current->next;
    }
    
    return depth;
}

// Create a VM value from an int8
vm_value_t vm_make_i1(int8_t value) {
    vm_value_t result;
    result.type = VM_TYPE_I1;
    result.value.i1 = value;
    return result;
}

// Create a VM value from an int16
vm_value_t vm_make_i2(int16_t value) {
    vm_value_t result;
    result.type = VM_TYPE_I2;
    result.value.i2 = value;
    return result;
}

// Create a VM value from an int32
vm_value_t vm_make_i4(int32_t value) {
    vm_value_t result;
    result.type = VM_TYPE_I4;
    result.value.i4 = value;
    return result;
}

// Create a VM value from an int64
vm_value_t vm_make_i8(int64_t value) {
    vm_value_t result;
    result.type = VM_TYPE_I8;
    result.value.i8 = value;
    return result;
}

// Create a VM value from an uint8
vm_value_t vm_make_u1(uint8_t value) {
    vm_value_t result;
    result.type = VM_TYPE_U1;
    result.value.u1 = value;
    return result;
}

// Create a VM value from an uint16
vm_value_t vm_make_u2(uint16_t value) {
    vm_value_t result;
    result.type = VM_TYPE_U2;
    result.value.u2 = value;
    return result;
}

// Create a VM value from an uint32
vm_value_t vm_make_u4(uint32_t value) {
    vm_value_t result;
    result.type = VM_TYPE_U4;
    result.value.u4 = value;
    return result;
}

// Create a VM value from an uint64
vm_value_t vm_make_u8(uint64_t value) {
    vm_value_t result;
    result.type = VM_TYPE_U8;
    result.value.u8 = value;
    return result;
}

// Create a VM value from a float
vm_value_t vm_make_r4(float value) {
    vm_value_t result;
    result.type = VM_TYPE_R4;
    result.value.r4 = value;
    return result;
}

// Create a VM value from a double
vm_value_t vm_make_r8(double value) {
    vm_value_t result;
    result.type = VM_TYPE_R8;
    result.value.r8 = value;
    return result;
}

// Create a VM value from a reference
vm_value_t vm_make_ref(void* value) {
    vm_value_t result;
    result.type = VM_TYPE_REF;
    result.value.ref = value;
    return result;
}

// Create a null reference
vm_value_t vm_make_null(void) {
    return vm_make_ref(NULL);
}

// Check if a type is an integer type
bool vm_is_integer_type(vm_type_t type) {
    return type >= VM_TYPE_I1 && type <= VM_TYPE_I8;
}

// Check if a type is a floating-point type
bool vm_is_floating_point_type(vm_type_t type) {
    return type == VM_TYPE_R4 || type == VM_TYPE_R8;
}

// Check if a type is a reference type
bool vm_is_reference_type(vm_type_t type) {
    return type == VM_TYPE_REF || type == VM_TYPE_STRING || type == VM_TYPE_OBJECT;
}

// Check if a type is signed
bool vm_is_signed_type(vm_type_t type) {
    switch (type) {
        case VM_TYPE_I1:
        case VM_TYPE_I2:
        case VM_TYPE_I4:
        case VM_TYPE_I8:
        case VM_TYPE_I:
        case VM_TYPE_R4:
        case VM_TYPE_R8:
            return true;
        default:
            return false;
    }
}

// Get the name of a type
const char* vm_type_name(vm_type_t type) {
    switch (type) {
        case VM_TYPE_I1: return "int8";
        case VM_TYPE_U1: return "uint8";
        case VM_TYPE_I2: return "int16";
        case VM_TYPE_U2: return "uint16";
        case VM_TYPE_I4: return "int32";
        case VM_TYPE_U4: return "uint32";
        case VM_TYPE_I8: return "int64";
        case VM_TYPE_U8: return "uint64";
        case VM_TYPE_I: return "int";
        case VM_TYPE_U: return "uint";
        case VM_TYPE_R4: return "float32";
        case VM_TYPE_R8: return "float64";
        case VM_TYPE_REF: return "ref";
        case VM_TYPE_PTR: return "ptr";
        case VM_TYPE_STRING: return "string";
        case VM_TYPE_OBJECT: return "object";
        case VM_TYPE_ARRAY: return "array";
        case VM_TYPE_BYREF: return "byref";
        case VM_TYPE_VALUETYPE: return "valuetype";
        case VM_TYPE_CLASS: return "class";
        case VM_TYPE_FNPTR: return "fnptr";
        case VM_TYPE_TYPEDBYREF: return "typedbyref";
        case VM_TYPE_METHOD: return "method";
        case VM_TYPE_SIGNATURE: return "signature";
        default: return "unknown";
    }
}

// Set an error in the execution state
void vm_set_error(vm_execution_state_t* state, const char* message) {
    if (state != NULL) {
        state->has_error = true;
        state->error_message = message;
    }
}

// Get the current error message
const char* vm_get_error(vm_execution_state_t* state) {
    return state != NULL ? state->error_message : NULL;
}

// Check if there's an error
bool vm_has_error(vm_execution_state_t* state) {
    return state != NULL && state->has_error;
}

// Clear any error state
void vm_clear_error(vm_execution_state_t* state) {
    if (state != NULL) {
        state->has_error = false;
        state->error_message = NULL;
    }
}

// Get the instruction count
uint32_t vm_get_instruction_count(vm_execution_state_t* state) {
    return state != NULL ? state->instruction_count : 0;
}

// Get the current stack depth
uint32_t vm_get_stack_depth(vm_execution_state_t* state) {
    return vm_stack_depth(state);
}

// Get the maximum stack depth
uint32_t vm_get_max_stack_depth(vm_execution_state_t* state) {
    return state != NULL ? state->stack_depth_max : 0;
}

// Execute a single instruction
bool vm_execute_instruction(vm_execution_state_t* state) {
    if (state == NULL || state->current_frame == NULL || state->current_frame->ip == NULL) {
        return false;
    }
    
    cil_instruction_t* ip = state->current_frame->ip;
    
    switch (ip->opcode) {
        case CIL_OPCODE_NOP:
            // No operation
            break;
            
        case CIL_OPCODE_ADD: {
            vm_value_t right, left, result;
            if (!vm_stack_pop(state, &right) || !vm_stack_pop(state, &left)) {
                vm_set_error(state, "ADD: Insufficient stack operands");
                return false;
            }
            
            if (!vm_add(&left, &right, &result)) {
                vm_set_error(state, "ADD: Type mismatch");
                return false;
            }
            
            vm_stack_push(state, &result);
            break;
        }
        
        case CIL_OPCODE_SUB: {
            vm_value_t right, left, result;
            if (!vm_stack_pop(state, &right) || !vm_stack_pop(state, &left)) {
                vm_set_error(state, "SUB: Insufficient stack operands");
                return false;
            }
            
            if (!vm_subtract(&left, &right, &result)) {
                vm_set_error(state, "SUB: Type mismatch");
                return false;
            }
            
            vm_stack_push(state, &result);
            break;
        }
        
        case CIL_OPCODE_MUL: {
            vm_value_t right, left, result;
            if (!vm_stack_pop(state, &right) || !vm_stack_pop(state, &left)) {
                vm_set_error(state, "MUL: Insufficient stack operands");
                return false;
            }
            
            if (!vm_multiply(&left, &right, &result)) {
                vm_set_error(state, "MUL: Type mismatch");
                return false;
            }
            
            vm_stack_push(state, &result);
            break;
        }
        
        case CIL_OPCODE_DIV: {
            vm_value_t right, left, result;
            if (!vm_stack_pop(state, &right) || !vm_stack_pop(state, &left)) {
                vm_set_error(state, "DIV: Insufficient stack operands");
                return false;
            }
            
            // Check for division by zero before calling vm_divide
            if (right.type == VM_TYPE_I4 && right.value.i4 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_I8 && right.value.i8 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_I2 && right.value.i2 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_I1 && right.value.i1 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_U4 && right.value.u4 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_U8 && right.value.u8 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_U2 && right.value.u2 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            if (right.type == VM_TYPE_U1 && right.value.u1 == 0) {
                vm_set_error(state, "DIV: Division by zero");
                return false;
            }
            
            if (!vm_divide(&left, &right, &result)) {
                vm_set_error(state, "DIV: Type mismatch");
                return false;
            }
            
            vm_stack_push(state, &result);
            break;
        }
        
        case CIL_OPCODE_LDC_I4_0: {
            vm_value_t val = vm_make_i4(0);
            vm_stack_push(state, &val);
            break;
        }
            
        case CIL_OPCODE_LDC_I4_1: {
            vm_value_t val = vm_make_i4(1);
            vm_stack_push(state, &val);
            break;
        }
            
        case CIL_OPCODE_LDC_I4_S: {
            vm_value_t val = vm_make_i4(ip->operand.byte_val);
            vm_stack_push(state, &val);
            break;
        }
            
        case CIL_OPCODE_LDC_I4: {
            vm_value_t val = vm_make_i4(ip->operand.int_val);
            vm_stack_push(state, &val);
            break;
        }
            
        case CIL_OPCODE_DUP: {
            vm_value_t top;
            if (!vm_stack_peek(state, &top)) {
                vm_set_error(state, "DUP: Stack underflow");
                return false;
            }
            vm_stack_push(state, &top);
            break;
        }
        
        case CIL_OPCODE_POP: {
            vm_value_t top;
            if (!vm_stack_pop(state, &top)) {
                vm_set_error(state, "POP: Stack underflow");
                return false;
            }
            break;
        }
        
        case CIL_OPCODE_RET: {
            // Method return - just mark the frame as complete
            state->current_frame = NULL;
            break;
        }
        
        default:
            vm_set_error(state, "Unsupported opcode");
            return false;
    }
    
    return true;
}

// Execute the next instruction
bool vm_execute_next_instruction(vm_execution_state_t* state) {
    return vm_execute_instruction(state);
}

// Arithmetic operations
bool vm_add(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle all integer types
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        result->type = VM_TYPE_I1;
        result->value.i1 = left->value.i1 + right->value.i1;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        result->type = VM_TYPE_I2;
        result->value.i2 = left->value.i2 + right->value.i2;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 + right->value.i4;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 + right->value.i8;
        return true;
    }
    
    // Handle unsigned integer types
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        result->type = VM_TYPE_U1;
        result->value.u1 = left->value.u1 + right->value.u1;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        result->type = VM_TYPE_U2;
        result->value.u2 = left->value.u2 + right->value.u2;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 + right->value.u4;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 + right->value.u8;
        return true;
    }
    
    // Handle floating-point types
    if (left->type == VM_TYPE_R4 && right->type == VM_TYPE_R4) {
        result->type = VM_TYPE_R4;
        result->value.r4 = left->value.r4 + right->value.r4;
        return true;
    }
    if (left->type == VM_TYPE_R8 && right->type == VM_TYPE_R8) {
        result->type = VM_TYPE_R8;
        result->value.r8 = left->value.r8 + right->value.r8;
        return true;
    }
    
    return false;
}

bool vm_subtract(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle all integer types
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        result->type = VM_TYPE_I1;
        result->value.i1 = left->value.i1 - right->value.i1;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        result->type = VM_TYPE_I2;
        result->value.i2 = left->value.i2 - right->value.i2;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 - right->value.i4;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 - right->value.i8;
        return true;
    }
    
    // Handle unsigned integer types
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        result->type = VM_TYPE_U1;
        result->value.u1 = left->value.u1 - right->value.u1;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        result->type = VM_TYPE_U2;
        result->value.u2 = left->value.u2 - right->value.u2;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 - right->value.u4;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 - right->value.u8;
        return true;
    }
    
    // Handle floating-point types
    if (left->type == VM_TYPE_R4 && right->type == VM_TYPE_R4) {
        result->type = VM_TYPE_R4;
        result->value.r4 = left->value.r4 - right->value.r4;
        return true;
    }
    if (left->type == VM_TYPE_R8 && right->type == VM_TYPE_R8) {
        result->type = VM_TYPE_R8;
        result->value.r8 = left->value.r8 - right->value.r8;
        return true;
    }
    
    return false;
}

bool vm_multiply(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle all integer types
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        result->type = VM_TYPE_I1;
        result->value.i1 = left->value.i1 * right->value.i1;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        result->type = VM_TYPE_I2;
        result->value.i2 = left->value.i2 * right->value.i2;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 * right->value.i4;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 * right->value.i8;
        return true;
    }
    
    // Handle unsigned integer types
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        result->type = VM_TYPE_U1;
        result->value.u1 = left->value.u1 * right->value.u1;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        result->type = VM_TYPE_U2;
        result->value.u2 = left->value.u2 * right->value.u2;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 * right->value.u4;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 * right->value.u8;
        return true;
    }
    
    // Handle floating-point types
    if (left->type == VM_TYPE_R4 && right->type == VM_TYPE_R4) {
        result->type = VM_TYPE_R4;
        result->value.r4 = left->value.r4 * right->value.r4;
        return true;
    }
    if (left->type == VM_TYPE_R8 && right->type == VM_TYPE_R8) {
        result->type = VM_TYPE_R8;
        result->value.r8 = left->value.r8 * right->value.r8;
        return true;
    }
    
    return false;
}

bool vm_divide(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle all integer types with division by zero check
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        if (right->value.i1 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_I1;
        result->value.i1 = left->value.i1 / right->value.i1;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        if (right->value.i2 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_I2;
        result->value.i2 = left->value.i2 / right->value.i2;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        if (right->value.i4 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 / right->value.i4;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        if (right->value.i8 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 / right->value.i8;
        return true;
    }
    
    // Handle unsigned integer types with division by zero check
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        if (right->value.u1 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U1;
        result->value.u1 = left->value.u1 / right->value.u1;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        if (right->value.u2 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U2;
        result->value.u2 = left->value.u2 / right->value.u2;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (right->value.u4 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 / right->value.u4;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        if (right->value.u8 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 / right->value.u8;
        return true;
    }
    
    // Handle floating-point types (division by zero allowed, results in inf/nan)
    if (left->type == VM_TYPE_R4 && right->type == VM_TYPE_R4) {
        result->type = VM_TYPE_R4;
        result->value.r4 = left->value.r4 / right->value.r4;
        return true;
    }
    if (left->type == VM_TYPE_R8 && right->type == VM_TYPE_R8) {
        result->type = VM_TYPE_R8;
        result->value.r8 = left->value.r8 / right->value.r8;
        return true;
    }
    
    return false;
}

// ==================== OVERFLOW ARITHMETIC OPERATIONS ====================

// Helper function to check for signed overflow in addition
static bool check_add_signed_overflow(vm_type_t type, int64_t a, int64_t b) {
    if (type == VM_TYPE_I4) {
        // For int32: check if a + b would overflow
        if (a > 0 && b > 0 && a > INT64_MAX - b) return true;
        if (a < 0 && b < 0 && a < INT64_MIN - b) return true;
        return false;
    }
    if (type == VM_TYPE_I2) {
        // For int16: check if a + b would overflow
        if (a > 0 && b > 0 && a > INT64_MAX - b) return true;
        if (a < 0 && b < 0 && a < INT64_MIN - b) return true;
        return false;
    }
    if (type == VM_TYPE_I1) {
        // For int8: check if a + b would overflow
        if (a > 0 && b > 0 && a > INT64_MAX - b) return true;
        if (a < 0 && b < 0 && a < INT64_MIN - b) return true;
        return false;
    }
    return false;
}

// Helper function to check for signed overflow in subtraction
static bool check_subtract_signed_overflow(vm_type_t type, int64_t a, int64_t b) {
    if (type == VM_TYPE_I4) {
        // For int32: check if a - b would overflow
        if (a > 0 && b < 0 && a > INT64_MAX + b) return true;
        if (a < 0 && b > 0 && a < INT64_MIN + b) return true;
        return false;
    }
    if (type == VM_TYPE_I2) {
        // For int16: check if a - b would overflow
        if (a > 0 && b < 0 && a > INT64_MAX + b) return true;
        if (a < 0 && b > 0 && a < INT64_MIN + b) return true;
        return false;
    }
    if (type == VM_TYPE_I1) {
        // For int8: check if a - b would overflow
        if (a > 0 && b < 0 && a > INT64_MAX + b) return true;
        if (a < 0 && b > 0 && a < INT64_MIN + b) return true;
        return false;
    }
    return false;
}

// Helper function to check for signed overflow in multiplication
static bool check_multiply_signed_overflow(vm_type_t type, int64_t a, int64_t b) {
    if (type == VM_TYPE_I4) {
        // For int32: check if a * b would overflow
        if (a > 0 && b > 0 && a > INT64_MAX / b) return true;
        if (a < 0 && b < 0 && a < INT64_MAX / b) return true;
        if (a > 0 && b < 0 && b < INT64_MIN / a) return true;
        if (a < 0 && b > 0 && a < INT64_MIN / b) return true;
        return false;
    }
    if (type == VM_TYPE_I2) {
        // For int16: check if a * b would overflow
        if (a > 0 && b > 0 && a > INT64_MAX / b) return true;
        if (a < 0 && b < 0 && a < INT64_MAX / b) return true;
        if (a > 0 && b < 0 && b < INT64_MIN / a) return true;
        if (a < 0 && b > 0 && a < INT64_MIN / b) return true;
        return false;
    }
    if (type == VM_TYPE_I1) {
        // For int8: check if a * b would overflow
        if (a > 0 && b > 0 && a > INT64_MAX / b) return true;
        if (a < 0 && b < 0 && a < INT64_MAX / b) return true;
        if (a > 0 && b < 0 && b < INT64_MIN / a) return true;
        if (a < 0 && b > 0 && a < INT64_MIN / b) return true;
        return false;
    }
    return false;
}

// Helper function for unsigned overflow
static bool check_unsigned_overflow(vm_type_t type, uint64_t a, uint64_t b) {
    if (type == VM_TYPE_I4) {
        // For uint32: check if a + b would overflow
        return a > UINT64_MAX - b;
    }
    if (type == VM_TYPE_I2) {
        // For uint16: check if a + b would overflow
        return a > UINT64_MAX - b;
    }
    if (type == VM_TYPE_I1) {
        // For uint8: check if a + b would overflow
        return a > UINT64_MAX - b;
    }
    return false;
}

// Helper function for unsigned underflow
static bool check_unsigned_underflow(vm_type_t type, uint64_t a, uint64_t b) {
    if (type == VM_TYPE_I4) {
        // For uint32: check if a - b would underflow
        return a < b;
    }
    if (type == VM_TYPE_I2) {
        // For uint16: check if a - b would underflow
        return a < b;
    }
    if (type == VM_TYPE_I1) {
        // For uint8: check if a - b would underflow
        return a < b;
    }
    return false;
}

bool vm_add_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle signed integer types with overflow check
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        int16_t temp = (int16_t)left->value.i1 + (int16_t)right->value.i1;
        if (check_signed_overflow(VM_TYPE_I1, left->value.i1, right->value.i1, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I1;
        result->value.i1 = (int8_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        int32_t temp = (int32_t)left->value.i2 + (int32_t)right->value.i2;
        if (check_signed_overflow(VM_TYPE_I2, left->value.i2, right->value.i2, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I2;
        result->value.i2 = (int16_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 + (int64_t)right->value.i4;
        if (check_signed_overflow(VM_TYPE_I4, left->value.i4, right->value.i4, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        // For int64, we can't easily check overflow in portable way
        // In a real implementation, we'd use compiler intrinsics or manual checks
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 + right->value.i8;
        return true; // Assume no overflow for int64 for now
    }
    
    return false;
}

bool vm_add_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle unsigned integer types with overflow check
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        uint16_t temp = (uint16_t)left->value.u1 + (uint16_t)right->value.u1;
        if (check_unsigned_overflow(VM_TYPE_U1, left->value.u1, right->value.u1, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_U1;
        result->value.u1 = (uint8_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        uint32_t temp = (uint32_t)left->value.u2 + (uint32_t)right->value.u2;
        if (check_unsigned_overflow(VM_TYPE_U2, left->value.u2, right->value.u2, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_U2;
        result->value.u2 = (uint16_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        uint64_t temp = (uint64_t)left->value.u4 + (uint64_t)right->value.u4;
        if (check_unsigned_overflow(VM_TYPE_U4, left->value.u4, right->value.u4, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_U4;
        result->value.u4 = (uint32_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 + right->value.u8;
        return true; // Assume no overflow for uint64 for now
    }
    
    return false;
}

bool vm_subtract_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle signed integer types with overflow check
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        int16_t temp = (int16_t)left->value.i1 - (int16_t)right->value.i1;
        if (check_signed_overflow(VM_TYPE_I1, left->value.i1, -right->value.i1, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I1;
        result->value.i1 = (int8_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        int32_t temp = (int32_t)left->value.i2 - (int32_t)right->value.i2;
        if (check_signed_overflow(VM_TYPE_I2, left->value.i2, -right->value.i2, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I2;
        result->value.i2 = (int16_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 - (int64_t)right->value.i4;
        if (check_signed_overflow(VM_TYPE_I4, left->value.i4, -right->value.i4, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 - right->value.i8;
        return true; // Assume no overflow for int64 for now
    }
    
    return false;
}

bool vm_subtract_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle unsigned integer types with overflow check (underflow)
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        if (left->value.u1 < right->value.u1) {
            return false; // Underflow
        }
        result->type = VM_TYPE_U1;
        result->value.u1 = left->value.u1 - right->value.u1;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        if (left->value.u2 < right->value.u2) {
            return false; // Underflow
        }
        result->type = VM_TYPE_U2;
        result->value.u2 = left->value.u2 - right->value.u2;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (left->value.u4 < right->value.u4) {
            return false; // Underflow
        }
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 - right->value.u4;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        if (left->value.u8 < right->value.u8) {
            return false; // Underflow
        }
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 - right->value.u8;
        return true;
    }
    
    return false;
}

bool vm_multiply_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle signed integer types with overflow check
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        int16_t temp = (int16_t)left->value.i1 * (int16_t)right->value.i1;
        if (check_signed_overflow(VM_TYPE_I1, left->value.i1, right->value.i1, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I1;
        result->value.i1 = (int8_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        int32_t temp = (int32_t)left->value.i2 * (int32_t)right->value.i2;
        if (check_signed_overflow(VM_TYPE_I2, left->value.i2, right->value.i2, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I2;
        result->value.i2 = (int16_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 * (int64_t)right->value.i4;
        if (check_signed_overflow(VM_TYPE_I4, left->value.i4, right->value.i4, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 * right->value.i8;
        return true; // Assume no overflow for int64 for now
    }
    
    return false;
}

bool vm_multiply_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // Handle unsigned integer types with overflow check
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        uint16_t temp = (uint16_t)left->value.u1 * (uint16_t)right->value.u1;
        if (check_unsigned_overflow(VM_TYPE_U1, left->value.u1, right->value.u1, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_U1;
        result->value.u1 = (uint8_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        uint32_t temp = (uint32_t)left->value.u2 * (uint32_t)right->value.u2;
        if (check_unsigned_overflow(VM_TYPE_U2, left->value.u2, right->value.u2, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_U2;
        result->value.u2 = (uint16_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        uint64_t temp = (uint64_t)left->value.u4 * (uint64_t)right->value.u4;
        if (check_unsigned_overflow(VM_TYPE_U4, left->value.u4, right->value.u4, temp)) {
            return false; // Overflow
        }
        result->type = VM_TYPE_U4;
        result->value.u4 = (uint32_t)temp;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 * right->value.u8;
        return true; // Assume no overflow for uint64 for now
    }
    
    return false;
}

bool vm_divide_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // For division, overflow can only happen with INT_MIN / -1
    if (left->type == VM_TYPE_I1 && right->type == VM_TYPE_I1) {
        if (right->value.i1 == 0) {
            return false; // Division by zero
        }
        if (left->value.i1 == INT8_MIN && right->value.i1 == -1) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I1;
        result->value.i1 = left->value.i1 / right->value.i1;
        return true;
    }
    if (left->type == VM_TYPE_I2 && right->type == VM_TYPE_I2) {
        if (right->value.i2 == 0) {
            return false; // Division by zero
        }
        if (left->value.i2 == INT16_MIN && right->value.i2 == -1) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I2;
        result->value.i2 = left->value.i2 / right->value.i2;
        return true;
    }
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        if (right->value.i4 == 0) {
            return false; // Division by zero
        }
        if (left->value.i4 == INT32_MIN && right->value.i4 == -1) {
            return false; // Overflow
        }
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 / right->value.i4;
        return true;
    }
    if (left->type == VM_TYPE_I8 && right->type == VM_TYPE_I8) {
        if (right->value.i8 == 0) {
            return false; // Division by zero
        }
        // For int64 division, INT64_MIN / -1 overflow is implementation-defined
        // We'll assume it's handled correctly by the platform
        result->type = VM_TYPE_I8;
        result->value.i8 = left->value.i8 / right->value.i8;
        return true;
    }
    
    return false;
}

bool vm_divide_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left == NULL || right == NULL || result == NULL) {
        return false;
    }
    
    // For unsigned division, overflow only happens with division by zero
    if (left->type == VM_TYPE_U1 && right->type == VM_TYPE_U1) {
        if (right->value.u1 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U1;
        result->value.u1 = left->value.u1 / right->value.u1;
        return true;
    }
    if (left->type == VM_TYPE_U2 && right->type == VM_TYPE_U2) {
        if (right->value.u2 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U2;
        result->value.u2 = left->value.u2 / right->value.u2;
        return true;
    }
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (right->value.u4 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 / right->value.u4;
        return true;
    }
    if (left->type == VM_TYPE_U8 && right->type == VM_TYPE_U8) {
        if (right->value.u8 == 0) {
            return false; // Division by zero
        }
        result->type = VM_TYPE_U8;
        result->value.u8 = left->value.u8 / right->value.u8;
        return true;
    }
    
    return false;
}

// Comparison operations
bool vm_compare_equal(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    // TODO: Implement comparison operations
    return false;
}

bool vm_compare_greater(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    // TODO: Implement comparison operations
    return false;
}

bool vm_compare_less(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    // TODO: Implement comparison operations
    return false;
}

// Type conversion
bool vm_convert(vm_value_t* source, vm_type_t target_type, vm_value_t* result) {
    // TODO: Implement type conversions
    return false;
}

bool vm_is_valid_conversion(vm_type_t source, vm_type_t target) {
    // TODO: Implement conversion validation
    return false;
}

// Memory operations
bool vm_alloc_object(vm_execution_state_t* state, uint32_t size, void** result) {
    // TODO: Implement object allocation
    return false;
}

bool vm_free_object(vm_execution_state_t* state, void* obj) {
    // TODO: Implement object deallocation
    return false;
}

bool vm_load_field(vm_execution_state_t* state, void* obj, uint32_t field_offset, vm_value_t* result) {
    // TODO: Implement field loading
    return false;
}

bool vm_store_field(vm_execution_state_t* state, void* obj, uint32_t field_offset, vm_value_t* value) {
    // TODO: Implement field storing
    return false;
}