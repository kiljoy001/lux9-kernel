/*
 * Simple TDD Test Suite - Phase 1A: Basic Arithmetic Operations
 * 
 * Based on ECMA-335 CIL Instruction Set specifications:
 * - ADD (0x58): Add two values, returning a new value
 * - SUB (0x59): Subtract value2 from value1, returning a new value  
 * - MUL (0x5A): Multiply values
 * - DIV (0x5B): Divide two values to return a quotient or floating-point result
 */

#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

// Test suite for Phase 1A - Basic Arithmetic Operations
TEST_SUITE(arithmetic_phase1a);

// Global test execution state
static vm_execution_state_t* test_state;
static vm_init_t test_init;

// Setup function
static void arithmetic_phase1a_setup(void) {
    memset(&test_init, 0, sizeof(test_init));
    test_state = vm_create_execution_state(&test_init);
    TEST_ASSERT_NOT_NULL(test_state);
    vm_clear_error(test_state);
}

// Teardown function
static void arithmetic_phase1a_teardown(void) {
    if (test_state != NULL) {
        vm_destroy_execution_state(test_state);
        test_state = NULL;
    }
}

// ==================== ADD OPERATION TESTS (ECMA-335 0x58) ====================

// Test ADD with int32 operands
TEST(test_add_int32_basic) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(5);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(15, result.value.i4);
    
    return TEST_PASSED;
}

// Test ADD with int32 negative operands
TEST(test_add_int32_negative) {
    vm_value_t val1 = vm_make_i4(-10);
    vm_value_t val2 = vm_make_i4(-5);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-15, result.value.i4);
    
    // Test mixed signs
    val1 = vm_make_i4(10);
    val2 = vm_make_i4(-5);
    success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    return TEST_PASSED;
}

// Test ADD with int32 overflow behavior (wraparound)
TEST(test_add_int32_overflow) {
    vm_value_t val1 = vm_make_i4(INT_MAX);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(INT_MIN, result.value.i4); // Wraparound
    
    // Test underflow
    val1 = vm_make_i4(INT_MIN);
    val2 = vm_make_i4(-1);
    success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(INT_MAX, result.value.i4); // Wraparound
    
    return TEST_PASSED;
}

// ==================== SUBTRACT OPERATION TESTS (ECMA-335 0x59) ====================

// Test SUB with int32 operands
TEST(test_subtract_int32_basic) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_subtract(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(7, result.value.i4);
    
    return TEST_PASSED;
}

// Test SUB with negative results
TEST(test_subtract_int32_negative) {
    vm_value_t val1 = vm_make_i4(5);
    vm_value_t val2 = vm_make_i4(10);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_subtract(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-5, result.value.i4);
    
    return TEST_PASSED;
}

// ==================== MULTIPLY OPERATION TESTS (ECMA-335 0x5A) ====================

// Test MUL with int32 operands
TEST(test_multiply_int32_basic) {
    vm_value_t val1 = vm_make_i4(6);
    vm_value_t val2 = vm_make_i4(7);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_multiply(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);
    
    return TEST_PASSED;
}

// ==================== DIVIDE OPERATION TESTS (ECMA-335 0x5B) ====================

// Test DIV with int32 operands
TEST(test_divide_int32_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    return TEST_PASSED;
}

// Test DIV by zero error handling
TEST(test_divide_int32_by_zero) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should return false for division by zero
    
    // Note: vm_divide() doesn't set execution state errors - 
    // that happens at the instruction execution level
    // For direct function calls, we just check the return value
    
    return TEST_PASSED;
}

// ==================== STACK OPERATION TESTS ====================

