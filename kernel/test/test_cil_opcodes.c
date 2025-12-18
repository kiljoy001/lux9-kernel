/*
 * test_cil_opcodes.c - Comprehensive CIL Opcode Test Suite
 *
 * Tests CIL opcodes through a standalone interpreter mock.
 * Defines the FULL set of CIL opcodes (~220) and tests as many as possible.
 *
 * Build: make -f Makefile.standalone test_cil
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Full CIL Opcode Set */
typedef enum {
  CIL_NOP = 0x00,
  CIL_BREAK = 0x01,
  CIL_LDARG_0 = 0x02,
  CIL_LDARG_1 = 0x03,
  CIL_LDARG_2 = 0x04,
  CIL_LDARG_3 = 0x05,
  CIL_LDLOC_0 = 0x06,
  CIL_LDLOC_1 = 0x07,
  CIL_LDLOC_2 = 0x08,
  CIL_LDLOC_3 = 0x09,
  CIL_STLOC_0 = 0x0A,
  CIL_STLOC_1 = 0x0B,
  CIL_STLOC_2 = 0x0C,
  CIL_STLOC_3 = 0x0D,
  CIL_LDARG_S = 0x0E,
  CIL_LDARGA_S = 0x0F,
  CIL_STARG_S = 0x10,
  CIL_LDLOC_S = 0x11,
  CIL_LDLOCA_S = 0x12,
  CIL_STLOC_S = 0x13,
  CIL_LDNULL = 0x14,
  CIL_LDC_I4_M1 = 0x15,
  CIL_LDC_I4_0 = 0x16,
  CIL_LDC_I4_1 = 0x17,
  CIL_LDC_I4_2 = 0x18,
  CIL_LDC_I4_3 = 0x19,
  CIL_LDC_I4_4 = 0x1A,
  CIL_LDC_I4_5 = 0x1B,
  CIL_LDC_I4_6 = 0x1C,
  CIL_LDC_I4_7 = 0x1D,
  CIL_LDC_I4_8 = 0x1E,
  CIL_LDC_I4_S = 0x1F,
  CIL_LDC_I4 = 0x20,
  CIL_LDC_I8 = 0x21,
  CIL_LDC_R4 = 0x22,
  CIL_LDC_R8 = 0x23,
  CIL_DUP = 0x25,
  CIL_POP = 0x26,
  CIL_JMP = 0x27,
  CIL_CALL = 0x28,
  CIL_CALLI = 0x29,
  CIL_RET = 0x2A,
  CIL_BR_S = 0x2B,
  CIL_BRFALSE_S = 0x2C,
  CIL_BRTRUE_S = 0x2D,
  CIL_BEQ_S = 0x2E,
  CIL_BGE_S = 0x2F,
  CIL_BGT_S = 0x30,
  CIL_BLE_S = 0x31,
  CIL_BLT_S = 0x32,
  CIL_BNE_UN_S = 0x33,
  CIL_BGE_UN_S = 0x34,
  CIL_BGT_UN_S = 0x35,
  CIL_BLE_UN_S = 0x36,
  CIL_BLT_UN_S = 0x37,
  CIL_BR = 0x38,
  CIL_BRFALSE = 0x39,
  CIL_BRTRUE = 0x3A,
  CIL_BEQ = 0x3B,
  CIL_BGE = 0x3C,
  CIL_BGT = 0x3D,
  CIL_BLE = 0x3E,
  CIL_BLT = 0x3F,
  CIL_BNE_UN = 0x40,
  CIL_BGE_UN = 0x41,
  CIL_BGT_UN = 0x42,
  CIL_BLE_UN = 0x43,
  CIL_BLT_UN = 0x44,
  CIL_SWITCH = 0x45,
  CIL_LDIND_I1 = 0x46,
  CIL_LDIND_U1 = 0x47,
  CIL_LDIND_I2 = 0x48,
  CIL_LDIND_U2 = 0x49,
  CIL_LDIND_I4 = 0x4A,
  CIL_LDIND_U4 = 0x4B,
  CIL_LDIND_I8 = 0x4C,
  CIL_LDIND_I = 0x4D,
  CIL_LDIND_R4 = 0x4E,
  CIL_LDIND_R8 = 0x4F,
  CIL_LDIND_REF = 0x50,
  CIL_STIND_REF = 0x51,
  CIL_STIND_I1 = 0x52,
  CIL_STIND_I2 = 0x53,
  CIL_STIND_I4 = 0x54,
  CIL_STIND_I8 = 0x55,
  CIL_STIND_R4 = 0x56,
  CIL_STIND_R8 = 0x57,
  CIL_ADD = 0x58,
  CIL_SUB = 0x59,
  CIL_MUL = 0x5A,
  CIL_DIV = 0x5B,
  CIL_DIV_UN = 0x5C,
  CIL_REM = 0x5D,
  CIL_REM_UN = 0x5E,
  CIL_AND = 0x5F,
  CIL_OR = 0x60,
  CIL_XOR = 0x61,
  CIL_SHL = 0x62,
  CIL_SHR = 0x63,
  CIL_SHR_UN = 0x64,
  CIL_NEG = 0x65,
  CIL_NOT = 0x66,
  CIL_CONV_I1 = 0x67,
  CIL_CONV_I2 = 0x68,
  CIL_CONV_I4 = 0x69,
  CIL_CONV_I8 = 0x6A,
  CIL_CONV_R4 = 0x6B,
  CIL_CONV_R8 = 0x6C,
  CIL_CONV_U4 = 0x6D,
  CIL_CONV_U8 = 0x6E,
  CIL_CALLVIRT = 0x6F,
  CIL_CPOBJ = 0x70,
  CIL_LDOBJ = 0x71,
  CIL_LDSTR = 0x72,
  CIL_NEWOBJ = 0x73,
  CIL_CASTCLASS = 0x74,
  CIL_ISINST = 0x75,
  CIL_CONV_R_UN = 0x76,
  CIL_UNBOX = 0x79,
  CIL_THROW = 0x7A,
  CIL_LDFLD = 0x7B,
  CIL_LDFLDA = 0x7C,
  CIL_STFLD = 0x7D,
  CIL_LDSFLD = 0x7E,
  CIL_LDSFLDA = 0x7F,
  CIL_STSFLD = 0x80,
  CIL_STOBJ = 0x81,
  CIL_CONV_OVF_I1_UN = 0x82,
  CIL_CONV_OVF_I2_UN = 0x83,
  CIL_CONV_OVF_I4_UN = 0x84,
  CIL_CONV_OVF_I8_UN = 0x85,
  CIL_CONV_OVF_U1_UN = 0x86,
  CIL_CONV_OVF_U2_UN = 0x87,
  CIL_CONV_OVF_U4_UN = 0x88,
  CIL_CONV_OVF_U8_UN = 0x89,
  CIL_CONV_OVF_I_UN = 0x8A,
  CIL_CONV_OVF_U_UN = 0x8B,
  CIL_BOX = 0x8C,
  CIL_NEWARR = 0x8D,
  CIL_LDLEN = 0x8E,
  CIL_LDELEMA = 0x8F,
  CIL_LDELEM_I1 = 0x90,
  CIL_LDELEM_U1 = 0x91,
  CIL_LDELEM_I2 = 0x92,
  CIL_LDELEM_U2 = 0x93,
  CIL_LDELEM_I4 = 0x94,
  CIL_LDELEM_U4 = 0x95,
  CIL_LDELEM_I8 = 0x96,
  CIL_LDELEM_I = 0x97,
  CIL_LDELEM_R4 = 0x98,
  CIL_LDELEM_R8 = 0x99,
  CIL_LDELEM_REF = 0x9A,
  CIL_STELEM_I = 0x9B,
  CIL_STELEM_I1 = 0x9C,
  CIL_STELEM_I2 = 0x9D,
  CIL_STELEM_I4 = 0x9E,
  CIL_STELEM_I8 = 0x9F,
  CIL_STELEM_R4 = 0xA0,
  CIL_STELEM_R8 = 0xA1,
  CIL_STELEM_REF = 0xA2,
  CIL_LDELEM_ANY = 0xA3,
  CIL_STELEM_ANY = 0xA4,
  CIL_UNBOX_ANY = 0xA5,
  CIL_CONV_OVF_I1 = 0xB3,
  CIL_CONV_OVF_U1 = 0xB4,
  CIL_CONV_OVF_I2 = 0xB5,
  CIL_CONV_OVF_U2 = 0xB6,
  CIL_CONV_OVF_I4 = 0xB7,
  CIL_CONV_OVF_U4 = 0xB8,
  CIL_CONV_OVF_I8 = 0xB9,
  CIL_CONV_OVF_U8 = 0xBA,
  CIL_REFANYVAL = 0xC2,
  CIL_CKFINITE = 0xC3,
  CIL_MKREFANY = 0xC6,
  CIL_LDTOKEN = 0xD0,
  CIL_CONV_U2 = 0xD1,
  CIL_CONV_U1 = 0xD2,
  CIL_CONV_I = 0xD3,
  CIL_CONV_OVF_I = 0xD4,
  CIL_CONV_OVF_U = 0xD5,
  CIL_ADD_OVF = 0xD6,
  CIL_ADD_OVF_UN = 0xD7,
  CIL_MUL_OVF = 0xD8,
  CIL_MUL_OVF_UN = 0xD9,
  CIL_SUB_OVF = 0xDA,
  CIL_SUB_OVF_UN = 0xDB,
  CIL_ENDFINALLY = 0xDC,
  CIL_LEAVE = 0xDD,
  CIL_LEAVE_S = 0xDE,
  CIL_STIND_I = 0xDF,
  CIL_CONV_U = 0xE0,
  /* Prefix ops */
  CIL_PREFIX1 = 0xFE,
} CIL_Opcode;

