/* cil_opcodes.c - Complete CIL Opcode to WASM Translation
 *
 * This is the SINGLE source of truth for all CIL->WASM opcode translation.
 * All values are treated as i64 on the WASM stack for uniformity.
 *
 * Symbolic computing opcodes (IL_SYM_*) hook into minigmp for
 * arbitrary-precision arithmetic.
 */

#include "cil_opcodes.h"
#include "../il_parser.h"
#include "wasm_buffer.h"

/* WASM Opcodes */
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
#define WASM_OP_DROP 0x1A
#define WASM_OP_SELECT 0x1B
#define WASM_OP_LOCAL_GET 0x20
#define WASM_OP_LOCAL_SET 0x21
#define WASM_OP_LOCAL_TEE 0x22
#define WASM_OP_GLOBAL_GET 0x23
#define WASM_OP_GLOBAL_SET 0x24

/* Memory */
#define WASM_OP_I32_LOAD 0x28
#define WASM_OP_I64_LOAD 0x29
#define WASM_OP_F32_LOAD 0x2A
#define WASM_OP_F64_LOAD 0x2B
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
#define WASM_OP_F32_STORE 0x38
#define WASM_OP_F64_STORE 0x39
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
#define WASM_OP_I64_EXTEND8_S 0xC3
#define WASM_OP_I64_EXTEND16_S 0xC4
#define WASM_OP_I64_EXTEND32_S 0xC5

/* Host import indices */
#define HOST_CLR_NEWOBJ 0
#define HOST_CLR_NEWARR 1
#define HOST_CLR_LDSTR 2
#define HOST_CLR_LDSFLD 3
#define HOST_CLR_STSFLD 4
#define HOST_CLR_LDFLD 5
#define HOST_CLR_STFLD 6
#define HOST_CLR_LDFLDA 7
#define HOST_CLR_LDSFLDA 8
#define HOST_CLR_LDLEN 9
#define HOST_CLR_BOX 10
#define HOST_CLR_UNBOX 11
#define HOST_CLR_ISINST 12
#define HOST_CLR_INITOBJ 13
#define HOST_CLR_LDELEM 14
#define HOST_CLR_STELEM 15
#define HOST_CLR_LDELEMA 16

/* Symbolic computing host imports (17-24) */
#define HOST_SYM_CREATE 17
#define HOST_SYM_EXPR 18
#define HOST_SYM_DIFF 19
#define HOST_SYM_INTEGRATE 20
#define HOST_SYM_SIMPLIFY 21
#define HOST_SYM_EVAL 22
#define HOST_SYM_MATCH 23
#define HOST_SYM_REWRITE 24

#define NUM_HOST_IMPORTS 25

/*
 * cil_emit_opcode - Emit WASM for a single CIL opcode
 *
 * Parameters:
 *   buf: Output WASM buffer
 *   il: CIL bytecode
 *   offset: Pointer to current offset (updated after emit)
 *   il_size: Total size of IL
 *
 * Returns: 0 on success, -1 on error, -100 for branch opcodes (handled by
 * relooper)
 */