// Test stack behavior
TEST(test_stack_operations) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(5);
    vm_value_t result = vm_make_i4(0);
    
    // Test stack depth
    TEST_ASSERT_EQUAL(0, vm_stack_depth(test_state));
    
    // Test stack push
    bool success = vm_stack_push(test_state, &val1);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, vm_stack_depth(test_state));
    
    // Test stack push second value
    success = vm_stack_push(test_state, &val2);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(2, vm_stack_depth(test_state));
    
    // Test stack pop
    success = vm_stack_pop(test_state, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    TEST_ASSERT_EQUAL(1, vm_stack_depth(test_state));
    
    // Test stack pop second value
    success = vm_stack_pop(test_state, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(10, result.value.i4);
    TEST_ASSERT_EQUAL(0, vm_stack_depth(test_state));
    
    return TEST_PASSED;
}

// ==================== ERROR HANDLING TESTS ====================

// Test error handling with NULL pointers
TEST(test_arithmetic_null_pointer_handling) {
    vm_value_t val = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    TEST_ASSERT_FALSE(vm_add(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_add(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_add(&val, &val, NULL));
    
    TEST_ASSERT_FALSE(vm_subtract(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_subtract(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_subtract(&val, &val, NULL));
    
    TEST_ASSERT_FALSE(vm_multiply(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_multiply(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_multiply(&val, &val, NULL));
    
    TEST_ASSERT_FALSE(vm_divide(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_divide(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_divide(&val, &val, NULL));
    
    return TEST_PASSED;
}

// ==================== MAIN TEST RUNNER ====================

int main(void) {
    printf("=== Phase 1A: Basic Arithmetic Operations TDD Tests ===\n");
    printf("ECMA-335 CIL Instruction Set Compliance Testing\n\n");
    
    // Register test suite
    test_register_suite(&suite_arithmetic_phase1a);
    
    // Set current suite
    current_suite = &suite_arithmetic_phase1a;
    
    // Test case declarations
    static test_case_t test_add_int32_basic = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_add_int32_basic", 
        .test_func = test_add_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_int32_negative = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_add_int32_negative",
        .test_func = test_add_int32_negative_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_int32_overflow = {
        .suite_name = "arithmetic_phase1a", 
        .test_name = "test_add_int32_overflow",
        .test_func = test_add_int32_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_int32_basic = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_subtract_int32_basic",
        .test_func = test_subtract_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_int32_negative = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_subtract_int32_negative",
        .test_func = test_subtract_int32_negative_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_int32_basic = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_multiply_int32_basic",
        .test_func = test_multiply_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_int32_basic = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_divide_int32_basic",
        .test_func = test_divide_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_int32_by_zero = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_divide_int32_by_zero",
        .test_func = test_divide_int32_by_zero_wrapper,
        .next = NULL
    };
    
    static test_case_t test_stack_operations = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_stack_operations",
        .test_func = test_stack_operations_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_null_pointer_handling = {
        .suite_name = "arithmetic_phase1a",
        .test_name = "test_arithmetic_null_pointer_handling",
        .test_func = test_arithmetic_null_pointer_handling_wrapper,
        .next = NULL
    };
    
    // Link all test cases in order
    test_add_int32_basic.next = &test_add_int32_negative;
    test_add_int32_negative.next = &test_add_int32_overflow;
    test_add_int32_overflow.next = &test_subtract_int32_basic;
    test_subtract_int32_basic.next = &test_subtract_int32_negative;
    test_subtract_int32_negative.next = &test_multiply_int32_basic;
    test_multiply_int32_basic.next = &test_divide_int32_basic;
    test_divide_int32_basic.next = &test_divide_int32_by_zero;
    test_divide_int32_by_zero.next = &test_stack_operations;
    test_stack_operations.next = &test_arithmetic_null_pointer_handling;
    test_arithmetic_null_pointer_handling.next = NULL;
    
    // Register first test case
    test_register_case(&test_add_int32_basic);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("arithmetic_phase1a");
    
    // Print results
    printf("\n=== Phase 1A Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All Phase 1A arithmetic tests passed!\n");
        printf("ECMA-335 basic arithmetic operations (ADD, SUB, MUL, DIV) verified.\n");
    } else {
        printf("\n✗ Some Phase 1A arithmetic tests failed!\n");
        printf("Implementation needs fixes to meet ECMA-335 compliance.\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}