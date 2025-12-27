#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include "../../include/il_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

// Test suite for arithmetic operations
TEST_SUITE(execution_arithmetic);

static void execution_arithmetic_setup(void) {
    // Setup code if needed
}

static void execution_arithmetic_teardown(void) {
    // Teardown code if needed
}

// Helper function to create execution state
static vm_execution_state_t* create_test_state(void) {
    vm_init_t init = {0};
    return vm_create_execution_state(&init);
}

// ==================== INTEGER ARITHMETIC TESTS ====================

// Test basic int32 addition
TEST(test_add_i4_basic) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test 5 + 3 = 8
    vm_value_t val1 = vm_make_i4(5);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(8, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 addition with negative numbers
TEST(test_add_i4_negative) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test (-5) + 3 = -2
    vm_value_t val1 = vm_make_i4(-5);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-2, result.value.i4);
    
    // Test 5 + (-3) = 2
    val1 = vm_make_i4(5);
    val2 = vm_make_i4(-3);
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(2, result.value.i4);
    
    // Test (-5) + (-3) = -8
    val1 = vm_make_i4(-5);
    val2 = vm_make_i4(-3);
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-8, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 addition overflow
TEST(test_add_i4_overflow) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test INT_MAX + 1 (should wrap around)
    vm_value_t val1 = vm_make_i4(INT_MAX);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(INT_MIN, result.value.i4);
    
    // Test INT_MIN + (-1) (should wrap around)
    val1 = vm_make_i4(INT_MIN);
    val2 = vm_make_i4(-1);
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(INT_MAX, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 subtraction
TEST(test_subtract_i4_basic) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test 10 - 3 = 7
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(7, result.value.i4);
    
    // Test 3 - 10 = -7
    val1 = vm_make_i4(3);
    val2 = vm_make_i4(10);
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-7, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 multiplication
TEST(test_multiply_i4_basic) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test 6 * 7 = 42
    vm_value_t val1 = vm_make_i4(6);
    vm_value_t val2 = vm_make_i4(7);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);
    
    // Test (-6) * 7 = -42
    val1 = vm_make_i4(-6);
    val2 = vm_make_i4(7);
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-42, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 multiplication overflow
TEST(test_multiply_i4_overflow) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test large multiplication that overflows
    vm_value_t val1 = vm_make_i4(100000);
    vm_value_t val2 = vm_make_i4(100000);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    // Should wrap around due to overflow
    int32_t expected = (int32_t)(100000 * 100000);
    TEST_ASSERT_EQUAL(expected, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 division
TEST(test_divide_i4_basic) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test 20 / 4 = 5
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    // Test (-20) / 4 = -5
    val1 = vm_make_i4(-20);
    val2 = vm_make_i4(4);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-5, result.value.i4);
    
    // Test 20 / (-4) = -5
    val1 = vm_make_i4(20);
    val2 = vm_make_i4(-4);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-5, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 division by zero
TEST(test_divide_i4_by_zero) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result;
    
    TEST_ASSERT_FALSE(vm_divide(&val1, &val2, &result));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int32 division with truncation toward zero
TEST(test_divide_i4_truncation) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test 7 / 3 = 2 (truncation toward zero)
    vm_value_t val1 = vm_make_i4(7);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result;
    
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(2, result.value.i4);
    
    // Test (-7) / 3 = -2 (truncation toward zero)
    val1 = vm_make_i4(-7);
    val2 = vm_make_i4(3);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-2, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int8 arithmetic operations
TEST(test_arithmetic_i8) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_i1(100);
    vm_value_t val2 = vm_make_i1(25);
    vm_value_t result;
    
    // Test 100 + 25 = 125 (will overflow int8)
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I1, result.type);
    TEST_ASSERT_EQUAL((int8_t)(100 + 25), result.value.i1);
    
    // Test 100 - 25 = 75
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I1, result.type);
    TEST_ASSERT_EQUAL(75, result.value.i1);
    
    // Test 100 * 2 = 200 (will overflow int8)
    val2 = vm_make_i1(2);
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I1, result.type);
    TEST_ASSERT_EQUAL((int8_t)(100 * 2), result.value.i1);
    
    // Test 100 / 4 = 25
    val2 = vm_make_i1(4);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I1, result.type);
    TEST_ASSERT_EQUAL(25, result.value.i1);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int16 arithmetic operations
