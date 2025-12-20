/* test_opcodes_phase1.c - Phase 1 IL Opcode Tests
 *
 * Tests: Branches + Arithmetic + Bitwise (~30 opcodes)
 * Pipeline: CIL → Fruity → QBE → ASM
 */

#define USERSPACE_TEST 1
#define _GNU_SOURCE

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Kernel Types Shim */
typedef uint32_t uint32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uintptr_t uintptr;

#include "../clr/fruity/fruity_to_qbe.h"
#include "../clr/fruity/qbe_buffer.h"
#include "../clr/il_parser.h"

/* --- Kernel Mocks (same as test_pipeline.c) --- */
void uartputs(char *s, int len) { fwrite(s, 1, len, stdout); }
void *xalloc(size_t size) { return calloc(1, size); }
void *xallocz(size_t size, int zero_init) { return xalloc(size); }
void xfree(void *ptr) { free(ptr); }
int print(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vprintf(fmt, ap);
  va_end(ap);
  return n;
}
int snprint(char *buf, int len, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, len, fmt, ap);
  va_end(ap);
  return n;
}
int exchange_fprintf(uintptr handle, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  QBEBuffer *qbuf = (QBEBuffer *)handle;
  return qbe_buffer_printf(qbuf, "%s", buf);
}

/* --- CIL Macros --- */
#define IL_NOP 0x00
#define IL_LDARG_0 0x02
#define IL_LDARG_1 0x03
#define IL_LDLOC_0 0x06
#define IL_LDLOC_1 0x07
#define IL_LDLOC_2 0x08
#define IL_LDLOC_3 0x09
#define IL_STLOC_0 0x0A
#define IL_STLOC_1 0x0B
#define IL_LDLOC_S 0x11
#define IL_STLOC_S 0x13
#define IL_LDC_I4_M1 0x15
#define IL_LDC_I4_0 0x16
#define IL_LDC_I4_1 0x17
#define IL_LDC_I4_2 0x18
#define IL_LDC_I4_3 0x19
#define IL_LDC_I4_4 0x1A
#define IL_LDC_I4_5 0x1B
#define IL_LDC_I4_6 0x1C
#define IL_LDC_I4_7 0x1D
#define IL_LDC_I4_8 0x1E
#define IL_LDC_I4_S 0x1F
#define IL_DUP 0x25
#define IL_POP 0x26
#define IL_CALL 0x28
#define IL_RET 0x2A
#define IL_BR_S 0x2B
#define IL_BRFALSE_S 0x2C
#define IL_BRTRUE_S 0x2D
#define IL_BEQ_S 0x2E
#define IL_BGE_S 0x2F
#define IL_BGT_S 0x30
#define IL_BLE_S 0x31
#define IL_BLT_S 0x32
#define IL_BNE_UN_S 0x33
#define IL_BR 0x38
#define IL_BRFALSE 0x39
#define IL_BRTRUE 0x3A
#define IL_BEQ 0x3B
#define IL_BGE 0x3C
#define IL_BGT 0x3D
#define IL_BLE 0x3E
#define IL_BLT 0x3F
#define IL_BNE_UN 0x40
#define IL_ADD 0x58
#define IL_SUB 0x59
#define IL_MUL 0x5A
#define IL_DIV 0x5B
#define IL_DIV_UN 0x5C
#define IL_REM 0x5D
#define IL_REM_UN 0x5E
#define IL_AND 0x5F
#define IL_OR 0x60
#define IL_XOR 0x61
#define IL_SHL 0x62
#define IL_SHR 0x63
#define IL_SHR_UN 0x64
#define IL_NEG 0x65
#define IL_NOT 0x66
#define IL_BGT_S_OPCODE 0x30

/* --- Test Helper --- */
typedef int32_t (*TestFunc)(int32_t);

int32_t compile_and_run(uint8_t *il, uint32_t il_len, int32_t input) {
  /* Mock IL Assembly */
  il_assembly_t assembly = {
      .methods = malloc(sizeof(il_method_t)),
      .method_count = 1,
  };

  il_method_t *method = &assembly.methods[0];
  method->code = il;
  method->code_size = il_len;
  method->max_stack = 10;
  method->local_vars_count = 5;
  method->token = 0x06000001;

  /* Translate IL → Fruity */
  fruity_module_t *fruity_mod = il_to_fruity_convert_assembly(&assembly);
  if (!fruity_mod) {
    printf("    ✗ IL→Fruity failed\n");
    free(assembly.methods);
    return -999;
  }

  /* Translate Fruity → QBE */
  QBEBuffer qbe_buf;
  qbe_buffer_init(&qbe_buf);
  fruity_to_qbe(fruity_mod, (uintptr)&qbe_buf);
  qbe_buffer_terminate(&qbe_buf);

  /* Compile QBE → x86-64 machine code */
  void *code_buf = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code_buf == MAP_FAILED) {
    printf("    ✗ mmap failed\n");
    return -999;
  }

  extern int qbe_compile_to_x86(const char *qbe_il, void *out_buf,
                                size_t buf_size);
  int code_size = qbe_compile_to_x86(qbe_buf.data, code_buf, 4096);
  if (code_size <= 0) {
    printf("    ✗ QBE compile failed\n");
    munmap(code_buf, 4096);
    return -999;
  }

  /* Execute */
  TestFunc func = (TestFunc)code_buf;
  int32_t result = func(input);

  munmap(code_buf, 4096);
  free(assembly.methods);
  return result;
}

