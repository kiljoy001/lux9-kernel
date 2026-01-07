/* test_prefixes.c - verification of prefix opcodes */
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

void test_constrained() {
  printf("Testing constrained...\n");
  // constrained. <token> callvirt <token>
  uint8_t code[] = {0xFE,        0x16, 0x01, 0x00, 0x00, 0x00, // constrained.
                    IL_CALLVIRT, 0x02, 0x00, 0x00, 0x00,       // callvirt
                    IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("constrained_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted constrained prefix");
  // [0] constrained, [1] callvirt
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_PREFIX_CONSTRAINED,
        "Opcode is FRUITY_PREFIX_CONSTRAINED");
  check(instr->next->opcode == FRUITY_CALL, "Next is CALL");
}

void test_readonly() {
  printf("Testing readonly...\n");
  // readonly. ldelema
  uint8_t code[] = {0xFE, 0x1E, IL_LDELEMA, 0x01, 0x00, 0x00, 0x00, IL_RET};
  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("readonly_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted readonly prefix");
  fruity_instruction_t *instr = func->blocks_head->instructions_head;
  check(instr->opcode == FRUITY_PREFIX_READONLY,
        "Opcode is FRUITY_PREFIX_READONLY");
  check(instr->next->opcode == FRUITY_LDELEMA, "Next is LDELEMA");
}

int main() {
  test_constrained();
  test_readonly();
  printf("All prefix tests passed.\n");
  return 0;
}
