#ifndef CIL_TO_WASM_H
#define CIL_TO_WASM_H

#include "../il_parser.h"
#include "wasm_buffer.h"

/* Compile a single CIL method body to WASM bytecode */
int cil_to_wasm_compile_method(il_method_t *method, wasm_buffer_t *buf);

/* Emit local variable declarations for a function */
int cil_to_wasm_emit_locals(wasm_buffer_t *buf, u32int local_count);

/* Build a complete WASM module from multiple methods */
int cil_to_wasm_build_module(il_method_t **methods, u32int method_count,
                             const char *entry_name, void **out_bytes,
                             u32int *out_len);

#endif
