/* test_opcodes_missing.c - verification of missing opcodes */
#include "fruity/fruity_ir.h"
#include "il_parser.h"
#include "il_to_fruity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock helpers */
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

static void test_unsigned_math() {
  printf("Testing Unsigned Math...\n");
  // div.un (0x5C), rem.un (0x5E), shr.un (0x64)
  // IL: ldarg.0, ldarg.1, div.un, ret
  uint8_t code_div[] = {IL_LDARG_0, IL_LDARG_1, 0x5C, IL_RET};

  il_assembly_t asm_dummy = {0};
  il_method_t *m =
      create_mock_method("unsigned_div", code_div, sizeof(code_div));

  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted div.un method");
  check(func->blocks_head->instructions_head->next->next->opcode ==
            FRUITY_DIV_UN,
        "Found FRUITY_DIV_UN");

  // shr.un (0x64)
  uint8_t code_shr[] = {IL_LDARG_0, IL_LDARG_1, 0x64, IL_RET};
  m = create_mock_method("unsigned_shr", code_shr, sizeof(code_shr));
  func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted shr.un method");
  check(func->blocks_head->instructions_head->next->next->opcode ==
            FRUITY_SHR_UN,
        "Found FRUITY_SHR_UN");
}

int main() {
  test_unsigned_math();
  printf("All missing opcode tests passed.\n");
  return 0;
}
