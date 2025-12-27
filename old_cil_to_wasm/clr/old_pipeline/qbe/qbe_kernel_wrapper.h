/* qbe_kernel_wrapper.h - High-level QBE compilation API for kernel
 *
 * Provides a simple interface for kernel code to invoke the QBE compiler
 * using exchange pages for zero-copy I/O.
 */

#ifndef QBE_KERNEL_WRAPPER_H
#define QBE_KERNEL_WRAPPER_H

#include <stddef.h>

/* Exchange handle type */
typedef unsigned long uintptr;

/*
 * Compile QBE IL from input exchange page to output exchange page
 *
 * Parameters:
 *   input  - Exchange handle containing QBE IL text
 *   output - Exchange handle for compiled machine code output
 *   errorbuf - Buffer for error messages (may be NULL)
 *   errorbuf_size - Size of error buffer
 *
 * Returns:
 *   0 on success
 *   -1 on error (error message written to errorbuf if provided)
 */
int qbe_compile_page(uintptr input, uintptr output,
                     char *errorbuf, size_t errorbuf_size);

#endif /* QBE_KERNEL_WRAPPER_H */
