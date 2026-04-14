/*
 * ECMA-335 Conversion Overflow Detection Tests
 * Copyright (c) 2025 GHOSTDAG IPC Consensus System
 * 
 * Comprehensive test suite for conv.ovf.* functions
 * Tests all conversion operations with overflow detection as specified in ECMA-335.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "overflow.h"

/* Test result tracking */
typedef enum {
    TEST_PASSED,
    TEST_FAILED,
    TEST_SKIPPED
} test_result_t;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_skipped = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ PASS: %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  ✗ FAIL: %s\n", message); \
        } \
    } while(0)

#define TEST_ASSERT_OVERFLOW(func_call, expected_overflow, message) \
    do { \
        tests_run++; \
        bool overflow = func_call; \
        if (overflow == expected_overflow) { \
            tests_passed++; \
            printf("  ✓ PASS: %s (overflow=%s)\n", message, overflow ? "true" : "false"); \
        } else { \
            tests_failed++; \
            printf("  ✗ FAIL: %s (expected overflow=%s, got overflow=%s)\n", \
                   message, expected_overflow ? "true" : "false", overflow ? "true" : "false"); \
        } \
    } while(0)

#define TEST_FUNCTION(name) \
    static void name(void); \
    static void name##_wrapper(void) { \
        printf("\n=== Running %s ===\n", #name); \
        name(); \
    } \
    static void name(void)

/*==============================================================================
 * Conversion Tests - int32 to smaller types
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_i1_from_i32)
{
    int8_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(0, &result), false, "conv_ovf_i1_from_i32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(127, &result), false, "conv_ovf_i1_from_i32(127)");
    assert(result == 127);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(-128, &result), false, "conv_ovf_i1_from_i32(-128)");
    assert(result == -128);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(128, &result), true, "conv_ovf_i1_from_i32(128) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(-129, &result), true, "conv_ovf_i1_from_i32(-129) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(1000, &result), true, "conv_ovf_i1_from_i32(1000) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(-1000, &result), true, "conv_ovf_i1_from_i32(-1000) should overflow");
}

TEST_FUNCTION(test_conv_ovf_u1_from_i32)
{
    uint8_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(0, &result), false, "conv_ovf_u1_from_i32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(255, &result), false, "conv_ovf_u1_from_i32(255)");
    assert(result == 255);
    
    /* Overflow cases (negative numbers) */
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(-1, &result), true, "conv_ovf_u1_from_i32(-1) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(-128, &result), true, "conv_ovf_u1_from_i32(-128) should overflow");
    
    /* Overflow cases (too large) */
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(256, &result), true, "conv_ovf_u1_from_i32(256) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(1000, &result), true, "conv_ovf_u1_from_i32(1000) should overflow");
}

TEST_FUNCTION(test_conv_ovf_i2_from_i32)
{
    int16_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(0, &result), false, "conv_ovf_i2_from_i32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(32767, &result), false, "conv_ovf_i2_from_i32(32767)");
    assert(result == 32767);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(-32768, &result), false, "conv_ovf_i2_from_i32(-32768)");
    assert(result == -32768);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(32768, &result), true, "conv_ovf_i2_from_i32(32768) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(-32769, &result), true, "conv_ovf_i2_from_i32(-32769) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(100000, &result), true, "conv_ovf_i2_from_i32(100000) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_i32(-100000, &result), true, "conv_ovf_i2_from_i32(-100000) should overflow");
}

TEST_FUNCTION(test_conv_ovf_u2_from_i32)
{
    uint16_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_i32(0, &result), false, "conv_ovf_u2_from_i32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_i32(65535, &result), false, "conv_ovf_u2_from_i32(65535)");
    assert(result == 65535);
    
    /* Overflow cases (negative numbers) */
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_i32(-1, &result), true, "conv_ovf_u2_from_i32(-1) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_i32(-32768, &result), true, "conv_ovf_u2_from_i32(-32768) should overflow");
    
    /* Overflow cases (too large) */
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_i32(65536, &result), true, "conv_ovf_u2_from_i32(65536) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_i32(100000, &result), true, "conv_ovf_u2_from_i32(100000) should overflow");
}

