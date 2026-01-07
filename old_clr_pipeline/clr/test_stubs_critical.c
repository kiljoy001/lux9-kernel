/* test_stubs_critical.c - verification of critical stubs */
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

void test_jmp() {
  printf("Testing jmp...\n");
  // jmp <token>
  uint8_t code[] = {IL_JMP, 0x01, 0x00, 0x00, 0x00};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("jmp_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted jmp");
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_JMP, "Opcode is FRUITY_JMP");
}

void test_sizeof() {
  printf("Testing sizeof...\n");
  // sizeof <token>
  uint8_t code[] = {0xFE, 0x1C, 0x02, 0x00, 0x00, 0x00, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("sizeof_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted sizeof");
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_SIZEOF, "Opcode is FRUITY_SIZEOF");
}

void test_ldtoken() {
  printf("Testing ldtoken...\n");
  // ldtoken <token>
  uint8_t code[] = {IL_LDTOKEN, 0x03, 0x00, 0x00, 0x00, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("ldtoken_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted ldtoken");
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_LDTOKEN, "Opcode is FRUITY_LDTOKEN");
}

void test_arglist() {
  printf("Testing arglist...\n");
  // arglist
  uint8_t code[] = {0xFE, 0x00, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("arglist_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted arglist");
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_ARGLIST, "Opcode is FRUITY_ARGLIST");
}

int main() {
  test_jmp();
  test_sizeof();
  test_ldtoken();
  test_arglist();
  printf("All critical stub tests passed.\n");
  return 0;
}
