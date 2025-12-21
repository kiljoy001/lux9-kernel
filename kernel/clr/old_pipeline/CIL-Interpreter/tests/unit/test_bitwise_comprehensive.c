#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Test suite for comprehensive bitwise operations
TEST_SUITE(bitwise_comprehensive);

// Global test execution state
static vm_execution_state_t* test_state;
static vm_init_t test_init;

// Setup function
static void bitwise_comprehensive_setup(void) {
    memset(&test_init, 0, sizeof(test_init));
    test_state = vm_create_execution_state(&test_init);
}

// Teardown function
static void bitwise_comprehensive_teardown(void) {
    if (test_state != NULL) {
        vm_destroy_execution_state(test_state);
        test_state = NULL;
    }
}

// Test AND operation
TEST(test_and_basic) {
    vm_value_t val1 = vm_make_i4(0b1100);  // 12
    vm_value_t val2 = vm_make_i4(0b1010);  // 10
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_and(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0b1000, result.value.i4);  // 12 & 10 = 8
    
    return TEST_PASSED;
}

// Test OR operation
TEST(test_or_basic) {
    vm_value_t val1 = vm_make_i4(0b1100);  // 12
    vm_value_t val2 = vm_make_i4(0b1010);  // 10
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_or(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0b1110, result.value.i4);  // 12 | 10 = 14
    
    return TEST_PASSED;
}

// Test XOR operation
TEST(test_xor_basic) {
    vm_value_t val1 = vm_make_i4(0b1100);  // 12
    vm_value_t val2 = vm_make_i4(0b1010);  // 10
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_xor(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0b0110, result.value.i4);  // 12 ^ 10 = 6
    
    return TEST_PASSED;
}

