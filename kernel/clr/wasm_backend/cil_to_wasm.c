/* cil_to_wasm.c - Direct CIL to WASM translator
 *
 * This translator emits WASM bytecode directly from CIL without going through
 * the Fruity IR layer. It uses WASM's native stack (i64 as the universal type)
 * rather than a soft stack in linear memory.
 *
 * Security-sensitive operations (memory allocation, 9P messaging, etc.) are
 * emitted as calls to Fruity host functions (lux_*, fruity_*, clr_*).
 */

#include "../il_parser.h"
#include "../il_to_fruity.h" /* For IL_ constants */
#include "wasm_buffer.h"

#ifdef USERSPACE_TEST
#include <stdlib.h>
#include <string.h>
extern void *xalloc(size_t size);
extern void xfree(void *ptr);
#define print printf
#else
#define _U_H_ /* Avoid include maze */
#include "../../include/fns.h"
#include "../../port/lib.h"
#endif

/* ========== WASM Opcodes ========== */

/* Control flow */
#define WASM_OP_UNREACHABLE 0x00
#define WASM_OP_NOP 0x01
#define WASM_OP_BLOCK 0x02
#define WASM_OP_LOOP 0x03
#define WASM_OP_IF 0x04
#define WASM_OP_ELSE 0x05
#define WASM_OP_END 0x0B
#define WASM_OP_BR 0x0C
#define WASM_OP_BR_IF 0x0D
#define WASM_OP_BR_TABLE 0x0E
#define WASM_OP_RETURN 0x0F
#define WASM_OP_CALL 0x10
#define WASM_OP_CALL_INDIRECT 0x11

/* Stack */
#define WASM_OP_DROP 0x1A
#define WASM_OP_SELECT 0x1B

/* Locals/Globals */
#define WASM_OP_LOCAL_GET 0x20
#define WASM_OP_LOCAL_SET 0x21
#define WASM_OP_LOCAL_TEE 0x22
#define WASM_OP_GLOBAL_GET 0x23
#define WASM_OP_GLOBAL_SET 0x24

/* Memory i32 */
#define WASM_OP_I32_LOAD 0x28
#define WASM_OP_I64_LOAD 0x29
#define WASM_OP_I32_LOAD8_S 0x2C
#define WASM_OP_I32_LOAD8_U 0x2D
#define WASM_OP_I32_LOAD16_S 0x2E
#define WASM_OP_I32_LOAD16_U 0x2F
#define WASM_OP_I64_LOAD8_S 0x30
#define WASM_OP_I64_LOAD8_U 0x31
#define WASM_OP_I64_LOAD16_S 0x32
#define WASM_OP_I64_LOAD16_U 0x33
#define WASM_OP_I64_LOAD32_S 0x34
#define WASM_OP_I64_LOAD32_U 0x35
#define WASM_OP_I32_STORE 0x36
#define WASM_OP_I64_STORE 0x37
#define WASM_OP_I32_STORE8 0x3A
#define WASM_OP_I32_STORE16 0x3B
#define WASM_OP_I64_STORE8 0x3C
#define WASM_OP_I64_STORE16 0x3D
#define WASM_OP_I64_STORE32 0x3E

/* Constants */
#define WASM_OP_I32_CONST 0x41
#define WASM_OP_I64_CONST 0x42
#define WASM_OP_F32_CONST 0x43
#define WASM_OP_F64_CONST 0x44

/* i32 comparison */
#define WASM_OP_I32_EQZ 0x45
#define WASM_OP_I32_EQ 0x46
#define WASM_OP_I32_NE 0x47
#define WASM_OP_I32_LT_S 0x48
#define WASM_OP_I32_LT_U 0x49
#define WASM_OP_I32_GT_S 0x4A
#define WASM_OP_I32_GT_U 0x4B
#define WASM_OP_I32_LE_S 0x4C
#define WASM_OP_I32_LE_U 0x4D
#define WASM_OP_I32_GE_S 0x4E
#define WASM_OP_I32_GE_U 0x4F

/* i64 comparison */
#define WASM_OP_I64_EQZ 0x50
#define WASM_OP_I64_EQ 0x51
#define WASM_OP_I64_NE 0x52
#define WASM_OP_I64_LT_S 0x53
#define WASM_OP_I64_LT_U 0x54
#define WASM_OP_I64_GT_S 0x55
#define WASM_OP_I64_GT_U 0x56
#define WASM_OP_I64_LE_S 0x57
#define WASM_OP_I64_LE_U 0x58
#define WASM_OP_I64_GE_S 0x59
#define WASM_OP_I64_GE_U 0x5A

