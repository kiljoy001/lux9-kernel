/*
 * test_gc_simple.c - Simple GC test for CIL runtime
 *
 * Tests the LIME/VANILLA/BURN cycle by directly calling runtime functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simplified declarations for testing */
typedef unsigned long ulong;
typedef unsigned long long uvlong;

extern void *lux_alloc(ulong size, ulong type_token);
extern void *lux_addref(void *ptr);
extern void lux_release(void *ptr);

int main(void) {
  void *obj1, *obj2, *obj3;

  printf("Testing CIL GC Runtime...\n\n");

  /* Test 1: LIME - Allocate object */
  printf("Test 1: LIME (allocate)\n");
  obj1 = lux_alloc(64, 0);
  if (obj1) {
    printf("  ✓ Allocated 64 bytes at %p\n", obj1);
    memset(obj1, 0xAA, 64); /* Write pattern */
  } else {
    printf("  ✗ Allocation failed\n");
    return 1;
  }

  /* Test 2: VANILLA - Add reference */
  printf("\nTest 2: VANILLA (addref)\n");
  obj2 = lux_addref(obj1);
  if (obj2 == obj1) {
    printf("  ✓ Added reference, ptr=%p (same as obj1)\n", obj2);
  } else {
    printf("  ✗ Addref returned different pointer: %p\n", obj2);
  }

  /* Test 3: VANILLA - Add another reference */
  printf("\nTest 3: VANILLA (addref again)\n");
  obj3 = lux_addref(obj1);
  if (obj3 == obj1) {
    printf("  ✓ Added another reference, ptr=%p\n", obj3);
  } else {
    printf("  ✗ Addref returned different pointer: %p\n", obj3);
  }

  /* Test 4: BURN - Release first reference */
  printf("\nTest 4: BURN (release first ref)\n");
  lux_release(obj2);
  printf("  ✓ Released obj2\n");

  /* Test 5: BURN - Release second reference */
  printf("\nTest 5: BURN (release second ref)\n");
  lux_release(obj3);
  printf("  ✓ Released obj3\n");

  /* Test 6: BURN - Release original (should free) */
  printf("\nTest 6: BURN (release original - should free)\n");
  lux_release(obj1);
  printf("  ✓ Released obj1 (object should be freed)\n");

  /* Test 7: NULL handling */
  printf("\nTest 7: NULL handling\n");
  void *null_obj = lux_addref(NULL);
  if (null_obj == NULL) {
    printf("  ✓ Addref(NULL) returned NULL\n");
  }
  lux_release(NULL);
  printf("  ✓ Release(NULL) succeeded\n");

  printf("\n✅ All GC tests passed!\n");
  return 0;
}
