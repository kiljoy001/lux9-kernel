/* test_wasm_translation.c - Standalone userspace test for Fruity -> WASM
 * translation
 *
 * Compile with:
 *   gcc -DUSERSPACE_TEST -I../fruity test_wasm_translation.c wasm_buffer.c
 * fruity_to_wasm.c ../fruity/fruity_ir.c -o test_wasm && ./test_wasm
 */

#define USERSPACE_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* fruity_standalone.h (included via fruity_types.h) provides:
 *   u8int, u32int, s32int, ulong, nil, etc.
 */

/* ===== Stubs for kernel functions ===== */
void *xalloc(size_t size) {
  void *p = malloc(size);
  if (p)
    memset(p, 0, size);
  return p;
}

void *xallocz(size_t size, int zero) {
  void *p = malloc(size);
  if (p && zero)
    memset(p, 0, size);
  return p;
}

void xfree(void *ptr) { free(ptr); }

int print(char *fmt, ...) {
  /* minimal stub */
  return 0;
}

/* ===== Include Fruity Headers ===== */
#include "../fruity/fruity_ir.h"
#include "fruity_to_wasm.h"

/* Helper to dump hex */
void dump_hex(u8int *data, ulong size) {
  printf("WASM Binary (%lu bytes):\n", (unsigned long)size);
  for (ulong i = 0; i < size; i++) {
    printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0)
      printf("\n");
  }
  if (size % 16 != 0)
    printf("\n");
}

int main() {
  printf("=== Testing Fruity -> WASM Translation ===\n");

  /* 1. Create Module */
  fruity_module_t *mod = fruity_module_create("TestModule");
  if (!mod) {
    printf("FAILED: Could not create module\n");
    return 1;
  }

  /* 2. Create Function: simple() { return 42; } */
  fruity_function_t *func = fruity_module_add_function(mod, "simple", 0x1234);
  if (!func) {
    printf("FAILED: Could not create function\n");
    return 1;
  }
  func->arg_count = 0;
  func->local_count = 0;

  /* Block 0 */
  fruity_basic_block_t *b0 = fruity_function_add_block(func);

  /* LDC.I4 42 */
  fruity_instruction_t *i1 = fruity_instruction_create(FRUITY_LDC_I4);
  i1->operand.type = FRUITY_OP_IMM_I32;
  i1->operand.value.i32 = 42;
  fruity_basic_block_add_instruction(b0, i1);

  /* RET */
  fruity_instruction_t *i2 = fruity_instruction_create(FRUITY_RET);
  fruity_basic_block_add_instruction(b0, i2);

  printf("Created Fruity IR: simple() { ldc.i4 42; ret; }\n");

  /* 3. Compile */
  fruity_wasm_result_t res;
  memset(&res, 0, sizeof(res));
  int err = fruity_compile_to_wasm(mod, &res);

  if (err == 0 && res.wasm_binary) {
    printf("SUCCESS! Generated %lu bytes\n", (unsigned long)res.wasm_size);
    dump_hex(res.wasm_binary, res.wasm_size);

    /* Verify magic number */
    if (res.wasm_size >= 8 && res.wasm_binary[0] == 0x00 &&
        res.wasm_binary[1] == 0x61 && res.wasm_binary[2] == 0x73 &&
        res.wasm_binary[3] == 0x6D) {
      printf("✓ WASM magic number correct (\\0asm)\n");
    } else {
      printf("✗ WASM magic number INCORRECT\n");
    }

    if (res.wasm_size >= 8 && res.wasm_binary[4] == 0x01 &&
        res.wasm_binary[5] == 0x00 && res.wasm_binary[6] == 0x00 &&
        res.wasm_binary[7] == 0x00) {
      printf("✓ WASM version correct (1)\n");
    } else {
      printf("✗ WASM version INCORRECT\n");
    }

    fruity_wasm_result_free(&res);
  } else {
    printf("FAILED: Compilation error %d\n", err);
  }

  fruity_module_destroy(mod);
  printf("=== Test Complete ===\n");
  return 0;
}