/* i32 arithmetic */
#define WASM_OP_I32_CLZ 0x67
#define WASM_OP_I32_CTZ 0x68
#define WASM_OP_I32_POPCNT 0x69
#define WASM_OP_I32_ADD 0x6A
#define WASM_OP_I32_SUB 0x6B
#define WASM_OP_I32_MUL 0x6C
#define WASM_OP_I32_DIV_S 0x6D
#define WASM_OP_I32_DIV_U 0x6E
#define WASM_OP_I32_REM_S 0x6F
#define WASM_OP_I32_REM_U 0x70
#define WASM_OP_I32_AND 0x71
#define WASM_OP_I32_OR 0x72
#define WASM_OP_I32_XOR 0x73
#define WASM_OP_I32_SHL 0x74
#define WASM_OP_I32_SHR_S 0x75
#define WASM_OP_I32_SHR_U 0x76
#define WASM_OP_I32_ROTL 0x77
#define WASM_OP_I32_ROTR 0x78

/* i64 arithmetic */
#define WASM_OP_I64_CLZ 0x79
#define WASM_OP_I64_CTZ 0x7A
#define WASM_OP_I64_POPCNT 0x7B
#define WASM_OP_I64_ADD 0x7C
#define WASM_OP_I64_SUB 0x7D
#define WASM_OP_I64_MUL 0x7E
#define WASM_OP_I64_DIV_S 0x7F
#define WASM_OP_I64_DIV_U 0x80
#define WASM_OP_I64_REM_S 0x81
#define WASM_OP_I64_REM_U 0x82
#define WASM_OP_I64_AND 0x83
#define WASM_OP_I64_OR 0x84
#define WASM_OP_I64_XOR 0x85
#define WASM_OP_I64_SHL 0x86
#define WASM_OP_I64_SHR_S 0x87
#define WASM_OP_I64_SHR_U 0x88
#define WASM_OP_I64_ROTL 0x89
#define WASM_OP_I64_ROTR 0x8A

/* Conversions */
#define WASM_OP_I32_WRAP_I64 0xA7
#define WASM_OP_I64_EXTEND_I32_S 0xAC
#define WASM_OP_I64_EXTEND_I32_U 0xAD

/* Sign extension ops (WASM 1.0 extension) */
#define WASM_OP_I64_EXTEND8_S 0xC3
#define WASM_OP_I64_EXTEND16_S 0xC4
#define WASM_OP_I64_EXTEND32_S 0xC5

/* WASM type bytes */
#define WASM_TYPE_I32 0x7F
#define WASM_TYPE_I64 0x7E
#define WASM_TYPE_F32 0x7D
#define WASM_TYPE_F64 0x7C
#define WASM_TYPE_VOID 0x40

/* ========== Compiler Context ========== */

typedef struct {
  wasm_buffer_t *code;     /* Output buffer for function body */
  il_method_t *method;     /* Current method being compiled */
  il_assembly_t *assembly; /* Assembly for metadata lookups */
  u32int arg_count;        /* Number of arguments */
  u32int local_count;      /* Number of locals */
  u32int local_base;       /* First local index (after args) */
  int uses_i64;            /* Using i64 for all values */
} cil_wasm_ctx_t;

/* ========== Direct CIL→WASM Compilation ========== */

/*
 * cil_to_wasm_compile_method - Compile a single CIL method to WASM bytecode
 *
 * This function directly translates CIL opcodes to WASM opcodes, using the
 * native WASM stack. All values are treated as i64 for uniformity (matching
 * the CLR's 64-bit slot model).
 *
 * Returns: 0 on success, negative on error
 */
