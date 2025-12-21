/* cil_fruity_to_wasm.h - CIL -> Fruity IR -> WASM translator */

#ifndef CIL_FRUITY_TO_WASM_H
#define CIL_FRUITY_TO_WASM_H

#include "../il_parser.h"
#include "fruity_to_wasm.h"

/* Compile a single IL method into a WASM module with Fruity IR actions. */
int cil_fruity_to_wasm_compile_method(il_assembly_t *assembly,
                                      il_method_t *method,
                                      fruity_wasm_result_t *result);

#endif
