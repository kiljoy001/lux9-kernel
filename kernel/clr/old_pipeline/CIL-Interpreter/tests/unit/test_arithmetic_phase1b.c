/*
 * TDD Test Suite - Phase 1B: Overflow Arithmetic Operations
 * 
 * Based on ECMA-335 CIL Instruction Set specifications:
 * - ADD.OVF (0xD6): Add with overflow check
 * - ADD.OVF.UN (0xD7): Add with overflow check (unsigned)
 * - MUL.OVF (0xD8): Multiply with overflow check
 * - MUL.OVF.UN (0xD9): Multiply with overflow check (unsigned)
 * - SUB.OVF (0xDA): Subtract with overflow check
 * - SUB.OVF.UN (0xDB): Subtract with overflow check (unsigned)
 * - DIV.OVF (0xDC): Divide with overflow check
 * - DIV.OVF.UN (0xDD): Divide with overflow check (unsigned)
 * 
 * Key differences from regular arithmetic:
 * - Throw OverflowException on signed overflow
 * - Throw OverflowException on unsigned overflow
 * - Different behavior for signed vs unsigned operations
 */

#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

// Test suite for Phase 1B - Overflow Arithmetic Operations
TEST_SUITE(arithmetic_phase1b);

// Global test execution state
static vm_execution_state_t* test_state;
static vm_init_t test_init;

// Setup function
static void arithmetic_phase1b_setup(void) {
    memset(&test_init, 0, sizeof(test_init));
    test_state = vm_create_execution_state(&test_init);
    TEST_ASSERT_NOT_NULL(test_state);
    vm_clear_error(test_state);
}

// Teardown function
static void arithmetic_phase1b_teardown(void) {
    if (test_state != NULL) {
        vm_destroy_execution_state(test_state);
        test_state = NULL;
    }
}

// ==================== ADD.OVF OPERATION TESTS (ECMA-335 0xD6) ====================

// Test ADD.OVF with int32 operands - no overflow
TEST(test_add_ovf_int32_basic) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(5);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(15, result.value.i4);
    
    return TEST_PASSED;
}

// Test ADD.OVF with int32 operands - signed overflow
TEST(test_add_ovf_int32_overflow) {
    vm_value_t val1 = vm_make_i4(INT_MAX);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    // ADD.OVF should fail on overflow
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test ADD.OVF with negative operands - no overflow
TEST(test_add_ovf_int32_negative) {
    vm_value_t val1 = vm_make_i4(-10);
    vm_value_t val2 = vm_make_i4(-5);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-15, result.value.i4);
    
    return TEST_PASSED;
}

