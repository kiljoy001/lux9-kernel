/* cil_to_wasm.c - Direct CIL to WASM translator
 *
 * This translator emits WASM bytecode directly from CIL without going through
 * the Fruity IR layer. It uses WASM's native stack (i64 as the universal type)
 * rather than a soft stack in linear memory.
 *
 * Security-sensitive operations (memory allocation, 9P messaging, etc.) are
 * emitted as calls to Fruity host functions (lux_*, fruity_*, clr_*).
 */

#include "cil_to_wasm.h"
#include "../il_parser.h"
#include "cil_opcodes.h"
#include "cil_relooper.h"
#include "wasm_buffer.h"

#include "../../9front-pc64/mem.h"
#include "../../include/dat.h"
#include "../../include/error.h"
#include "../../include/fns.h"
#include "../../include/u.h"
#include "../../port/lib.h"

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

extern il_assembly_t *current_assembly;

#define NUM_HOST_IMPORTS 30
#define MAX_METHODS 256

/* Global method token → WASM func_idx mapping table (row-indexed) */
static u32int method_func_map[MAX_METHODS];
static u32int method_func_map_count = 0;

/* Get WASM func_idx for a MethodDef row (1-indexed) */
u32int get_wasm_func_idx_for_row(u32int row) {
  if (row > 0 && row <= method_func_map_count) {
    return method_func_map[row - 1];
  }
  /* Fallback to calculation if not in map */
  return NUM_HOST_IMPORTS + (row - 1);
}

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

/* ========== Direct CIL→WASM Compilation ========== */

/*
 * cil_to_wasm_compile_method - Compile a single CIL method to WASM bytecode
 *
 * Uses Ramsey's "Beyond Relooper" algorithm for control flow translation.
 * The algorithm works by induction over the dominator tree, using reverse
 * postorder numbering to determine block nesting.
 *
 * Returns: 0 on success, negative on error
 */
int cil_to_wasm_compile_method(il_method_t *method, il_assembly_t *assembly,
                               wasm_buffer_t *buf) {
  if (!method || !buf)
    return -1;

  /* Use Ramsey algorithm exclusively for all methods */
  /* NOTE: Locals are already emitted by cil_to_wasm_build_module before calling
   * this */

  int err = reloop_compile_method_ramsey(method, assembly, buf);
  if (err < 0) {
    print("CIL-WASM: Ramsey algorithm failed: %d\n", err);
    return err;
  }

  /* NOTE: END opcode is emitted by cil_to_wasm_build_module after this returns
   */

  return 0;
}

/*
 * cil_to_wasm_emit_one_opcode - Emit a single CIL opcode to WASM
 *
 * This is a thin wrapper around the centralized cil_emit_opcode function.
 * The implementation lives in cil_opcodes.c for cleaner code organization.
 *
 * Parameters:
 *   buf: Output WASM buffer
 *   il: CIL bytecode
 *   offset: Pointer to current offset (updated after emit)
 *   il_size: Total size of IL
 *
 * Returns: 0 on success, negative on error, -100 for branch opcodes
 */
int cil_to_wasm_emit_one_opcode(wasm_buffer_t *buf, u8int *il, u32int *offset,
                                u32int il_size, cil_wasm_ctx_t *ctx) {
  return cil_emit_opcode(buf, il, offset, il_size, ctx);
}

/*
 * cil_to_wasm_emit_function_header - Emit WASM function header
 *
 * Emits the local variable declarations for a WASM function.
 */

/* Helper to read compressed unsigned integer from blob signature */
static u32int read_blob_compressed_u32(u8int **ptr) {
  u8int b1 = *(*ptr)++;
  if ((b1 & 0x80) == 0) {
    return b1;
  } else if ((b1 & 0xC0) == 0x80) {
    u8int b2 = *(*ptr)++;
    return ((b1 & 0x3F) << 8) | b2;
  } else {
    u8int b2 = *(*ptr)++;
    u8int b3 = *(*ptr)++;
    u8int b4 = *(*ptr)++;
    return ((b1 & 0x1F) << 24) | (b2 << 16) | (b3 << 8) | b4;
  }
}

