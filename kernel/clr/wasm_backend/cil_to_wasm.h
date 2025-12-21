#ifndef CIL_TO_WASM_H
#define CIL_TO_WASM_H

#include "../il_parser.h"
#include "wasm_buffer.h"

int cil_to_wasm_compile_method(il_method_t *method, wasm_buffer_t *buf);

#endif
