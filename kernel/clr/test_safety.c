/* test_safety.c - verification of safety check opcodes */
#include "fruity/fruity_ir.h"
#include "il_parser.h"
#include "il_to_fruity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void test_ckfinite() {
  printf("Testing ckfinite...\n");
  // ldc.r4 1.0; ckfinite
  uint8_t code[] = {IL_LDC_R4,   0x00,  0x00, 0x80, 0x3f, // 1.0
                    IL_CKFINITE, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("ckfinite_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted ckfinite");
  // [0] ldc.r4, [1] ckfinite
  fruity_instruction_t *instr = func->blocks_head->instructions_head->next;
  check(instr->opcode == FRUITY_CKFINITE, "Opcode is FRUITY_CKFINITE");
}

void test_add_ovf() {
  printf("Testing add.ovf...\n");
  uint8_t code[] = {IL_LDC_I4_1, IL_LDC_I4_1, IL_ADD_OVF, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("add_ovf_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted add.ovf");
  // [0] ldc, [1] ldc, [2] add.ovf
  fruity_instruction_t *instr =
      func->blocks_head->instructions_head->next->next;
  check(instr->opcode == FRUITY_ADD_OVF, "Opcode is FRUITY_ADD_OVF");
}

void test_mul_ovf() {
  printf("Testing mul.ovf...\n");
  uint8_t code[] = {IL_LDC_I4_1, IL_LDC_I4_1, IL_MUL_OVF, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("mul_ovf_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted mul.ovf");
  fruity_instruction_t *instr =
      func->blocks_head->instructions_head->next->next;
  check(instr->opcode == FRUITY_MUL_OVF, "Opcode is FRUITY_MUL_OVF");
}

void test_mul_ovf_un() {
  printf("Testing mul.ovf.un...\n");
  uint8_t code[] = {IL_LDC_I4_1, IL_LDC_I4_1, IL_MUL_OVF_UN, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("mul_ovf_un_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted mul.ovf.un");
  fruity_instruction_t *instr =
      func->blocks_head->instructions_head->next->next;
  check(instr->opcode == FRUITY_MUL_OVF_UN, "Opcode is FRUITY_MUL_OVF_UN");
}

int main() {
  test_ckfinite();
  test_add_ovf();
  test_mul_ovf();
  test_mul_ovf_un();
  printf("All safety check tests passed.\n");
  return 0;
}
