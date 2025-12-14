#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "il_decoder.h"

// Symbolic Expression Types
typedef enum {
    SYM_VAR,
    SYM_CONST,
    SYM_ADD,
    SYM_SUB,
    SYM_MUL,
    SYM_DIV,
    SYM_POW
} sym_type_t;

typedef struct sym_expr {
    sym_type_t type;
    int ref_count;
    union {
        char* name; // For SYM_VAR
        int64_t value; // For SYM_CONST
        struct {
            struct sym_expr* left;
            struct sym_expr* right;
        } binary;
    } data;
} sym_expr_t;

// VM Stack types
typedef enum {
    VM_TYPE_I1 = 0x01,
    VM_TYPE_U1 = 0x02,
    VM_TYPE_I2 = 0x03,
    VM_TYPE_U2 = 0x04,
    VM_TYPE_I4 = 0x05,
    VM_TYPE_U4 = 0x06,
    VM_TYPE_I8 = 0x07,
    VM_TYPE_U8 = 0x08,
    VM_TYPE_I = 0x09,
    VM_TYPE_U = 0x0A,
    VM_TYPE_R4 = 0x0B,
    VM_TYPE_R8 = 0x0C,
    VM_TYPE_REF = 0x0D,
    VM_TYPE_PTR = 0x0E,
    VM_TYPE_VALUETYPE = 0x0F,
    VM_TYPE_CLASS = 0x10,
    VM_TYPE_TYPEDBYREF = 0x16,
    VM_TYPE_METHOD = 0x18,
    VM_TYPE_SIGNATURE = 0x19,
    VM_TYPE_FNPTR = 0x1B,
    VM_TYPE_OBJECT = 0x1C,
    VM_TYPE_ARRAY = 0x1D,
    VM_TYPE_BYREF = 0x1E,
    VM_TYPE_STRING = 0x1F
} vm_type_t;

// VM Stack value
typedef struct {
    vm_type_t type;
    union {
        int8_t i1;
        uint8_t u1;
        int16_t i2;
        uint16_t u2;
        int32_t i4;
        uint32_t u4;
        int64_t i8;
        uint64_t u8;
        intptr_t i;
        uintptr_t u;
        float r4;
        double r8;
        void* ref;
    } value;
} vm_value_t;

// Object Layout Definitions
typedef struct {
    void* vtable;           // Pointer to method table
    uint32_t type_token;    // Metadata token for the type
    uint32_t sync_block;    // Sync block index / hash code
} clr_object_header_t;

typedef struct {
    clr_object_header_t header;
    uint32_t length;
    uint16_t chars[];       // Flexible array member
} clr_string_t;

typedef struct {
    clr_object_header_t header;
    uint32_t length;
    uint8_t data[];         // Flexible array member
} clr_array_t;

// VM Stack
typedef struct vm_stack {
    vm_value_t value;
    struct vm_stack* next;
} vm_stack_t;

// Runtime Type Representation (for Generics)
typedef struct clr_runtime_type clr_runtime_type_t;
struct clr_runtime_type {
    vm_type_t element_type;      // Underlying type (OBJECT, I4, etc.)
    uint32_t token;              // TypeDef or TypeRef token
    uint32_t num_generic_args;   // Number of generic arguments
    clr_runtime_type_t** generic_args; // Array of pointers to type arguments
    
    // Cached info
    uint32_t size;               // Size in bytes
    void* vtable;                // VTable for this instantiation
};

// Generic Context (Class and Method instantiations)
typedef struct {
    uint32_t class_type_arg_count;
    clr_runtime_type_t** class_type_args;
    
    uint32_t method_type_arg_count;
    clr_runtime_type_t** method_type_args;
} vm_generic_context_t;

// VM Frame (method execution context)
typedef struct vm_frame {
    cil_instruction_t* ip;              // Instruction pointer
    vm_stack_t* stack;               // Method stack
    vm_value_t* locals;              // Local variables
    uint32_t local_count;           // Number of locals
    vm_value_t* args;                // Method arguments
    uint32_t arg_count;             // Number of arguments
    struct vm_frame* caller_frame;    // Previous frame
    size_t return_address;          // Return address in caller
    vm_generic_context_t generic_context; // Context for generic parameters (!0, !!0)
} vm_frame_t;

