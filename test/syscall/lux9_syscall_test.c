/*
 * Lux9 Unified System Call Test Suite
 * 
 * Comprehensive testing for the unified syscall system that resolves
 * WASM/Pebble conflicts and provides backward compatibility.
 * 
 * Version: 1.0
 * Last Updated: 2026-01-12
 */

#include "u.h"
#include "portdat.h"
#include "lux9_syscall_unified.h"

/* Test result tracking */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int warnings;
} test_results_t;

static test_results_t test_results = {0};

/* Test framework macros */
#define TEST_START(name) \
    do { \
        print("TEST: %s\n", name); \
        test_results.total_tests++; \
    } while(0)

#define TEST_PASS(name) \
    do { \
        print("  ✓ PASS: %s\n", name); \
        test_results.passed_tests++; \
    } while(0)

#define TEST_FAIL(name, reason) \
    do { \
        print("  ✗ FAIL: %s - %s\n", name, reason); \
        test_results.failed_tests++; \
    } while(0)

#define TEST_WARN(name, reason) \
    do { \
        print("  ⚠ WARN: %s - %s\n", name, reason); \
        test_results.warnings++; \
    } while(0)

/*
 * ============================================================================
 * SYSCALL NUMBER VALIDATION TESTS
 * ============================================================================
 */

static void test_syscall_number_definitions(void) {
    TEST_START("Syscall Number Definitions");
    
    /* Test that WASM syscalls are correctly numbered */
    TEST_START("WASM Syscall Numbers");
    if (LUX9_SYS_WASM_COMPILE == (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_WASM | 160)) {
        TEST_PASS("LUX9_SYS_WASM_COMPILE correctly defined as 160");
    } else {
        TEST_FAIL("LUX9_SYS_WASM_COMPILE", "Wrong value");
    }
    
    if (LUX9_SYS_WASM_EXECUTE == (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_WASM | 161)) {
        TEST_PASS("LUX9_SYS_WASM_EXECUTE correctly defined as 161");
    } else {
        TEST_FAIL("LUX9_SYS_WASM_EXECUTE", "Wrong value");
    }
    
    if (LUX9_SYS_WASM_DESTROY == (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_WASM | 162)) {
        TEST_PASS("LUX9_SYS_WASM_DESTROY correctly defined as 162");
    } else {
        TEST_FAIL("LUX9_SYS_WASM_DESTROY", "Wrong value");
    }
    
    /* Test that Plan 9 compatibility syscalls are correctly numbered */
    TEST_START("Plan 9 Compatibility Syscalls");
    if (LUX9_SYS_OPEN == (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 14)) {
        TEST_PASS("LUX9_SYS_OPEN correctly defined");
    } else {
        TEST_FAIL("LUX9_SYS_OPEN", "Wrong value");
    }
    
    if (LUX9_SYS_READ == (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 15)) {
        TEST_PASS("LUX9_SYS_READ correctly defined");
    } else {
        TEST_FAIL("LUX9_SYS_READ", "Wrong value");
    }
    
    if (LUX9_SYS_WRITE == (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 20)) {
        TEST_PASS("LUX9_SYS_WRITE correctly defined");
    } else {
        TEST_FAIL("LUX9_SYS_WRITE", "Wrong value");
    }
}

/*
 * ============================================================================
 * ARCHITECTURE CLASSIFICATION TESTS
 * ============================================================================
 */