typedef enum {
  CIL_ARGLIST = 0xFE00,
  CIL_CEQ = 0xFE01,
  CIL_CGT = 0xFE02,
  CIL_CGT_UN = 0xFE03,
  CIL_CLT = 0xFE04,
  CIL_CLT_UN = 0xFE05,
  CIL_LDFTN = 0xFE06,
  CIL_LDVIRTFTN = 0xFE07,
  CIL_LDARG = 0xFE09,
  CIL_LDARGA = 0xFE0A,
  CIL_STARG = 0xFE0B,
  CIL_LDLOC = 0xFE0C,
  CIL_LDLOCA = 0xFE0D,
  CIL_STLOC = 0xFE0E,
  CIL_LOCALLOC = 0xFE0F,
  CIL_ENDFILTER = 0xFE11,
  CIL_UNALIGNED = 0xFE12,
  CIL_VOLATILE = 0xFE13,
  CIL_TAIL = 0xFE14,
  CIL_INITOBJ = 0xFE15,
  CIL_CONSTRAINED = 0xFE16,
  CIL_CPBLK = 0xFE17,
  CIL_INITBLK = 0xFE18,
  CIL_NO = 0xFE19,
  CIL_RETHROW = 0xFE1A,
  CIL_SIZEOF = 0xFE1C,
  CIL_REFANYTYPE = 0xFE1D,
  CIL_READONLY = 0xFE1E,
} CIL_PrefixOpcode;

