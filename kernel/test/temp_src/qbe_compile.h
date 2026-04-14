/* qbe_compile.h - QBE IL → Native Code Compiler Interface */

#ifndef QBE_COMPILE_H
#define QBE_COMPILE_H

#ifndef _U_H_
#include <u.h>
#endif

/*
 * Compile QBE IL to native assembly code
 *
 * Arguments:
 *   qbe_page: Physical address of page containing QBE IL text
 *   asm_page: Physical address of page to write assembly output
 *   errorbuf: Optional buffer for error messages
 *   errorbuf_size: Size of error buffer
 *
 * Returns:
 *   0 on success, -1 on error
 */
int qbe_compile_page(uintptr qbe_page, uintptr asm_page, char *errorbuf,
                     usize errorbuf_size);

#endif /* QBE_COMPILE_H */