// Test ADD.OVF with negative overflow
TEST(test_add_ovf_int32_negative_overflow) {
    vm_value_t val1 = vm_make_i4(INT_MIN);
    vm_value_t val2 = vm_make_i4(-1);
    vm_value_t result = vm_make_i4(0);
    
    // ADD.OVF should fail on signed overflow
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test ADD.OVF.UN with uint32 operands - no overflow
TEST(test_add_ovf_un_uint32_basic) {
    vm_value_t val1 = vm_make_u4(1000u);
    vm_value_t val2 = vm_make_u4(500u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_add_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(1500u, result.value.u4);
    
    return TEST_PASSED;
}

// Test ADD.OVF.UN with uint32 operands - overflow
TEST(test_add_ovf_un_uint32_overflow) {
    vm_value_t val1 = vm_make_u4(UINT_MAX);
    vm_value_t val2 = vm_make_u4(1u);
    vm_value_t result = vm_make_u4(0);
    
    // ADD.OVF.UN should fail on unsigned overflow
    bool success = vm_add_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// ==================== SUB.OVF OPERATION TESTS (ECMA-335 0xDA) ====================

// Test SUB.OVF with int32 operands - no overflow
TEST(test_subtract_ovf_int32_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(5);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_subtract_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(15, result.value.i4);
    
    return TEST_PASSED;
}

// Test SUB.OVF with int32 operands - signed overflow
TEST(test_subtract_ovf_int32_overflow) {
    vm_value_t val1 = vm_make_i4(INT_MIN);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    // SUB.OVF should fail on signed overflow
    bool success = vm_subtract_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test SUB.OVF.UN with uint32 operands - no overflow
TEST(test_subtract_ovf_un_uint32_basic) {
    vm_value_t val1 = vm_make_u4(1000u);
    vm_value_t val2 = vm_make_u4(500u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_subtract_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(500u, result.value.u4);
    
    return TEST_PASSED;
}

// Test SUB.OVF.UN with uint32 operands - underflow
TEST(test_subtract_ovf_un_uint32_underflow) {
    vm_value_t val1 = vm_make_u4(0u);
    vm_value_t val2 = vm_make_u4(1u);
    vm_value_t result = vm_make_u4(0);
    
    // SUB.OVF.UN should fail on unsigned underflow
    bool success = vm_subtract_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to underflow
    
    return TEST_PASSED;
}

// ==================== MUL.OVF OPERATION TESTS (ECMA-335 0xD8) ====================

// Test MUL.OVF with int32 operands - no overflow
TEST(test_multiply_ovf_int32_basic) {
    vm_value_t val1 = vm_make_i4(6);
    vm_value_t val2 = vm_make_i4(7);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);
    
    return TEST_PASSED;
}

// Test MUL.OVF with int32 operands - overflow
TEST(test_multiply_ovf_int32_overflow) {
    vm_value_t val1 = vm_make_i4(100000);
    vm_value_t val2 = vm_make_i4(100000);
    vm_value_t result = vm_make_i4(0);
    
    // MUL.OVF should fail on overflow
    bool success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test MUL.OVF.UN with uint32 operands - no overflow
TEST(test_multiply_ovf_un_uint32_basic) {
    vm_value_t val1 = vm_make_u4(1000u);
    vm_value_t val2 = vm_make_u4(2000u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_multiply_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(2000000u, result.value.u4);
    
    return TEST_PASSED;
}

// Test MUL.OVF.UN with uint32 operands - overflow
TEST(test_multiply_ovf_un_uint32_overflow) {
    vm_value_t val1 = vm_make_u4(100000u);
    vm_value_t val2 = vm_make_u4(100000u);
    vm_value_t result = vm_make_u4(0);
    
    // MUL.OVF.UN should fail on unsigned overflow
    bool success = vm_multiply_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// ==================== DIV.OVF OPERATION TESTS (ECMA-335 0xDC) ====================

// Test DIV.OVF with int32 operands - basic division
TEST(test_divide_ovf_int32_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    return TEST_PASSED;
}

// Test DIV.OVF with division by zero
TEST(test_divide_ovf_int32_by_zero) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to division by zero
    
    return TEST_PASSED;
}

// Test DIV.OVF.UN with uint32 operands - basic division
TEST(test_divide_ovf_un_uint32_basic) {
    vm_value_t val1 = vm_make_u4(2000u);
    vm_value_t val2 = vm_make_u4(500u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_divide_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(4u, result.value.u4);
    
    return TEST_PASSED;
}

// Test DIV.OVF.UN with division by zero
TEST(test_divide_ovf_un_uint32_by_zero) {
    vm_value_t val1 = vm_make_u4(20u);
    vm_value_t val2 = vm_make_u4(0u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_divide_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to division by zero
    
    return TEST_PASSED;
}

// ==================== EDGE CASE TESTS ====================

// Test overflow with INT_MIN and INT_MAX
TEST(test_overflow_extreme_values) {
    vm_value_t val1, val2, result;
    
    // Test INT_MIN + 1 (should succeed)
    val1 = vm_make_i4(INT_MIN);
    val2 = vm_make_i4(1);
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(INT_MIN + 1, result.value.i4);
    
    // Test INT_MAX + 1 (should fail)
    val1 = vm_make_i4(INT_MAX);
    val2 = vm_make_i4(1);
    success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    // Test INT_MIN - 1 (should fail)
    val1 = vm_make_i4(INT_MIN);
    val2 = vm_make_i4(1);
    success = vm_subtract_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    return TEST_PASSED;
}

// Test overflow with negative results
TEST(test_overflow_negative_results) {
    vm_value_t val1, val2, result;
    
    // Test (-1) * (-1) = 1 (should succeed)
    val1 = vm_make_i4(-1);
    val2 = vm_make_i4(-1);
    bool success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.value.i4);
    
    // Test INT_MIN * (-1) (should fail - overflow)
    val1 = vm_make_i4(INT_MIN);
    val2 = vm_make_i4(-1);
    success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test overflow with small values
TEST(test_overflow_small_values) {
    vm_value_t val1, val2, result;
    
    // Test int8 overflow
    val1 = vm_make_i1(127);  // INT8_MAX
    val2 = vm_make_i1(1);
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    // Test int16 overflow
    val1 = vm_make_i2(32767);  // INT16_MAX
    val2 = vm_make_i2(1);
    success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    return TEST_PASSED;
}

// Test error handling with NULL pointers
TEST(test_overflow_null_pointer_handling) {
    vm_value_t val = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    // Test all overflow operations with NULL pointers
    TEST_ASSERT_FALSE(vm_add_ovf(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_add_ovf(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_add_ovf(&val, &val, NULL));
    
    TEST_ASSERT_FALSE(vm_subtract_ovf(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_subtract_ovf(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_subtract_ovf(&val, &val, NULL));
    
    TEST_ASSERT_FALSE(vm_multiply_ovf(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_multiply_ovf(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_multiply_ovf(&val, &val, NULL));
    
    TEST_ASSERT_FALSE(vm_divide_ovf(NULL, &val, &result));
    TEST_ASSERT_FALSE(vm_divide_ovf(&val, NULL, &result));
    TEST_ASSERT_FALSE(vm_divide_ovf(&val, &val, NULL));
    
    return TEST_PASSED;
}

// Test type validation for overflow operations
TEST(test_overflow_type_validation) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_r4(5.0f);  // Mismatched types
    vm_value_t result = vm_make_i4(0);
    
    // Test type mismatches
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to type mismatch
    
    success = vm_subtract_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    success = vm_divide_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);
    
    return TEST_PASSED;
}

// ==================== MAIN TEST RUNNER ====================

int main(void) {
    printf("=== Phase 1B: Overflow Arithmetic Operations TDD Tests ===\n");
    printf("ECMA-335 CIL Instruction Set Compliance Testing\n\n");
    
    // Register test suite
    test_register_suite(&suite_arithmetic_phase1b);
    
    // Set current suite
    current_suite = &suite_arithmetic_phase1b;
    
    // Test case declarations
    static test_case_t test_add_ovf_int32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_add_ovf_int32_basic", 
        .test_func = test_add_ovf_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_int32_overflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_add_ovf_int32_overflow",
        .test_func = test_add_ovf_int32_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_int32_negative = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_add_ovf_int32_negative",
        .test_func = test_add_ovf_int32_negative_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_int32_negative_overflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_add_ovf_int32_negative_overflow",
        .test_func = test_add_ovf_int32_negative_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_un_uint32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_add_ovf_un_uint32_basic",
        .test_func = test_add_ovf_un_uint32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_un_uint32_overflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_add_ovf_un_uint32_overflow",
        .test_func = test_add_ovf_un_uint32_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_ovf_int32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_subtract_ovf_int32_basic",
        .test_func = test_subtract_ovf_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_ovf_int32_overflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_subtract_ovf_int32_overflow",
        .test_func = test_subtract_ovf_int32_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_ovf_un_uint32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_subtract_ovf_un_uint32_basic",
        .test_func = test_subtract_ovf_un_uint32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_ovf_un_uint32_underflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_subtract_ovf_un_uint32_underflow",
        .test_func = test_subtract_ovf_un_uint32_underflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_ovf_int32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_multiply_ovf_int32_basic",
        .test_func = test_multiply_ovf_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_ovf_int32_overflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_multiply_ovf_int32_overflow",
        .test_func = test_multiply_ovf_int32_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_ovf_un_uint32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_multiply_ovf_un_uint32_basic",
        .test_func = test_multiply_ovf_un_uint32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_ovf_un_uint32_overflow = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_multiply_ovf_un_uint32_overflow",
        .test_func = test_multiply_ovf_un_uint32_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_ovf_int32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_divide_ovf_int32_basic",
        .test_func = test_divide_ovf_int32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_ovf_int32_by_zero = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_divide_ovf_int32_by_zero",
        .test_func = test_divide_ovf_int32_by_zero_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_ovf_un_uint32_basic = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_divide_ovf_un_uint32_basic",
        .test_func = test_divide_ovf_un_uint32_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_ovf_un_uint32_by_zero = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_divide_ovf_un_uint32_by_zero",
        .test_func = test_divide_ovf_un_uint32_by_zero_wrapper,
        .next = NULL
    };
    
    static test_case_t test_overflow_extreme_values = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_overflow_extreme_values",
        .test_func = test_overflow_extreme_values_wrapper,
        .next = NULL
    };
    
    static test_case_t test_overflow_negative_results = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_overflow_negative_results",
        .test_func = test_overflow_negative_results_wrapper,
        .next = NULL
    };
    
    static test_case_t test_overflow_small_values = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_overflow_small_values",
        .test_func = test_overflow_small_values_wrapper,
        .next = NULL
    };
    
    static test_case_t test_overflow_null_pointer_handling = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_overflow_null_pointer_handling",
        .test_func = test_overflow_null_pointer_handling_wrapper,
        .next = NULL
    };
    
    static test_case_t test_overflow_type_validation = {
        .suite_name = "arithmetic_phase1b",
        .test_name = "test_overflow_type_validation",
        .test_func = test_overflow_type_validation_wrapper,
        .next = NULL
    };
    
    // Link all test cases in order
    test_add_ovf_int32_basic.next = &test_add_ovf_int32_overflow;
    test_add_ovf_int32_overflow.next = &test_add_ovf_int32_negative;
    test_add_ovf_int32_negative.next = &test_add_ovf_int32_negative_overflow;
    test_add_ovf_int32_negative_overflow.next = &test_add_ovf_un_uint32_basic;
    test_add_ovf_un_uint32_basic.next = &test_add_ovf_un_uint32_overflow;
    test_add_ovf_un_uint32_overflow.next = &test_subtract_ovf_int32_basic;
    test_subtract_ovf_int32_basic.next = &test_subtract_ovf_int32_overflow;
    test_subtract_ovf_int32_overflow.next = &test_subtract_ovf_un_uint32_basic;
    test_subtract_ovf_un_uint32_basic.next = &test_subtract_ovf_un_uint32_underflow;
    test_subtract_ovf_un_uint32_underflow.next = &test_multiply_ovf_int32_basic;
    test_multiply_ovf_int32_basic.next = &test_multiply_ovf_int32_overflow;
    test_multiply_ovf_int32_overflow.next = &test_multiply_ovf_un_uint32_basic;
    test_multiply_ovf_un_uint32_basic.next = &test_multiply_ovf_un_uint32_overflow;
    test_multiply_ovf_un_uint32_overflow.next = &test_divide_ovf_int32_basic;
    test_divide_ovf_int32_basic.next = &test_divide_ovf_int32_by_zero;
    test_divide_ovf_int32_by_zero.next = &test_divide_ovf_un_uint32_basic;
    test_divide_ovf_un_uint32_basic.next = &test_divide_ovf_un_uint32_by_zero;
    test_divide_ovf_un_uint32_by_zero.next = &test_overflow_extreme_values;
    test_overflow_extreme_values.next = &test_overflow_negative_results;
    test_overflow_negative_results.next = &test_overflow_small_values;
    test_overflow_small_values.next = &test_overflow_null_pointer_handling;
    test_overflow_null_pointer_handling.next = &test_overflow_type_validation;
    test_overflow_type_validation.next = NULL;
    
    // Register first test case
    test_register_case(&test_add_ovf_int32_basic);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("arithmetic_phase1b");
    
    // Print results
    printf("\n=== Phase 1B Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All Phase 1B overflow arithmetic tests passed!\n");
        printf("ECMA-335 overflow arithmetic operations (ADD.OVF, SUB.OVF, MUL.OVF, DIV.OVF) verified.\n");
    } else {
        printf("\n✗ Some Phase 1B overflow arithmetic tests failed!\n");
        printf("Implementation needs fixes to meet ECMA-335 compliance.\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}