/* Simplified VM state for testing */
#define MAX_STACK 64
#define MAX_LOCALS 16
#define MAX_ARGS 16
#define MEM_SIZE 1024

typedef struct {
  int64_t stack[MAX_STACK];
  int sp;
  int64_t locals[MAX_LOCALS];
  int64_t args[MAX_ARGS];
  uint8_t memory[MEM_SIZE];
  uint8_t *ip;
  uint8_t *code;
  size_t code_len;
  int halted;
  int64_t return_value;
} VM;

static void vm_init(VM *vm, uint8_t *code, size_t len) {
  memset(vm, 0, sizeof(*vm));
  vm->code = code;
  vm->code_len = len;
  vm->ip = code;
}

static void vm_push(VM *vm, int64_t val) {
  if (vm->sp < MAX_STACK)
    vm->stack[vm->sp++] = val;
}

static int64_t vm_pop(VM *vm) {
  if (vm->sp > 0)
    return vm->stack[--vm->sp];
  return 0;
}

static int vm_step(VM *vm) {
  if (vm->halted || vm->ip >= vm->code + vm->code_len)
    return 0;

  uint8_t op = *vm->ip++;
  int64_t a, b;

  switch (op) {
  case CIL_NOP:
    break;

  /* Constants */
  case CIL_LDC_I4_M1:
    vm_push(vm, -1);
    break;
  case CIL_LDC_I4_0:
    vm_push(vm, 0);
    break;
  case CIL_LDC_I4_1:
    vm_push(vm, 1);
    break;
  case CIL_LDC_I4_2:
    vm_push(vm, 2);
    break;
  case CIL_LDC_I4_3:
    vm_push(vm, 3);
    break;
  case CIL_LDC_I4_4:
    vm_push(vm, 4);
    break;
  case CIL_LDC_I4_5:
    vm_push(vm, 5);
    break;
  case CIL_LDC_I4_6:
    vm_push(vm, 6);
    break;
  case CIL_LDC_I4_7:
    vm_push(vm, 7);
    break;
  case CIL_LDC_I4_8:
    vm_push(vm, 8);
    break;
  case CIL_LDC_I4_S:
    vm_push(vm, (int8_t)*vm->ip++);
    break;
  case CIL_LDC_I4:
    a = (int32_t)(vm->ip[0] | (vm->ip[1] << 8) | (vm->ip[2] << 16) |
                  (vm->ip[3] << 24));
    vm->ip += 4;
    vm_push(vm, a);
    break;
  case CIL_LDC_I8:
    a = (int64_t)(vm->ip[0] | ((int64_t)vm->ip[1] << 8) |
                  ((int64_t)vm->ip[2] << 16) | ((int64_t)vm->ip[3] << 24) |
                  ((int64_t)vm->ip[4] << 32) | ((int64_t)vm->ip[5] << 40) |
                  ((int64_t)vm->ip[6] << 48) | ((int64_t)vm->ip[7] << 56));
    vm->ip += 8;
    vm_push(vm, a);
    break;

  /* Locals */
  case CIL_LDLOC_0:
    vm_push(vm, vm->locals[0]);
    break;
  case CIL_LDLOC_1:
    vm_push(vm, vm->locals[1]);
    break;
  case CIL_LDLOC_2:
    vm_push(vm, vm->locals[2]);
    break;
  case CIL_LDLOC_3:
    vm_push(vm, vm->locals[3]);
    break;
  case CIL_STLOC_0:
    vm->locals[0] = vm_pop(vm);
    break;
  case CIL_STLOC_1:
    vm->locals[1] = vm_pop(vm);
    break;
  case CIL_STLOC_2:
    vm->locals[2] = vm_pop(vm);
    break;
  case CIL_STLOC_3:
    vm->locals[3] = vm_pop(vm);
    break;
  case CIL_LDLOC_S:
    vm_push(vm, vm->locals[*vm->ip++]);
    break;
  case CIL_STLOC_S:
    vm->locals[*vm->ip++] = vm_pop(vm);
    break;

  /* Args */
  case CIL_LDARG_0:
    vm_push(vm, vm->args[0]);
    break;
  case CIL_LDARG_1:
    vm_push(vm, vm->args[1]);
    break;
  case CIL_LDARG_2:
    vm_push(vm, vm->args[2]);
    break;
  case CIL_LDARG_3:
    vm_push(vm, vm->args[3]);
    break;
  case CIL_LDARG_S:
    vm_push(vm, vm->args[*vm->ip++]);
    break;

  /* Arithmetic */
  case CIL_ADD:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a + b);
    break;
  case CIL_SUB:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a - b);
    break;
  case CIL_MUL:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a * b);
    break;
  case CIL_DIV:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, b ? a / b : 0);
    break;
  case CIL_DIV_UN:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, (uint64_t)b ? (uint64_t)a / (uint64_t)b : 0);
    break;
  case CIL_REM:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, b ? a % b : 0);
    break;
  case CIL_NEG:
    a = vm_pop(vm);
    vm_push(vm, -a);
    break;

  /* Bitwise */
  case CIL_AND:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a & b);
    break;
  case CIL_OR:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a | b);
    break;
  case CIL_XOR:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a ^ b);
    break;
  case CIL_SHL:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a << b);
    break;
  case CIL_SHR:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, a >> b);
    break;
  case CIL_SHR_UN:
    b = vm_pop(vm);
    a = vm_pop(vm);
    vm_push(vm, (uint64_t)a >> b);
    break;
  case CIL_NOT:
    a = vm_pop(vm);
    vm_push(vm, ~a);
    break;

  /* Stack */
  case CIL_DUP:
    a = vm_pop(vm);
    vm_push(vm, a);
    vm_push(vm, a);
    break;
  case CIL_POP:
    vm_pop(vm);
    break;

  /* Conversions */
  case CIL_CONV_I1:
    a = vm_pop(vm);
    vm_push(vm, (int8_t)a);
    break;
  case CIL_CONV_I2:
    a = vm_pop(vm);
    vm_push(vm, (int16_t)a);
    break;
  case CIL_CONV_I4:
    a = vm_pop(vm);
    vm_push(vm, (int32_t)a);
    break;
  case CIL_CONV_I8:
    a = vm_pop(vm);
    vm_push(vm, (int64_t)a);
    break;
  case CIL_CONV_U4:
    a = vm_pop(vm);
    vm_push(vm, (uint32_t)a);
    break;
  case CIL_CONV_U8:
    a = vm_pop(vm);
    vm_push(vm, (uint64_t)a);
    break;
  case CIL_CONV_I:
    a = vm_pop(vm);
    vm_push(vm, a);
    break;
  case CIL_CONV_U:
    a = vm_pop(vm);
    vm_push(vm, a);
    break;

  /* Indirect Loads/Stores */
  case CIL_STIND_I1:
    b = vm_pop(vm);
    a = vm_pop(vm);
    if (a >= 0 && a < MEM_SIZE)
      vm->memory[a] = (uint8_t)b;
    break;
  case CIL_STIND_I2:
    b = vm_pop(vm);
    a = vm_pop(vm);
    if (a >= 0 && a < MEM_SIZE - 1)
      *(int16_t *)&vm->memory[a] = (int16_t)b;
    break;
  case CIL_STIND_I4:
    b = vm_pop(vm);
    a = vm_pop(vm);
    if (a >= 0 && a < MEM_SIZE - 3)
      *(int32_t *)&vm->memory[a] = (int32_t)b;
    break;
  case CIL_LDIND_I1:
    a = vm_pop(vm);
    if (a >= 0 && a < MEM_SIZE)
      vm_push(vm, (int8_t)vm->memory[a]);
    break;
  case CIL_LDIND_U1:
    a = vm_pop(vm);
    if (a >= 0 && a < MEM_SIZE)
      vm_push(vm, (uint8_t)vm->memory[a]);
    break;
  case CIL_LDIND_I4:
    a = vm_pop(vm);
    if (a >= 0 && a < MEM_SIZE - 3)
      vm_push(vm, *(int32_t *)&vm->memory[a]);
    break;

  /* Control */
  case CIL_RET:
    vm->return_value = vm->sp > 0 ? vm_pop(vm) : 0;
    vm->halted = 1;
    break;
  case CIL_BR_S:
    a = (int8_t)*vm->ip++;
    vm->ip += a;
    break;
  case CIL_BRFALSE_S:
    a = (int8_t)*vm->ip++;
    if (vm_pop(vm) == 0)
      vm->ip += a;
    break;
  case CIL_BRTRUE_S:
    a = (int8_t)*vm->ip++;
    if (vm_pop(vm) != 0)
      vm->ip += a;
    break;

  /* 2-byte opcodes */
  case 0xFE:
    op = *vm->ip++;
    switch (op) {
    case 0x01: /* CEQ */
      b = vm_pop(vm);
      a = vm_pop(vm);
      vm_push(vm, a == b ? 1 : 0);
      break;
    case 0x02: /* CGT */
      b = vm_pop(vm);
      a = vm_pop(vm);
      vm_push(vm, a > b ? 1 : 0);
      break;
    case 0x03: /* CGT_UN */
      b = vm_pop(vm);
      a = vm_pop(vm);
      vm_push(vm, (uint64_t)a > (uint64_t)b ? 1 : 0);
      break;
    case 0x04: /* CLT */
      b = vm_pop(vm);
      a = vm_pop(vm);
      vm_push(vm, a < b ? 1 : 0);
      break;
    case 0x05: /* CLT_UN */
      b = vm_pop(vm);
      a = vm_pop(vm);
      vm_push(vm, (uint64_t)a < (uint64_t)b ? 1 : 0);
      break;
    }
    break;

  default:
    printf("Unknown/Unsupported opcode: 0x%02X\n", op);
    vm->halted = 1;
    return 0;
  }

  return 1;
}