/* ===== ARITHMETIC TESTS ===== */

void test_div() {
  printf("Test: DIV (division)\n");
  uint8_t il[] = {IL_LDC_I4_S, 20, // Load 20
                  IL_LDC_I4_S, 4,  // Load 4
                  IL_DIV,          // 20 / 4 = 5
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 5) {
    printf("  ✓ DIV: 20 / 4 = 5\n");
  } else {
    printf("  ✗ DIV failed: expected 5, got %d\n", result);
  }
}

void test_rem() {
  printf("\nTest: REM (remainder)\n");
  uint8_t il[] = {IL_LDC_I4_S, 23, // Load 23
                  IL_LDC_I4_S, 5,  // Load 5
                  IL_REM,          // 23 % 5 = 3
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 3) {
    printf("  ✓ REM: 23 %% 5 = 3\n");
  } else {
    printf("  ✗ REM failed: expected 3, got %d\n", result);
  }
}

void test_neg() {
  printf("\nTest: NEG (negate)\n");
  uint8_t il[] = {IL_LDC_I4_S, 42, // Load 42
                  IL_NEG,          // -42
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == -42) {
    printf("  ✓ NEG: -42\n");
  } else {
    printf("  ✗ NEG failed: expected -42, got %d\n", result);
  }
}

/* ===== BITWISE TESTS ===== */

void test_and() {
  printf("\nTest: AND (bitwise and)\n");
  uint8_t il[] = {IL_LDC_I4_S, 0xF0, // 11110000
                  IL_LDC_I4_S, 0x0F, // 00001111
                  IL_AND,            // 00000000 = 0
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 0) {
    printf("  ✓ AND: 0xF0 & 0x0F = 0x00\n");
  } else {
    printf("  ✗ AND failed: expected 0, got %d\n", result);
  }
}

void test_or() {
  printf("\nTest: OR (bitwise or)\n");
  uint8_t il[] = {IL_LDC_I4_S, 0xF0, // 11110000
                  IL_LDC_I4_S, 0x0F, // 00001111
                  IL_OR,             // 11111111 = 255
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 0xFF) {
    printf("  ✓ OR: 0xF0 | 0x0F = 0xFF\n");
  } else {
    printf("  ✗ OR failed: expected 255, got %d\n", result);
  }
}

void test_xor() {
  printf("\nTest: XOR (bitwise xor)\n");
  uint8_t il[] = {IL_LDC_I4_S, 0xAA, // 10101010
                  IL_LDC_I4_S, 0x55, // 01010101
                  IL_XOR,            // 11111111 = 255
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 0xFF) {
    printf("  ✓ XOR: 0xAA ^ 0x55 = 0xFF\n");
  } else {
    printf("  ✗ XOR failed: expected 255, got %d\n", result);
  }
}

void test_shl() {
  printf("\nTest: SHL (shift left)\n");
  uint8_t il[] = {IL_LDC_I4_S, 5, // 00000101
                  IL_LDC_I4_2,    // Shift by 2
                  IL_SHL,         // 00010100 = 20
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 20) {
    printf("  ✓ SHL: 5 << 2 = 20\n");
  } else {
    printf("  ✗ SHL failed: expected 20, got %d\n", result);
  }
}

void test_shr() {
  printf("\nTest: SHR (shift right)\n");
  uint8_t il[] = {IL_LDC_I4_S, 20, // 00010100
                  IL_LDC_I4_2,     // Shift by 2
                  IL_SHR,          // 00000101 = 5
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 5) {
    printf("  ✓ SHR: 20 >> 2 = 5\n");
  } else {
    printf("  ✗ SHR failed: expected 5, got %d\n", result);
  }
}

void test_not() {
  printf("\nTest: NOT (bitwise not)\n");
  uint8_t il[] = {IL_LDC_I4_0, // 0
                  IL_NOT,      // ~0 = -1
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == -1) {
    printf("  ✓ NOT: ~0 = -1\n");
  } else {
    printf("  ✗ NOT failed: expected -1, got %d\n", result);
  }
}

/* ===== BRANCH TESTS ===== */

void test_beq() {
  printf("\nTest: BEQ (branch if equal)\n");
  uint8_t il[] = {IL_LDARG_0,                    // Load arg (input)
                  IL_LDC_I4_5,                   // Load 5
                  IL_BEQ_S,    4,                // If equal, skip to ret 100
                  IL_LDC_I4_0,                   // Not equal: ret 0
                  IL_RET,      IL_LDC_I4_S, 100, // Equal: ret 100
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 5);
  if (result == 100) {
    printf("  ✓ BEQ: 5 == 5 → 100\n");
  } else {
    printf("  ✗ BEQ failed: expected 100, got %d\n", result);
  }
}