static void test_architecture_classification(void) {
    TEST_START("Architecture Classification");
    
    /* Test Plan 9 architecture detection */
    uint test_plan9_syscall = LUX9_SYS_OPEN;
    if (LUX9_IS_PLAN9_SYSCALL(test_plan9_syscall)) {
        TEST_PASS("Plan 9 syscall correctly classified");
    } else {
        TEST_FAIL("Plan 9 syscall classification", "Not detected");
    }
    
    /* Test Lux9 native architecture detection */
    uint test_lux9_syscall = LUX9_SYS_WASM_COMPILE;
    if (LUX9_IS_LUX9_NATIVE(test_lux9_syscall)) {
        TEST_PASS("Lux9 native syscall correctly classified");
    } else {
        TEST_FAIL("Lux9 native syscall classification", "Not detected");
    }
    
    /* Test subsystem identification */
    TEST_START("Subsystem Identification");
    uint wasm_syscall = LUX9_SYS_WASM_COMPILE;
    if (LUX9_SYSCALL_IS_WASM(wasm_syscall)) {
        TEST_PASS("WASM syscall correctly identified");
    } else {
        TEST_FAIL("WASM syscall identification", "Not detected");
    }
    
    uint pebble_syscall = LUX9_SYS_PEBBLE_RED_COPY;
    if (LUX9_SYSCALL_IS_PEBBLE(pebble_syscall)) {
        TEST_PASS("Pebble syscall correctly identified");
    } else {
        TEST_FAIL("Pebble syscall identification", "Not detected");
    }
    
    uint exchange_syscall = LUX9_SYS_EXCHANGE_ALLOC;
    if (LUX9_SYSCALL_IS_EXCHANGE(exchange_syscall)) {
        TEST_PASS("Exchange syscall correctly identified");
    } else {
        TEST_FAIL("Exchange syscall identification", "Not detected");
    }
}

/*
 * ============================================================================
 * BACKWARD COMPATIBILITY TESTS
 * ============================================================================
 */

static void test_backward_compatibility(void) {
    TEST_START("Backward Compatibility");
    
    /* Test that legacy syscall constants still work */
    TEST_START("Legacy WASM Syscall Compatibility");
    if (SYS_WASM_COMPILE == LUX9_SYS_WASM_COMPILE) {
        TEST_PASS("SYS_WASM_COMPILE compatibility macro works");
    } else {
        TEST_FAIL("SYS_WASM_COMPILE compatibility", "Value mismatch");
    }
    
    if (SYS_WASM_EXECUTE == LUX9_SYS_WASM_EXECUTE) {
        TEST_PASS("SYS_WASM_EXECUTE compatibility macro works");
    } else {
        TEST_FAIL("SYS_WASM_EXECUTE compatibility", "Value mismatch");
    }
    
    if (SYS_WASM_DESTROY == LUX9_SYS_WASM_DESTROY) {
        TEST_PASS("SYS_WASM_DESTROY compatibility macro works");
    } else {
        TEST_FAIL("SYS_WASM_DESTROY compatibility", "Value mismatch");
    }
    
    /* Test Plan 9 compatibility */
    TEST_START("Plan 9 Syscall Compatibility");
    if (OPEN == LUX9_SYS_OPEN) {
        TEST_PASS("OPEN compatibility macro works");
    } else {
        TEST_FAIL("OPEN compatibility", "Value mismatch");
    }
    
    if (READ == LUX9_SYS_READ) {
        TEST_PASS("READ compatibility macro works");
    } else {
        TEST_FAIL("READ compatibility", "Value mismatch");
    }
    
    if (WRITE == LUX9_SYS_WRITE) {
        TEST_PASS("WRITE compatibility macro works");
    } else {
        TEST_FAIL("WRITE compatibility", "Value mismatch");
    }
}

/*
 * ============================================================================
 * SYSCALL DISPATCH VALIDATION TESTS
 * ============================================================================
 */

static void test_syscall_dispatch_validation(void) {
    TEST_START("Syscall Dispatch Validation");
    
    /* Test that we can validate syscall numbers */
    int valid_wasm = lux9_syscall_validate(LUX9_SYS_WASM_COMPILE);
    if (valid_wasm) {
        TEST_PASS("WASM syscall number validation works");
    } else {
        TEST_FAIL("WASM syscall validation", "Rejected valid syscall");
    }
    
    int valid_plan9 = lux9_syscall_validate(LUX9_SYS_OPEN);
    if (valid_plan9) {
        TEST_PASS("Plan 9 syscall number validation works");
    } else {
        TEST_FAIL("Plan 9 syscall validation", "Rejected valid syscall");
    }
    
    int invalid_syscall = lux9_syscall_validate(999999);
    if (!invalid_syscall) {
        TEST_PASS("Invalid syscall correctly rejected");
    } else {
        TEST_FAIL("Invalid syscall validation", "Accepted invalid syscall");
    }
}

