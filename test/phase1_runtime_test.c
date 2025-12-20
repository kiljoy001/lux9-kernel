/*
 * Runtime alignment verification test
 * This test validates that our Phase 1 alignment improvements work correctly
 */

#include "../kernel/include/u.h"
#include "../kernel/include/mem.h"
#include "../kernel/include/dat.h"

/* Forward declarations for test functions */
extern void* bootstrap_alloc(ulong size);
extern void* bootstrap_alloc_aligned(ulong size, ulong alignment);

/* Test results tracking */
static int tests_passed = 0;
static int tests_total = 0;

/* Simple print function for test output */
void test_print(char *msg) {
    // In kernel context, this would use print()
    // For now, we'll simulate output
}

/* Test macro */
#define RUN_TEST(name, func) do { \
    tests_total++; \
    if (func()) { \
        tests_passed++; \
        test_print("PASS: " #name "\n"); \
    } else { \
        test_print("FAIL: " #name "\n"); \
    } \
} while(0)

/* Test 1: Verify bootstrap_alloc provides cache-line alignment by default */
int test_bootstrap_default_alignment(void) {
    void *ptr = bootstrap_alloc(32);
    return (ptr != nil) && (((uintptr)ptr) % 64 == 0);
}

/* Test 2: Verify bootstrap_alloc_aligned with 16-byte alignment */
int test_bootstrap_16byte_alignment(void) {
    void *ptr = bootstrap_alloc_aligned(64, 16);
    return (ptr != nil) && (((uintptr)ptr) % 16 == 0);
}

/* Test 3: Verify bootstrap_alloc_aligned with 32-byte alignment */
int test_bootstrap_32byte_alignment(void) {
    void *ptr = bootstrap_alloc_aligned(128, 32);
    return (ptr != nil) && (((uintptr)ptr) % 32 == 0);
}

/* Test 4: Verify bootstrap_alloc_aligned with 64-byte alignment */
int test_bootstrap_64byte_alignment(void) {
    void *ptr = bootstrap_alloc_aligned(256, 64);
    return (ptr != nil) && (((uintptr)ptr) % 64 == 0);
}

/* Test 5: Verify structure sizes are properly aligned */
int test_structure_size_alignment(void) {
    int all_aligned = 1;
    
    /* Check that critical structures are multiples of their alignment */
    if (sizeof(struct Lock) % 64 != 0) all_aligned = 0;
    if (sizeof(struct FPssestate) % 64 != 0) all_aligned = 0;
    if (sizeof(struct Mach) % 64 != 0) all_aligned = 0;
    
    return all_aligned;
}

/* Test 6: Verify FPssestate SIMD field alignment */
int test_fpssestate_simd_alignment(void) {
    struct FPssestate test_fpsse;
    uchar *xmm_ptr = test_fpsse.xmm;
    
    /* xmm field should be at least 16-byte aligned for SSE */
    return (((uintptr)xmm_ptr) % 16 == 0);
}

/* Test 7: Verify boundary condition handling */
int test_boundary_conditions(void) {
    /* Test invalid alignments return nil */
    void *ptr1 = bootstrap_alloc_aligned(32, 0);      /* Invalid: 0 alignment */
    void *ptr2 = bootstrap_alloc_aligned(32, 3);      /* Invalid: not power of 2 */
    
    /* Test valid alignments work */
    void *ptr3 = bootstrap_alloc_aligned(32, 1);      /* Valid: 1-byte alignment */
    void *ptr4 = bootstrap_alloc_aligned(32, 2);      /* Valid: 2-byte alignment */
    
    return (ptr1 == nil) && (ptr2 == nil) && (ptr3 != nil) && (ptr4 != nil);
}

/* Test 8: Verify type size consistency */
int test_type_size_consistency(void) {
    /* All these should be 8 bytes on 64-bit platform */
    return (sizeof(ulong) == 8) && 
           (sizeof(uintptr) == 8) && 
           (sizeof(usize) == 8) && 
           (sizeof(void*) == 8);
}

/* Main test runner */
void run_phase1_validation_tests(void) {
    test_print("=== Phase 1 Validation Tests ===\n");
    
    RUN_TEST("Bootstrap default alignment", test_bootstrap_default_alignment);
    RUN_TEST("Bootstrap 16-byte alignment", test_bootstrap_16byte_alignment);
    RUN_TEST("Bootstrap 32-byte alignment", test_bootstrap_32byte_alignment);
    RUN_TEST("Bootstrap 64-byte alignment", test_bootstrap_64byte_alignment);
    RUN_TEST("Structure size alignment", test_structure_size_alignment);
    RUN_TEST("FPssestate SIMD alignment", test_fpssestate_simd_alignment);
    RUN_TEST("Boundary conditions", test_boundary_conditions);
    RUN_TEST("Type size consistency", test_type_size_consistency);
    
    test_print("=== Test Results ===\n");
    test_print("Passed: "); 
    // Print number - in real implementation this would use print()
    test_print(" / ");
    test_print("\n");
    
    if (tests_passed == tests_total) {
        test_print("🎉 ALL TESTS PASSED\n");
    } else {
        test_print("❌ SOME TESTS FAILED\n");
    }
}

/* Entry point */
int main(void) {
    run_phase1_validation_tests();
    return 0;
}