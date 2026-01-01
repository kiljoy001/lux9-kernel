#pragma once

#include "wasm3/wasm3_defs.h"

void *wasm_heap_alloc(size_t size);
void wasm_heap_free(void *ptr);
void *wasm_heap_realloc(void *ptr, size_t new_size, size_t old_size);
int wasm_linear_charge_reserve(uint32_t new_size, uint32_t old_size);
void wasm_linear_charge_rollback(uint32_t old_size);