/*==============================================================================
 * Conversion Tests - uint32 to smaller types
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_i1_from_u32)
{
    int8_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_u32(0, &result), false, "conv_ovf_i1_from_u32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_u32(127, &result), false, "conv_ovf_i1_from_u32(127)");
    assert(result == 127);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_u32(128, &result), true, "conv_ovf_i1_from_u32(128) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_u32(255, &result), true, "conv_ovf_i1_from_u32(255) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_u32(1000, &result), true, "conv_ovf_i1_from_u32(1000) should overflow");
}

TEST_FUNCTION(test_conv_ovf_u1_from_u32)
{
    uint8_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_u32(0, &result), false, "conv_ovf_u1_from_u32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_u32(255, &result), false, "conv_ovf_u1_from_u32(255)");
    assert(result == 255);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_u32(256, &result), true, "conv_ovf_u1_from_u32(256) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_u32(1000, &result), true, "conv_ovf_u1_from_u32(1000) should overflow");
}

TEST_FUNCTION(test_conv_ovf_i2_from_u32)
{
    int16_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_u32(0, &result), false, "conv_ovf_i2_from_u32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_u32(32767, &result), false, "conv_ovf_i2_from_u32(32767)");
    assert(result == 32767);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_u32(32768, &result), true, "conv_ovf_i2_from_u32(32768) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_u32(65535, &result), true, "conv_ovf_i2_from_u32(65535) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_u32(100000, &result), true, "conv_ovf_i2_from_u32(100000) should overflow");
}

TEST_FUNCTION(test_conv_ovf_u2_from_u32)
{
    uint16_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_u32(0, &result), false, "conv_ovf_u2_from_u32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_u32(65535, &result), false, "conv_ovf_u2_from_u32(65535)");
    assert(result == 65535);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_u32(65536, &result), true, "conv_ovf_u2_from_u32(65536) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_u2_from_u32(100000, &result), true, "conv_ovf_u2_from_u32(100000) should overflow");
}

/*==============================================================================
 * Conversion Tests - int32/uint32 to larger types (should never overflow)
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_i8_from_i32)
{
    int64_t result;
    
    /* All conversions should succeed */
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_i32(0, &result), false, "conv_ovf_i8_from_i32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_i32(INT32_MAX, &result), false, "conv_ovf_i8_from_i32(INT32_MAX)");
    assert(result == INT32_MAX);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_i32(INT32_MIN, &result), false, "conv_ovf_i8_from_i32(INT32_MIN)");
    assert(result == INT32_MIN);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_i32(1000000, &result), false, "conv_ovf_i8_from_i32(1000000)");
    assert(result == 1000000);
}

TEST_FUNCTION(test_conv_ovf_u8_from_u32)
{
    uint64_t result;
    
    /* All conversions should succeed */
    TEST_ASSERT_OVERFLOW(conv_ovf_u8_from_u32(0, &result), false, "conv_ovf_u8_from_u32(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_u8_from_u32(UINT32_MAX, &result), false, "conv_ovf_u8_from_u32(UINT32_MAX)");
    assert(result == UINT32_MAX);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_u8_from_u32(1000000, &result), false, "conv_ovf_u8_from_u32(1000000)");
    assert(result == 1000000);
}

/*==============================================================================
 * Conversion Tests - int64 to smaller types
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_i1_from_i64)
{
    int8_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(0, &result), false, "conv_ovf_i1_from_i64(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(127, &result), false, "conv_ovf_i1_from_i64(127)");
    assert(result == 127);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(-128, &result), false, "conv_ovf_i1_from_i64(-128)");
    assert(result == -128);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(128, &result), true, "conv_ovf_i1_from_i64(128) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(-129, &result), true, "conv_ovf_i1_from_i64(-129) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(1000, &result), true, "conv_ovf_i1_from_i64(1000) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(INT64_MAX, &result), true, "conv_ovf_i1_from_i64(INT64_MAX) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i64(INT64_MIN, &result), true, "conv_ovf_i1_from_i64(INT64_MIN) should overflow");
}

TEST_FUNCTION(test_conv_ovf_i4_from_i64)
{
    int32_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_i64(0, &result), false, "conv_ovf_i4_from_i64(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_i64(INT32_MAX, &result), false, "conv_ovf_i4_from_i64(INT32_MAX)");
    assert(result == INT32_MAX);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_i64(INT32_MIN, &result), false, "conv_ovf_i4_from_i64(INT32_MIN)");
    assert(result == INT32_MIN);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_i64(INT32_MAX + 1LL, &result), true, "conv_ovf_i4_from_i64(INT32_MAX + 1) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_i64(INT32_MIN - 1LL, &result), true, "conv_ovf_i4_from_i64(INT32_MIN - 1) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_i64(INT64_MAX, &result), true, "conv_ovf_i4_from_i64(INT64_MAX) should overflow");
}

/*==============================================================================
 * Conversion Tests - uint64 to smaller types
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_i8_from_u64)
{
    int64_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_u64(0, &result), false, "conv_ovf_i8_from_u64(0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_u64(INT64_MAX, &result), false, "conv_ovf_i8_from_u64(INT64_MAX)");
    assert(result == INT64_MAX);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_u64((uint64_t)INT64_MAX + 1, &result), true, "conv_ovf_i8_from_u64(INT64_MAX + 1) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i8_from_u64(UINT64_MAX, &result), true, "conv_ovf_i8_from_u64(UINT64_MAX) should overflow");
}

/*==============================================================================
 * Conversion Tests - floating point conversions
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_i4_from_r4)
{
    int32_t result;
    
    /* Valid conversions */
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(0.0f, &result), false, "conv_ovf_i4_from_r4(0.0)");
    assert(result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(123.45f, &result), false, "conv_ovf_i4_from_r4(123.45)");
    assert(result == 123);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(1000000.0f, &result), false, "conv_ovf_i4_from_r4(1000000.0)");
    assert(result == 1000000);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(-1000000.0f, &result), false, "conv_ovf_i4_from_r4(-1000000.0)");
    assert(result == -1000000);
    
    /* Overflow cases */
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(3.4028235e+38f, &result), true, "conv_ovf_i4_from_r4(3.4e38) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(-3.4028235e+38f, &result), true, "conv_ovf_i4_from_r4(-3.4e38) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(INFINITY, &result), true, "conv_ovf_i4_from_r4(INFINITY) should overflow");
    TEST_ASSERT_OVERFLOW(conv_ovf_i4_from_r4(NAN, &result), true, "conv_ovf_i4_from_r4(NAN) should overflow");
}