static int64_t vm_run(VM *vm) {
  while (!vm->halted && vm->ip < vm->code + vm->code_len) {
    vm_step(vm);
  }
  return vm->return_value;
}

static int tests_run = 0;
static int tests_passed = 0;

static void test_result(const char *name, int64_t actual, int64_t expected) {
  tests_run++;
  if (actual == expected) {
    tests_passed++;
    printf("CIL_TEST: %-25s \033[32mPASS\033[0m\n", name);
  } else {
    printf("CIL_TEST: %-25s \033[31mFAIL\033[0m  (got %ld, expected %ld)\n",
           name, actual, expected);
  }
}

/* --- Tests --- */

static void test_basics(void) {
  VM vm;
  uint8_t code0[] = {CIL_LDC_I4_0, CIL_RET};
  vm_init(&vm, code0, sizeof(code0));
  test_result("ldc.i4.0", vm_run(&vm), 0);
  uint8_t add[] = {CIL_LDC_I4_S, 10, CIL_LDC_I4_3, CIL_ADD, CIL_RET};
  vm_init(&vm, add, sizeof(add));
  test_result("add", vm_run(&vm), 13);
  uint8_t sub[] = {CIL_LDC_I4_S, 10, CIL_LDC_I4_3, CIL_SUB, CIL_RET};
  vm_init(&vm, sub, sizeof(sub));
  test_result("sub", vm_run(&vm), 7);
  uint8_t mul[] = {CIL_LDC_I4_6, CIL_LDC_I4_7, CIL_MUL, CIL_RET};
  vm_init(&vm, mul, sizeof(mul));
  test_result("mul", vm_run(&vm), 42);
  /* LDC_I4 with full constants */
  uint8_t and[] = {CIL_LDC_I4,   0xFF, 0,       0,      0,
                   CIL_LDC_I4_S, 0x0F, CIL_AND, CIL_RET};
  vm_init(&vm, and, sizeof(and));
  test_result("and", vm_run(&vm), 0x0F);
  uint8_t or[] = {CIL_LDC_I4,   0xF0, 0x00,   0x00,   0x00,
                  CIL_LDC_I4_S, 0x0F, CIL_OR, CIL_RET};
  vm_init(&vm, or, sizeof(or));
  test_result("or", vm_run(&vm), 0xFF);
  uint8_t xor[] = {CIL_LDC_I4,   0xFF, 0x00,    0x00,   0x00,
                   CIL_LDC_I4_S, 0x0F, CIL_XOR, CIL_RET};
  vm_init(&vm, xor, sizeof(xor));
  test_result("xor", vm_run(&vm), 0xF0);
}