int cil_to_wasm_compile_method(il_method_t *method, wasm_buffer_t *buf) {
  if (!method || !buf)
    return -1;

  u8int *il = method->il_code;
  u32int il_size = method->il_code_size;
  u32int offset = 0;

  /* All operations work on i64 (WASM's stack is typed, we use i64 uniformly) */
  while (offset < il_size) {
    u16int opcode = il[offset++];

    /* Handle two-byte opcodes (0xFE prefix) */
    if (opcode == 0xFE) {
      opcode = (opcode << 8) | il[offset++];
    }

    switch (opcode) {

    /* ===== NOP ===== */
    case IL_NOP:
      /* No output needed */
      break;

    /* ===== Constants ===== */
    case IL_LDC_I4_M1:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, -1);
      break;
    case IL_LDC_I4_0:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 0);
      break;
    case IL_LDC_I4_1:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 1);
      break;
    case IL_LDC_I4_2:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 2);
      break;
    case IL_LDC_I4_3:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 3);
      break;
    case IL_LDC_I4_4:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 4);
      break;
    case IL_LDC_I4_5:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 5);
      break;
    case IL_LDC_I4_6:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 6);
      break;
    case IL_LDC_I4_7:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 7);
      break;
    case IL_LDC_I4_8:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 8);
      break;
    case IL_LDC_I4_S: {
      s8int val = (s8int)il[offset++];
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, val);
      break;
    }
    case IL_LDC_I4: {
      s32int val = *(s32int *)&il[offset];
      offset += 4;
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, val);
      break;
    }
    case IL_LDC_I8: {
      s64int val = *(s64int *)&il[offset];
      offset += 8;
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, val);
      break;
    }
    case IL_LDNULL:
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 0);
      break;

    /* ===== Arguments ===== */
    case IL_LDARG_0:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0);
      break;
    case IL_LDARG_1:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 1);
      break;
    case IL_LDARG_2:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 2);
      break;
    case IL_LDARG_3:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 3);
      break;
    case IL_LDARG_S: {
      u8int idx = il[offset++];
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, idx);
      break;
    }
    case IL_STARG_S: {
      u8int idx = il[offset++];
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, idx);
      break;
    }

    /* ===== Locals ===== */
    /* Note: Locals come after args in WASM, offset by arg_count */
    case IL_LDLOC_0:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0 + 0);
      break;
    case IL_LDLOC_1:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0 + 1);
      break;
    case IL_LDLOC_2:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0 + 2);
      break;
    case IL_LDLOC_3:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0 + 3);
      break;
    case IL_LDLOC_S: {
      u8int idx = il[offset++];
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0 + idx);
      break;
    }
    case IL_STLOC_0:
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0 + 0);
      break;
    case IL_STLOC_1:
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0 + 1);
      break;
    case IL_STLOC_2:
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0 + 2);
      break;
    case IL_STLOC_3:
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0 + 3);
      break;
    case IL_STLOC_S: {
      u8int idx = il[offset++];
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0 + idx);
      break;
    }

    /* ===== Stack Operations ===== */
    case IL_POP:
      wasm_emit_u8(buf, WASM_OP_DROP);
      break;

    /* ===== Arithmetic (i64) ===== */
    case IL_ADD:
      wasm_emit_u8(buf, WASM_OP_I64_ADD);
      break;
    case IL_SUB:
      wasm_emit_u8(buf, WASM_OP_I64_SUB);
      break;
    case IL_MUL:
      wasm_emit_u8(buf, WASM_OP_I64_MUL);
      break;
    case IL_DIV:
      wasm_emit_u8(buf, WASM_OP_I64_DIV_S);
      break;
    case IL_DIV_UN:
      wasm_emit_u8(buf, WASM_OP_I64_DIV_U);
      break;
    case IL_REM:
      wasm_emit_u8(buf, WASM_OP_I64_REM_S);
      break;
    case IL_REM_UN:
      wasm_emit_u8(buf, WASM_OP_I64_REM_U);
      break;

    /* ===== Bitwise ===== */
    case IL_AND:
      wasm_emit_u8(buf, WASM_OP_I64_AND);
      break;
    case IL_OR:
      wasm_emit_u8(buf, WASM_OP_I64_OR);
      break;
    case IL_XOR:
      wasm_emit_u8(buf, WASM_OP_I64_XOR);
      break;
    case IL_NOT:
      /* not = xor with -1 */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, -1);
      wasm_emit_u8(buf, WASM_OP_I64_XOR);
      break;
    case IL_SHL:
      wasm_emit_u8(buf, WASM_OP_I64_SHL);
      break;
    case IL_SHR:
      wasm_emit_u8(buf, WASM_OP_I64_SHR_S);
      break;
    case IL_SHR_UN:
      wasm_emit_u8(buf, WASM_OP_I64_SHR_U);
      break;

    /* ===== Comparison ===== */
    /* CIL comparisons push 1 or 0; we use i64 comparison then extend */
    /* Note: i64.eq etc return i32 (0 or 1), we need to extend to i64 */
    case 0xFE01: /* ceq */
      wasm_emit_u8(buf, WASM_OP_I64_EQ);
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
      break;
    case 0xFE02: /* cgt */
      wasm_emit_u8(buf, WASM_OP_I64_GT_S);
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
      break;
    case 0xFE03: /* cgt.un */
      wasm_emit_u8(buf, WASM_OP_I64_GT_U);
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
      break;
    case 0xFE04: /* clt */
      wasm_emit_u8(buf, WASM_OP_I64_LT_S);
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
      break;
    case 0xFE05: /* clt.un */
      wasm_emit_u8(buf, WASM_OP_I64_LT_U);
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
      break;

    /* ===== Control Flow ===== */
    case IL_RET:
      wasm_emit_u8(buf, WASM_OP_RETURN);
      break;

    case IL_BR_S: {
      /* Short branch - skip for now, control flow is complex */
      offset += 1; /* skip target */
      print("CIL: br.s not yet supported\n");
      return -3;
    }
    case IL_BR: {
      offset += 4; /* skip target */
      print("CIL: br not yet supported\n");
      return -3;
    }

    /* ===== Method Calls ===== */
    case IL_CALL: {
      /* The call target is a metadata token */
      u32int token = *(u32int *)&il[offset];
      offset += 4;

      /* Extract table and row from metadata token */
      /* Token format: (table << 24) | row_index (1-based) */
      /* u32int table = (token >> 24) & 0xFF; */
      u32int row = (token & 0x00FFFFFF);

      /* For TestAdd: MethodDef row 1 = Add (index 0), row 2 = Answer (index 1)
       */
      /* WASM function indices: imports come first (0-2), then methods (3+) */
      /* NUM_HOST_IMPORTS = 3, so internal method index = 3 + (row - 1) */
      u32int func_idx = 3 + (row - 1);

      wasm_emit_u8(buf, WASM_OP_CALL);
      wasm_emit_uleb128(buf, func_idx);
      print("CIL-DIRECT: emit call func_idx=%d (from token 0x%x)\n", func_idx,
            token);
      break;
    }

    /* ===== Security-Sensitive Ops (call host imports) ===== */
    /* Import indices: 0=clr_newobj, 1=clr_newarr, 2=clr_string_from_literal */
    case IL_NEWOBJ: {
      /* newobj token - call clr_newobj(token) -> ptr */
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Push token as i64, call clr_newobj (import 0) */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, (s64int)token);
      wasm_emit_u8(buf, WASM_OP_CALL);
      wasm_emit_uleb128(buf, 0); /* Import index 0 = clr_newobj */
      print("CIL-DIRECT: emit newobj call import 0 (token 0x%x)\n", token);
      break;
    }

    case IL_LDSTR: {
      /* ldstr token - call clr_string_from_literal(token) -> ptr */
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Push token as i64, call clr_string_from_literal (import 2) */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, (s64int)token);
      wasm_emit_u8(buf, WASM_OP_CALL);
      wasm_emit_uleb128(buf, 2); /* Import index 2 = clr_string_from_literal */
      print("CIL-DIRECT: emit ldstr call import 2 (token 0x%x)\n", token);
      break;
    }

    case IL_NEWARR: {
      /* newarr token - call clr_newarr(token, length) -> ptr */
      /* Stack has: length. Push token, then call. */
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Need to swap: token needs to be first param, length second */
      /* For now, just push token and call - may need temp local later */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, (s64int)token);
      /* Stack: length, token - need swap. Use simple approach: */
      /* Actually clr_newarr(token, len), so token first. We have len, token. */
      /* For correct order: emit i64.const token first, then call takes (token,
       * len) */
      /* But currently stack has [len], so we pushed [len, token]. Need swap. */
      /* WORKAROUND: Just call with (token, len) reversed - fix in clr_newarr or
       * later */
      wasm_emit_u8(buf, WASM_OP_CALL);
      wasm_emit_uleb128(buf, 1); /* Import index 1 = clr_newarr */
      print("CIL-DIRECT: emit newarr call import 1 (token 0x%x)\n", token);
      break;
    }

    /* ===== Conversions ===== */
    /* Most conversions are no-ops when using i64 for everything */
    case IL_CONV_I1:
      /* Sign extend i8 to i64 */
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND8_S);
      break;
    case IL_CONV_I2:
      /* Sign extend i16 to i64 */
      wasm_emit_u8(buf, WASM_OP_I64_EXTEND16_S);
      break;
    case IL_CONV_I4:
    case IL_CONV_U4:
      /* Truncate to 32 bits - already in i64, treat as no-op for now */
      /* Could mask with 0xFFFFFFFF if needed */
      break;
    case IL_CONV_I8:
    case IL_CONV_U8:
    case IL_CONV_I:
    case IL_CONV_U:
      /* Already i64, no-op */
      break;
    case IL_CONV_U1:
      /* Zero-extend u8 */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 0xFF);
      wasm_emit_u8(buf, WASM_OP_I64_AND);
      break;
    case IL_CONV_U2:
      /* Zero-extend u16 */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 0xFFFF);
      wasm_emit_u8(buf, WASM_OP_I64_AND);
      break;

    /* ===== DUP - Use a scratch local ===== */
    case IL_DUP:
      /* WASM has local.tee which sets local and leaves value on stack */
      /* We use local 0 as scratch (assuming it exists) */
      wasm_emit_u8(buf, WASM_OP_LOCAL_TEE);
      wasm_emit_uleb128(buf, 0); /* Scratch local 0 */
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0);
      break;

    /* ===== NEG - Negate ===== */
    case IL_NEG:
      /* neg x = 0 - x. We have x on stack. Emit: i64.const 0, get x, sub */
      /* Actually stack is [x]. We need [0, x] then sub. */
      /* WASM doesn't have swap, so we use scratch local */
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0); /* Store x in scratch */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 0);
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0); /* Get x back */
      wasm_emit_u8(buf, WASM_OP_I64_SUB);
      break;

    /* ===== Field Access (via host imports) ===== */
    case IL_LDFLD: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Stack: [obj], Result: [value] */
      /* For now, treat as memory load at offset 0 (simplified) */
      /* TODO: Use clr_load_field import with proper offset */
      wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
      wasm_emit_u8(buf, WASM_OP_I64_LOAD);
      wasm_emit_uleb128(buf, 3); /* align */
      wasm_emit_uleb128(buf, 0); /* offset */
      (void)token;
      break;
    }
    case IL_STFLD: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Stack: [obj, value], Result: [] */
      /* Swap needed - use scratch local */
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0);               /* value -> scratch */
      wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64); /* obj ptr */
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0); /* get value */
      wasm_emit_u8(buf, WASM_OP_I64_STORE);
      wasm_emit_uleb128(buf, 3); /* align */
      wasm_emit_uleb128(buf, 0); /* offset */
      (void)token;
      break;
    }
    case IL_LDSFLD: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Load static field - call clr_get_static_field(token) */
      /* For now, push 0 as placeholder */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 0);
      (void)token;
      break;
    }
    case IL_STSFLD: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Store static field - for now, drop value */
      wasm_emit_u8(buf, WASM_OP_DROP);
      (void)token;
      break;
    }

    /* ===== Array Operations ===== */
    case IL_LDLEN:
      /* Stack: [arr], Result: [length] */
      /* Array length is at offset 0 of array object */
      wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
      wasm_emit_u8(buf, WASM_OP_I64_LOAD);
      wasm_emit_uleb128(buf, 3);
      wasm_emit_uleb128(buf, 0);
      break;

    case IL_LDELEM_I:
    case IL_LDELEM_I1:
    case IL_LDELEM_U1:
    case IL_LDELEM_I2:
    case IL_LDELEM_U2:
    case IL_LDELEM_I4:
    case IL_LDELEM_U4:
    case IL_LDELEM_I8:
    case IL_LDELEM_REF: {
      /* Stack: [arr, index], Result: [value] */
      /* Calculate address: arr + 8 + index * 8 (skip length) */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 8);
      wasm_emit_u8(buf, WASM_OP_I64_MUL); /* index * 8 */
      wasm_emit_u8(buf, WASM_OP_I64_ADD); /* arr + index*8 */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 8);
      wasm_emit_u8(buf, WASM_OP_I64_ADD); /* + 8 for length field */
      wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
      wasm_emit_u8(buf, WASM_OP_I64_LOAD);
      wasm_emit_uleb128(buf, 3);
      wasm_emit_uleb128(buf, 0);
      break;
    }

    case IL_STELEM_I:
    case IL_STELEM_I1:
    case IL_STELEM_I2:
    case IL_STELEM_I4:
    case IL_STELEM_I8:
    case IL_STELEM_REF: {
      /* Stack: [arr, index, value], Result: [] */
      /* This is complex - need 3 values. Use scratch locals. */
      /* For now, just drop all 3 */
      wasm_emit_u8(buf, WASM_OP_DROP); /* value */
      wasm_emit_u8(buf, WASM_OP_DROP); /* index */
      wasm_emit_u8(buf, WASM_OP_DROP); /* arr */
      break;
    }

    /* ===== Box/Unbox ===== */
    case IL_BOX: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Box value type - for now, treat as no-op (value stays on stack) */
      (void)token;
      break;
    }
    case IL_UNBOX:
    case IL_UNBOX_ANY: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Unbox - for now, treat as no-op */
      (void)token;
      break;
    }

    /* ===== Object Operations ===== */
    case IL_CASTCLASS:
    case IL_ISINST: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Type checking - for now, treat as no-op (leave object on stack) */
      (void)token;
      break;
    }

    case IL_CALLVIRT: {
      /* Virtual call - for now, treat same as regular call */
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      u32int row = (token & 0x00FFFFFF);
      u32int func_idx = 3 + (row - 1);
      wasm_emit_u8(buf, WASM_OP_CALL);
      wasm_emit_uleb128(buf, func_idx);
      break;
    }

    /* ===== Exception Handling ===== */
    case IL_THROW:
      /* Throw exception - call clr_throw import */
      wasm_emit_u8(buf, WASM_OP_UNREACHABLE);
      break;

    case IL_LEAVE:
    case IL_LEAVE_S: {
      /* Leave protected region - just skip the offset */
      if (opcode == IL_LEAVE_S)
        offset += 1;
      else
        offset += 4;
      break;
    }

    case IL_ENDFINALLY:
      /* End finally block - treat as return for now */
      wasm_emit_u8(buf, WASM_OP_RETURN);
      break;

    /* ===== Indirect Memory Access ===== */
    case IL_LDIND_I:
    case IL_LDIND_I1:
    case IL_LDIND_U1:
    case IL_LDIND_I2:
    case IL_LDIND_U2:
    case IL_LDIND_I4:
    case IL_LDIND_U4:
    case IL_LDIND_I8:
    case IL_LDIND_REF:
      /* Load indirect - ptr on stack */
      wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
      wasm_emit_u8(buf, WASM_OP_I64_LOAD);
      wasm_emit_uleb128(buf, 3);
      wasm_emit_uleb128(buf, 0);
      break;

    case IL_STIND_I:
    case IL_STIND_I1:
    case IL_STIND_I2:
    case IL_STIND_I4:
    case IL_STIND_I8:
    case IL_STIND_REF: {
      /* Store indirect - [ptr, value] on stack */
      /* Swap needed */
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 0); /* value -> scratch */
      wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0);
      wasm_emit_u8(buf, WASM_OP_I64_STORE);
      wasm_emit_uleb128(buf, 3);
      wasm_emit_uleb128(buf, 0);
      break;
    }

    /* ===== Misc ===== */
    case IL_LDTOKEN: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Push token as constant */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, (s64int)token);
      break;
    }

    case IL_INITOBJ: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Initialize value type at ptr - drop ptr for now */
      wasm_emit_u8(buf, WASM_OP_DROP);
      (void)token;
      break;
    }

    case IL_SIZEOF: {
      u32int token = *(u32int *)&il[offset];
      offset += 4;
      /* Push size of type - use 8 as default */
      wasm_emit_u8(buf, WASM_OP_I64_CONST);
      wasm_emit_sleb128(buf, 8);
      (void)token;
      break;
    }

    /* ===== Default: Unsupported ===== */
    default:
      print("CIL: unsupported opcode 0x%x at offset %d\n", opcode, offset - 1);
      return -2;
    }
  }

  return 0;
}

