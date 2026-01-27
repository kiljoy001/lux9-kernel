/*
 * pebble_kernel.h - Lightweight Pebble Allocation for Kernel
 *
 * Kernel-side Pebble allocation with WHITE/BLACK semantics and MSGORD tracking.
 * Skips Blind Ledger overhead for performance while maintaining color
 * consistency.
 */

#ifndef _PEBBLE_KERNEL_H_
#define _PEBBLE_KERNEL_H_

/* Kernel allocation tracking structure (opaque to users) */
typedef struct PebbleKernelAlloc PebbleKernelAlloc;

/*
 * Three-phase allocation API
 */

/* Reserve memory from COLORLESS bank (→ WHITE) */
PebbleKernelAlloc *pebble_kernel_reserve(ulong size);

/* Activate reserved memory (WHITE → BLACK) */
void pebble_kernel_activate(PebbleKernelAlloc *alloc);

/* Free allocation back to COLORLESS bank (BLACK → COLORLESS) */
void pebble_kernel_free(PebbleKernelAlloc *alloc);

/*
 * Convenience API
 */

/* Single-call: reserve + activate */
void *pebble_kernel_alloc(ulong size);
/* Free by pointer lookup */
void pebble_kernel_free_ptr(void *ptr);

/*
 * Statistics
 */
void pebble_kernel_stats(uvlong *reserves, uvlong *activates, uvlong *frees,
                         uvlong *white, uvlong *black);

/*
 * Initialization
 */
void pebble_kernel_init(void);

#endif /* _PEBBLE_KERNEL_H_ */