void test_bne() {
  printf("\nTest: BNE (branch if not equal)\n");
  uint8_t il[] = {IL_LDARG_0,                    // Load arg
                  IL_LDC_I4_5,                   // Load 5
                  IL_BNE_UN_S, 4,                // If not equal, skip
                  IL_LDC_I4_0,                   // Equal: ret 0
                  IL_RET,      IL_LDC_I4_S, 100, // Not equal: ret 100
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 7);
  if (result == 100) {
    printf("  ✓ BNE: 7 != 5 → 100\n");
  } else {
    printf("  ✗ BNE failed: expected 100, got %d\n", result);
  }
}

void test_blt() {
  printf("\nTest: BLT (branch if less than)\n");
  uint8_t il[] = {IL_LDARG_0,                    // Load arg
                  IL_LDC_I4_S, 10,               // Load 10
                  IL_BLT_S,    4,                // If arg < 10, skip
                  IL_LDC_I4_0,                   // Not less: ret 0
                  IL_RET,      IL_LDC_I4_S, 100, // Less: ret 100
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 3);
  if (result == 100) {
    printf("  ✓ BLT: 3 < 10 → 100\n");
  } else {
    printf("  ✗ BLT failed: expected 100, got %d\n", result);
  }
}

void test_ble() {
  printf("\nTest: BLE (branch if less or equal)\n");
  uint8_t il[] = {IL_LDARG_0,                    // Load arg
                  IL_LDC_I4_S, 10,               // Load 10
                  IL_BLE_S,    4,                // If arg <= 10, skip
                  IL_LDC_I4_0,                   // Not <=: ret 0
                  IL_RET,      IL_LDC_I4_S, 100, // <=: ret 100
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 10);
  if (result == 100) {
    printf("  ✓ BLE: 10 <= 10 → 100\n");
  } else {
    printf("  ✗ BLE failed: expected 100, got %d\n", result);
  }
}

void test_bge() {
  printf("\nTest: BGE (branch if greater or equal)\n");
  uint8_t il[] = {IL_LDARG_0,                    // Load arg
                  IL_LDC_I4_5,                   // Load 5
                  IL_BGE_S,    4,                // If arg >= 5, skip
                  IL_LDC_I4_0,                   // Not >=: ret 0
                  IL_RET,      IL_LDC_I4_S, 100, // >=: ret 100
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 7);
  if (result == 100) {
    printf("  ✓ BGE: 7 >= 5 → 100\n");
  } else {
    printf("  ✗ BGE failed: expected 100, got %d\n", result);
  }
}

void test_brfalse() {
  printf("\nTest: BRFALSE (branch if zero)\n");
  uint8_t il[] = {IL_LDARG_0,      // Load arg
                  IL_BRFALSE_S, 4, // If zero, skip
                  IL_LDC_I4_1,     // Non-zero: ret 1
                  IL_RET,
                  IL_LDC_I4_0, // Zero: ret 0
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 0);
  if (result == 0) {
    printf("  ✓ BRFALSE: 0 → branch taken\n");
  } else {
    printf("  ✗ BRFALSE failed: expected 0, got %d\n", result);
  }
}

void test_brtrue() {
  printf("\nTest: BRTRUE (branch if non-zero)\n");
  uint8_t il[] = {IL_LDARG_0,     // Load arg
                  IL_BRTRUE_S, 4, // If non-zero, skip
                  IL_LDC_I4_0,    // Zero: ret 0
                  IL_RET,
                  IL_LDC_I4_1, // Non-zero: ret 1
                  IL_RET};
  int32_t result = compile_and_run(il, sizeof(il), 5);
  if (result == 1) {
    printf("  ✓ BRTRUE: 5 → branch taken\n");
  } else {
    printf("  ✗ BRTRUE failed: expected 1, got %d\n", result);
  }
}

/* ===== MAIN ===== */

int main(void) {
  setbuf(stdout, NULL);

  printf("==============================================\n");
  printf("  Phase 1: IL Opcode Tests\n");
  printf("  Branches + Arithmetic + Bitwise\n");
  printf("==============================================\n\n");

  /* Arithmetic */
  printf("--- ARITHMETIC OPERATIONS ---\n");
  test_div();
  test_rem();
  test_neg();

  /* Bitwise */
  printf("\n--- BITWISE OPERATIONS ---\n");
  test_and();
  test_or();
  test_xor();
  test_shl();
  test_shr();
  test_not();

  /* Branches */
  printf("\n--- CONDITIONAL BRANCHES ---\n");
  test_beq();
  test_bne();
  test_blt();
  test_ble();
  test_bge();
  test_brfalse();
  test_brtrue();

  printf("\n==============================================\n");
  printf("✅ Phase 1 Tests Complete!\n");
  printf("==============================================\n");

  return 0;
}