TEST(test_arithmetic_i2) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_i2(30000);
    vm_value_t val2 = vm_make_i2(10000);
    vm_value_t result;
    
    // Test 30000 + 10000 = 40000 (will overflow int16)
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I2, result.type);
    TEST_ASSERT_EQUAL((int16_t)(30000 + 10000), result.value.i2);
    
    // Test 30000 - 10000 = 20000
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I2, result.type);
    TEST_ASSERT_EQUAL(20000, result.value.i2);
    
    // Test 30000 / 1000 = 30
    val2 = vm_make_i2(1000);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I2, result.type);
    TEST_ASSERT_EQUAL(30, result.value.i2);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test int64 arithmetic operations
TEST(test_arithmetic_i8_extended) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_i8(1000000000LL);
    vm_value_t val2 = vm_make_i8(2000000000LL);
    vm_value_t result;
    
    // Test 1000000000 + 2000000000 = 3000000000
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I8, result.type);
    TEST_ASSERT_EQUAL(3000000000LL, result.value.i8);
    
    // Test 2000000000 - 1000000000 = 1000000000
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I8, result.type);
    TEST_ASSERT_EQUAL(-1000000000LL, result.value.i8);
    
    // Test 1000000000 * 2 = 2000000000
    val2 = vm_make_i8(2);
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I8, result.type);
    TEST_ASSERT_EQUAL(2000000000LL, result.value.i8);
    
    // Test 2000000000 / 2 = 1000000000
    val2 = vm_make_i8(2);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I8, result.type);
    TEST_ASSERT_EQUAL(1000000000LL, result.value.i8);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test unsigned integer arithmetic
TEST(test_arithmetic_unsigned) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // Test uint32 arithmetic
    vm_value_t val1 = vm_make_u4(4000000000u);
    vm_value_t val2 = vm_make_u4(1000000000u);
    vm_value_t result;
    
    // Test 4000000000 + 1000000000 = 5000000000 (will wrap)
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    uint32_t expected = 4000000000u + 1000000000u;
    TEST_ASSERT_EQUAL(expected, result.value.u4);
    
    // Test 4000000000 - 1000000000 = 3000000000
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(3000000000u, result.value.u4);
    
    // Test underflow: 1000000000 - 4000000000 = wrapped value
    TEST_ASSERT_TRUE(vm_subtract(&val2, &val1, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    uint32_t underflow = 1000000000u - 4000000000u;
    TEST_ASSERT_EQUAL(underflow, result.value.u4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test mixed type arithmetic (should fail)
TEST(test_arithmetic_mixed_types) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_r4(3.5f);
    vm_value_t result;
    
    // Test adding int32 to float32 (should fail)
    TEST_ASSERT_FALSE(vm_add(&val1, &val2, &result));
    
    // Test multiplying int32 with int8 (should fail for now)
    val2 = vm_make_i1(5);
    TEST_ASSERT_FALSE(vm_multiply(&val1, &val2, &result));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// ==================== FLOATING-POINT ARITHMETIC TESTS ====================

// Test float32 arithmetic operations
TEST(test_arithmetic_r4) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_r4(10.5f);
    vm_value_t val2 = vm_make_r4(3.5f);
    vm_value_t result;
    
    // Test 10.5 + 3.5 = 14.0
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, result.type);
    TEST_ASSERT_FLOAT_EQUAL(14.0f, result.value.r4, 0.0001f);
    
    // Test 10.5 - 3.5 = 7.0
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, result.type);
    TEST_ASSERT_FLOAT_EQUAL(7.0f, result.value.r4, 0.0001f);
    
    // Test 10.5 * 2.0 = 21.0
    val2 = vm_make_r4(2.0f);
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, result.type);
    TEST_ASSERT_FLOAT_EQUAL(21.0f, result.value.r4, 0.0001f);
    
    // Test 10.5 / 2.0 = 5.25
    val2 = vm_make_r4(2.0f);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, result.type);
    TEST_ASSERT_FLOAT_EQUAL(5.25f, result.value.r4, 0.0001f);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test float64 arithmetic operations
TEST(test_arithmetic_r8) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_r8(100.123456789);
    vm_value_t val2 = vm_make_r8(50.987654321);
    vm_value_t result;
    
    // Test 100.123456789 + 50.987654321 = 151.111111110
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R8, result.type);
    TEST_ASSERT_FLOAT_EQUAL(151.111111110, result.value.r8, 0.000000001);
    
    // Test 100.123456789 - 50.987654321 = 49.135802468
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R8, result.type);
    TEST_ASSERT_FLOAT_EQUAL(49.135802468, result.value.r8, 0.000000001);
    
    // Test special values
    val1 = vm_make_r8(INFINITY);
    val2 = vm_make_r8(10.0);
    
    // Test INFINITY + 10.0 = INFINITY
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R8, result.type);
    TEST_ASSERT_TRUE(isinf(result.value.r8) && result.value.r8 > 0);
    
    // Test NaN handling
    val1 = vm_make_r8(NAN);
    val2 = vm_make_r8(10.0);
    
    // Test NAN + 10.0 = NAN
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R8, result.type);
    TEST_ASSERT_TRUE(isnan(result.value.r8));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// Test division by zero for floating-point
