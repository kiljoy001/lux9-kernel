/*
 * acsl_bounds.h - Machine-realistic bounds for ACSL verification
 *
 * Defines concrete, hardware-realistic limits for ACSL specifications.
 * Nothing is infinite in a real computer - all memory and numbers have bounds.
 */

#ifndef _ACSL_BOUNDS_H
#define _ACSL_BOUNDS_H

/*
 * String and Buffer Bounds
 */
#define ACSL_MAXSTR 4096  /* Maximum reasonable string length */
#define ACSL_MAXBUF 8192  /* Maximum buffer size for formatting */
#define ACSL_MAXPATH 1024 /* Maximum path length */
#define ACSL_MAXNAME 256  /* Maximum name length */

/*
 * Format String Bounds
 */
#define ACSL_MAX_FMT_ARGS 32 /* Maximum format arguments */
#define ACSL_MAX_FMT_LEN 512 /* Maximum format string length */

/*
 * Numeric Bounds (matching actual hardware)
 */
#define ACSL_MAX_INT32 2147483647
#define ACSL_MIN_INT32 (-2147483647 - 1)
#define ACSL_MAX_UINT32 4294967295U
#define ACSL_MAX_INT64 9223372036854775807LL
#define ACSL_MIN_INT64 (-9223372036854775807LL - 1)
#define ACSL_MAX_UINT64 18446744073709551615ULL

/*
 * Memory Bounds
 */
#define ACSL_MAX_ALLOC (1ULL << 40) /* 1TB max allocation */
#define ACSL_PAGE_SIZE 4096

#endif /* _ACSL_BOUNDS_H */
