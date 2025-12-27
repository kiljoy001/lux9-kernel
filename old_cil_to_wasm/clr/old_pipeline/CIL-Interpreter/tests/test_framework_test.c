#include "tests/framework/test_framework.h"
#include <stdio.h>
#include <string.h>

// Sample test suite
static test_suite_t suite_sample_suite = {
    .name = "sample_suite",
    .setup = NULL,
    .teardown = NULL,
    .tests = NULL,
    .next = NULL
};

// Sample test case
static test_result_t test_example_impl(void) {
    TEST_ASSERT_EQUAL(1, 1);
    return TEST_PASSED;
}

static test_result_t test_example_wrapper(void) {
    test_result_t result = test_example_impl();
    return result;
}

static test_result_t test_string_impl(void) {
    const char* expected = "hello";
    const char* actual = "hello";
    TEST_ASSERT_STRING_EQUAL(expected, actual);
    return TEST_PASSED;
}

static test_result_t test_string_wrapper(void) {
    test_result_t result = test_string_impl();
    return result;
}

int main(void) {
    printf("Testing CIL interpreter test framework...\n");
    
    // Register the suite
    test_register_suite(&suite_sample_suite);
    
    // Create test cases
    test_case_t example_test = {
        .suite_name = "sample_suite",
        .test_name = "test_example",
        .test_func = test_example_wrapper,
        .next = NULL
    };
    
    test_case_t string_test = {
        .suite_name = "sample_suite",
        .test_name = "test_string",
        .test_func = test_string_wrapper,
        .next = NULL
    };
    
    // Set current suite and register tests
    current_suite = &suite_sample_suite;
    test_register_case(&example_test);
    test_register_case(&string_test);
    
    // Run the tests
    test_set_verbose(true);
    test_result_t result1 = test_run_single("sample_suite", "test_example");
    test_result_t result2 = test_run_single("sample_suite", "test_string");
    
    if (result1 == TEST_PASSED && result2 == TEST_PASSED) {
        printf("Framework test passed!\n");
        return 0;
    } else {
        printf("Framework test failed!\n");
        return 1;
    }
}