/*
 * ============================================================================
 * CONFLICT RESOLUTION TESTS
 * ============================================================================
 */

static void test_conflict_resolution(void) {
    TEST_START("WASM/Pebble Conflict Resolution");
    
    /* Test that WASM and Pebble syscalls are in different ranges */
    uint wasm_compile = LUX9_SYS_WASM_COMPILE;
    uint pebble_red = LUX9_SYS_PEBBLE_RED_COPY;
    
    if (wasm_compile != pebble_red) {
        TEST_PASS("WASM and Pebble syscalls use different numbers");
    } else {
        TEST_FAIL("WASM/Pebble conflict", "Same syscall number used");
    }
    
    /* Test that the ranges don't overlap */
    uint wasm_range_start = LUX9_SYS_WASM_COMPILE;
    uint wasm_range_end = LUX9_SYS_WASM_DESTROY;
    uint pebble_range_start = LUX9_SYS_PEBBLE_BLACK_ALLOC;
    uint pebble_range_end = LUX9_SYS_PEBBLE_BLUE_DISCARD;
    
    if (wasm_range_end < pebble_range_start || 
        pebble_range_end < wasm_range_start) {
        TEST_PASS("WASM and Pebble syscall ranges don't overlap");
    } else {
        TEST_FAIL("Range overlap", "WASM and Pebble ranges overlap");
    }
}

/*
 * ============================================================================
 * PERFORMANCE TESTS
 * ============================================================================
 */

static void test_syscall_performance(void) {
    TEST_START("Syscall Performance");
    
    /* Test syscall classification speed */
    int iterations = 10000;
    uint test_syscall = LUX9_SYS_WASM_COMPILE;
    int start_time, end_time;
    
    start_time = getmilliseconds();
    for (int i = 0; i < iterations; i++) {
        volatile int result = LUX9_IS_LUX9_NATIVE(test_syscall);
        (void)result;
    }
    end_time = getmilliseconds();
    
    long elapsed = end_time - start_time;
    if (elapsed < 100) {  // Should complete in under 100ms
        TEST_PASS("Syscall classification performance acceptable");
    } else {
        TEST_WARN("Syscall classification", "Performance may be slow");
    }
    
    /* Test syscall validation speed */
    start_time = getmilliseconds();
    for (int i = 0; i < iterations; i++) {
        volatile int result = lux9_syscall_validate(test_syscall);
        (void)result;
    }
    end_time = getmilliseconds();
    
    elapsed = end_time - start_time;
    if (elapsed < 200) {  // Should complete in under 200ms
        TEST_PASS("Syscall validation performance acceptable");
    } else {
        TEST_WARN("Syscall validation", "Performance may be slow");
    }
}

/*
 * ============================================================================
 * MAIN TEST FUNCTION
 * ============================================================================
 */

static void print_test_results(void) {
    print("\n");
    print("═══════════════════════════════════════════════════════════════\n");
    print("               SYSCALL TEST RESULTS SUMMARY\n");
    print("═══════════════════════════════════════════════════════════════\n");
    print("Total Tests:  %d\n", test_results.total_tests);
    print("Passed:       %d ✓\n", test_results.passed_tests);
    print("Failed:       %d ✗\n", test_results.failed_tests);
    print("Warnings:     %d ⚠\n", test_results.warnings);
    
    if (test_results.failed_tests == 0) {
        print("\n🎉 ALL TESTS PASSED! Unified syscall system is working correctly.\n");
    } else {
        print("\n❌ SOME TESTS FAILED! Please review the issues above.\n");
    }
    
    print("═══════════════════════════════════════════════════════════════\n");
}

/*
 * Public interface - run all syscall tests
 */
void lux9_syscall_test_suite(void) {
    print("Lux9 Unified System Call Test Suite\n");
    print("====================================\n\n");
    
    /* Initialize test results */
    memset(&test_results, 0, sizeof(test_results));
    
    /* Run all test categories */
    test_syscall_number_definitions();
    test_architecture_classification();
    test_backward_compatibility();
    test_syscall_dispatch_validation();
    test_conflict_resolution();
    test_syscall_performance();
    
    /* Print summary */
    print_test_results();
}