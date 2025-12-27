#include "tests/framework/test_framework.h"
#include <stdio.h>
#include <string.h>

// Simple test function
static test_result_t test_basic_impl(void) {
    TEST_ASSERT_EQUAL(1, 1);
    return TEST_PASSED;
}

static test_result_t test_basic_wrapper(void) {
    return test_basic_impl();
}

int main(void) {
    printf("Testing CIL interpreter test framework...\n");
    
    // Create a simple test suite
    static test_suite_t suite_simple = {
        .name = "simple_suite",
        .setup = NULL,
        .teardown = NULL,
        .tests = NULL,
        .next = NULL
    };
    
    // Create a simple test case
    static test_case_t basic_test = {
        .suite_name = "simple_suite",
        .test_name = "basic_test",
        .test_func = test_basic_wrapper,
        .next = NULL
    };
    
    // Test registering suite and case
    test_register_suite(&suite_simple);
    
    // Set current suite and register test
    current_suite = &suite_simple;
    test_register_case(&basic_test);
    
    // Test verbose mode
    test_set_verbose(1);
    
    // Run the test
    test_result_t result = test_run_single("simple_suite", "basic_test");
    
    if (result == TEST_PASSED) {
        printf("Framework basic test passed!\n");
        return 0;
    } else {
        printf("Framework basic test failed!\n");
        return 1;
    }
}