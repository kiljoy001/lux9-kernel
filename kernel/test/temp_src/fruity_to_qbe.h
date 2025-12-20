/* fruity_to_qbe.h - Fruity IR to QBE IL Translator
 *
 * Converts in-memory Fruity IR structs to textual QBE IL format.
 * Writes directly to exchange pages via zero-copy I/O.
 *
 * Architecture:
 *   Input: fruity_module_t (in-memory IR)
 *   Output: QBE IL text written to exchange page
 *   Process: Text generation via exchange_fprintf() - just pointer arithmetic
 *
 * API Design:
 *   - Single entry point: fruity_to_qbe()
 *   - Returns 0 on success, -1 on error
 *   - Error messages written to optional error buffer
 */

#ifndef FRUITY_TO_QBE_H
#define FRUITY_TO_QBE_H

#include <stddef.h>
#include "fruity_ir.h"

/*
 * Translate Fruity IR module to QBE IL text
 *
 * Arguments:
 *   module: Fruity IR module to translate
 *   out_handle: Exchange page handle for output (physical address)
 *   errorbuf: Optional buffer for error messages (NULL if not needed)
 *   errorbuf_size: Size of error buffer
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Output Format:
 *   QBE IL text with AMD64 SysV ABI calling convention
 *   Includes runtime ABI declarations ($lux_alloc, etc.)
 */
int fruity_to_qbe(fruity_module_t *module,
                  uintptr out_handle,
                  char *errorbuf,
                  size_t errorbuf_size);

#endif /* FRUITY_TO_QBE_H */
