/* cil_fruity_to_wasm.c - CIL -> Fruity IR -> WASM translator */

#include "../il_to_fruity.h"
#include "fruity_to_wasm.h"
#include <string.h>

int cil_fruity_to_wasm_compile_method(il_assembly_t *assembly,
                                      il_method_t *method,
                                      fruity_wasm_result_t *result) {
  if (assembly == nil || method == nil || result == nil)
    return -1;

  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  if (func == nil)
    return -1;

  fruity_module_t temp_mod;
  memset(&temp_mod, 0, sizeof(temp_mod));
  temp_mod.functions_head = func;
  temp_mod.functions_tail = func;
  temp_mod.function_count = 1;

  return fruity_compile_to_wasm(&temp_mod, result);
}
