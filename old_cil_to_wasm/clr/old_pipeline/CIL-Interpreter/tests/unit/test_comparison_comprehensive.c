#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Test suite for comprehensive comparison operations
TEST_SUITE(comparison_comprehensive);

// Global test execution state
static vm_execution_state_t* test_state;
static vm_init_t test_init;

// Setup function
static void comparison_comprehensive_setup(void) {
    memset(&test_init, 0, sizeof(test_init));
    test_state = vm_create_execution_state(&test_init);
}

// Teardown function
static void comparison_comprehensive_teardown(void) {
    if (test_state != NULL) {
        vm_destroy_execution_state(test_state);
        test_state = NULL;
    }
}

// Test CEQ (compare equal) operation - equal values
TEST(test_ceq_equal) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_equal(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(1, result.value.i4);  // True (equal)
    
    return TEST_PASSED;
}

// Test CEQ (compare equal) operation - different values
TEST(test_ceq_not_equal) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(43);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_equal(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // False (not equal)
    
    return TEST_PASSED;
}

// Test CGT (compare greater than) operation - greater value
TEST(test_cgt_greater) {
    vm_value_t val1 = vm_make_i4(43);
    vm_value_t val2 = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_greater(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(1, result.value.i4);  // True (43 > 42)
    
    return TEST_PASSED;
}

// Test CGT (compare greater than) operation - equal values
TEST(test_cgt_equal) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_greater(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // False (42 is not > 42)
    
    return TEST_PASSED;
}

// Test CGT (compare greater than) operation - lesser value
TEST(test_cgt_less) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(43);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_greater(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // False (42 is not > 43)
    
    return TEST_PASSED;
}

// Test CLT (compare less than) operation - lesser value
TEST(test_clt_less) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(43);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_less(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(1, result.value.i4);  // True (42 < 43)
    
    return TEST_PASSED;
}

// Test CLT (compare less than) operation - equal values
TEST(test_clt_equal) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_less(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // False (42 is not < 42)
    
    return TEST_PASSED;
}

// Test CLT (compare less than) operation - greater value
TEST(test_clt_greater) {
    vm_value_t val1 = vm_make_i4(43);
    vm_value_t val2 = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_less(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // False (43 is not < 42)
    
    return TEST_PASSED;
}

// Test CGT_UN (compare greater than unsigned) operation
TEST(test_cgt_un_basic) {
    vm_value_t val1 = vm_make_i4(-1);  // 0xFFFFFFFF as signed, large unsigned
    vm_value_t val2 = vm_make_i4(1);   // 1
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_greater_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(1, result.value.i4);  // True (-1 as unsigned > 1)
    
    return TEST_PASSED;
}

// Test CLT_UN (compare less than unsigned) operation
TEST(test_clt_un_basic) {
    vm_value_t val1 = vm_make_i4(1);   // 1
    vm_value_t val2 = vm_make_i4(-1);  // 0xFFFFFFFF as signed, large unsigned
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_less_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(1, result.value.i4);  // True (1 < -1 as unsigned)
    
    return TEST_PASSED;
}

// Test CEQ with different types (should fail)
TEST(test_ceq_type_mismatch) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_r4(42.0f);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_compare_equal(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to type mismatch
    
    return TEST_PASSED;
}

// Test edge case: zero comparisons
TEST(test_zero_comparisons) {
    vm_value_t val1 = vm_make_i4(0);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    // 0 == 0 should be true
    bool success = vm_compare_equal(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.value.i4);
    
    // 0 > 0 should be false
    success = vm_compare_greater(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, result.value.i4);
    
    // 0 < 0 should be false
    success = vm_compare_less(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, result.value.i4);
    
    return TEST_PASSED;
}

// Test edge case: negative number comparisons
TEST(test_negative_comparisons) {
    vm_value_t val1 = vm_make_i4(-10);
    vm_value_t val2 = vm_make_i4(-5);
    vm_value_t result = vm_make_i4(0);
    
    // -10 == -5 should be false
    bool success = vm_compare_equal(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, result.value.i4);
    
    // -10 > -5 should be false
    success = vm_compare_greater(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, result.value.i4);
    
    // -10 < -5 should be true
    success = vm_compare_less(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.value.i4);
    
    return TEST_PASSED;
}

// Test unsigned comparisons with negative values
TEST(test_unsigned_negative_comparisons) {
    vm_value_t val1 = vm_make_i4(-1);  // 0xFFFFFFFF as unsigned
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    // -1 as unsigned > 1 should be true
    bool success = vm_compare_greater_un(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.value.i4);
    
    // 1 < -1 as unsigned should be true
    success = vm_compare_less_un(&val2, &val1, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.value.i4);
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== Comprehensive Comparison Operations Test Suite ===\n");
    
    // Register test suite
    test_register_suite(&suite_comparison_comprehensive);
    
    // Set current suite
    current_suite = &suite_comparison_comprehensive;
    
    // Test case declarations
    static test_case_t test_ceq_equal = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_ceq_equal",
        .test_func = test_ceq_equal_wrapper,
        .next = NULL
    };
    
    static test_case_t test_ceq_not_equal = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_ceq_not_equal",
        .test_func = test_ceq_not_equal_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cgt_greater = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_cgt_greater",
        .test_func = test_cgt_greater_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cgt_equal = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_cgt_equal",
        .test_func = test_cgt_equal_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cgt_less = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_cgt_less",
        .test_func = test_cgt_less_wrapper,
        .next = NULL
    };
    
    static test_case_t test_clt_less = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_clt_less",
        .test_func = test_clt_less_wrapper,
        .next = NULL
    };
    
    static test_case_t test_clt_equal = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_clt_equal",
        .test_func = test_clt_equal_wrapper,
        .next = NULL
    };
    
    static test_case_t test_clt_greater = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_clt_greater",
        .test_func = test_clt_greater_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cgt_un_basic = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_cgt_un_basic",
        .test_func = test_cgt_un_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_clt_un_basic = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_clt_un_basic",
        .test_func = test_clt_un_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_ceq_type_mismatch = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_ceq_type_mismatch",
        .test_func = test_ceq_type_mismatch_wrapper,
        .next = NULL
    };
    
    static test_case_t test_zero_comparisons = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_zero_comparisons",
        .test_func = test_zero_comparisons_wrapper,
        .next = NULL
    };
    
    static test_case_t test_negative_comparisons = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_negative_comparisons",
        .test_func = test_negative_comparisons_wrapper,
        .next = NULL
    };
    
    static test_case_t test_unsigned_negative_comparisons = {
        .suite_name = "comparison_comprehensive",
        .test_name = "test_unsigned_negative_comparisons",
        .test_func = test_unsigned_negative_comparisons_wrapper,
        .next = NULL
    };
    
    // Link all test cases
    test_ceq_equal.next = &test_ceq_not_equal;
    test_ceq_not_equal.next = &test_cgt_greater;
    test_cgt_greater.next = &test_cgt_equal;
    test_cgt_equal.next = &test_cgt_less;
    test_cgt_less.next = &test_clt_less;
    test_clt_less.next = &test_clt_equal;
    test_clt_equal.next = &test_clt_greater;
    test_clt_greater.next = &test_cgt_un_basic;
    test_cgt_un_basic.next = &test_clt_un_basic;
    test_clt_un_basic.next = &test_ceq_type_mismatch;
    test_ceq_type_mismatch.next = &test_zero_comparisons;
    test_zero_comparisons.next = &test_negative_comparisons;
    test_negative_comparisons.next = &test_unsigned_negative_comparisons;
    test_unsigned_negative_comparisons.next = NULL;
    
    // Register first test case
    test_register_case(&test_ceq_equal);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("comparison_comprehensive");
    
    // Print results
    printf("\n=== Comparison Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All comparison tests passed!\n");
    } else {
        printf("\n✗ Some comparison tests failed!\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}