TEST(test_arithmetic_r4_r8_divide_by_zero) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_r4(10.0f);
    vm_value_t val2 = vm_make_r4(0.0f);
    vm_value_t result;
    
    // Test 10.0 / 0.0 = INFINITY
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, result.type);
    TEST_ASSERT_TRUE(isinf(result.value.r4) && result.value.r4 > 0);
    
    // Test 0.0 / 0.0 = NAN
    val1 = vm_make_r4(0.0f);
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_R4, result.type);
    TEST_ASSERT_TRUE(isnan(result.value.r4));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// ==================== OVERFLOW-CHECKING ARITHMETIC TESTS TODO: Add ====================

// overflow-checking arithmetic tests when implementation is ready

// ==================== TYPE CONVERSION ARITHMETIC TESTS ====================

// Test arithmetic with type conversions
TEST(test_arithmetic_with_conversions) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // This will be implemented when conversion functions are ready
    // For now, just test that conversion functions exist
    
    vm_value_t val = vm_make_i4(42);
    vm_value_t result;
    
    // Test basic conversion validation
    TEST_ASSERT_TRUE(vm_is_valid_conversion(VM_TYPE_I4, VM_TYPE_I8));
    TEST_ASSERT_TRUE(vm_is_valid_conversion(VM_TYPE_I4, VM_TYPE_R4));
    TEST_ASSERT_TRUE(vm_is_valid_conversion(VM_TYPE_I4, VM_TYPE_U4));
    TEST_ASSERT_FALSE(vm_is_valid_conversion(VM_TYPE_REF, VM_TYPE_I4));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// ==================== STACK-BASED EXECUTION TESTS ====================

// Test stack-based arithmetic execution
TEST(test_stack_arithmetic_execution) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    // This will be implemented when the full execution engine is ready
    // For now, just test basic stack operations
    
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(5);
    vm_value_t result;
    
    // Test stack push/pop
    TEST_ASSERT_TRUE(vm_stack_push(state, &val1));
    TEST_ASSERT_TRUE(vm_stack_push(state, &val2));
    TEST_ASSERT_EQUAL(2, vm_stack_depth(state));
    
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(10, result.value.i4);
    
    TEST_ASSERT_EQUAL(0, vm_stack_depth(state));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// ==================== ERROR HANDLING TESTS ====================

// Test error handling in arithmetic operations
TEST(test_arithmetic_error_handling) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(0);
    vm_value_t result;
    
    // Test division by zero
    TEST_ASSERT_FALSE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_TRUE(vm_has_error(state));
    vm_clear_error(state);
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Test with NULL pointers
    TEST_ASSERT_FALSE(vm_add(NULL, &val2, &result));
    TEST_ASSERT_FALSE(vm_add(&val1, NULL, &result));
    TEST_ASSERT_FALSE(vm_add(&val1, &val2, NULL));
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

// ==================== EDGE CASE TESTS ====================

