#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

// Test suite for comprehensive arithmetic operations
TEST_SUITE(arithmetic_comprehensive);

// Global test execution state
static vm_execution_state_t* test_state;
static vm_init_t test_init;

// Setup function
static void arithmetic_comprehensive_setup(void) {
    memset(&test_init, 0, sizeof(test_init));
    test_state = vm_create_execution_state(&test_init);
}

// Teardown function
static void arithmetic_comprehensive_teardown(void) {
    if (test_state != NULL) {
        vm_destroy_execution_state(test_state);
        test_state = NULL;
    }
}

// Test ADD operation
TEST(test_add_basic) {
    vm_value_t val1 = vm_make_i4(5);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(8, result.value.i4);
    
    return TEST_PASSED;
}

// Test SUB operation
TEST(test_sub_basic) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_subtract(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(6, result.value.i4);
    
    return TEST_PASSED;
}

// Test MUL operation
TEST(test_mul_basic) {
    vm_value_t val1 = vm_make_i4(7);
    vm_value_t val2 = vm_make_i4(6);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_multiply(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);
    
    return TEST_PASSED;
}

// Test DIV operation
TEST(test_div_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    return TEST_PASSED;
}

// Test DIV_UN operation
TEST(test_div_un_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(6, result.value.i4);  // 20/3 = 6 (integer division)
    
    return TEST_PASSED;
}

