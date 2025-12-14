#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include "../../include/il_decoder.h"
#include <stdio.h>
#include <stdlib.h>

// Test suite for Phase 1: Core Infrastructure & Stack Operations - Missing Opcodes
TEST_SUITE(phase1_missing_opcodes);

static void phase1_missing_setup(void) {
    // Setup code for VM execution tests
}

static void phase1_missing_teardown(void) {
    // Teardown code for VM execution tests
}

// Test REM (remainder) operation - should fail initially
TEST(test_rem_operation) {
    // Create a simple execution state
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 10, LDC_I4 3, REM
    uint8_t bytecode[] = {
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x20, 0x03, 0x00, 0x00, 0x00,  // LDC_I4 3
        0x5D,                          // REM
    };
    
    // Create IL decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Execute the method
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    // Verify stack has remainder (10 % 3 = 1)
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    // Clean up
    destroy_cil_decoder(decoder);
    vm_destroy_execution_state(state);
    
    return TEST_PASSED;
}

// Test REM_UN (unsigned remainder) operation
TEST(test_rem_un_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_U4 10, LDC_U4 3, REM_UN
    uint8_t bytecode[] = {
        0x22, 0x0A, 0x00, 0x00, 0x00,  // LDC_U4 10
        0x22, 0x03, 0x00, 0x00, 0x00,  // LDC_U4 3
        0x5E,                          // REM_UN
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_U4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.u4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test SHL (shift left) operation
TEST(test_shl_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 4, LDC_I4 2, SHL
    uint8_t bytecode[] = {
        0x20, 0x04, 0x00, 0x00, 0x00,  // LDC_I4 4
        0x20, 0x02, 0x00, 0x00, 0x00,  // LDC_I4 2
        0x62,                          // SHL
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(16, top.value.i4); // 4 << 2 = 16
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test SHR (shift right) operation
TEST(test_shr_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 16, LDC_I4 2, SHR
    uint8_t bytecode[] = {
        0x20, 0x10, 0x00, 0x00, 0x00,  // LDC_I4 16
        0x20, 0x02, 0x00, 0x00, 0x00,  // LDC_I4 2
        0x63,                          // SHR
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(4, top.value.i4); // 16 >> 2 = 4
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test SHR_UN (unsigned shift right) operation
TEST(test_shr_un_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_U4 16, LDC_U4 2, SHR_UN
    uint8_t bytecode[] = {
        0x22, 0x10, 0x00, 0x00, 0x00,  // LDC_U4 16
        0x22, 0x02, 0x00, 0x00, 0x00,  // LDC_U4 2
        0x64,                          // SHR_UN
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_U4, top.type);
    TEST_ASSERT_EQUAL(4, top.value.u4); // 16 >> 2 = 4
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDC_I8 (load 64-bit integer constant)
TEST(test_ldc_i8_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I8 123456789012345
    uint8_t bytecode[] = {
        0x21, 0x15, 0xCD, 0x5B, 0x07, 0x65, 0x4A, 0x00, 0x00,  // LDC_I8 123456789012345
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I8, top.type);
    TEST_ASSERT_EQUAL(123456789012345LL, top.value.i8);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDC_R4 (load float32 constant)
TEST(test_ldc_r4_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_R4 3.14f
    uint8_t bytecode[] = {
        0x22, 0xC3, 0xF5, 0x48, 0x40,  // LDC_R4 3.14f
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, top.type);
    TEST_ASSERT_FLOAT_EQUAL(3.14f, top.value.r4, 0.001f);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDC_R8 (load float64 constant)
TEST(test_ldc_r8_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_R8 3.141592653589793
    uint8_t bytecode[] = {
        0x23, 0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x10, 0x3F, 0xF0,  // LDC_R8 3.141592653589793
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_R8, top.type);
    TEST_ASSERT_FLOAT_EQUAL(3.141592653589793, top.value.r8, 0.0000001);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDNULL (load null reference)
TEST(test_ldnull_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDNULL
    uint8_t bytecode[] = {
        0x14,  // LDNULL
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_REF, top.type);
    TEST_ASSERT_NULL(top.value.ref);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDC_I4_M1 (load integer constant -1)
TEST(test_ldc_i4_m1_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4_M1
    uint8_t bytecode[] = {
        0x15,  // LDC_I4_M1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(-1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDC_I4_S with extended range
TEST(test_ldc_i4_s_extended_range) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4_S 127 (should work)
    uint8_t bytecode[] = {
        0x1F, 0x7F,  // LDC_I4_S 127
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(127, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test STARG_0 (store to argument 0)
TEST(test_starg_0_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 42, STARG_0
    uint8_t bytecode[] = {
        0x20, 0x2A, 0x00, 0x00, 0x00,  // LDC_I4 42
        0x0A,                          // STARG_0
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 1); // 1 local for arg0
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test STARG_S (store to argument with short index)
TEST(test_starg_s_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 99, STARG_S 5
    uint8_t bytecode[] = {
        0x20, 0x63, 0x00, 0x00, 0x00,  // LDC_I4 99
        0x13, 0x05,                    // STARG_S 5
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 6); // 6 locals for args 0-5
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test LDC_I4_S with negative values
TEST(test_ldc_i4_s_negative_values) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4_S -128 (should work)
    uint8_t bytecode[] = {
        0x1F, 0x80,  // LDC_I4_S -128 (signed byte)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(-128, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test NEG (negate) operation
TEST(test_neg_operation) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 42, NEG
    uint8_t bytecode[] = {
        0x20, 0x2A, 0x00, 0x00, 0x00,  // LDC_I4 42
        0x65,                          // NEG
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result); // Should succeed after implementation
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(-42, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Main test runner
int main(void) {
    printf("=== Phase 1: Core Infrastructure & Stack Operations - Missing Opcodes Tests ===\n");
    printf("TDD RED Phase: Testing missing opcodes that should initially fail\n\n");
    
    // Register all test cases
    test_register_case(&test_rem_operation);
    test_register_case(&test_rem_un_operation);
    test_register_case(&test_shl_operation);
    test_register_case(&test_shr_operation);
    test_register_case(&test_shr_un_operation);
    test_register_case(&test_ldc_i8_operation);
    test_register_case(&test_ldc_r4_operation);
    test_register_case(&test_ldc_r8_operation);
    test_register_case(&test_ldnull_operation);
    test_register_case(&test_ldc_i4_m1_operation);
    test_register_case(&test_ldc_i4_s_extended_range);
    test_register_case(&test_starg_0_operation);
    test_register_case(&test_starg_s_operation);
    test_register_case(&test_ldc_i4_s_negative_values);
    test_register_case(&test_neg_operation);
    
    // Run all tests
    test_run_all();
    
    return 0;
}