// VM Execution State
typedef struct {
    vm_frame_t* current_frame;     // Current execution frame
    vm_stack_t* eval_stack;        // Evaluation stack
    bool has_error;              // Error state
    const char* error_message;     // Error description
    uint32_t instruction_count;    // Instructions executed
    uint32_t stack_depth_max;    // Maximum stack depth
    void* assembly;              // Pointer to il_assembly_t (void* to avoid circular dependency)
} vm_execution_state_t;

// VM Initialization
typedef struct {
    // Memory management
    void* heap_start;
    size_t heap_size;
    void* heap_current;
    
    // Method resolution
    void* method_table;
    void* type_table;
    
    // Exception handling
    vm_frame_t* exception_frame;
    
    // Statistics
    uint32_t methods_executed;
    uint32_t instructions_executed;
    uint32_t stack_overflows;
} vm_init_t;

// Execution engine functions
vm_execution_state_t* vm_create_execution_state(vm_init_t* init);
void vm_destroy_execution_state(vm_execution_state_t* state);
bool vm_execute_method(vm_execution_state_t* state, const uint8_t* il_code, size_t code_size, uint32_t local_count);

// Stack operations
bool vm_stack_push(vm_execution_state_t* state, vm_value_t* value);
bool vm_stack_pop(vm_execution_state_t* state, vm_value_t* value);
bool vm_stack_peek(vm_execution_state_t* state, vm_value_t* value);
uint32_t vm_stack_depth(vm_execution_state_t* state);

// Instruction execution
bool vm_execute_instruction(vm_execution_state_t* state);
bool vm_execute_next_instruction(vm_execution_state_t* state);

// Utility functions
vm_value_t vm_make_i1(int8_t value);
vm_value_t vm_make_i2(int16_t value);
vm_value_t vm_make_i4(int32_t value);
vm_value_t vm_make_i8(int64_t value);
vm_value_t vm_make_u1(uint8_t value);
vm_value_t vm_make_u2(uint16_t value);
vm_value_t vm_make_u4(uint32_t value);
vm_value_t vm_make_u8(uint64_t value);
vm_value_t vm_make_r4(float value);
vm_value_t vm_make_r8(double value);
vm_value_t vm_make_ref(void* value);
vm_value_t vm_make_null(void);

// Error handling functions
void vm_set_error(vm_execution_state_t* state, const char* message);
const char* vm_get_error(vm_execution_state_t* state);
bool vm_has_error(vm_execution_state_t* state);

// Type conversion
bool vm_convert(vm_value_t* source, vm_type_t target_type, vm_value_t* result);
bool vm_is_valid_conversion(vm_type_t source, vm_type_t target);

