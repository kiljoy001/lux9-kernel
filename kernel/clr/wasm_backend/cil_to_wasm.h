#ifndef CIL_TO_WASM_H
#define CIL_TO_WASM_H

#include "../il_parser.h"
#include "wasm_buffer.h"

/* Compile a single CIL method body to WASM bytecode */
int cil_to_wasm_compile_method(il_method_t *method, il_assembly_t *assembly,
                               wasm_buffer_t *buf);

/* Emit local variable declarations for a function */
int cil_to_wasm_emit_locals(wasm_buffer_t *buf, il_method_t *method);

/* Build a complete WASM module from multiple methods */
int cil_to_wasm_build_module(il_assembly_t *assembly, il_method_t **methods,
                             u32int method_count, const char *entry_name,
                             void **out_bytes, u32int *out_len);

u8int get_wasm_return_type(il_assembly_t *assembly, il_method_t *method);
u32int get_wasm_arg_count(il_assembly_t *assembly, il_method_t *method);
u32int cil_get_local_count(il_assembly_t *assembly, il_method_t *method);

/* Get WASM func_idx for a MethodDef row (1-indexed) */
u32int get_wasm_func_idx_for_row(u32int row);

#endif