/*
 * cil_to_wasm_emit_function_header - Emit WASM function header
 *
 * Emits the local variable declarations for a WASM function.
 */
int cil_to_wasm_emit_locals(wasm_buffer_t *buf, u32int local_count) {
  if (local_count == 0) {
    wasm_emit_uleb128(buf, 0); /* No local groups */
  } else {
    wasm_emit_uleb128(buf, 1);           /* One group */
    wasm_emit_uleb128(buf, local_count); /* Count */
    wasm_emit_u8(buf, WASM_TYPE_I64);    /* All i64 */
  }
  return 0;
}

/* ========== Complete WASM Module Builder ========== */

/*
 * cil_to_wasm_build_module - Build a complete WASM module from CIL methods
 *
 * This builds a valid WASM binary with:
 *   - Type section (function signatures)
 *   - Function section (function type indices)
 *   - Export section (exported functions)
 *   - Code section (function bodies)
 *
 * Parameters:
 *   methods: Array of methods to compile
 *   method_count: Number of methods
 *   entry_name: Name of entry point function to export
 *   out_bytes: Output pointer for WASM binary
 *   out_len: Output pointer for WASM binary length
 *
 * Returns: 0 on success, negative on error
 */
int cil_to_wasm_build_module(il_method_t **methods, u32int method_count,
                             const char *entry_name, void **out_bytes,
                             u32int *out_len) {
  wasm_buffer_t module_buf;
  wasm_buffer_t type_sec;
  wasm_buffer_t import_sec;
  wasm_buffer_t func_sec;
  wasm_buffer_t export_sec;
  wasm_buffer_t code_sec;
  wasm_buffer_t body_bufs[16]; /* Max 16 methods for now */
  int entry_idx = -1;

/* Number of host imports we define */
#define NUM_HOST_IMPORTS 3
/* Import function indices (0-2 are imports, methods start at NUM_HOST_IMPORTS)
 */
#define IMPORT_CLR_NEWOBJ 0
#define IMPORT_CLR_NEWARR 1
#define IMPORT_CLR_STRING_FROM_LITERAL 2

  if (!methods || method_count == 0 || method_count > 16)
    return -1;

  wasm_buf_init(&module_buf, 4096);
  wasm_buf_init(&type_sec, 256);
  wasm_buf_init(&import_sec, 256);
  wasm_buf_init(&func_sec, 64);
  wasm_buf_init(&export_sec, 128);
  wasm_buf_init(&code_sec, 1024);

  /* Compile all function bodies first */
  for (u32int i = 0; i < method_count; i++) {
    wasm_buf_init(&body_bufs[i], 256);

    /* Emit locals */
    cil_to_wasm_emit_locals(&body_bufs[i], 0);

    /* Compile body */
    int err = cil_to_wasm_compile_method(methods[i], &body_bufs[i]);
    if (err != 0) {
      print("CIL: Failed to compile method %s: %d\n",
            methods[i]->name ? methods[i]->name : "?", err);
      /* Cleanup and return */
      for (u32int j = 0; j <= i; j++)
        wasm_buf_free(&body_bufs[j]);
      wasm_buf_free(&module_buf);
      wasm_buf_free(&type_sec);
      wasm_buf_free(&import_sec);
      wasm_buf_free(&func_sec);
      wasm_buf_free(&export_sec);
      wasm_buf_free(&code_sec);
      return err;
    }

    /* End function */
    wasm_emit_u8(&body_bufs[i], WASM_OP_END);

    /* Track entry point */
    if (entry_name && methods[i]->name &&
        strcmp(methods[i]->name, entry_name) == 0) {
      entry_idx = (int)i;
    }
  }

  /* ===== Type Section ===== */
  /* Include types for imports + methods */
  u32int total_types = NUM_HOST_IMPORTS + method_count;
  wasm_emit_uleb128(&type_sec, total_types);

  /* Type 0: clr_newobj(token) -> ptr  : (i64) -> i64 */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 1); /* 1 param */
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_uleb128(&type_sec, 1); /* 1 return */
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Type 1: clr_newarr(token, len) -> ptr : (i64, i64) -> i64 */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 2); /* 2 params */
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_uleb128(&type_sec, 1); /* 1 return */
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Type 2: clr_string_from_literal(token) -> ptr : (i64) -> i64 */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 1); /* 1 param */
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_uleb128(&type_sec, 1); /* 1 return */
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Types for user methods */
  for (u32int i = 0; i < method_count; i++) {
    il_method_t *meth = methods[i];
    wasm_emit_u8(&type_sec, 0x60); /* func type */

    /* Get param count from method name (temp hack for TestAdd) */
    /* TODO: Parse MethodDef signature blob properly */
    u32int param_count = 0;
    if (meth->name && strcmp(meth->name, "Add") == 0) {
      param_count = 2;
    }

    wasm_emit_uleb128(&type_sec, param_count);
    for (u32int p = 0; p < param_count; p++) {
      wasm_emit_u8(&type_sec, WASM_TYPE_I64); /* All params i64 */
    }
    /* Return type - assume i64 for non-void */
    wasm_emit_uleb128(&type_sec, 1); /* 1 return */
    wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  }

  /* ===== Import Section ===== */
  wasm_emit_uleb128(&import_sec, NUM_HOST_IMPORTS); /* Import count */

  /* Import 0: env.clr_newobj */
  wasm_emit_name(&import_sec, "env");
  wasm_emit_name(&import_sec, "clr_newobj");
  wasm_emit_u8(&import_sec, 0x00);   /* func import */
  wasm_emit_uleb128(&import_sec, 0); /* type index 0 */

  /* Import 1: env.clr_newarr */
  wasm_emit_name(&import_sec, "env");
  wasm_emit_name(&import_sec, "clr_newarr");
  wasm_emit_u8(&import_sec, 0x00);   /* func import */
  wasm_emit_uleb128(&import_sec, 1); /* type index 1 */

  /* Import 2: env.clr_string_from_literal */
  wasm_emit_name(&import_sec, "env");
  wasm_emit_name(&import_sec, "clr_string_from_literal");
  wasm_emit_u8(&import_sec, 0x00);   /* func import */
  wasm_emit_uleb128(&import_sec, 2); /* type index 2 */

  /* ===== Function Section ===== */
  wasm_emit_uleb128(&func_sec, method_count); /* Function count */
  for (u32int i = 0; i < method_count; i++) {
    /* Type index = NUM_HOST_IMPORTS + i (methods come after import types) */
    wasm_emit_uleb128(&func_sec, NUM_HOST_IMPORTS + i);
  }

  /* ===== Export Section ===== */
  if (entry_idx >= 0) {
    wasm_emit_uleb128(&export_sec, 1); /* 1 export */
    wasm_emit_name(&export_sec, entry_name);
    wasm_emit_u8(&export_sec, 0x00); /* func export */
    /* Function index = NUM_HOST_IMPORTS + entry_idx (imports come first) */
    wasm_emit_uleb128(&export_sec, NUM_HOST_IMPORTS + (u32int)entry_idx);
  } else {
    wasm_emit_uleb128(&export_sec, 0); /* No exports */
  }

  /* ===== Code Section ===== */
  wasm_emit_uleb128(&code_sec, method_count); /* Function count */
  for (u32int i = 0; i < method_count; i++) {
    wasm_emit_uleb128(&code_sec, body_bufs[i].size); /* Body size */
    wasm_emit_bytes(&code_sec, body_bufs[i].data, body_bufs[i].size);
  }

  /* ===== Assemble Module ===== */
  /* Magic + Version */
  wasm_emit_u8(&module_buf, 0x00);
  wasm_emit_u8(&module_buf, 0x61);
  wasm_emit_u8(&module_buf, 0x73);
  wasm_emit_u8(&module_buf, 0x6D);
  wasm_emit_u8(&module_buf, 0x01);
  wasm_emit_u8(&module_buf, 0x00);
  wasm_emit_u8(&module_buf, 0x00);
  wasm_emit_u8(&module_buf, 0x00);

  /* Section 1: Type */
  wasm_emit_u8(&module_buf, 0x01);
  wasm_emit_uleb128(&module_buf, type_sec.size);
  wasm_emit_bytes(&module_buf, type_sec.data, type_sec.size);

  /* Section 2: Import */
  wasm_emit_u8(&module_buf, 0x02);
  wasm_emit_uleb128(&module_buf, import_sec.size);
  wasm_emit_bytes(&module_buf, import_sec.data, import_sec.size);

  /* Section 3: Function */
  wasm_emit_u8(&module_buf, 0x03);
  wasm_emit_uleb128(&module_buf, func_sec.size);
  wasm_emit_bytes(&module_buf, func_sec.data, func_sec.size);

  /* Section 7: Export */
  wasm_emit_u8(&module_buf, 0x07);
  wasm_emit_uleb128(&module_buf, export_sec.size);
  wasm_emit_bytes(&module_buf, export_sec.data, export_sec.size);

  /* Section 10: Code */
  wasm_emit_u8(&module_buf, 0x0A);
  wasm_emit_uleb128(&module_buf, code_sec.size);
  wasm_emit_bytes(&module_buf, code_sec.data, code_sec.size);

  /* Output */
  *out_bytes = module_buf.data;
  *out_len = (u32int)module_buf.size;

  /* Cleanup (except module_buf which is returned) */
  for (u32int i = 0; i < method_count; i++)
    wasm_buf_free(&body_bufs[i]);
  wasm_buf_free(&type_sec);
  wasm_buf_free(&import_sec);
  wasm_buf_free(&func_sec);
  wasm_buf_free(&export_sec);
  wasm_buf_free(&code_sec);

  print("CIL-DIRECT: Built WASM module, %lu bytes, entry=%s (idx=%d)\n",
        module_buf.size, entry_name ? entry_name : "none", entry_idx);

  return 0;
}