int cil_to_wasm_emit_locals(wasm_buffer_t *buf, il_method_t *method,
                            il_assembly_t *assembly) {
  u32int local_count = 0;

  if (method && method->local_var_sig_token && assembly) {
    u32int rid = method->local_var_sig_token & 0x00FFFFFF;
    standalonesig_row_t *row = il_get_standalonesig(assembly, rid);

    if (row) {
      u8int *sig = assembly->blob_heap + row->signature;
      read_blob_compressed_u32(&sig); // Read length
      if (*sig == 0x07) {             // ELEMENT_TYPE_VAR
        sig++;                        // Skip 0x07
        local_count = read_blob_compressed_u32(&sig);
      }
    }
  }

  /* Always emit at least one group for CIL locals + 1 scratch local */
  wasm_emit_uleb128(buf, 1);
  wasm_emit_uleb128(buf, local_count + 1); /* +1 for scratch local */
  wasm_emit_u8(buf, WASM_TYPE_I64);        // All locals are I64

  return local_count;
}

/* ========== Complete WASM Module Builder ========== */

u8int get_wasm_return_type(il_assembly_t *assembly, il_method_t *method) {
  if (!assembly || !method || !method->signature_index || !assembly->blob_heap)
    return 0; /* Void */

  u8int *blob = assembly->blob_heap + method->signature_index;

  /* Blob format: Length (compressed) + Data */
  u32int length = read_blob_compressed_u32(&blob);
  if (!length)
    return 0;

  /* Signature format: CallConv (1) + ParamCount (compressed) + RetType */
  /* Skip CallConv */
  blob++;

  /* Read ParamCount */
  u32int param_count = read_blob_compressed_u32(&blob);

  /* Read RetType */
  u8int element_type = *blob;              /* Peek first byte */
  int has_return = (element_type != 0x01); /* 0x01 = ELEMENT_TYPE_VOID */

  /* Map (param_count, has_return) to type index:
   * Type 0: () -> ()      (0 args, no return)
   * Type 1: (I) -> I      (1 arg, has return)
   * Type 2: (II) -> I     (2 args, has return)
   * Type 3: (II) -> ()    (2 args, no return)
   * Type 4: (III) -> ()   (3 args, no return)
   * Type 5: (III) -> I    (3 args, has return)
   * Type 6: (IIII) -> ()  (4 args, no return)
   * Type 7: () -> I       (0 args, has return)
   * Type 8: (I) -> ()     (1 arg, no return)
   */
  if (param_count == 0) {
    return has_return ? 7 : 0;
  } else if (param_count == 1) {
    return has_return ? 1 : 8;
  } else if (param_count == 2) {
    return has_return ? 2 : 3;
  } else if (param_count == 3) {
    return has_return ? 5 : 4;
  } else if (param_count == 4) {
    return has_return ? 5 : 6; /* Fallback to type 5/6 for 4+ args */
  } else {
    /* Fallback for >4 args - use closest match */
    return has_return ? 5 : 6;
  }
}

u32int get_wasm_arg_count(il_assembly_t *assembly, il_method_t *method) {
  if (!assembly || !method || !method->signature_index || !assembly->blob_heap)
    return 0;

  u8int *blob = assembly->blob_heap + method->signature_index;
  read_blob_compressed_u32(&blob); /* length */
  blob++;                          /* CallConv */
  return read_blob_compressed_u32(&blob);
}

u32int cil_get_local_count(il_assembly_t *assembly, il_method_t *method) {
  u32int local_count = 0;
  if (method->local_var_sig_token && assembly) {
    u32int rid = method->local_var_sig_token & 0x00FFFFFF;
    standalonesig_row_t *row = il_get_standalonesig(assembly, rid);
    if (row) {
      u8int *sig = assembly->blob_heap + row->signature;
      read_blob_compressed_u32(&sig);
      if (*sig == 0x07) {
        sig++;
        local_count = read_blob_compressed_u32(&sig);
      }
    }
  }
  return local_count;
}