// Arithmetic operations
bool vm_add(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_subtract(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_multiply(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_divide(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_divide_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_remainder(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_remainder_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);

// Bitwise operations
bool vm_and(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_or(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_xor(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_not(vm_value_t* value, vm_value_t* result);
bool vm_neg(vm_value_t* value, vm_value_t* result);
bool vm_shl(vm_value_t* value, vm_value_t* amount, vm_value_t* result);
bool vm_shr(vm_value_t* value, vm_value_t* amount, vm_value_t* result);
bool vm_shr_un(vm_value_t* value, vm_value_t* amount, vm_value_t* result);

// Overflow arithmetic operations
bool vm_add_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_add_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_subtract_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_subtract_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_multiply_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_multiply_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_divide_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_divide_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);

// Overflow conversion operations
bool vm_convert_ovf_i1(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_u1(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_i2(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_u2(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_i4(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_u4(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_i8(vm_value_t* source, vm_value_t* result);
bool vm_convert_ovf_u8(vm_value_t* source, vm_value_t* result);

// Symbolic computing operations
bool vm_symbolic_create(vm_value_t* var_name, vm_value_t* result);
bool vm_symbolic_add(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_symbolic_mul(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_symbolic_expr(vm_value_t* left, vm_value_t* right, vm_value_t* result); // Deprecated generic
bool vm_symbolic_differentiate(vm_value_t* expr, vm_value_t* var, vm_value_t* result);
bool vm_symbolic_integrate(vm_value_t* expr, vm_value_t* var, vm_value_t* result);
bool vm_symbolic_simplify(vm_value_t* expr, vm_value_t* result);

// Comparison operations
bool vm_compare_equal(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_compare_greater(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_compare_greater_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_compare_less(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_compare_less_un(vm_value_t* left, vm_value_t* right, vm_value_t* result);

// Memory operations
bool vm_alloc_object(vm_execution_state_t* state, uint32_t size, void** result);
bool vm_free_object(vm_execution_state_t* state, void* obj);
bool vm_load_field(vm_execution_state_t* state, void* obj, uint32_t field_offset, vm_value_t* result);
bool vm_store_field(vm_execution_state_t* state, void* obj, uint32_t field_offset, vm_value_t* value);
bool vm_load_indirect(vm_value_t* addr, vm_type_t load_type, vm_value_t* result);
bool vm_store_indirect(vm_value_t* addr, vm_value_t* value);
bool vm_new_array(vm_value_t* size, vm_value_t* result);
bool vm_load_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* result);
bool vm_store_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* value);
bool vm_load_array_element_address(vm_value_t* array, vm_value_t* index, vm_value_t* result);
bool vm_get_array_length(vm_value_t* array, vm_value_t* result);
bool vm_convert_ovf_un(vm_value_t* value, vm_type_t target_type, vm_value_t* result);
bool vm_local_alloc(vm_value_t* size, vm_value_t* result);
bool vm_init_object(vm_value_t* addr, vm_value_t* type_token);
bool vm_size_of(vm_value_t* type_token, vm_value_t* result);
bool vm_ref_any_type(vm_value_t* typed_ref, vm_value_t* result);
bool vm_copy_memory(vm_value_t* src, vm_value_t* dest, vm_value_t* len);
bool vm_init_memory(vm_value_t* addr, vm_value_t* value, vm_value_t* len);
bool vm_symbolic_create(vm_value_t* var_name, vm_value_t* result);
bool vm_symbolic_expr(vm_value_t* left, vm_value_t* right, vm_value_t* result);
bool vm_symbolic_differentiate(vm_value_t* expr, vm_value_t* var, vm_value_t* result);
bool vm_symbolic_integrate(vm_value_t* expr, vm_value_t* var, vm_value_t* result);
bool vm_symbolic_simplify(vm_value_t* expr, vm_value_t* result);
bool vm_symbolic_eval(vm_value_t* expr, vm_value_t* env, vm_value_t* result);
bool vm_symbolic_match(vm_value_t* pattern, vm_value_t* expr, vm_value_t* result);
bool vm_symbolic_rewrite(vm_value_t* expr, vm_value_t* rules, vm_value_t* result);
bool vm_convert_ovf(vm_value_t* value, vm_type_t target_type, vm_value_t* result);
bool vm_call_indirect(vm_value_t* method_ptr, vm_value_t* result);
bool vm_call_virtual(vm_value_t* method_token, vm_value_t* result);
bool vm_throw_exception(vm_value_t* exception_obj);
bool vm_rethrow_exception(vm_value_t* exception_obj);
bool vm_load_array_element_any(vm_value_t* array, vm_value_t* index, vm_value_t* type_token, vm_value_t* result);
bool vm_store_array_element_any(vm_value_t* array, vm_value_t* index, vm_value_t* type_token, vm_value_t* value);
bool vm_constrained_prefix(vm_value_t* type_token);
bool vm_end_filter(vm_value_t* value);

// Missing function declarations
bool vm_new_object(vm_execution_state_t* state, clr_runtime_type_t* type, vm_value_t* result);
bool vm_load_string_constant(vm_execution_state_t* state, vm_value_t* string_token, vm_value_t* result);
bool vm_box_value(vm_value_t* value, vm_value_t* box_type, vm_value_t* result);
bool vm_unbox_value(vm_value_t* obj, vm_value_t* unbox_type, vm_value_t* result);
bool vm_load_field_object(vm_value_t* obj, vm_value_t* field_token, vm_value_t* result);
bool vm_store_field_object(vm_value_t* obj, vm_value_t* field_token, vm_value_t* value);
bool vm_load_static_field(vm_value_t* field_token, vm_value_t* result);
bool vm_store_static_field(vm_value_t* field_token, vm_value_t* value);
bool vm_load_field_address(vm_value_t* obj, vm_value_t* field_token, vm_value_t* result);
bool vm_load_static_field_address(vm_value_t* field_token, vm_value_t* result);
bool vm_cast_class(vm_value_t* obj, vm_value_t* cast_type, vm_value_t* result);
bool vm_is_instance(vm_value_t* obj, vm_value_t* test_type, vm_value_t* result);
bool vm_call_indirect(vm_value_t* method_ptr, vm_value_t* result);
bool vm_call_virtual(vm_value_t* method_token, vm_value_t* result);
bool vm_call_method(vm_execution_state_t* state, vm_value_t* method_token, vm_value_t* result);
bool vm_load_function_ptr(vm_value_t* method_token, vm_value_t* result);
bool vm_load_virtual_function_ptr(vm_value_t* obj, vm_value_t* method_token, vm_value_t* result);
bool vm_load_object(vm_value_t* addr, vm_value_t* result);
bool vm_store_object(vm_value_t* addr, vm_value_t* value);
bool vm_unbox_any(vm_value_t* obj, vm_value_t* type_token, vm_value_t* result);
bool vm_get_argument_list(vm_value_t* result);
bool vm_convert_ovf(vm_value_t* value, vm_type_t target_type, vm_value_t* result);
bool vm_convert_ovf_un(vm_value_t* value, vm_type_t target_type, vm_value_t* result);
bool vm_throw_exception(vm_value_t* exception_obj);
bool vm_rethrow_exception(vm_value_t* exception_obj);
bool vm_end_filter(vm_value_t* value);
bool vm_constrained_prefix(vm_value_t* type_token);
bool vm_copy_object(vm_value_t* dest, vm_value_t* src, vm_value_t* type_token);
bool vm_init_object(vm_value_t* addr, vm_value_t* type_token);
bool vm_load_argument(vm_value_t* arg_index, vm_value_t* result);
bool vm_load_argument_address(vm_value_t* arg_index, vm_value_t* result);
bool vm_load_local(vm_value_t* local_index, vm_value_t* result);
bool vm_load_local_address(vm_value_t* local_index, vm_value_t* result);
bool vm_store_argument(vm_value_t* arg_index, vm_value_t* value);
bool vm_store_local(vm_value_t* local_index, vm_value_t* value);
bool vm_copy_memory(vm_value_t* src, vm_value_t* dest, vm_value_t* len);
bool vm_init_memory(vm_value_t* addr, vm_value_t* value, vm_value_t* len);
bool vm_load_array_element_address(vm_value_t* array, vm_value_t* index, vm_value_t* result);
bool vm_load_array_element_any(vm_value_t* array, vm_value_t* index, vm_value_t* type_token, vm_value_t* result);
bool vm_store_array_element(vm_value_t* array, vm_value_t* index, vm_type_t element_type, vm_value_t* value);
bool vm_store_array_element_any(vm_value_t* array, vm_value_t* index, vm_value_t* type_token, vm_value_t* value);
bool vm_ref_any_type(vm_value_t* typed_ref, vm_value_t* result);
bool vm_size_of(vm_value_t* type_token, vm_value_t* result);
bool vm_get_array_length(vm_value_t* array, vm_value_t* result);
bool vm_symbolic_eval(vm_value_t* expr, vm_value_t* env, vm_value_t* result);
bool vm_symbolic_match(vm_value_t* pattern, vm_value_t* expr, vm_value_t* result);
bool vm_symbolic_rewrite(vm_value_t* expr, vm_value_t* rules, vm_value_t* result);

// Generics Support
clr_runtime_type_t* vm_parse_type_signature(void* assembly, const uint8_t** sig_ptr, vm_generic_context_t* context);
clr_runtime_type_t* vm_resolve_type_token(void* assembly, uint32_t token, vm_generic_context_t* context);

#endif // EXECUTION_ENGINE_H