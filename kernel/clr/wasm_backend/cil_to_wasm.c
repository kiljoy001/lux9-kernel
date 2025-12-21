/* cil_to_wasm.c - Direct CIL to WASM translator */

#include "../il_parser.h"
#include "../il_to_fruity.h" /* For IL_ constants */
#include "wasm_buffer.h"

#ifdef USERSPACE_TEST
#include <stdlib.h>
#include <string.h>
extern void *xalloc(size_t size);
extern void xfree(void *ptr);
#else
#define _U_H_ /* Avoid include maze */
#include "../../include/fns.h"
#include "../../port/lib.h"
#endif

/* WASM Opcodes */
#define WASM_OP_END 0x0B
#define WASM_OP_RETURN 0x0F
#define WASM_OP_CALL 0x10
#define WASM_OP_DROP 0x1A
#define WASM_OP_LOCAL_GET 0x20
#define WASM_OP_LOCAL_SET 0x21
#define WASM_OP_I32_LOAD 0x28
#define WASM_OP_I32_LOAD8_S 0x2C
#define WASM_OP_I32_STORE 0x36
#define WASM_OP_I32_CONST 0x41
#define WASM_OP_I64_CONST 0x42
#define WASM_OP_BR 0x0C
#define WASM_OP_BR_IF 0x0D
#define WASM_OP_I32_EQZ 0x45
#define WASM_OP_I32_ADD 0x6A
#define WASM_OP_I32_SUB 0x6B
#define WASM_OP_I32_MUL 0x6C
#define WASM_OP_I32_DIV_S 0x6D
#define WASM_OP_I32_REM_S 0x6F
#define WASM_OP_I32_AND 0x71
#define WASM_OP_I32_OR 0x72
#define WASM_OP_I32_XOR 0x73

/* Helper: Emit CIL opcode mapping to WASM */
int cil_to_wasm_compile_method(il_method_t *method, wasm_buffer_t *buf) {
  if (!method || !buf)
    return -1;

  u8int *il = method->il_code;
  u32int il_size = method->il_code_size;
  u32int offset = 0;

  while (offset < il_size) {
    u16int opcode = il[offset++];
    if (opcode == 0xFE) {
      opcode = (opcode << 8) | il[offset++];
    }

    switch (opcode) {
    case IL_NOP:
      break;

    /* Arithmetic */
    case IL_ADD:
      wasm_emit_u8(buf, WASM_OP_I32_ADD);
      break;
    case IL_SUB:
      wasm_emit_u8(buf, WASM_OP_I32_SUB);
      break;
    case IL_MUL:
      wasm_emit_u8(buf, WASM_OP_I32_MUL);
      break;
    case IL_DIV:
      wasm_emit_u8(buf, WASM_OP_I32_DIV_S);
      break;

    /* Constants */
    case IL_LDC_I4_0:
      wasm_emit_u8(buf, WASM_OP_I32_CONST);
      wasm_emit_sleb128(buf, 0);
      break;
    case IL_LDC_I4_1:
      wasm_emit_u8(buf, WASM_OP_I32_CONST);
      wasm_emit_sleb128(buf, 1);
      break;
    case IL_LDC_I4_S: {
      s8int val = (s8int)il[offset++];
      wasm_emit_u8(buf, WASM_OP_I32_CONST);
      wasm_emit_sleb128(buf, val);
      break;
    }
    case IL_LDC_I4: {
      s32int val = *(s32int *)&il[offset];
      offset += 4;
      wasm_emit_u8(buf, WASM_OP_I32_CONST);
      wasm_emit_sleb128(buf, val);
      break;
    }

    /* Stack */
    case IL_POP:
      wasm_emit_u8(buf, WASM_OP_DROP);
      break;

    /* Locals & Args (Simplified mapping) */
    case IL_LDARG_0:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 0);
      break;
    case IL_LDLOC_0:
      wasm_emit_u8(buf, WASM_OP_LOCAL_GET);
      wasm_emit_uleb128(buf, 4); /* Assuming 4 args for now */
      break;
    case IL_STLOC_0:
      wasm_emit_u8(buf, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(buf, 4);
      break;

    /* Arrays (Simplified: no bounds check yet) */
    case IL_LDELEM_I4:
      /* Pop index, Pop array (WASM stack: [base, index]) */
      /* Base of data is at array + 4 (length) */
      wasm_emit_u8(buf, WASM_OP_I32_CONST);
      wasm_emit_sleb128(buf, 4);
      wasm_emit_u8(buf, WASM_OP_I32_ADD);
      /* index * 4 */
      wasm_emit_u8(buf, WASM_OP_I32_CONST);
      wasm_emit_sleb128(buf, 4);
      wasm_emit_u8(buf, WASM_OP_I32_MUL);
      wasm_emit_u8(buf, WASM_OP_I32_ADD);
      /* Load */
      wasm_emit_u8(buf, WASM_OP_I32_LOAD);
      wasm_emit_uleb128(buf, 2); /* align 2 (4 bytes) */
      wasm_emit_uleb128(buf, 0); /* offset 0 */
      break;

    case IL_LDLEN:
      /* Array length is at array[0] */
      wasm_emit_u8(buf, WASM_OP_I32_LOAD);
      wasm_emit_uleb128(buf, 2);
      wasm_emit_uleb128(buf, 0);
      break;

    /* Control Flow */
    case IL_RET:
      wasm_emit_u8(buf, WASM_OP_RETURN);
      break;

    case IL_BR_S:
      offset += 1; /* Skip placeholder */
      wasm_emit_u8(buf, WASM_OP_BR);
      wasm_emit_uleb128(buf, 0); /* Depth placeholder */
      break;

    default:
      /* Skip operands for unknown opcodes to avoid offset desync */
      return -2;
    }
  }

  return 0;
}