int cil_to_wasm_build_module(il_assembly_t *assembly, il_method_t **methods,
                             u32int method_count, const char *entry_method_name,
                             void **out_wasm, u32int *out_len) {
  wasm_buffer_t module_buf, type_sec, import_sec, func_sec, export_sec,
      code_sec;
  u32int i;

  if (!methods || method_count == 0 || !assembly)
    return -1;

  wasm_buf_init(&module_buf, 4096);
  wasm_buf_init(&type_sec, 256);
  wasm_buf_init(&import_sec, 256);
  wasm_buf_init(&func_sec, 64);
  wasm_buf_init(&export_sec, 128);
  wasm_buf_init(&code_sec, 1024);

  /* Compile all function bodies first */
  /* Using simpler wasm_buffer_t array allocation */
  wasm_buffer_t *body_bufs = xalloc(sizeof(wasm_buffer_t) * method_count);

  for (i = 0; i < method_count; i++) {
    print("CIL-WASM: Compiling method %d/%d: %s\n", i + 1, method_count,
          methods[i] ? (methods[i]->name ? methods[i]->name : "(unnamed)")
                     : "(NULL)");
    wasm_buf_init(&body_bufs[i], 256);

    /* Handle NULL/Invalid methods gracefully */
    if (!methods[i]) {
      print("CIL-WASM: Skipping NULL method at index %d (emitting stub)\n", i);
      wasm_emit_uleb128(&body_bufs[i], 0); /* No locals */
      wasm_emit_u8(&body_bufs[i], WASM_OP_UNREACHABLE);
      wasm_emit_u8(&body_bufs[i], WASM_OP_END);
      continue;
    }

    /* Emit locals */
    cil_to_wasm_emit_locals(&body_bufs[i], methods[i], assembly);

    /* Compile method body using Ramsey algorithm */
    int err = cil_to_wasm_compile_method(methods[i], assembly, &body_bufs[i]);
    if (err < 0) {
      print("CIL-WASM: Failed to compile method '%s': %d\n",
            methods[i]->name ? methods[i]->name : "?", err);
    }

    /* End of function body */
    wasm_emit_u8(&body_bufs[i], WASM_OP_END);
  }

  print("CIL-WASM: Method compilation complete, building module...\n");

  /* ===== Module Header ===== */
  /* Magic + Version */
  wasm_emit_u8(&module_buf, 0x00);
  wasm_emit_u8(&module_buf, 0x61);
  wasm_emit_u8(&module_buf, 0x73);
  wasm_emit_u8(&module_buf, 0x6D);
  wasm_emit_u8(&module_buf, 0x01);
  wasm_emit_u8(&module_buf, 0x00);
  wasm_emit_u8(&module_buf, 0x00);
  wasm_emit_u8(&module_buf, 0x00);

  /* ===== Type Section (1) ===== */
  u32int total_types = 9;
  wasm_emit_uleb128(&type_sec, total_types);

  /* Type 0: () -> () (Void-Void) */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_u8(&type_sec, 0x00);
  wasm_emit_u8(&type_sec, 0x00);

  /* Type 1: (I) -> I */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 1);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_uleb128(&type_sec, 1);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Type 2: (II) -> I */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 2);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_uleb128(&type_sec, 1);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Type 3: (II) -> () */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 2);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, 0x00);

  /* Type 4: (III) -> () */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 3);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, 0x00);

  /* Type 5: (III) -> I */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 3);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_uleb128(&type_sec, 1);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Type 6: (IIII) -> () */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 4);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, 0x00);

  /* Type 7: () -> i64 */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_u8(&type_sec, 0x00);
  wasm_emit_uleb128(&type_sec, 1);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);

  /* Type 8: (I64) -> () */
  wasm_emit_u8(&type_sec, 0x60);
  wasm_emit_uleb128(&type_sec, 1);
  wasm_emit_u8(&type_sec, WASM_TYPE_I64);
  wasm_emit_u8(&type_sec, 0x00);

  /* ===== Import Section (2) ===== */
  wasm_emit_uleb128(&import_sec, NUM_HOST_IMPORTS);

