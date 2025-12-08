/* Unit test for xalloc bootstrap allocator alignment improvements */
#include "../kernel/include/u.h"
#include "../kernel/include/mem.h"
#include "../kernel/include/dat.h"
#include "../kernel/include/fns.h"

void test_bootstrap_allocator(void) {
	/* Test basic 8-byte alignment */
	void *ptr1 = bootstrap_alloc(16);
	if (((uintptr)ptr1) % 8 != 0) {
		print("FAIL: bootstrap_alloc should provide 8-byte alignment\n");
		return;
	}
	
	/* Test 16-byte alignment requirement */
	void *ptr2 = bootstrap_alloc_aligned(32, 16);
	if (((uintptr)ptr2) % 16 != 0) {
		print("FAIL: bootstrap_alloc_aligned should provide 16-byte alignment\n");
		return;
	}
	
	/* Test 64-byte alignment (cache line) */
	void *ptr3 = bootstrap_alloc_aligned(128, 64);
	if (((uintptr)ptr3) % 64 != 0) {
		print("FAIL: bootstrap_alloc_aligned should provide 64-byte alignment\n");
		return;
	}
	
	/* Test FPsave structure allocation */
	FPsave *fpsave = (FPsave*)bootstrap_alloc_aligned(sizeof(FPsave), 64);
	if (((uintptr)fpsave) % 64 != 0) {
		print("FAIL: FPsave should be 64-byte aligned\n");
		return;
	}
	
	/* Validate FPssestate.xmm field alignment */
	if (((uintptr)&fpsave->avx_state.sse_state.xmm[0]) % 16 != 0) {
		print("FAIL: FPssestate.xmm should be 16-byte aligned\n");
		return;
	}
	
	print("PASS: All bootstrap allocator alignment tests\n");
}

int main(void) {
	test_bootstrap_allocator();
	return 0;
}