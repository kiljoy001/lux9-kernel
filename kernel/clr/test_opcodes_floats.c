/* test_opcodes_floats.c - verification of float literal opcodes */
#include "fruity/fruity_ir.h"
#include "il_parser.h"
#include "il_to_fruity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock helpers (duplicated for standalone nature) */
static void check(int condition, const char *desc) {
  if (condition)
    printf("[PASS] %s\n", desc);
  else {
    printf("[FAIL] %s\n", desc);
    exit(1);
  }
}

static il_method_t *create_mock_method(const char *name, uint8_t *code,
                                       size_t size) {
  il_method_t *m = calloc(1, sizeof(il_method_t));
  m->name = strdup(name);
  m->il_code = malloc(size);
  memcpy(m->il_code, code, size);
  m->il_code_size = size;
  m->max_stack = 8;
  return m;
}

static void test_float_literals() {
  printf("Testing Float Literals...\n");

  // ldc.r4 (0x22) + 1.23f
  float f_val = 1.23f;
  uint32_t f_bits = *(uint32_t *)&f_val;
  uint8_t code_r4[1 + 4 + 1]; // opcode + 4 bytes + ret
  code_r4[0] = IL_LDC_R4;
  *(uint32_t *)(code_r4 + 1) = f_bits;
  code_r4[5] = IL_RET;

  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("float_r4", code_r4, sizeof(code_r4));

  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted ldc.r4 method");
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_LDC_R4, "Found FRUITY_LDC_R4");
  // Simple verification of value approximate
  check(instr->operand.value.r32 > 1.22f && instr->operand.value.r32 < 1.24f,
        "Value correct (approx 1.23)");

  // ldc.r8 (0x23) + 3.14159
  double d_val = 3.14159;
  uint64_t d_bits = *(uint64_t *)&d_val;
  uint8_t code_r8[1 + 8 + 1]; // opcode + 8 bytes + ret
  code_r8[0] = IL_LDC_R8;
  *(uint64_t *)(code_r8 + 1) = d_bits;
  code_r8[9] = IL_RET;

  m = create_mock_method("float_r8", code_r8, sizeof(code_r8));
  func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted ldc.r8 method");
  instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_LDC_R8, "Found FRUITY_LDC_R8");
  check(instr->operand.value.r64 > 3.14f && instr->operand.value.r64 < 3.15f,
        "Value correct (approx 3.14)");
}

int main() {
  test_float_literals();
  printf("All float opcode tests passed.\n");
  return 0;
}