/*==============================================================================
 * Edge case and boundary tests
 *============================================================================*/

TEST_FUNCTION(test_conv_ovf_edge_cases)
{
    int8_t i8_result;
    int16_t i16_result;
    int32_t i32_result;
    uint8_t u8_result;
    uint16_t u16_result;
    
    /* Test boundary values exactly */
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(INT8_MAX, &i8_result), false, "conv_ovf_i1_from_i32(INT8_MAX)");
    assert(i8_result == INT8_MAX);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(INT8_MIN, &i8_result), false, "conv_ovf_i1_from_i32(INT8_MIN)");
    assert(i8_result == INT8_MIN);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(INT8_MAX + 1, &i8_result), true, "conv_ovf_i1_from_i32(INT8_MAX + 1) should overflow");
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i1_from_i32(INT8_MIN - 1, &i8_result), true, "conv_ovf_i1_from_i32(INT8_MIN - 1) should overflow");
    
    /* Test sign changes */
    TEST_ASSERT_OVERFLOW(conv_ovf_u1_from_i32(0, &u8_result), false, "conv_ovf_u1_from_i32(0) to unsigned");
    assert(u8_result == 0);
    
    TEST_ASSERT_OVERFLOW(conv_ovf_i2_from_u32(UINT16_MAX, &i16_result), true, "conv_ovf_i2_from_u32(UINT16_MAX) should overflow");
}

/*==============================================================================
 * Main test runner
 *============================================================================*/

int main(void)
{
    printf("=== ECMA-335 Conversion Overflow Detection Test Suite ===\n");
    printf("Testing conv.ovf.* functions for ORSI 1b completion\n\n");
    
    /* Run all test functions */
    test_conv_ovf_i1_from_i32_wrapper();
    test_conv_ovf_u1_from_i32_wrapper();
    test_conv_ovf_i2_from_i32_wrapper();
    test_conv_ovf_u2_from_i32_wrapper();
    
    test_conv_ovf_i1_from_u32_wrapper();
    test_conv_ovf_u1_from_u32_wrapper();
    test_conv_ovf_i2_from_u32_wrapper();
    test_conv_ovf_u2_from_u32_wrapper();
    
    test_conv_ovf_i8_from_i32_wrapper();
    test_conv_ovf_u8_from_u32_wrapper();
    
    test_conv_ovf_i1_from_i64_wrapper();
    test_conv_ovf_i4_from_i64_wrapper();
    
    test_conv_ovf_i8_from_u64_wrapper();
    
    test_conv_ovf_i4_from_r4_wrapper();
    
    test_conv_ovf_edge_cases_wrapper();
    
    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Tests skipped: %d\n", tests_skipped);
    
    if (tests_failed == 0) {
        printf("\n🎉 All tests passed! ORSI 1b conversion overflow detection is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Check the implementation.\n");
        return 1;
    }
}