// Test REM operation
TEST(test_rem_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_remainder(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(2, result.value.i4);  // 20 % 3 = 2
    
    return TEST_PASSED;
}

// Test REM_UN operation
TEST(test_rem_un_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_remainder_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(2, result.value.i4);  // 20 % 3 = 2
    
    return TEST_PASSED;
}

// Test NEG operation
TEST(test_neg_basic) {
    vm_value_t val = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_neg(&val, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-42, result.value.i4);
    
    return TEST_PASSED;
}

// Test ADD_OVF operation - no overflow
TEST(test_add_ovf_no_overflow) {
    vm_value_t val1 = vm_make_i4(100);
    vm_value_t val2 = vm_make_i4(200);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(300, result.value.i4);
    
    return TEST_PASSED;
}

// Test ADD_OVF operation - signed overflow
TEST(test_add_ovf_signed_overflow) {
    vm_value_t val1 = vm_make_i4(INT_MAX);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    // ADD.OVF should fail on overflow
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test MUL_OVF operation - no overflow
TEST(test_mul_ovf_no_overflow) {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(20);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(200, result.value.i4);
    
    return TEST_PASSED;
}

// Test MUL_OVF operation - overflow
TEST(test_mul_ovf_overflow) {
    vm_value_t val1 = vm_make_i4(100000);
    vm_value_t val2 = vm_make_i4(100000);
    vm_value_t result = vm_make_i4(0);
    
    // MUL.OVF should fail on overflow
    bool success = vm_multiply_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test SUB_OVF operation - no overflow
TEST(test_sub_ovf_no_overflow) {
    vm_value_t val1 = vm_make_i4(100);
    vm_value_t val2 = vm_make_i4(30);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_subtract_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(70, result.value.i4);
    
    return TEST_PASSED;
}

// Test SUB_OVF operation - signed overflow
TEST(test_sub_ovf_signed_overflow) {
    vm_value_t val1 = vm_make_i4(INT_MIN);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    // SUB.OVF should fail on signed overflow
    bool success = vm_subtract_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test DIV_OVF operation - basic division
TEST(test_div_ovf_basic) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    return TEST_PASSED;
}

// Test DIV_OVF operation - division by zero
TEST(test_div_ovf_by_zero) {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to division by zero
    
    return TEST_PASSED;
}

// Test DIV_OVF operation - overflow edge case
TEST(test_div_ovf_overflow_edge_case) {
    vm_value_t val1 = vm_make_i4(INT_MIN);
    vm_value_t val2 = vm_make_i4(-1);
    vm_value_t result = vm_make_i4(0);
    
    // This should fail due to overflow (INT_MIN / -1 would overflow)
    bool success = vm_divide_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

// Test DIV_OVF_UN operation - basic division
TEST(test_div_ovf_un_basic) {
    vm_value_t val1 = vm_make_u4(2000u);
    vm_value_t val2 = vm_make_u4(500u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_divide_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(4u, result.value.u4);
    
    return TEST_PASSED;
}

// Test DIV_OVF_UN operation - division by zero
TEST(test_div_ovf_un_by_zero) {
    vm_value_t val1 = vm_make_u4(20u);
    vm_value_t val2 = vm_make_u4(0u);
    vm_value_t result = vm_make_u4(0);
    
    bool success = vm_divide_ovf_un(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to division by zero
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== Comprehensive Arithmetic Operations Test Suite ===\n");
    
    // Register test suite
    test_register_suite(&suite_arithmetic_comprehensive);
    
    // Set current suite
    current_suite = &suite_arithmetic_comprehensive;
    
    // Test case declarations
    static test_case_t test_add_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_add_basic",
        .test_func = test_add_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_sub_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_sub_basic",
        .test_func = test_sub_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_mul_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_mul_basic",
        .test_func = test_mul_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_basic",
        .test_func = test_div_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_un_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_un_basic",
        .test_func = test_div_un_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_rem_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_rem_basic",
        .test_func = test_rem_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_rem_un_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_rem_un_basic",
        .test_func = test_rem_un_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_neg_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_neg_basic",
        .test_func = test_neg_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_no_overflow = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_add_ovf_no_overflow",
        .test_func = test_add_ovf_no_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_ovf_signed_overflow = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_add_ovf_signed_overflow",
        .test_func = test_add_ovf_signed_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_mul_ovf_no_overflow = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_mul_ovf_no_overflow",
        .test_func = test_mul_ovf_no_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_mul_ovf_overflow = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_mul_ovf_overflow",
        .test_func = test_mul_ovf_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_sub_ovf_no_overflow = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_sub_ovf_no_overflow",
        .test_func = test_sub_ovf_no_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_sub_ovf_signed_overflow = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_sub_ovf_signed_overflow",
        .test_func = test_sub_ovf_signed_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_ovf_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_ovf_basic",
        .test_func = test_div_ovf_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_ovf_by_zero = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_ovf_by_zero",
        .test_func = test_div_ovf_by_zero_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_ovf_overflow_edge_case = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_ovf_overflow_edge_case",
        .test_func = test_div_ovf_overflow_edge_case_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_ovf_un_basic = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_ovf_un_basic",
        .test_func = test_div_ovf_un_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_div_ovf_un_by_zero = {
        .suite_name = "arithmetic_comprehensive",
        .test_name = "test_div_ovf_un_by_zero",
        .test_func = test_div_ovf_un_by_zero_wrapper,
        .next = NULL
    };
    
    // Link all test cases
    test_add_basic.next = &test_sub_basic;
    test_sub_basic.next = &test_mul_basic;
    test_mul_basic.next = &test_div_basic;
    test_div_basic.next = &test_div_un_basic;
    test_div_un_basic.next = &test_rem_basic;
    test_rem_basic.next = &test_rem_un_basic;
    test_rem_un_basic.next = &test_neg_basic;
    test_neg_basic.next = &test_add_ovf_no_overflow;
    test_add_ovf_no_overflow.next = &test_add_ovf_signed_overflow;
    test_add_ovf_signed_overflow.next = &test_mul_ovf_no_overflow;
    test_mul_ovf_no_overflow.next = &test_mul_ovf_overflow;
    test_mul_ovf_overflow.next = &test_sub_ovf_no_overflow;
    test_sub_ovf_no_overflow.next = &test_sub_ovf_signed_overflow;
    test_sub_ovf_signed_overflow.next = &test_div_ovf_basic;
    test_div_ovf_basic.next = &test_div_ovf_by_zero;
    test_div_ovf_by_zero.next = &test_div_ovf_overflow_edge_case;
    test_div_ovf_overflow_edge_case.next = &test_div_ovf_un_basic;
    test_div_ovf_un_basic.next = &test_div_ovf_un_by_zero;
    test_div_ovf_un_by_zero.next = NULL;
    
    // Register first test case
    test_register_case(&test_add_basic);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("arithmetic_comprehensive");
    
    // Print results
    printf("\n=== Arithmetic Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All arithmetic tests passed!\n");
    } else {
        printf("\n✗ Some arithmetic tests failed!\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}