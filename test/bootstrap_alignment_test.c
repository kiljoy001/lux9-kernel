/*
 * Bootstrap allocator alignment test
 * Validates that bootstrap_alloc provides proper alignment guarantees
 */

#include "../kernel/include/u.h"
#include "../kernel/include/mem.h"
#include "../kernel/include/dat.h"

/* Mock the bootstrap pool for testing */
static uchar test_bootstrap_pool[8192];
static ulong test_bootstrap_offset = 0;

/* Test versions of bootstrap functions */
void*
test_bootstrap_alloc(ulong size)
{
	return bootstrap_alloc_aligned(size, 64);  /* Default to cache-line alignment */
}

void*
test_bootstrap_alloc_aligned(ulong size, ulong alignment)
{
	ulong aligned_size;
	ulong aligned_offset;
	
	/* Validate alignment - must be power of 2 and reasonable */
	if (alignment == 0 || (alignment & (alignment - 1)) != 0 || alignment > 1024)
		return nil;
	
	/* Align the size to the requested boundary */
	aligned_size = (size + alignment - 1) & ~(alignment - 1);
	
	/* Align the offset to the requested boundary */
	aligned_offset = (test_bootstrap_offset + alignment - 1) & ~(alignment - 1);
	
	/* Check if we have enough space */
	if (aligned_offset + aligned_size > sizeof(test_bootstrap_pool)) {
		return nil;
	}
	
	/* Return the allocated space */
	void *ptr = &test_bootstrap_pool[aligned_offset];
	test_bootstrap_offset = aligned_offset + aligned_size;
	return ptr;
}

/* Reset test state */
void
reset_test_bootstrap(void)
{
	test_bootstrap_offset = 0;
}

/* Test basic 8-byte alignment */
int
test_8byte_alignment(void)
{
	reset_test_bootstrap();
	void *ptr1 = test_bootstrap_alloc_aligned(16, 8);
	void *ptr2 = test_bootstrap_alloc_aligned(32, 8);
	
	if (((uintptr)ptr1) % 8 != 0) {
		return 0;  /* FAIL */
	}
	
	if (((uintptr)ptr2) % 8 != 0) {
		return 0;  /* FAIL */
	}
	
	return 1;  /* PASS */
}

/* Test 16-byte alignment */
int
test_16byte_alignment(void)
{
	reset_test_bootstrap();
	void *ptr1 = test_bootstrap_alloc_aligned(32, 16);
	void *ptr2 = test_bootstrap_alloc_aligned(64, 16);
	
	if (((uintptr)ptr1) % 16 != 0) {
		return 0;  /* FAIL */
	}
	
	if (((uintptr)ptr2) % 16 != 0) {
		return 0;  /* FAIL */
	}
	
	return 1;  /* PASS */
}

/* Test 64-byte (cache-line) alignment */
int
test_64byte_alignment(void)
{
	reset_test_bootstrap();
	void *ptr1 = test_bootstrap_alloc_aligned(128, 64);
	void *ptr2 = test_bootstrap_alloc_aligned(256, 64);
	
	if (((uintptr)ptr1) % 64 != 0) {
		return 0;  /* FAIL */
	}
	
	if (((uintptr)ptr2) % 64 != 0) {
		return 0;  /* FAIL */
	}
	
	return 1;  /* PASS */
}

/* Test FPsave structure alignment */
int
test_fpsave_alignment(void)
{
	reset_test_bootstrap();
	FPsave *fpsave = (FPsave*)test_bootstrap_alloc_aligned(sizeof(FPsave), 64);
	
	if (((uintptr)fpsave) % 64 != 0) {
		return 0;  /* FAIL */
	}
	
	/* Validate FPssestate.xmm field alignment */
	if (((uintptr)&fpsave->avx_state.sse_state.xmm[0]) % 16 != 0) {
		return 0;  /* FAIL */
	}
	
	return 1;  /* PASS */
}

/* Test boundary conditions */
int
test_boundary_conditions(void)
{
	reset_test_bootstrap();
	
	/* Test invalid alignments */
	void *ptr1 = test_bootstrap_alloc_aligned(32, 0);      /* Should fail */
	void *ptr2 = test_bootstrap_alloc_aligned(32, 3);      /* Should fail (not power of 2) */
	void *ptr3 = test_bootstrap_alloc_aligned(32, 2048);   /* Should fail (too large) */
	
	if (ptr1 != nil || ptr2 != nil || ptr3 != nil) {
		return 0;  /* FAIL */
	}
	
	/* Test valid alignments */
	void *ptr4 = test_bootstrap_alloc_aligned(32, 1);      /* Should work (1-byte alignment) */
	void *ptr5 = test_bootstrap_alloc_aligned(32, 2);      /* Should work (2-byte alignment) */
	void *ptr6 = test_bootstrap_alloc_aligned(32, 4);      /* Should work (4-byte alignment) */
	
	if (ptr4 == nil || ptr5 == nil || ptr6 == nil) {
		return 0;  /* FAIL */
	}
	
	return 1;  /* PASS */
}

/* Test space exhaustion */
int
test_space_exhaustion(void)
{
	reset_test_bootstrap();
	
	/* Fill most of the pool */
	void *ptr1 = test_bootstrap_alloc_aligned(8000, 64);
	if (ptr1 == nil) {
		return 0;  /* FAIL */
	}
	
	/* Try to allocate more than remaining space */
	void *ptr2 = test_bootstrap_alloc_aligned(1000, 64);
	if (ptr2 != nil) {
		return 0;  /* FAIL - should have failed */
	}
	
	return 1;  /* PASS */
}

/* Run all tests */
int
run_bootstrap_alignment_tests(void)
{
	int passed = 0;
	int total = 0;
	
	total++; if (test_8byte_alignment()) passed++; else print("FAIL: 8-byte alignment test\n");
	total++; if (test_16byte_alignment()) passed++; else print("FAIL: 16-byte alignment test\n");
	total++; if (test_64byte_alignment()) passed++; else print("FAIL: 64-byte alignment test\n");
	total++; if (test_fpsave_alignment()) passed++; else print("FAIL: FPsave alignment test\n");
	total++; if (test_boundary_conditions()) passed++; else print("FAIL: boundary conditions test\n");
	total++; if (test_space_exhaustion()) passed++; else print("FAIL: space exhaustion test\n");
	
	print("Bootstrap Allocator Alignment Tests: %d/%d passed\n", passed, total);
	
	return (passed == total) ? 1 : 0;
}