int cil_emit_opcode(wasm_buffer_t *buf, u8int *il, u32int *offset,
                    u32int il_size) {
  if (!buf || !il || !offset || *offset >= il_size)
    return -1;

  u16int opcode = il[(*offset)++];

  /* Handle two-byte opcodes (0xFE prefix) */
  if (opcode == 0xFE && *offset < il_size) {
    opcode = (opcode << 8) | il[(*offset)++];
  }

  switch (opcode) {
  /* ===== NOP ===== */
  case IL_NOP:
  case IL_BREAK:
    /* No-op in WASM */
    break;

  /* ===== CONSTANTS ===== */
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
    s8int val = (s8int)il[(*offset)++];
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, val);
    break;
  }
  case IL_LDC_I4: {
    s32int val = *(s32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, val);
    break;
  }
  case IL_LDC_I8: {
    s64int val = *(s64int *)&il[*offset];
    *offset += 8;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, val);
    break;
  }
  case IL_LDC_R4: {
    /* Float constant - load as bits */
    u32int bits = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, bits);
    break;
  }
  case IL_LDC_R8: {
    /* Double constant - load as bits */
    u64int bits = *(u64int *)&il[*offset];
    *offset += 8;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, bits);
    break;
  }
  case IL_LDNULL:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0);
    break;

  /* ===== ARGUMENTS ===== */
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
    u8int idx = il[(*offset)++];
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_LDARGA_S: {
    /* Load address of argument - return pointer to local slot */
    u8int idx = il[(*offset)++];
    /* For now, just load the value (address semantics need linear memory) */
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_STARG_S: {
    u8int idx = il[(*offset)++];
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_LDARG: {
    u16int idx = *(u16int *)&il[*offset];
    *offset += 2;
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_LDARGA: {
    u16int idx = *(u16int *)&il[*offset];
    *offset += 2;
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_STARG: {
    u16int idx = *(u16int *)&il[*offset];
    *offset += 2;
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, idx);
    break;
  }

  /* ===== LOCALS ===== */
  case IL_LDLOC_0:
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, 0);
    break;
  case IL_LDLOC_1:
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, 1);
    break;
  case IL_LDLOC_2:
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, 2);
    break;
  case IL_LDLOC_3:
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, 3);
    break;
  case IL_LDLOC_S: {
    u8int idx = il[(*offset)++];
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_LDLOCA_S: {
    u8int idx = il[(*offset)++];
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_STLOC_0:
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, 0);
    break;
  case IL_STLOC_1:
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, 1);
    break;
  case IL_STLOC_2:
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, 2);
    break;
  case IL_STLOC_3:
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, 3);
    break;
  case IL_STLOC_S: {
    u8int idx = il[(*offset)++];
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_LDLOC: {
    u16int idx = *(u16int *)&il[*offset];
    *offset += 2;
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_LDLOCA: {
    u16int idx = *(u16int *)&il[*offset];
    *offset += 2;
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, idx);
    break;
  }
  case IL_STLOC: {
    u16int idx = *(u16int *)&il[*offset];
    *offset += 2;
    wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(buf, idx);
    break;
  }

  /* ===== STACK ===== */
  case IL_POP:
    wasm_emit_u8(buf, WASM_OP_DROP);
    break;
  case IL_DUP:
    /* Duplicate top of stack using local.tee */
    wasm_emit_u8(buf, WASM_OP_LOCAL_TEE);
    wasm_emit_uleb128(buf, 0); /* Use local 0 as scratch */
    wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(buf, 0);
    break;

  /* ===== ARITHMETIC ===== */
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
  case IL_NEG:
    /* Negate: ~x + 1 */
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, -1);
    wasm_emit_u8(buf, WASM_OP_I64_XOR);
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 1);
    wasm_emit_u8(buf, WASM_OP_I64_ADD);
    break;

  /* Overflow variants - treat as regular for now */
  case IL_ADD_OVF:
  case IL_ADD_OVF_UN:
    wasm_emit_u8(buf, WASM_OP_I64_ADD);
    break;
  case IL_SUB_OVF:
  case IL_SUB_OVF_UN:
    wasm_emit_u8(buf, WASM_OP_I64_SUB);
    break;
  case IL_MUL_OVF:
  case IL_MUL_OVF_UN:
    wasm_emit_u8(buf, WASM_OP_I64_MUL);
    break;

  /* ===== BITWISE ===== */
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

  /* ===== COMPARISONS ===== */
  case IL_CEQ:
    wasm_emit_u8(buf, WASM_OP_I64_EQ);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
    break;
  case IL_CGT:
    wasm_emit_u8(buf, WASM_OP_I64_GT_S);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
    break;
  case IL_CGT_UN:
    wasm_emit_u8(buf, WASM_OP_I64_GT_U);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
    break;
  case IL_CLT:
    wasm_emit_u8(buf, WASM_OP_I64_LT_S);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
    break;
  case IL_CLT_UN:
    wasm_emit_u8(buf, WASM_OP_I64_LT_U);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_U);
    break;

  /* ===== CONVERSIONS ===== */
  case IL_CONV_I1:
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND8_S);
    break;
  case IL_CONV_I2:
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND16_S);
    break;
  case IL_CONV_I4:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_S);
    break;
  case IL_CONV_I8:
  case IL_CONV_I:
    /* Already i64 */
    break;
  case IL_CONV_U1:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0xFF);
    wasm_emit_u8(buf, WASM_OP_I64_AND);
    break;
  case IL_CONV_U2:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0xFFFF);
    wasm_emit_u8(buf, WASM_OP_I64_AND);
    break;
  case IL_CONV_U4:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0xFFFFFFFF);
    wasm_emit_u8(buf, WASM_OP_I64_AND);
    break;
  case IL_CONV_U8:
  case IL_CONV_U:
    /* Already i64 */
    break;
  case IL_CONV_R4:
  case IL_CONV_R8:
  case IL_CONV_R_UN:
    /* Float conversion - treat as no-op for now (bits stay same) */
    break;

  /* Overflow conversions - treat as regular */
  case IL_CONV_OVF_I1:
  case IL_CONV_OVF_I1_UN:
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND8_S);
    break;
  case IL_CONV_OVF_I2:
  case IL_CONV_OVF_I2_UN:
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND16_S);
    break;
  case IL_CONV_OVF_I4:
  case IL_CONV_OVF_I4_UN:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_EXTEND_I32_S);
    break;
  case IL_CONV_OVF_I8:
  case IL_CONV_OVF_I8_UN:
  case IL_CONV_OVF_I:
  case IL_CONV_OVF_I_UN:
    break;
  case IL_CONV_OVF_U1:
  case IL_CONV_OVF_U1_UN:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0xFF);
    wasm_emit_u8(buf, WASM_OP_I64_AND);
    break;
  case IL_CONV_OVF_U2:
  case IL_CONV_OVF_U2_UN:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0xFFFF);
    wasm_emit_u8(buf, WASM_OP_I64_AND);
    break;
  case IL_CONV_OVF_U4:
  case IL_CONV_OVF_U4_UN:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0xFFFFFFFF);
    wasm_emit_u8(buf, WASM_OP_I64_AND);
    break;
  case IL_CONV_OVF_U8:
  case IL_CONV_OVF_U8_UN:
  case IL_CONV_OVF_U:
  case IL_CONV_OVF_U_UN:
    break;

  /* ===== INDIRECT LOAD ===== */
  case IL_LDIND_I1:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD8_S);
    wasm_emit_u8(buf, 0); /* align */
    wasm_emit_u8(buf, 0); /* offset */
    break;
  case IL_LDIND_U1:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD8_U);
    wasm_emit_u8(buf, 0);
    wasm_emit_u8(buf, 0);
    break;
  case IL_LDIND_I2:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD16_S);
    wasm_emit_u8(buf, 1);
    wasm_emit_u8(buf, 0);
    break;
  case IL_LDIND_U2:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD16_U);
    wasm_emit_u8(buf, 1);
    wasm_emit_u8(buf, 0);
    break;
  case IL_LDIND_I4:
  case IL_LDIND_U4:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD32_S);
    wasm_emit_u8(buf, 2);
    wasm_emit_u8(buf, 0);
    break;
  case IL_LDIND_I8:
  case IL_LDIND_I:
  case IL_LDIND_REF:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD);
    wasm_emit_u8(buf, 3);
    wasm_emit_u8(buf, 0);
    break;
  case IL_LDIND_R4:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD32_U);
    wasm_emit_u8(buf, 2);
    wasm_emit_u8(buf, 0);
    break;
  case IL_LDIND_R8:
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD);
    wasm_emit_u8(buf, 3);
    wasm_emit_u8(buf, 0);
    break;

  /* ===== INDIRECT STORE ===== */
  case IL_STIND_I1:
    wasm_emit_u8(buf, WASM_OP_I64_STORE8);
    wasm_emit_u8(buf, 0);
    wasm_emit_u8(buf, 0);
    break;
  case IL_STIND_I2:
    wasm_emit_u8(buf, WASM_OP_I64_STORE16);
    wasm_emit_u8(buf, 1);
    wasm_emit_u8(buf, 0);
    break;
  case IL_STIND_I4:
    wasm_emit_u8(buf, WASM_OP_I64_STORE32);
    wasm_emit_u8(buf, 2);
    wasm_emit_u8(buf, 0);
    break;
  case IL_STIND_I8:
  case IL_STIND_I:
  case IL_STIND_REF:
    wasm_emit_u8(buf, WASM_OP_I64_STORE);
    wasm_emit_u8(buf, 3);
    wasm_emit_u8(buf, 0);
    break;
  case IL_STIND_R4:
    wasm_emit_u8(buf, WASM_OP_I64_STORE32);
    wasm_emit_u8(buf, 2);
    wasm_emit_u8(buf, 0);
    break;
  case IL_STIND_R8:
    wasm_emit_u8(buf, WASM_OP_I64_STORE);
    wasm_emit_u8(buf, 3);
    wasm_emit_u8(buf, 0);
    break;

  /* ===== CALL ===== */
  case IL_CALL:
  case IL_CALLVIRT: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    u32int row = (token & 0x00FFFFFF);
    u32int func_idx = NUM_HOST_IMPORTS + (row - 1);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, func_idx);
    break;
  }
  case IL_CALLI: {
    /* Indirect call - skip signature token */
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_CALL_INDIRECT);
    wasm_emit_uleb128(buf, 0); /* type index */
    wasm_emit_uleb128(buf, 0); /* table index */
    break;
  }
  case IL_JMP: {
    /* Jump to method - treat as tail call */
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    u32int row = (token & 0x00FFFFFF);
    u32int func_idx = NUM_HOST_IMPORTS + (row - 1);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, func_idx);
    wasm_emit_u8(buf, WASM_OP_RETURN);
    break;
  }

  /* ===== FIELDS ===== */
  case IL_LDSFLD: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDSFLD);
    break;
  }
  case IL_STSFLD: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_STSFLD);
    break;
  }
  case IL_LDFLD: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDFLD);
    break;
  }
  case IL_STFLD: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_STFLD);
    break;
  }
  case IL_LDFLDA: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDFLDA);
    break;
  }
  case IL_LDSFLDA: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDSFLDA);
    break;
  }

  /* ===== OBJECTS ===== */
  case IL_NEWOBJ: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_NEWOBJ);
    break;
  }
  case IL_NEWARR: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_NEWARR);
    break;
  }
  case IL_LDSTR: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDSTR);
    break;
  }
  case IL_LDLEN:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDLEN);
    break;
  case IL_LDTOKEN: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    break;
  }
  case IL_BOX: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_BOX);
    break;
  }
  case IL_UNBOX:
  case IL_UNBOX_ANY: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_UNBOX);
    break;
  }
  case IL_CASTCLASS:
  case IL_ISINST: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_ISINST);
    break;
  }
  case IL_INITOBJ: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_INITOBJ);
    break;
  }
  case IL_CPOBJ:
  case IL_LDOBJ: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    /* Load object at address - for now just load i64 */
    wasm_emit_u8(buf, WASM_OP_I32_WRAP_I64);
    wasm_emit_u8(buf, WASM_OP_I64_LOAD);
    wasm_emit_u8(buf, 3);
    wasm_emit_u8(buf, 0);
    (void)token;
    break;
  }
  case IL_STOBJ: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_STORE);
    wasm_emit_u8(buf, 3);
    wasm_emit_u8(buf, 0);
    (void)token;
    break;
  }

  /* ===== ARRAYS ===== */
  case IL_LDELEM_I1:
  case IL_LDELEM_U1:
  case IL_LDELEM_I2:
  case IL_LDELEM_U2:
  case IL_LDELEM_I4:
  case IL_LDELEM_U4:
  case IL_LDELEM_I8:
  case IL_LDELEM_I:
  case IL_LDELEM_R4:
  case IL_LDELEM_R8:
  case IL_LDELEM_REF:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, opcode);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDELEM);
    break;
  case IL_LDELEM: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDELEM);
    break;
  }
  case IL_STELEM_I:
  case IL_STELEM_I1:
  case IL_STELEM_I2:
  case IL_STELEM_I4:
  case IL_STELEM_I8:
  case IL_STELEM_R4:
  case IL_STELEM_R8:
  case IL_STELEM_REF:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, opcode);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_STELEM);
    break;
  case IL_STELEM: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_STELEM);
    break;
  }
  case IL_LDELEMA: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, token);
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_CLR_LDELEMA);
    break;
  }

  /* ===== MISC ===== */
  case IL_LOCALLOC:
    /* Stack allocation - not directly supported, use host import */
    wasm_emit_u8(buf, WASM_OP_DROP);
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0);
    break;
  case IL_SIZEOF: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    /* Default size 8 bytes */
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 8);
    (void)token;
    break;
  }
  case IL_ARGLIST:
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0);
    break;
  case IL_LDFTN:
  case IL_LDVIRTFTN: {
    u32int token = *(u32int *)&il[*offset];
    *offset += 4;
    u32int row = (token & 0x00FFFFFF);
    u32int func_idx = NUM_HOST_IMPORTS + (row - 1);
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, func_idx);
    break;
  }
  case IL_CPBLK:
  case IL_INITBLK:
    /* Memory block operations - drop args */
    wasm_emit_u8(buf, WASM_OP_DROP);
    wasm_emit_u8(buf, WASM_OP_DROP);
    wasm_emit_u8(buf, WASM_OP_DROP);
    break;
  case IL_REFANYVAL:
  case IL_REFANYTYPE:
  case IL_MKREFANY:
    /* TypedReference - not supported, return 0 */
    wasm_emit_u8(buf, WASM_OP_DROP);
    wasm_emit_u8(buf, WASM_OP_I64_CONST);
    wasm_emit_sleb128(buf, 0);
    break;
  case IL_CKFINITE:
    /* Check finite - no-op for now */
    break;
  case IL_VOLATILE:
  case IL_UNALIGNED:
  case IL_TAIL:
  case IL_READONLY:
  case IL_CONSTRAINED:
    /* Prefixes - consume operand if needed and no-op */
    if (opcode == IL_UNALIGNED || opcode == IL_CONSTRAINED) {
      if (opcode == IL_UNALIGNED) {
        (*offset)++; /* 1 byte alignment */
      } else {
        *offset += 4; /* 4 byte token */
      }
    }
    break;

  /* ===== SYMBOLIC COMPUTING ===== */
  case IL_SYM_CREATE:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_CREATE);
    break;
  case IL_SYM_EXPR:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_EXPR);
    break;
  case IL_SYM_DIFF:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_DIFF);
    break;
  case IL_SYM_INTEGRATE:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_INTEGRATE);
    break;
  case IL_SYM_SIMPLIFY:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_SIMPLIFY);
    break;
  case IL_SYM_EVAL:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_EVAL);
    break;
  case IL_SYM_MATCH:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_MATCH);
    break;
  case IL_SYM_REWRITE:
    wasm_emit_u8(buf, WASM_OP_CALL);
    wasm_emit_uleb128(buf, HOST_SYM_REWRITE);
    break;

  /* ===== BRANCHING (handled by relooper) ===== */
  case IL_RET:
  case IL_BR_S:
  case IL_BR:
  case IL_BRFALSE_S:
  case IL_BRFALSE:
  case IL_BRTRUE_S:
  case IL_BRTRUE:
  case IL_BEQ_S:
  case IL_BGE_S:
  case IL_BGT_S:
  case IL_BLE_S:
  case IL_BLT_S:
  case IL_BNE_UN_S:
  case IL_BGE_UN_S:
  case IL_BGT_UN_S:
  case IL_BLE_UN_S:
  case IL_BLT_UN_S:
  case IL_BEQ:
  case IL_BGE:
  case IL_BGT:
  case IL_BLE:
  case IL_BLT:
  case IL_BNE_UN:
  case IL_BGE_UN:
  case IL_BGT_UN:
  case IL_BLE_UN:
  case IL_BLT_UN:
  case IL_SWITCH:
  case IL_LEAVE:
  case IL_LEAVE_S:
  case IL_THROW:
  case IL_RETHROW:
  case IL_ENDFINALLY:
  case IL_ENDFILTER:
    return -100; /* Signal branch opcode for relooper */

  default:
    /* Unknown opcode */
    return -1;
  }

  return 0;
}
