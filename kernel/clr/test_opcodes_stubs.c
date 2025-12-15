/* test_opcodes_stubs.c - verification of stubbed opcodes */
#include "fruity/fruity_ir.h"
#include "il_parser.h"
#include "il_to_fruity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock helpers (duplicated for standalone) */
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

static void test_cpobj() {
  printf("Testing cpobj (0x70)...\n");
  // cpobj <type_token>
  // Stack: dest, src -> empty
  // Expect: FRUITY_CPOBJ (not NOP)

  uint8_t code[] = {IL_LDARG_0,                   // dest
                    IL_LDARG_1,                   // src
                    IL_CPOBJ,                     // cpobj
                    0x01,       0x00, 0x00, 0x00, // token (dummy)
                    IL_RET};

  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("cpobj_test", code, sizeof(code));

  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted cpobj method");
  // [0] ldarg, [1] ldarg, [2] cpobj
  fruity_instruction_t *instr =
      func->blocks_head->instructions_head->next->next;

  check(instr->opcode == FRUITY_CPOBJ, "Opcode is FRUITY_CPOBJ");
  check(instr->operand.type == FRUITY_OP_TYPE, "Operand type is TYPE");
  check(instr->operand.value.token == 1, "Token is correct");
}

static void test_ldflda() {
  printf("Testing ldflda (0x7C)...\n");
  // ldflda <field_token>
  // Stack: obj -> addr
  // Expect: FRUITY_LOAD_FIELD_ADDR (or similar equivalent, NOT NOP)
  // NOTE: If your IR maps ldflda to something else (e.g. combined add), adjust
  // check. Based on fruity_opcodes.h, we haven't seen an explicit FRUITY_LDFLDA
  // yet. Userspace code usually maps this to `FRUITY_ADD` with offset if
  // optimization is on, or a dedicated opcode. Let's assume we map it to
  // FRUITY_LOAD_FIELD with specific semantics OR create a new FRUITY_LDFLDA.

  // BUT WAIT: The goal is to fixing the STUB.
  // Current stub is NOP.
  // Let's expect FRUITY_LOAD_FIELD initially, but check `fruity_opcodes.h` if
  // we need to add it.

  uint8_t code[] = {IL_LDARG_0,                   // obj
                    IL_LDFLDA,                    // ldflda
                    0x02,       0x00, 0x00, 0x00, // token
                    IL_RET};

  il_assembly_t asm_dummy = {0};
  il_method_t *m = create_mock_method("ldflda_test", code, sizeof(code));
  il_to_fruity_error_t err;
  fruity_function_t *func = il_to_fruity_convert_method(&asm_dummy, m, &err);

  check(func != NULL, "Converted ldflda method");
  fruity_instruction_t *instr = func->blocks_head->instructions_head->next;

  // We will verify it is NOT NOP.
  check(instr->opcode != FRUITY_NOP, "Opcode is NOT NOP");
  // Ideally it should be related to field address
}

int main() {
  test_cpobj();
  test_ldflda();
  printf("All stub opcode tests passed.\n");
  return 0;
}