#define EMIT_IMPORT(name, type_idx)                                            \
  wasm_emit_name(&import_sec, "env");                                          \
  wasm_emit_name(&import_sec, name);                                           \
  wasm_emit_u8(&import_sec, 0x00);                                             \
  wasm_emit_uleb128(&import_sec, type_idx);

  EMIT_IMPORT("clr_newobj", 1);
  EMIT_IMPORT("clr_newarr", 2);
  EMIT_IMPORT("clr_string_from_literal", 1);
  EMIT_IMPORT("clr_ldsfld", 1);
  EMIT_IMPORT("clr_stsfld", 3);
  EMIT_IMPORT("clr_ldfld", 2);
  EMIT_IMPORT("clr_stfld", 4);
  EMIT_IMPORT("clr_ldflda", 2);
  EMIT_IMPORT("clr_ldsflda", 1);
  EMIT_IMPORT("clr_ldlen", 1);
  EMIT_IMPORT("clr_box", 2);
  EMIT_IMPORT("clr_unbox", 2);
  EMIT_IMPORT("clr_isinst", 2);
  EMIT_IMPORT("clr_initobj", 3);
  EMIT_IMPORT("clr_ldelem", 5);
  EMIT_IMPORT("clr_stelem", 6);
  EMIT_IMPORT("clr_ldelema", 5);
  /* Symbolic computing host imports (17-24) */
  EMIT_IMPORT("sym_create", 1);           /* 17: (I) -> I */
  EMIT_IMPORT("sym_expr", 5);             /* 18: (III) -> I */
  EMIT_IMPORT("sym_diff", 2);             /* 19: (II) -> I */
  EMIT_IMPORT("sym_integrate", 2);        /* 20: (II) -> I */
  EMIT_IMPORT("sym_simplify", 1);         /* 21: (I) -> I */
  EMIT_IMPORT("sym_eval", 2);             /* 22: (II) -> I */
  EMIT_IMPORT("sym_match", 2);            /* 23: (II) -> I */
  EMIT_IMPORT("sym_rewrite", 2);          /* 24: (II) -> I */
  EMIT_IMPORT("cap_check_permission", 8); /* 25 */
  EMIT_IMPORT("lux9_send_9p", 2);         /* 26: (II) -> I */
  EMIT_IMPORT("lux9_debug_print", 8);     /* 27: (I) -> () */
  EMIT_IMPORT("lux9_yield", 0);           /* 28: () -> () */
  EMIT_IMPORT("lux9_print_i64", 8);       /* 29: (I) -> () */

