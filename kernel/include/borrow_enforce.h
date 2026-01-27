/*
 * Borrow Checker Enforcement
 *
 * Runtime enforcement of ownership rules for kernel memory operations.
 * This completes the "Rust safety in C" promise by actually checking
 * ownership during memory accesses, not just tracking it.
 *
 * Integration points:
 *   - xalloc.c: Ownership acquired on allocation, released on free
 *   - page.c: Ownership validated before user page mappings
 *   - fault.c: Ownership validated in page fault handler
 */

#pragma once

#include "borrowchecker.h"
#include "mem.h"

/* Global enforcement toggle - disabled during early boot */
extern int borrow_enforcement_enabled;

/*
 * PADDR macro - convert virtual address to physical
 * Uses HHDM offset for kernel virtual addresses
 */
#ifndef PADDR
extern uintptr saved_limine_hhdm_offset;
#define PADDR(va) ((uintptr)(va) - saved_limine_hhdm_offset)
#endif

/*
 * Enforcement macros for memory operations
 *
 * These check ownership before allowing memory access.
 * If enforcement is disabled or check passes, no-op.
 * If check fails, panic with detailed error.
 */

#define BORROW_ENFORCE_WRITE(addr, size, owner)                                \
  do {                                                                         \
    if (borrow_enforcement_enabled &&                                          \
        !borrow_can_access_range_phys(PADDR(addr), (size), (owner))) {         \
      panic("Borrow: ownership violation WRITE addr=%#p size=%lud owner=%d",   \
            (void *)(addr), (ulong)(size), (int)(owner));                      \
    }                                                                          \
  } while (0)

#define BORROW_ENFORCE_READ(addr, size, owner)                                 \
  do {                                                                         \
    if (borrow_enforcement_enabled &&                                          \
        !borrow_can_access_range_phys(PADDR(addr), (size), (owner))) {         \
      panic("Borrow: ownership violation READ addr=%#p size=%lud owner=%d",    \
            (void *)(addr), (ulong)(size), (int)(owner));                      \
    }                                                                          \
  } while (0)

/*
 * TCB-aware enforcement macros
 *
 * Per the Lux9 architecture: TCB processes (kp == 1) must be exempt
 * from certain enforcement to prevent circular dependencies.
 */
#define BORROW_ENFORCE_WRITE_IF_NOT_TCB(addr, size, owner)                     \
  do {                                                                         \
    extern Proc *up;                                                           \
    if (up == nil || !up->kp) {                                                \
      BORROW_ENFORCE_WRITE(addr, size, owner);                                 \
    }                                                                          \
  } while (0)

#define BORROW_ENFORCE_READ_IF_NOT_TCB(addr, size, owner)                      \
  do {                                                                         \
    extern Proc *up;                                                           \
    if (up == nil || !up->kp) {                                                \
      BORROW_ENFORCE_READ(addr, size, owner);                                  \
    }                                                                          \
  } while (0)

/*
 * Ownership acquisition/release for allocated memory
 */
#define BORROW_ACQUIRE_ALLOC(addr, size)                                       \
  do {                                                                         \
    if (borrow_enforcement_enabled) {                                          \
      enum BorrowError _err =                                                  \
          borrow_acquire_range_phys(PADDR(addr), (size), OWNER_KERNEL);        \
      if (_err != BORROW_OK) {                                                 \
        panic("Borrow: failed to acquire ownership addr=%#p size=%lud "        \
              "err=%d",                                                        \
              (void *)(addr), (ulong)(size), (int)_err);                       \
      }                                                                        \
    }                                                                          \
  } while (0)

#define BORROW_RELEASE_ALLOC(addr)                                             \
  do {                                                                         \
    if (borrow_enforcement_enabled) {                                          \
      borrow_release_system((uintptr)(addr), OWNER_KERNEL);                    \
    }                                                                          \
  } while (0)

/*
 * Function prototypes for checked memory operations
 */
void *borrow_checked_memmove(void *dst, const void *src, usize n);
void *borrow_checked_memcpy(void *dst, const void *src, usize n);
void *borrow_checked_memset(void *dst, int c, usize n);

/*
 * Enable enforcement after boot completes
 */
void borrow_enforcement_enable(void);

/*
 * Query enforcement state
 */
int borrow_enforcement_is_enabled(void);
