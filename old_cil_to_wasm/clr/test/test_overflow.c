/* test_overflow.c - Unit test for Fruity->QBE overflow checks */

#define USERSPACE_TEST 1
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock types and headers usually needed by fruity_to_qbe.c */
#include "../fruity/fruity_ir.h"
#include "../fruity/qbe_buffer.h"

/* Implement QBEBuffer mocks */
int qbe_buffer_init(QBEBuffer *buf) {
  buf->capacity = 1024;
  buf->len = 0;
  buf->data = malloc(buf->capacity);
  buf->data[0] = 0;
  return 0;
}

void qbe_buffer_free(QBEBuffer *buf) { free(buf->data); }

int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  // Ensure capacity
  if (buf->len + 128 > buf->capacity) {
    buf->capacity *= 2;
    buf->data = realloc(buf->data, buf->capacity);
  }

  int n = vsnprintf(buf->data + buf->len, buf->capacity - buf->len, fmt, args);
  if (n > 0) {
    buf->len += n;
  }

  va_end(args);
  return n;
}

char *qbe_buffer_data(QBEBuffer *buf) { return buf->data; }

usize qbe_buffer_len(QBEBuffer *buf) { return buf->len; }

/* Mock clr_get_type_size */
unsigned long clr_get_type_size(uint32_t token) {
  return 4; // Default to 4 bytes for testing
}

/* Mock symbols for fruity_to_qbe.c */
void uartputs(char *s, int n) {
  // printf("UART: %.*s", n, s);
}

/* Include the file under test directly to access static functions if needed,
 * or just to compile it in this unit (it has USERSPACE_TEST guards) */
#include "../fruity/fruity_to_qbe.c"

/* Helper to create dummy instruction */
fruity_instruction_t *create_instr(fruity_opcode_t op) {
  fruity_instruction_t *instr = calloc(1, sizeof(fruity_instruction_t));
  instr->opcode = op;
  return instr;
}

void check_contains(const char *text, const char *pattern, const char *desc) {
  if (strstr(text, pattern)) {
    printf("[PASS] %s\n", desc);
  } else {
    printf("[FAIL] %s: Pattern not found: '%s'\n", desc, pattern);
    // printf("DEBUG Output:\n%s\n", text);
    exit(1);
  }
}

void test_add_ovf() {
  printf("Testing ADD_OVF...\n");

  fruity_module_t mod = {0};
  mod.name = "TestMod";

  fruity_function_t func = {0};
  func.name = "TestAddOvf";
  func.arg_count = 0;
  func.local_count = 2;

  fruity_basic_block_t bb = {0};
  bb.block_id = 1;

  // push 10, push 20, add.ovf
  fruity_instruction_t *i1 = create_instr(FRUITY_LDC_I4);
  i1->operand.type = FRUITY_OP_IMM_I64;
  i1->operand.value.i64 = 10;

  fruity_instruction_t *i2 = create_instr(FRUITY_LDC_I4);
  i2->operand.type = FRUITY_OP_IMM_I64;
  i2->operand.value.i64 = 20;

  fruity_instruction_t *i3 = create_instr(FRUITY_ADD_OVF);

  i1->next = i2;
  i2->next = i3;

  bb.instructions_head = i1;

  func.blocks_head = &bb;
  mod.functions_head = &func;
  mod.functions_tail = &func;
  mod.function_count = 1;

  // Pass 1: Size query
  unsigned long size = 0;
  int res = fruity_to_qbe(&mod, 0, &size, NULL, 0);
  if (res < 0) {
    printf("[FAIL] Pass 1 returned %d\n", res);
    exit(1);
  }
  printf("[INFO] Pass 1 size: %lu\n", size);

  // Pass 2: Generate
  char *out_buf = malloc(size);
  res = fruity_to_qbe(&mod, (unsigned long)out_buf, &size, NULL, 0);
  if (res < 0) {
    printf("[FAIL] Pass 2 returned %d\n", res);
    exit(1);
  }

  // Verify content
  // pattern: xor ... xor ... and ... csltw ... jnz ... failfast
  check_contains(out_buf, "call $System_Environment_FailFast(l 0)",
                 "Contains FailFast call");
  check_contains(out_buf, "csltw", "Contains csltw check");

  free(out_buf);
  free(i1);
  free(i2);
  free(i3);
}

int main() {
  test_add_ovf();
  printf("All tests passed.\n");
  return 0;
}
