#include "tests/framework/test_framework.h"
#include <stdio.h>
#include <string.h>

// Test suite with setup/teardown
static void sample_suite_setup(void) {
    printf("  Setup called\n");
}

static void sample_suite_teardown(void) {
    printf("  Teardown called\n");
}

static test_suite_t suite_sample = {
    .name = "sample_suite",
    .setup = sample_suite_setup,
    .teardown = sample_suite_teardown,
    .tests = NULL,
    .next = NULL
};

// Test that passes
static test_result_t test_pass_impl(void) {
    TEST_ASSERT_EQUAL(42, 42);
    TEST_ASSERT_STRING_EQUAL("hello", "hello");
    TEST_ASSERT_NOT_NULL("test");
    return TEST_PASSED;
}

static test_result_t test_pass_wrapper(void) {
    return test_pass_impl();
}

// Test that fails
static test_result_t test_fail_impl(void) {
    TEST_ASSERT_EQUAL(1, 2);  // This will fail
    return TEST_PASSED;
}

static test_result_t test_fail_wrapper(void) {
    return test_fail_impl();
}

int main(void) {
    printf("=== CIL Interpreter Test Framework Comprehensive Test ===\n\n");
    
    // Create test cases
    static test_case_t pass_test = {
        .suite_name = "sample_suite",
        .test_name = "test_pass",
        .test_func = test_pass_wrapper,
        .next = NULL
    };
    
    static test_case_t fail_test = {
        .suite_name = "sample_suite",
        .test_name = "test_fail",
        .test_func = test_fail_wrapper,
        .next = NULL
    };
    
    // Register the suite
    test_register_suite(&suite_sample);
    
    // Register the tests
    current_suite = &suite_sample;
    test_register_case(&pass_test);
    test_register_case(&fail_test);
    
    // Test verbose mode
    printf("1. Testing verbose mode:\n");
    test_set_verbose(1);
    
    printf("\n2. Running passing test:\n");
    test_result_t result1 = test_run_single("sample_suite", "test_pass");
    
    printf("\n3. Running failing test (expect failure output):\n");
    test_result_t result2 = test_run_single("sample_suite", "test_fail");
    
    printf("\n4. Test Results:\n");
    printf("   Pass test result: %s\n", result1 == TEST_PASSED ? "PASSED" : "FAILED");
    printf("   Fail test result: %s\n", result2 == TEST_PASSED ? "PASSED" : "FAILED");
    
    if (result1 == TEST_PASSED && result2 == TEST_FAILED) {
        printf("\n✓ All tests behaved as expected!\n");
        return 0;
    } else {
        printf("\n✗ Unexpected test results!\n");
        return 1;
    }
}