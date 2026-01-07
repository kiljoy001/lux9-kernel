#ifndef USERSPACE_TEST
#define USERSPACE_TEST
#endif

#include "../fruity/fruity_ir.h" /* For struct definitions */
#include "../fruity/fruity_opcodes.h"
#include "fruity_sljit.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock xalloc/xfree for testing */
void *xalloc(size_t size) {
  void *p = calloc(1, size);
  if (!p) {
    fprintf(stderr, "Allocation failed\n");
    exit(1);
  }
  return p;
}

void xfree(void *p) { free(p); }

fruity_instruction_t *create_instr(fruity_opcode_t op) {
  fruity_instruction_t *i = xalloc(sizeof(fruity_instruction_t));
  i->opcode = op;
  i->operand.type = FRUITY_OP_NONE;
  return i;
}

/* Helper to create: return arg0 + 42 */
fruity_function_t *create_test_function() {
  fruity_function_t *func = xalloc(sizeof(fruity_function_t));
  func->local_count = 0;
  func->block_count = 1;
  func->arg_count = 1; /* One argument */

  fruity_basic_block_t *block = xalloc(sizeof(fruity_basic_block_t));
  block->block_id = 0;
  func->blocks_head = block;

  /* 1. LOAD_ARG 0 */
  fruity_instruction_t *i1 = create_instr(FRUITY_LOAD_ARG);
  i1->operand.type = FRUITY_OP_ARG;
  i1->operand.value.index = 0;
  i1->msil_offset = 0;

  /* 2. LDC_I8 42 */
  fruity_instruction_t *i2 = create_instr(FRUITY_LDC_I8);
  i2->operand.type = FRUITY_OP_IMM_I64;
  i2->operand.value.i64 = 42;
  i2->msil_offset = 1;

  /* 3. ADD */
  fruity_instruction_t *i3 = create_instr(FRUITY_ADD);
  i3->msil_offset = 2;

  /* 4. RET */
  fruity_instruction_t *i4 = create_instr(FRUITY_RET);
  i4->msil_offset = 3;

  /* Chain them */
  block->instructions_head = i1;
  i1->next = i2;
  i2->next = i3;
  i3->next = i4;
  i4->next = NULL;

  return func;
}

void test_jit() {
  printf("[TEST] JIT Compilation & Execution\n");
  fruity_function_t *func = create_test_function();
  fruity_jit_ctx_t *ctx = fruity_jit_create();
  fruity_jit_result_t result;

  int ret = fruity_jit_compile(ctx, func, &result);
  if (ret < 0) {
    printf("FAILED: JIT Compile %s\n", result.error_msg);
    exit(1);
  }

  int64_t inputs[] = {100};
  int64_t retval = 0;
  ret = fruity_jit_execute(&result, inputs, 1, &retval);

  if (ret < 0) {
    printf("FAILED: JIT Execute\n");
    exit(1);
  }

  printf("Result: %ld (Expected 142)\n", retval);
  assert(retval == 142);

  fruity_jit_free_code(&result);
  fruity_jit_destroy(ctx);
  printf("PASSED: JIT\n\n");
}

void test_aot() {
  printf("[TEST] AOT Serialization Round-Trip\n");
  fruity_function_t *func = create_test_function();
  fruity_jit_ctx_t *ctx = fruity_jit_create();

  void *buffer = NULL;
  size_t size = 0;

  int ret = fruity_aot_compile(ctx, func, &buffer, &size);
  if (ret < 0) {
    printf("FAILED: AOT Compile\n");
    exit(1);
  }
  printf("Serialized size: %zu bytes\n", size);

  fruity_jit_destroy(ctx);

  fruity_jit_result_t result;
  ret = fruity_aot_load(buffer, size, &result);
  if (ret < 0) {
    printf("FAILED: AOT Load %s\n", result.error_msg);
    exit(1);
  }

  int64_t inputs[] = {100};
  int64_t retval = 0;
  ret = fruity_jit_execute(&result, inputs, 1, &retval);

  if (ret < 0) {
    printf("FAILED: AOT Execute\n");
    exit(1);
  }

  printf("Result: %ld (Expected 142)\n", retval);
  assert(retval == 142);

  fruity_jit_free_code(&result);
  free(buffer);
  printf("PASSED: AOT\n");
}

int main() {
  test_jit();
  test_aot();
  return 0;
}
