#ifndef CIL_TEST_FRAMEWORK_H
#define CIL_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Test result types
typedef enum {
    TEST_PASSED = 0,
    TEST_FAILED = 1,
    TEST_SKIPPED = 2
} test_result_t;

// Test case structure
typedef struct test_case {
    const char* suite_name;
    const char* test_name;
    test_result_t (*test_func)(void);
    struct test_case* next;
} test_case_t;

// Test suite structure  
typedef struct test_suite {
    const char* name;
    void (*setup)(void);
    void (*teardown)(void);
    test_case_t* tests;
    struct test_suite* next;
} test_suite_t;

// Global test runner state
typedef struct {
    test_suite_t* suites;
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    bool verbose;
} test_runner_state_t;

// Test macros
#define TEST_SUITE(suite_name) \
    static void suite_name##_setup(void); \
    static void suite_name##_teardown(void); \
    static test_suite_t suite_##suite_name = { \
        .name = #suite_name, \
        .setup = suite_name##_setup, \
        .teardown = suite_name##_teardown, \
        .tests = NULL, \
        .next = NULL \
    }

#define TEST(test_name) \
    static test_result_t test_name##_wrapper(void); \
    static test_result_t test_name##_impl(void); \
    static test_result_t test_name##_wrapper(void) { \
        if (test_runner_state.verbose) { \
            printf("  Running %s::%s...\n", current_suite->name, #test_name); \
        } \
        test_result_t result = TEST_SUCCESS; \
        if (current_suite->setup) { \
            current_suite->setup(); \
        } \
        result = test_name##_impl(); \
        if (current_suite->teardown) { \
            current_suite->teardown(); \
        } \
        return result; \
    } \
    static test_result_t test_name##_impl(void)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("  FAIL: %s:%d Expected %lld, got %lld\n", \
                   __FILE__, __LINE__, \
                   (long long)(expected), (long long)(actual)); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("  FAIL: %s:%d Expected non-NULL pointer\n", __FILE__, __LINE__); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("  FAIL: %s:%d Expected NULL pointer, got %p\n", \
                   __FILE__, __LINE__, (ptr)); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("  FAIL: %s:%d Expected true, got false\n", __FILE__, __LINE__); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf("  FAIL: %s:%d Expected false, got true\n", __FILE__, __LINE__); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (strcmp(expected, actual) != 0) { \
            printf("  FAIL: %s:%d Expected '%s', got '%s'\n", \
                   __FILE__, __LINE__, expected, actual); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_MEM_EQUAL(expected, actual, size) \
    do { \
        if (memcmp(expected, actual, size) != 0) { \
            printf("  FAIL: %s:%d Memory mismatch\n", __FILE__, __LINE__); \
            return TEST_FAILED; \
        } \
    } while(0)

// Test runner functions
void test_register_suite(test_suite_t* suite);
void test_register_case(test_case_t* test_case);
void test_run_all(void);
void test_run_suite(const char* suite_name);
void test_run_single(const char* suite_name, const char* test_name);
void test_set_verbose(bool verbose);

// Utility functions
void test_skip(const char* reason);
test_result_t test_success(void);
test_result_t test_fail(const char* message);

// Global state (internal)
extern test_suite_t* test_suites;
extern test_suite_t* current_suite;
extern test_runner_state_t test_runner_state;

#endif // CIL_TEST_FRAMEWORK_H