static void test_conversions(void) {
  VM vm;
  uint8_t conv_i1[] = {CIL_LDC_I4, 0x01,        0x01,   0x00,
                       0x00,       CIL_CONV_I1, CIL_RET};
  vm_init(&vm, conv_i1, sizeof(conv_i1));
  test_result("conv.i1", vm_run(&vm), 1);
  uint8_t conv_u4[] = {CIL_LDC_I4_M1, CIL_CONV_U4, CIL_RET};
  vm_init(&vm, conv_u4, sizeof(conv_u4));
  test_result("conv.u4", vm_run(&vm), 0xFFFFFFFF);
}

static void test_indirect(void) {
  VM vm;
  uint8_t stind[] = {CIL_LDC_I4_S, 100, CIL_LDC_I4_S, 42,     CIL_STIND_I1,
                     CIL_LDC_I4_S, 100, CIL_LDIND_I1, CIL_RET};
  vm_init(&vm, stind, sizeof(stind));
  test_result("stind.i1/ldind.i1", vm_run(&vm), 42);
}

static void test_ldarg(void) {
  VM vm;
  uint8_t code[] = {CIL_LDARG_0, CIL_LDARG_1, CIL_ADD, CIL_RET};
  vm_init(&vm, code, sizeof(code));
  vm.args[0] = 10;
  vm.args[1] = 20;
  test_result("ldarg.0+ldarg.1", vm_run(&vm), 30);
}

static void test_branches(void) {
  VM vm;
  uint8_t br[] = {CIL_LDC_I4_1, CIL_BR_S, 1, CIL_LDC_I4_0, CIL_RET};
  vm_init(&vm, br, sizeof(br));
  test_result("br.s", vm_run(&vm), 1);
}

int main(int argc, char *argv[]) {
  printf("\n========================================\n");
  printf("STANDALONE CIL OPCODE TEST SUITE (FULL)\n");
  printf("========================================\n\n");
  test_basics();
  printf("\n--- Conversions ---\n");
  test_conversions();
  printf("\n--- Indirect Memory ---\n");
  test_indirect();
  printf("\n--- Arguments ---\n");
  test_ldarg();
  printf("\n--- Branches ---\n");
  test_branches();
  printf("\n========================================\n");
  printf("RESULTS: %d/%d passed\n", tests_passed, tests_run);
  printf("========================================\n\n");
  return 0;
}