// Test NOT operation
TEST(test_not_basic) {
    vm_value_t val = vm_make_i4(0b1100);  // 12
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_not(&val, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(~0b1100, result.value.i4);  // ~12
    
    return TEST_PASSED;
}

// Test SHL (shift left) operation
TEST(test_shl_basic) {
    vm_value_t val = vm_make_i4(5);   // 101 in binary
    vm_value_t shift = vm_make_i4(2); // shift by 2
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shl(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(20, result.value.i4);  // 5 << 2 = 20 (10100 in binary)
    
    return TEST_PASSED;
}

// Test SHR (shift right) operation
TEST(test_shr_basic) {
    vm_value_t val = vm_make_i4(20);  // 10100 in binary
    vm_value_t shift = vm_make_i4(2); // shift by 2
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shr(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);  // 20 >> 2 = 5
    
    return TEST_PASSED;
}

// Test SHR_UN (unsigned shift right) operation
TEST(test_shr_un_basic) {
    vm_value_t val = vm_make_i4(-8);  // 11111111111111111111111111111000 in binary
    vm_value_t shift = vm_make_i4(2); // shift by 2
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shr_un(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    // For unsigned shift, -8 becomes a large positive number, then shifted right
    // The result should be a large positive number
    TEST_ASSERT_TRUE(result.value.i4 > 0);
    
    return TEST_PASSED;
}

// Test AND with zero
TEST(test_and_with_zero) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_and(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // Anything & 0 = 0
    
    return TEST_PASSED;
}

// Test OR with all bits set
TEST(test_or_with_all_bits) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(0xFFFFFFFF);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_or(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, result.value.i4);  // Anything | 0xFFFFFFFF = 0xFFFFFFFF
    
    return TEST_PASSED;
}

// Test XOR with self (should be zero)
TEST(test_xor_with_self) {
    vm_value_t val1 = vm_make_i4(42);
    vm_value_t val2 = vm_make_i4(42);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_xor(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0, result.value.i4);  // X ^ X = 0
    
    return TEST_PASSED;
}

// Test NOT double application (should return original value)
TEST(test_not_double) {
    vm_value_t val = vm_make_i4(42);
    vm_value_t result1 = vm_make_i4(0);
    vm_value_t result2 = vm_make_i4(0);
    
    // Apply NOT twice
    bool success1 = vm_not(&val, &result1);
    TEST_ASSERT_TRUE(success1);
    
    bool success2 = vm_not(&result1, &result2);
    TEST_ASSERT_TRUE(success2);
    
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result2.type);
    TEST_ASSERT_EQUAL(42, result2.value.i4);  // ~~X = X
    
    return TEST_PASSED;
}

// Test SHL with zero shift
TEST(test_shl_zero_shift) {
    vm_value_t val = vm_make_i4(42);
    vm_value_t shift = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shl(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);  // X << 0 = X
    
    return TEST_PASSED;
}

// Test SHR with zero shift
TEST(test_shr_zero_shift) {
    vm_value_t val = vm_make_i4(42);
    vm_value_t shift = vm_make_i4(0);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shr(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);  // X >> 0 = X
    
    return TEST_PASSED;
}

// Test large shift values (should mask to 5 bits for 32-bit values)
TEST(test_shl_large_shift) {
    vm_value_t val = vm_make_i4(1);
    vm_value_t shift = vm_make_i4(33);  // 33 & 31 = 1 for 32-bit values
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shl(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(2, result.value.i4);  // 1 << 1 = 2 (33 masked to 1)
    
    return TEST_PASSED;
}

// Test edge case: maximum shift
TEST(test_shl_max_shift) {
    vm_value_t val = vm_make_i4(1);
    vm_value_t shift = vm_make_i4(31);  // Maximum shift for 32-bit signed int
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_shl(&val, &shift, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0x80000000, result.value.i4);  // 1 << 31 = -2147483648 (sign bit set)
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== Comprehensive Bitwise Operations Test Suite ===\n");
    
    // Register test suite
    test_register_suite(&suite_bitwise_comprehensive);
    
    // Set current suite
    current_suite = &suite_bitwise_comprehensive;
    
    // Test case declarations
    static test_case_t test_and_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_and_basic",
        .test_func = test_and_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_or_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_or_basic",
        .test_func = test_or_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_xor_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_xor_basic",
        .test_func = test_xor_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_not_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_not_basic",
        .test_func = test_not_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shl_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shl_basic",
        .test_func = test_shl_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shr_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shr_basic",
        .test_func = test_shr_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shr_un_basic = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shr_un_basic",
        .test_func = test_shr_un_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_and_with_zero = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_and_with_zero",
        .test_func = test_and_with_zero_wrapper,
        .next = NULL
    };
    
    static test_case_t test_or_with_all_bits = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_or_with_all_bits",
        .test_func = test_or_with_all_bits_wrapper,
        .next = NULL
    };
    
    static test_case_t test_xor_with_self = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_xor_with_self",
        .test_func = test_xor_with_self_wrapper,
        .next = NULL
    };
    
    static test_case_t test_not_double = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_not_double",
        .test_func = test_not_double_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shl_zero_shift = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shl_zero_shift",
        .test_func = test_shl_zero_shift_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shr_zero_shift = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shr_zero_shift",
        .test_func = test_shr_zero_shift_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shl_large_shift = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shl_large_shift",
        .test_func = test_shl_large_shift_wrapper,
        .next = NULL
    };
    
    static test_case_t test_shl_max_shift = {
        .suite_name = "bitwise_comprehensive",
        .test_name = "test_shl_max_shift",
        .test_func = test_shl_max_shift_wrapper,
        .next = NULL
    };
    
    // Link all test cases
    test_and_basic.next = &test_or_basic;
    test_or_basic.next = &test_xor_basic;
    test_xor_basic.next = &test_not_basic;
    test_not_basic.next = &test_shl_basic;
    test_shl_basic.next = &test_shr_basic;
    test_shr_basic.next = &test_shr_un_basic;
    test_shr_un_basic.next = &test_and_with_zero;
    test_and_with_zero.next = &test_or_with_all_bits;
    test_or_with_all_bits.next = &test_xor_with_self;
    test_xor_with_self.next = &test_not_double;
    test_not_double.next = &test_shl_zero_shift;
    test_shl_zero_shift.next = &test_shr_zero_shift;
    test_shr_zero_shift.next = &test_shl_large_shift;
    test_shl_large_shift.next = &test_shl_max_shift;
    test_shl_max_shift.next = NULL;
    
    // Register first test case
    test_register_case(&test_and_basic);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("bitwise_comprehensive");
    
    // Print results
    printf("\n=== Bitwise Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All bitwise tests passed!\n");
    } else {
        printf("\n✗ Some bitwise tests failed!\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}