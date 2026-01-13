#pragma once

#include "../../include/portlib.h"
#include "../../include/u.h"

/* Standard type compatibility */
typedef usize size_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef u16int uint16_t;
typedef u8int uint8_t;

/* Disable floating point support for kernel (no SSE/FPU usage) */
#define d_m3HasFloat 0

/* Fix const correctness for kernel strcmp */
#define strcmp(s1, s2) strcmp((char *)(s1), (char *)(s2))

#include "wasm3/wasm3_defs.h"

void *wasm_heap_alloc(size_t size);
void wasm_heap_free(void *ptr);
void *wasm_heap_realloc(void *ptr, size_t new_size, size_t old_size);
int wasm_linear_charge_reserve(uint32_t new_size, uint32_t old_size);
void wasm_linear_charge_rollback(uint32_t old_size);
