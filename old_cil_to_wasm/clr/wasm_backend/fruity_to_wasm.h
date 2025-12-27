/* fruity_to_wasm.h - Fruity IR to WASM Binary Converter
 *
 * Converts a Fruity IR module into a standard WebAssembly binary module.
 * Implements the "Loop-Switch" pattern for control flow to handle unstructured
 * goto.
 */

#ifndef FRUITY_TO_WASM_H
#define FRUITY_TO_WASM_H

#ifdef USERSPACE_TEST
/* Types defined in test harness */
#else
#include "../../include/portlib.h"
#include "../../include/u.h"
#endif

#include "../fruity/fruity_ir.h"

/* result structure */
typedef struct {
  u8int *wasm_binary;
  ulong wasm_size;
  char error_msg[256];
  int success; /* 0 on success, <0 on error */
} fruity_wasm_result_t;

/* Main compilation entry point */
int fruity_compile_to_wasm(fruity_module_t *module,
                           fruity_wasm_result_t *result);

/* Cleanup */
void fruity_wasm_result_free(fruity_wasm_result_t *result);

#endif