// Test arithmetic with extreme values
TEST(test_arithmetic_edge_cases) {
    vm_execution_state_t* state = create_test_state();
    TEST_ASSERT_NOT_NULL(state);
    
    vm_value_t val1, val2, result;
    
    // Test with INT_MAX and INT_MIN
    val1 = vm_make_i4(INT_MAX);
    val2 = vm_make_i4(INT_MIN);
    
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-1, result.value.i4);
    
    TEST_ASSERT_TRUE(vm_subtract(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-1, result.value.i4);
    
    // Test with 0
    val1 = vm_make_i4(0);
    val2 = vm_make_i4(42);
    
    TEST_ASSERT_TRUE(vm_add(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(42, result.value.i4);
    
    TEST_ASSERT_TRUE(vm_multiply(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(0, result.value.i4);
    
    // Test 0 / 42 = 0
    TEST_ASSERT_TRUE(vm_divide(&val1, &val2, &result));
    TEST_ASSERT_EQUAL(0, result.value.i4);
    
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}

int main(void) {
    printf("=== Execution Engine Arithmetic Tests ===\n\n");
    
    // Register the test suite
    test_register_suite(&suite_execution_arithmetic);
    
    // Set current suite
    current_suite = &suite_execution_arithmetic;
    
    // Register test cases - Integer Arithmetic
    static test_case_t test_add_i4_basic = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_add_i4_basic",
        .test_func = test_add_i4_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_i4_negative = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_add_i4_negative",
        .test_func = test_add_i4_negative_wrapper,
        .next = NULL
    };
    
    static test_case_t test_add_i4_overflow = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_add_i4_overflow",
        .test_func = test_add_i4_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_subtract_i4_basic = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_subtract_i4_basic",
        .test_func = test_subtract_i4_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_i4_basic = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_multiply_i4_basic",
        .test_func = test_multiply_i4_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_multiply_i4_overflow = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_multiply_i4_overflow",
        .test_func = test_multiply_i4_overflow_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_i4_basic = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_divide_i4_basic",
        .test_func = test_divide_i4_basic_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_i4_by_zero = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_divide_i4_by_zero",
        .test_func = test_divide_i4_by_zero_wrapper,
        .next = NULL
    };
    
    static test_case_t test_divide_i4_truncation = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_divide_i4_truncation",
        .test_func = test_divide_i4_truncation_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_i8 = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_i8",
        .test_func = test_arithmetic_i8_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_i2 = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_i2",
        .test_func = test_arithmetic_i2_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_i8_extended = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_i8_extended",
        .test_func = test_arithmetic_i8_extended_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_unsigned = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_unsigned",
        .test_func = test_arithmetic_unsigned_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_mixed_types = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_mixed_types",
        .test_func = test_arithmetic_mixed_types_wrapper,
        .next = NULL
    };
    
    // Register Floating-Point tests
    static test_case_t test_arithmetic_r4 = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_r4",
        .test_func = test_arithmetic_r4_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_r8 = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_r8",
        .test_func = test_arithmetic_r8_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_r4_r8_divide_by_zero = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_r4_r8_divide_by_zero",
        .test_func = test_arithmetic_r4_r8_divide_by_zero_wrapper,
        .next = NULL
    };
    
    // Register Type Conversion tests
    static test_case_t test_arithmetic_with_conversions = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_with_conversions",
        .test_func = test_arithmetic_with_conversions_wrapper,
        .next = NULL
    };
    
    // Register Stack Execution tests
    static test_case_t test_stack_arithmetic_execution = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_stack_arithmetic_execution",
        .test_func = test_stack_arithmetic_execution_wrapper,
        .next = NULL
    };
    
    // Register Error Handling tests
    static test_case_t test_arithmetic_error_handling = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_error_handling",
        .test_func = test_arithmetic_error_handling_wrapper,
        .next = NULL
    };
    
    // Register Edge Case tests
    static test_case_t test_arithmetic_edge_cases = {
        .suite_name = "execution_arithmetic",
        .test_name = "test_arithmetic_edge_cases",
        .test_func = test_arithmetic_edge_cases_wrapper,
        .next = NULL
    };
    
    // Link test cases
    test_add_i4_basic.next = &test_add_i4_negative;
    test_add_i4_negative.next = &test_add_i4_overflow;
    test_add_i4_overflow.next = &test_subtract_i4_basic;
    test_subtract_i4_basic.next = &test_multiply_i4_basic;
    test_multiply_i4_basic.next = &test_multiply_i4_overflow;
    test_multiply_i4_overflow.next = &test_divide_i4_basic;
    test_divide_i4_basic.next = &test_divide_i4_by_zero;
    test_divide_i4_by_zero.next = &test_divide_i4_truncation;
    test_divide_i4_truncation.next = &test_arithmetic_i8;
    test_arithmetic_i8.next = &test_arithmetic_i2;
    test_arithmetic_i2.next = &test_arithmetic_i8_extended;
    test_arithmetic_i8_extended.next = &test_arithmetic_unsigned;
    test_arithmetic_unsigned.next = &test_arithmetic_mixed_types;
    test_arithmetic_mixed_types.next = &test_arithmetic_r4;
    test_arithmetic_r4.next = &test_arithmetic_r8;
    test_arithmetic_r8.next = &test_arithmetic_r4_r8_divide_by_zero;
    test_arithmetic_r4_r8_divide_by_zero.next = &test_arithmetic_with_conversions;
    test_arithmetic_with_conversions.next = &test_stack_arithmetic_execution;
    test_stack_arithmetic_execution.next = &test_arithmetic_error_handling;
    test_arithmetic_error_handling.next = &test_arithmetic_edge_cases;
    test_arithmetic_edge_cases.next = NULL;
    
    // Register first test case
    test_register_case(&test_add_i4_basic);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("execution_arithmetic");
    
    // Print results
    printf("\n=== Test Results ===\n");
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
