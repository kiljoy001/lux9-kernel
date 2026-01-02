/*
 * Borrow Checker Enforcement Implementation
 *
 * Provides runtime enforcement of ownership rules for kernel memory.
 * This module bridges the gap between ownership tracking (bookkeeping)
 * and actual enforcement during memory operations.
 *
 * SMT: Validated by proofs/borrow/borrow_core.v
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "u.h"

#include "borrow_enforce.h"
#include "borrowchecker.h"

/*
 * Global enforcement toggle
 *
 * Disabled during early boot to avoid panics when ownership tracking
 * is incomplete. Enabled after boot_memory_coordination_init() and
 * memory zones are fully established.
 */
int borrow_enforcement_enabled = 0;

/*
 * Enable enforcement after boot completes
 *
 * Called from main() or equivalent after:
 *   1. borrowinit() has initialized the borrow pool
 *   2. boot_memory_coordination_init() has set up zones
 *   3. Memory allocators are fully operational
 */
void borrow_enforcement_enable(void) {
  /*@
    @ ensures borrow_enforcement_enabled == 1;
    @ assigns borrow_enforcement_enabled;
    @*/
  print("borrow_enforce: enabling runtime ownership enforcement\n");
  borrow_enforcement_enabled = 1;
}

/*
 * Query enforcement state
 */
int borrow_enforcement_is_enabled(void) {
  /*@
    @ ensures \result == borrow_enforcement_enabled;
    @ assigns \nothing;
    @*/
  return borrow_enforcement_enabled;
}

/*
 * Checked memory operations
 *
 * These wrappers validate ownership before performing memory operations.
 * They are optional replacements for memmove/memcpy/memset in
 * security-critical code paths.
 */

void *borrow_checked_memmove(void *dst, const void *src, usize n) {
  /*@
    @ requires n == 0 || (\valid((char *)dst + (0..n-1)) &&
    @                    \valid((char *)src + (0..n-1)));
    @ requires !borrow_enforcement_enabled ||
    @          borrow_can_access_range_phys(PADDR(src), n, OWNER_KERNEL);
    @ requires !borrow_enforcement_enabled ||
    @          borrow_can_access_range_phys(PADDR(dst), n, OWNER_KERNEL);
    @ ensures \result == dst;
    @*/
  if (borrow_enforcement_enabled) {
    /* Verify KERNEL can read source */
    if (!borrow_can_access_range_phys(PADDR(src), n, OWNER_KERNEL)) {
      panic("borrow_checked_memmove: ownership violation READ src=%#p n=%lud",
            src, (ulong)n);
    }
    /* Verify KERNEL can write destination */
    if (!borrow_can_access_range_phys(PADDR(dst), n, OWNER_KERNEL)) {
      panic("borrow_checked_memmove: ownership violation WRITE dst=%#p n=%lud",
            dst, (ulong)n);
    }
  }
  return memmove(dst, src, n);
}

void *borrow_checked_memcpy(void *dst, const void *src, usize n) {
  /*@
    @ requires n == 0 || (\valid((char *)dst + (0..n-1)) &&
    @                    \valid((char *)src + (0..n-1)));
    @ requires !borrow_enforcement_enabled ||
    @          borrow_can_access_range_phys(PADDR(src), n, OWNER_KERNEL);
    @ requires !borrow_enforcement_enabled ||
    @          borrow_can_access_range_phys(PADDR(dst), n, OWNER_KERNEL);
    @ ensures \result == dst;
    @*/
  if (borrow_enforcement_enabled) {
    /* Verify KERNEL can read source */
    if (!borrow_can_access_range_phys(PADDR(src), n, OWNER_KERNEL)) {
      panic("borrow_checked_memcpy: ownership violation READ src=%#p n=%lud",
            src, (ulong)n);
    }
    /* Verify KERNEL can write destination */
    if (!borrow_can_access_range_phys(PADDR(dst), n, OWNER_KERNEL)) {
      panic("borrow_checked_memcpy: ownership violation WRITE dst=%#p n=%lud",
            dst, (ulong)n);
    }
  }
  /* Use memmove since 9front kernel doesn't declare memcpy */
  return memmove(dst, src, n);
}

void *borrow_checked_memset(void *dst, int c, usize n) {
  /*@
    @ requires n == 0 || \valid((char *)dst + (0..n-1));
    @ requires !borrow_enforcement_enabled ||
    @          borrow_can_access_range_phys(PADDR(dst), n, OWNER_KERNEL);
    @ ensures \result == dst;
    @*/
  if (borrow_enforcement_enabled) {
    /* Verify KERNEL can write destination */
    if (!borrow_can_access_range_phys(PADDR(dst), n, OWNER_KERNEL)) {
      panic("borrow_checked_memset: ownership violation WRITE dst=%#p n=%lud",
            dst, (ulong)n);
    }
  }
  return memset(dst, c, n);
}
