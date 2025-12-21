#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include "../../include/il_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Test suite for Phase 1: Missing Control Flow Operations (TDD Red Phase)
TEST_SUITE(phase1_missing_control_flow);

static void phase1_missing_control_flow_setup(void) {
    // Setup code for control flow tests
}

static void phase1_missing_control_flow_teardown(void) {
    // Teardown code for control flow tests
}

// Test BEQ_S (short branch if equal)
TEST(test_beq_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 10, LDC_I4 10, BEQ_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 and execute LDC_I4 1 when equal
    uint8_t bytecode[] = {
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x2E, 0x03,                    // BEQ_S +3 (skip next 3 bytes)
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (should be skipped)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    // Should have value 1 on stack (equal branch taken)
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BEQ_S fallthrough (not equal)
TEST(test_beq_s_fallthrough) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 10, LDC_I4 20, BEQ_S +3, LDC_I4 0, LDC_I4 1
    // Should execute LDC_I4 0 (not equal, fall through)
    uint8_t bytecode[] = {
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x20, 0x14, 0x00, 0x00, 0x00,  // LDC_I4 20
        0x2E, 0x03,                    // BEQ_S +3 (skip next 3 bytes)
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (should execute)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    // Should have values 1 then 0 on stack (branch not taken)
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(0, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BGE_S (short branch if greater than or equal)
TEST(test_bge_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 20, LDC_I4 10, BGE_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (20 >= 10)
    uint8_t bytecode[] = {
        0x20, 0x14, 0x00, 0x00, 0x00,  // LDC_I4 20
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x2F, 0x03,                    // BGE_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BGE_S equal case
TEST(test_bge_s_equal) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 10, LDC_I4 10, BGE_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (10 >= 10)
    uint8_t bytecode[] = {
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x2F, 0x03,                    // BGE_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BGT_S (short branch if greater than)
TEST(test_bgt_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 20, LDC_I4 10, BGT_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (20 > 10)
    uint8_t bytecode[] = {
        0x20, 0x14, 0x00, 0x00, 0x00,  // LDC_I4 20
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x30, 0x03,                    // BGT_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BLE_S (short branch if less than or equal)
TEST(test_ble_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 5, LDC_I4 10, BLE_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (5 <= 10)
    uint8_t bytecode[] = {
        0x20, 0x05, 0x00, 0x00, 0x00,  // LDC_I4 5
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x31, 0x03,                    // BLE_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BLT_S (short branch if less than)
TEST(test_blt_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 5, LDC_I4 10, BLT_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (5 < 10)
    uint8_t bytecode[] = {
        0x20, 0x05, 0x00, 0x00, 0x00,  // LDC_I4 5
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x32, 0x03,                    // BLT_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BNE_UN_S (short unsigned branch if not equal)
TEST(test_bne_un_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_U4 10, LDC_U4 20, BNE_UN_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (10 != 20)
    uint8_t bytecode[] = {
        0x22, 0x0A, 0x00, 0x00, 0x00,  // LDC_U4 10
        0x22, 0x14, 0x00, 0x00, 0x00,  // LDC_U4 20
        0x33, 0x03,                    // BNE_UN_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test BGE_UN_S (short unsigned branch if greater than or equal)
TEST(test_bge_un_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_U4 0xFFFFFFFF, LDC_U4 0x7FFFFFFF, BGE_UN_S +3, LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0 (unsigned comparison: 0xFFFFFFFF >= 0x7FFFFFFF)
    uint8_t bytecode[] = {
        0x22, 0xFF, 0xFF, 0xFF, 0xFF,  // LDC_U4 0xFFFFFFFF
        0x22, 0xFF, 0xFF, 0xFF, 0x7F,  // LDC_U4 0x7FFFFFFF
        0x34, 0x03,                    // BGE_UN_S +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test long branch BGE (branch if greater than or equal)
TEST(test_bge_long_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Bytecode: LDC_I4 20, LDC_I4 10, BGE +3 (3 bytes offset), LDC_I4 0, LDC_I4 1
    // Should skip LDC_I4 0
    uint8_t bytecode[] = {
        0x20, 0x14, 0x00, 0x00, 0x00,  // LDC_I4 20
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x3C, 0x03, 0x00, 0x00, 0x00,  // BGE +3
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (skip)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    TEST_ASSERT_TRUE(result);
    
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Main test runner
int main(void) {
    printf("=== Phase 1: Missing Control Flow Operations - TDD Red Phase ===\n");
    printf("These tests should FAIL initially (opcodes not implemented)\n");
    printf("Running tests with TDD approach...\n\n");
    
    // Set verbose mode for better debugging
    test_runner_state.verbose = true;
    
    // Register all test cases (use the wrapper functions created by TEST macro)
    test_register_suite(&suite_phase1_missing_control_flow);
    test_register_case(&test_beq_s_short_branch_wrapper);
    test_register_case(&test_beq_s_fallthrough_wrapper);
    test_register_case(&test_bge_s_short_branch_wrapper);
    test_register_case(&test_bge_s_equal_wrapper);
    test_register_case(&test_bgt_s_short_branch_wrapper);
    test_register_case(&test_ble_s_short_branch_wrapper);
    test_register_case(&test_blt_s_short_branch_wrapper);
    test_register_case(&test_bne_un_s_short_branch_wrapper);
    test_register_case(&test_bge_un_s_short_branch_wrapper);
    test_register_case(&test_bge_long_branch_wrapper);
    
    // Run all tests
    test_run_all();
    
    printf("\n=== TDD Red Phase Complete ===\n");
    printf("All tests should have FAILED (opcodes not implemented)\n");
    printf("Next: Implement missing opcodes to make tests pass (Green Phase)\n");
    
    return 0;
}