#undef EMIT_IMPORT

  /* ===== Function Section (3) ===== */
  wasm_emit_uleb128(&func_sec, method_count);

  u32int entry_func_idx = 0xFFFFFFFF;

  for (i = 0; i < method_count; i++) {
    il_method_t *meth = methods[i];
    u32int type_idx = 0;

    if (meth) {
      type_idx = get_wasm_return_type(assembly, meth);
      /* Store the WASM func_idx in the method for call resolution */
      meth->wasm_func_idx = NUM_HOST_IMPORTS + i;

      /* Also populate global method_func_map for call opcode resolution */
      if (meth->method_token) {
        u32int row = meth->method_token & 0x00FFFFFF;
        if (row > 0 && row <= MAX_METHODS) {
          method_func_map[row - 1] = NUM_HOST_IMPORTS + i;
          if (row > method_func_map_count) {
            method_func_map_count = row;
          }
        }
      }

      print("CIL: Assigning WASM func_idx=%d to method '%s' -> Type %d\n",
            NUM_HOST_IMPORTS + i, meth->name ? meth->name : "?", type_idx);

      /* Check if this is entry point */
      if (entry_method_name && meth->name &&
          strcmp(meth->name, entry_method_name) == 0) {
        entry_func_idx = NUM_HOST_IMPORTS + i;
      }
    } else {
      print("CIL: Assigning WASM func_idx=%d to NULL -> Type 0\n",
            NUM_HOST_IMPORTS + i);
    }

    wasm_emit_uleb128(&func_sec, type_idx);
  }

  /* ===== Export Section (7) ===== */
  u32int export_count = 1; /* Memory */
  if (entry_func_idx != 0xFFFFFFFF)
    export_count++;

  wasm_emit_uleb128(&export_sec, export_count);

  /* Export Memory */
  wasm_emit_name(&export_sec, "memory");
  wasm_emit_u8(&export_sec, 0x02);   /* Kind: Memory */
  wasm_emit_uleb128(&export_sec, 0); /* Index 0 */

  /* Export Entry Point */
  if (entry_func_idx != 0xFFFFFFFF) {
    wasm_emit_name(&export_sec, entry_method_name);
    wasm_emit_u8(&export_sec, 0x00); /* Kind: Function */
    wasm_emit_uleb128(&export_sec, entry_func_idx);
    print("CIL: Exporting entry point '%s' as func_idx=%d\n", entry_method_name,
          entry_func_idx);
  } else {
    print("CIL: Warning: Entry point '%s' not found to export\n",
          entry_method_name ? entry_method_name : "NULL");
  }

  /* ===== Code Section (10) ===== */
  wasm_emit_uleb128(&code_sec, method_count);
  for (i = 0; i < method_count; i++) {
    /* Write body size + body */
    wasm_emit_uleb128(&code_sec, body_bufs[i].size);
    wasm_emit_bytes(&code_sec, body_bufs[i].data, body_bufs[i].size);
  }

  /* ===== Assemble Final Module ===== */

  /* Type (1) */
  wasm_emit_u8(&module_buf, 0x01);
  wasm_emit_uleb128(&module_buf, type_sec.size);
  wasm_emit_bytes(&module_buf, type_sec.data, type_sec.size);

  /* Import (2) */
  wasm_emit_u8(&module_buf, 0x02);
  wasm_emit_uleb128(&module_buf, import_sec.size);
  wasm_emit_bytes(&module_buf, import_sec.data, import_sec.size);

  /* Function (3) */
  wasm_emit_u8(&module_buf, 0x03);
  wasm_emit_uleb128(&module_buf, func_sec.size);
  wasm_emit_bytes(&module_buf, func_sec.data, func_sec.size);

  /* Manual Memory (5) */
  /* 4 bytes payload: 1 (count), 0x01 (flags), 1 (min), 16 (max) */
  wasm_emit_u8(&module_buf, 0x05);
  wasm_emit_uleb128(&module_buf, 4);
  wasm_emit_uleb128(&module_buf, 1);
  wasm_emit_u8(&module_buf, 0x01);
  wasm_emit_uleb128(&module_buf, 1);
  wasm_emit_uleb128(&module_buf, 16);

  /* Export (7) */
  wasm_emit_u8(&module_buf, 0x07);
  wasm_emit_uleb128(&module_buf, export_sec.size);
  wasm_emit_bytes(&module_buf, export_sec.data, export_sec.size);

  /* Code (10) */
  wasm_emit_u8(&module_buf, 0x0A);
  wasm_emit_uleb128(&module_buf, code_sec.size);
  wasm_emit_bytes(&module_buf, code_sec.data, code_sec.size);

  /* Output */
  *out_wasm = module_buf.data;
  *out_len = (u32int)module_buf.size;

  /* Cleanup */
  for (i = 0; i < method_count; i++)
    wasm_buf_free(&body_bufs[i]);

  wasm_buf_free(&type_sec);
  wasm_buf_free(&import_sec);
  wasm_buf_free(&func_sec);
  wasm_buf_free(&export_sec);
  wasm_buf_free(&code_sec);

  return 0;
}
