#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state
test_suite_t* test_suites = NULL;
test_suite_t* current_suite = NULL;
test_runner_state_t test_runner_state = {0};

// Register a test suite
void test_register_suite(test_suite_t* suite) {
    if (!test_suites) {
        test_suites = suite;
    } else {
        test_suite_t* current = test_suites;
        while (current->next) {
            current = current->next;
        }
        current->next = suite;
    }
}

// Register a test case
void test_register_case(test_case_t* test_case) {
    if (!current_suite) {
        fprintf(stderr, "Error: No test suite active\n");
        return;
    }
    
    if (!current_suite->tests) {
        current_suite->tests = test_case;
    } else {
        test_case_t* current = current_suite->tests;
        while (current->next) {
            current = current->next;
        }
        current->next = test_case;
    }
}

// Register test suite setup/teardown functions
void test_set_suite(const char* suite_name) {
    test_suite_t* suite = test_suites;
    while (suite) {
        if (strcmp(suite->name, suite_name) == 0) {
            current_suite = suite;
            return;
        }
        suite = suite->next;
    }
    fprintf(stderr, "Warning: Test suite '%s' not found\n", suite_name);
}

// Set verbose mode
void test_set_verbose(bool verbose) {
    test_runner_state.verbose = verbose;
}

// Run a single test
test_result_t test_run_single(const char* suite_name, const char* test_name) {
    test_suite_t* suite = test_suites;
    while (suite) {
        if (strcmp(suite->name, suite_name) == 0) {
            test_case_t* test_case = suite->tests;
            while (test_case) {
                if (strcmp(test_case->test_name, test_name) == 0) {
                    // Execute setup
                    if (suite->setup) {
                        suite->setup();
                    }
                    
                    // Execute test
                    test_result_t result = test_case->test_func();
                    
                    // Execute teardown
                    if (suite->teardown) {
                        suite->teardown();
                    }
                    
                    return result;
                }
                test_case = test_case->next;
            }
            fprintf(stderr, "Test '%s' not found in suite '%s'\n", test_name, suite_name);
            return TEST_FAILED;
        }
        suite = suite->next;
    }
    fprintf(stderr, "Test suite '%s' not found\n", suite_name);
    return TEST_FAILED;
}

// Run all tests in a suite
test_result_t test_run_suite(const char* suite_name) {
    test_suite_t* suite = test_suites;
    while (suite) {
        if (strcmp(suite->name, suite_name) == 0) {
            if (test_runner_state.verbose) {
                printf("Running test suite: %s\n", suite_name);
            }
            
            test_case_t* test_case = suite->tests;
            while (test_case) {
                if (test_runner_state.verbose) {
                    printf("  Running %s::%s...\n", suite_name, test_case->test_name);
                }
                
                // Execute setup
                if (suite->setup) {
                    suite->setup();
                }
                
                // Execute test
                test_result_t result = test_case->test_func();
                test_runner_state.total_tests++;
                
                if (result == TEST_PASSED) {
                    test_runner_state.passed_tests++;
                    if (test_runner_state.verbose) {
                        printf("    PASS\n");
                    }
                } else if (result == TEST_SKIPPED) {
                    test_runner_state.skipped_tests++;
                    if (test_runner_state.verbose) {
                        printf("    SKIP\n");
                    }
                } else {
                    test_runner_state.failed_tests++;
                    if (test_runner_state.verbose) {
                        printf("    FAIL\n");
                    }
                }
                
                // Execute teardown
                if (suite->teardown) {
                    suite->teardown();
                }
                
                test_case = test_case->next;
            }
            
            return TEST_PASSED;
        }
        suite = suite->next;
    }
    fprintf(stderr, "Test suite '%s' not found\n", suite_name);
    return TEST_FAILED;
}

// Run all tests
void test_run_all(void) {
    test_suite_t* suite = test_suites;
    while (suite) {
        test_run_suite(suite->name);
        suite = suite->next;
    }
}

// Print test results
void test_print_results(void) {
    printf("\n=== Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }
}

// Utility functions
void test_skip(const char* reason) {
    if (test_runner_state.verbose) {
        printf("  SKIP: %s\n", reason);
    }
    exit(TEST_SKIPPED);
}

test_result_t test_success(void) {
    return TEST_PASSED;
}

test_result_t test_fail(const char* message) {
    printf("  FAIL: %s\n", message);
    return TEST_FAILED;
}
