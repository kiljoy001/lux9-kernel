#include "u.h"
#include "dat.h"
#include "fns.h"

/*
 * The current kernel image does not link the full swap device implementation.
 * Keep the compatibility surface in a dedicated file so globals.c only carries
 * shared platform state.
 */
Image *swapimage = nil;

void putswap(Page *p) { (void)p; }

int swapcount(uintptr pa) {
  (void)pa;
  return 0;
}

void kickpager(void) { wakeup(&swapalloc.r); }

int swapfull(void) { return 0; }

void dupswap(Page *p) { (void)p; }
