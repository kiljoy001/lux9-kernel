/* test_final.c - verification of break and remaining prefixes */
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

void test_break() {
  printf("Testing break...\n");
  uint8_t code[] = {IL_BREAK, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("break_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);
  check(func != NULL, "Converted break");
  check(func->blocks_head->instructions_head->opcode == FRUITY_BREAK,
        "Opcode is FRUITY_BREAK");
}

void test_tail() {
  printf("Testing tail...\n");
  // tail. call
  uint8_t code[] = {0xFE, 0x14, IL_CALL, 0x01, 0x00, 0x00, 0x00, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("tail_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);
  check(func != NULL, "Converted tail prefix");
  check(func->blocks_head->instructions_head->opcode == FRUITY_PREFIX_TAIL,
        "Opcode is FRUITY_PREFIX_TAIL");
}

void test_volatile() {
  printf("Testing volatile...\n");
  // volatile. ldind.i4
  uint8_t code[] = {0xFE, 0x13, IL_LDIND_I4, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("volatile_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);
  check(func != NULL, "Converted volatile prefix");
  check(func->blocks_head->instructions_head->opcode == FRUITY_PREFIX_VOLATILE,
        "Opcode is FRUITY_PREFIX_VOLATILE");
}

void test_unaligned() {
  printf("Testing unaligned...\n");
  // unaligned. 1 ldind.i4
  uint8_t code[] = {0xFE, 0x12, 0x01, IL_LDIND_I4, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("unaligned_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);
  check(func != NULL, "Converted unaligned prefix");
  check(func->blocks_head->instructions_head->opcode == FRUITY_PREFIX_UNALIGNED,
        "Opcode is FRUITY_PREFIX_UNALIGNED");
}

int main() {
  test_break();
  test_tail();
  test_volatile();
  test_unaligned();
  printf("All final tests passed.\n");
  return 0;
}
