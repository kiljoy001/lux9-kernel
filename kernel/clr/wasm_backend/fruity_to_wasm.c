/* fruity_to_wasm.c - Fruity IR to WASM Converter implementation */

#include "fruity_to_wasm.h"
#include "wasm_buffer.h"

#ifdef USERSPACE_TEST
#include <stdlib.h>
#include <string.h>
extern void *xalloc(size_t size);
extern void xfree(void *ptr);
#else
#include "../../include/fns.h" /* xalloc */
#include "../../include/lib.h" /* memset, memmove */
#endif

/* WASM Opcodes */
#define WASM_OP_UNREACHABLE 0x00
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

#define WASM_OP_LOCAL_GET 0x20
#define WASM_OP_LOCAL_SET 0x21
#define WASM_OP_LOCAL_TEE 0x22
#define WASM_OP_GLOBAL_GET 0x23
#define WASM_OP_GLOBAL_SET 0x24

#define WASM_OP_I64_LOAD 0x29
#define WASM_OP_I64_STORE 0x37

#define WASM_OP_I32_CONST 0x41
#define WASM_OP_I64_CONST 0x42

#define WASM_OP_I32_EQZ 0x45

#define WASM_OP_I64_EQZ 0x50
#define WASM_OP_I64_EQ 0x51
#define WASM_OP_I64_NE 0x52
#define WASM_OP_I64_LT_S 0x53
#define WASM_OP_I64_GT_S 0x55
#define WASM_OP_I64_LE_S 0x57
#define WASM_OP_I64_GE_S 0x59

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

#define WASM_OP_I64_EXTEND_I32_S 0xAC
#define WASM_OP_I64_EXTEND_I32_U 0xAD
#define WASM_OP_I32_WRAP_I64 0xA7

/* Types */
#define WASM_TYPE_I32 0x7F
#define WASM_TYPE_I64 0x7E
#define WASM_TYPE_FUNC 0x60
#define WASM_TYPE_EMPTY 0x40

/* WASM Section IDs */
#define WASM_SEC_TYPE 1
#define WASM_SEC_IMPORT 2
#define WASM_SEC_FUNCTION 3
#define WASM_SEC_MEMORY 5
#define WASM_SEC_GLOBAL 6
#define WASM_SEC_EXPORT 7
#define WASM_SEC_CODE 10

#define CALL_SCRATCH_MAX 16

/* Helpers */
static void emit_push_i64_from_local(wasm_buffer_t *code, u32int stack_ptr_local,
                                     u32int value_local) {
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, stack_ptr_local);
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, value_local);
  wasm_emit_u8(code, WASM_OP_I64_STORE);
  wasm_emit_uleb128(code, 3);
  wasm_emit_uleb128(code, 0);

  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, stack_ptr_local);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 8);
  wasm_emit_u8(code, 0x6A); /* i32.add */
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, stack_ptr_local);
}

static void emit_push_i64_const(wasm_buffer_t *code, u32int stack_ptr_local,
                                u32int scratch_local, s64int value) {
  wasm_emit_u8(code, WASM_OP_I64_CONST);
  wasm_emit_sleb128(code, value);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, scratch_local);
  emit_push_i64_from_local(code, stack_ptr_local, scratch_local);
}

static void emit_pop_i64_to_local(wasm_buffer_t *code, u32int stack_ptr_local,
                                  u32int dst_local) {
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, stack_ptr_local);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 8);
  wasm_emit_u8(code, 0x6B); /* i32.sub */
  wasm_emit_u8(code, WASM_OP_LOCAL_TEE);
  wasm_emit_uleb128(code, stack_ptr_local);

  wasm_emit_u8(code, WASM_OP_I64_LOAD);
  wasm_emit_uleb128(code, 3);
  wasm_emit_uleb128(code, 0);

  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, dst_local);
}

static void emit_peek_i64_to_local(wasm_buffer_t *code, u32int stack_ptr_local,
                                   u32int dst_local) {
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, stack_ptr_local);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 8);
  wasm_emit_u8(code, 0x6B); /* i32.sub */
  wasm_emit_u8(code, WASM_OP_I64_LOAD);
  wasm_emit_uleb128(code, 3);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, dst_local);
}

static void emit_binary_op_i64(wasm_buffer_t *code, u32int stack_ptr_local,
                               u32int scratch_a, u32int scratch_b, u8int op) {
  emit_pop_i64_to_local(code, stack_ptr_local, scratch_b);
  emit_pop_i64_to_local(code, stack_ptr_local, scratch_a);
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, scratch_a);
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, scratch_b);
  wasm_emit_u8(code, op);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, scratch_a);
  emit_push_i64_from_local(code, stack_ptr_local, scratch_a);
}

static void emit_compare_i64(wasm_buffer_t *code, u32int stack_ptr_local,
                             u32int scratch_a, u32int scratch_b, u8int op) {
  emit_pop_i64_to_local(code, stack_ptr_local, scratch_b);
  emit_pop_i64_to_local(code, stack_ptr_local, scratch_a);
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, scratch_a);
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, scratch_b);
  wasm_emit_u8(code, op);
  wasm_emit_u8(code, WASM_OP_I64_EXTEND_I32_U);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, scratch_a);
  emit_push_i64_from_local(code, stack_ptr_local, scratch_a);
}

static void emit_set_target_and_jump(wasm_buffer_t *code, u32int target_local,
                                     u32int block_id, u32int depth) {
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, block_id);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, target_local);
  wasm_emit_u8(code, WASM_OP_BR);
  wasm_emit_uleb128(code, depth);
}

static u8int valtype_to_wasm(clr_value_type_t t) {
  switch (t) {
  case CLR_INT32:
  case CLR_BOOL:
    return WASM_TYPE_I32;
  case CLR_INT64:
  case CLR_REF:
  default:
    return WASM_TYPE_I64;
  }
}

/* Resolve function index in module list */
static fruity_function_t *find_function_by_token(fruity_module_t *module,
                                                 u32int method_token) {
  for (fruity_function_t *f = module->functions_head; f; f = f->next) {
    if (f->method_token == method_token)
      return f;
  }
  return nil;
}

static int resolve_function_index(fruity_module_t *module, u32int method_token) {
  int idx = 0;
  for (fruity_function_t *f = module->functions_head; f; f = f->next, idx++) {
    if (f->method_token == method_token)
      return idx;
  }
  return -1;
}

static int resolve_function_index_by_name(fruity_module_t *module,
                                          const char *name) {
  int idx = 0;
  for (fruity_function_t *f = module->functions_head; f; f = f->next, idx++) {
    if (f->name && name && strcmp(f->name, name) == 0)
      return idx;
  }
  return -1;
}

typedef struct {
  int lux_alloc;
  int lux_addref;
  int lux_release;
  int lux_snapshot;
  int lux_commit;
  int lux_rollback;
  int clr_string_from_literal;
  int clr_get_type_size;
  int clr_get_static_field;
  int clr_is_instance_of;
  int clr_ptr_add;
  int clr_load_i64;
  int clr_store_i64;
  int clr_memmove;
  int clr_memset;
  int clr_newobj;
  int clr_newarr;
  int clr_array_len;
  int clr_array_get;
  int clr_array_set;
  int clr_array_elem_addr;
  int clr_box;
  int clr_unbox;
  int clr_unbox_any;
  int clr_initobj;
  int clr_cpobj;
  int clr_ldobj;
  int clr_stobj;
  int clr_throw;
} runtime_imports_t;

static void resolve_runtime_imports(fruity_module_t *module,
                                    runtime_imports_t *imp) {
  memset(imp, 0xFF, sizeof(*imp));
  imp->lux_alloc = resolve_function_index_by_name(module, "lux_alloc");
  imp->lux_addref = resolve_function_index_by_name(module, "lux_addref");
  imp->lux_release = resolve_function_index_by_name(module, "lux_release");
  imp->lux_snapshot = resolve_function_index_by_name(module, "lux_snapshot");
  imp->lux_commit = resolve_function_index_by_name(module, "lux_commit");
  imp->lux_rollback = resolve_function_index_by_name(module, "lux_rollback");
  imp->clr_string_from_literal =
      resolve_function_index_by_name(module, "clr_string_from_literal");
  imp->clr_get_type_size =
      resolve_function_index_by_name(module, "clr_get_type_size");
  imp->clr_get_static_field =
      resolve_function_index_by_name(module, "clr_get_static_field");
  imp->clr_is_instance_of =
      resolve_function_index_by_name(module, "clr_is_instance_of");
  imp->clr_ptr_add = resolve_function_index_by_name(module, "clr_ptr_add");
  imp->clr_load_i64 = resolve_function_index_by_name(module, "clr_load_i64");
  imp->clr_store_i64 =
      resolve_function_index_by_name(module, "clr_store_i64");
  imp->clr_memmove = resolve_function_index_by_name(module, "clr_memmove");
  imp->clr_memset = resolve_function_index_by_name(module, "clr_memset");
  imp->clr_newobj = resolve_function_index_by_name(module, "clr_newobj");
  imp->clr_newarr = resolve_function_index_by_name(module, "clr_newarr");
  imp->clr_array_len =
      resolve_function_index_by_name(module, "clr_array_len");
  imp->clr_array_get =
      resolve_function_index_by_name(module, "clr_array_get");
  imp->clr_array_set =
      resolve_function_index_by_name(module, "clr_array_set");
  imp->clr_array_elem_addr =
      resolve_function_index_by_name(module, "clr_array_elem_addr");
  imp->clr_box = resolve_function_index_by_name(module, "clr_box");
  imp->clr_unbox = resolve_function_index_by_name(module, "clr_unbox");
  imp->clr_unbox_any = resolve_function_index_by_name(module, "clr_unbox_any");
  imp->clr_initobj = resolve_function_index_by_name(module, "clr_initobj");
  imp->clr_cpobj = resolve_function_index_by_name(module, "clr_cpobj");
  imp->clr_ldobj = resolve_function_index_by_name(module, "clr_ldobj");
  imp->clr_stobj = resolve_function_index_by_name(module, "clr_stobj");
  imp->clr_throw = resolve_function_index_by_name(module, "clr_throw");
}

static void emit_call_import(wasm_buffer_t *code, int idx) {
  if (idx < 0)
    return;
  wasm_emit_u8(code, WASM_OP_CALL);
  wasm_emit_uleb128(code, (u32int)idx);
}

static void emit_call_throw(wasm_buffer_t *code, runtime_imports_t *imp) {
  if (imp->clr_throw < 0)
    return;
  wasm_emit_u8(code, WASM_OP_I64_CONST);
  wasm_emit_sleb128(code, 0);
  emit_call_import(code, imp->clr_throw);
}

static void emit_call_method(fruity_module_t *module, wasm_buffer_t *code,
                             runtime_imports_t *imp, u32int stack_ptr_local,
                             u32int scratch_a, u32int scratch_b,
                             u32int call_arg_base, u32int method_token) {
  fruity_function_t *target = find_function_by_token(module, method_token);
  int target_idx = resolve_function_index(module, method_token);
  if (target_idx < 0 || target == nil) {
    emit_call_throw(code, imp);
    return;
  }
  if (target->arg_count > CALL_SCRATCH_MAX) {
    emit_call_throw(code, imp);
    return;
  }

  /* Pop args into call_arg locals (reverse order) */
  for (int i = (int)target->arg_count - 1; i >= 0; i--) {
    emit_pop_i64_to_local(code, stack_ptr_local, call_arg_base + i);
  }

  /* Push args for call (in order) */
  for (u32int i = 0; i < target->arg_count; i++) {
    wasm_emit_u8(code, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(code, call_arg_base + i);
  }

  /* Call target */
  wasm_emit_u8(code, WASM_OP_CALL);
  wasm_emit_uleb128(code, (u32int)target_idx);

  if (target->return_type != CLR_VOID) {
    wasm_emit_u8(code, WASM_OP_LOCAL_SET);
    wasm_emit_uleb128(code, scratch_a);
    emit_push_i64_from_local(code, stack_ptr_local, scratch_a);
  }
}

/* Compile a single function body */
static int compile_function_body(fruity_module_t *module, fruity_function_t *func,
                                 runtime_imports_t *imp, wasm_buffer_t *code) {
  u32int arg_count = func->arg_count;
  u32int local_base = arg_count;

  u32int local_target = local_base + 0;      /* i32 */
  u32int local_frame_base = local_base + 1;  /* i32 */
  u32int local_stack_ptr = local_base + 2;   /* i32 */
  u32int local_i64_base = local_base + 3;    /* i64 */
  u32int local_scratch_a = local_i64_base + 0;
  u32int local_scratch_b = local_i64_base + 1;
  u32int local_scratch_c = local_i64_base + 2;
  u32int local_call_base = local_i64_base + 3;
  u32int local_user_base = local_i64_base + 3 + CALL_SCRATCH_MAX;

  u32int i32_locals = 3;
  u32int i64_locals = 3 + CALL_SCRATCH_MAX + func->local_count;

  /* Declare locals */
  wasm_emit_uleb128(code, 2); /* 2 groups */
  wasm_emit_uleb128(code, i32_locals);
  wasm_emit_u8(code, WASM_TYPE_I32);
  wasm_emit_uleb128(code, i64_locals);
  wasm_emit_u8(code, WASM_TYPE_I64);

  /* Frame layout */
  u32int args_offset = 0;
  u32int locals_offset = args_offset + (u32int)(arg_count * 8);
  u32int stack_offset = locals_offset + (u32int)(func->local_count * 8);
  u32int frame_size = stack_offset + (u32int)(func->max_stack_depth * 8);

  /* Prologue: frame_base = global_sp; global_sp += frame_size; stack_ptr = base + stack_offset */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, local_frame_base);

  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, frame_size);
  wasm_emit_u8(code, 0x6A); /* i32.add */
  wasm_emit_u8(code, WASM_OP_GLOBAL_SET);
  wasm_emit_uleb128(code, 0);

  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, local_frame_base);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, stack_offset);
  wasm_emit_u8(code, 0x6A); /* i32.add */
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, local_stack_ptr);

  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, local_target);

  /* Store args into frame memory */
  for (u32int i = 0; i < arg_count; i++) {
    wasm_emit_u8(code, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(code, local_frame_base);
    wasm_emit_u8(code, WASM_OP_I32_CONST);
    wasm_emit_sleb128(code, args_offset + (i * 8));
    wasm_emit_u8(code, 0x6A); /* i32.add */
    wasm_emit_u8(code, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(code, i);
    wasm_emit_u8(code, WASM_OP_I64_STORE);
    wasm_emit_uleb128(code, 3);
    wasm_emit_uleb128(code, 0);
  }

  /* Zero locals */
  for (u32int i = 0; i < func->local_count; i++) {
    wasm_emit_u8(code, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(code, local_frame_base);
    wasm_emit_u8(code, WASM_OP_I32_CONST);
    wasm_emit_sleb128(code, locals_offset + (i * 8));
    wasm_emit_u8(code, 0x6A); /* i32.add */
    wasm_emit_u8(code, WASM_OP_I64_CONST);
    wasm_emit_sleb128(code, 0);
    wasm_emit_u8(code, WASM_OP_I64_STORE);
    wasm_emit_uleb128(code, 3);
    wasm_emit_uleb128(code, 0);
  }

  /* Loop-Switch Wrapper */
  wasm_emit_u8(code, WASM_OP_BLOCK);
  wasm_emit_u8(code, WASM_TYPE_EMPTY);
  wasm_emit_u8(code, WASM_OP_LOOP);
  wasm_emit_u8(code, WASM_TYPE_EMPTY);

  ulong block_count = func->block_count;

  for (ulong i = 0; i < block_count; i++) {
    wasm_emit_u8(code, WASM_OP_BLOCK);
    wasm_emit_u8(code, WASM_TYPE_EMPTY);
  }

  wasm_emit_u8(code, WASM_OP_BLOCK); /* Dispatch block */
  wasm_emit_u8(code, WASM_TYPE_EMPTY);

  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, local_target);

  wasm_emit_u8(code, WASM_OP_BR_TABLE);
  wasm_emit_uleb128(code, block_count);
  for (ulong i = 0; i < block_count; i++) {
    wasm_emit_uleb128(code, i + 1);
  }
  wasm_emit_uleb128(code, block_count);

  wasm_emit_u8(code, WASM_OP_END); /* End Dispatch block */

  fruity_basic_block_t *bb = func->blocks_head;
  for (ulong i = 0; i < block_count; i++) {
    fruity_basic_block_t *current_bb = bb;

    if (current_bb) {
      fruity_instruction_t *instr = current_bb->instructions_head;
      while (instr) {
        switch (instr->opcode) {
        case FRUITY_NOP:
        case FRUITY_BREAK:
          break;

        case FRUITY_LDC_I4:
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a,
                              instr->operand.value.i32);
          break;
        case FRUITY_LDC_I8:
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a,
                              instr->operand.value.i64);
          break;
        case FRUITY_LDC_R4: {
          u32int bits;
          float v = instr->operand.value.r32;
          memmove(&bits, &v, sizeof(bits));
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a, bits);
          break;
        }
        case FRUITY_LDC_R8: {
          u64int bits;
          double v = instr->operand.value.r64;
          memmove(&bits, &v, sizeof(bits));
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a,
                              (s64int)bits);
          break;
        }
        case FRUITY_LDNULL:
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a, 0);
          break;

        case FRUITY_ADD:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_ADD);
          break;
        case FRUITY_SUB:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_SUB);
          break;
        case FRUITY_MUL:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_MUL);
          break;
        case FRUITY_DIV:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_DIV_S);
          break;
        case FRUITY_DIV_UN:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_DIV_U);
          break;
        case FRUITY_REM:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_REM_S);
          break;
        case FRUITY_REM_UN:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_REM_U);
          break;
        case FRUITY_NEG:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, 0);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_SUB);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;

        case FRUITY_AND:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_AND);
          break;
        case FRUITY_OR:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_OR);
          break;
        case FRUITY_XOR:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_XOR);
          break;
        case FRUITY_NOT:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, -1);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_XOR);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_SHL:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_SHL);
          break;
        case FRUITY_SHR:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_SHR_S);
          break;
        case FRUITY_SHR_UN:
          emit_binary_op_i64(code, local_stack_ptr, local_scratch_a,
                             local_scratch_b, WASM_OP_I64_SHR_U);
          break;

        case FRUITY_CEQ:
          emit_compare_i64(code, local_stack_ptr, local_scratch_a,
                           local_scratch_b, WASM_OP_I64_EQ);
          break;
        case FRUITY_CNE:
          emit_compare_i64(code, local_stack_ptr, local_scratch_a,
                           local_scratch_b, WASM_OP_I64_NE);
          break;
        case FRUITY_CLT:
          emit_compare_i64(code, local_stack_ptr, local_scratch_a,
                           local_scratch_b, WASM_OP_I64_LT_S);
          break;
        case FRUITY_CLE:
          emit_compare_i64(code, local_stack_ptr, local_scratch_a,
                           local_scratch_b, WASM_OP_I64_LE_S);
          break;
        case FRUITY_CGT:
          emit_compare_i64(code, local_stack_ptr, local_scratch_a,
                           local_scratch_b, WASM_OP_I64_GT_S);
          break;
        case FRUITY_CGE:
          emit_compare_i64(code, local_stack_ptr, local_scratch_a,
                           local_scratch_b, WASM_OP_I64_GE_S);
          break;

        case FRUITY_CONV_I4:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I32_WRAP_I64);
          wasm_emit_u8(code, WASM_OP_I64_EXTEND_I32_S);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_CONV_I8:
        case FRUITY_CONV_R4:
        case FRUITY_CONV_R8:
          /* Kernel mode: leave as i64 */
          break;

        case FRUITY_DUP:
          emit_peek_i64_to_local(code, local_stack_ptr, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_DUP_REF:
          emit_peek_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_addref);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_POP:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_POP_REF:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_release);
          break;

        case FRUITY_LIME:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, 0);
          emit_call_import(code, imp->lux_alloc);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_VANILLA:
          emit_peek_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_addref);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_BURN:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_release);
          break;
        case FRUITY_LOAD_STRING:
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.i32);
          emit_call_import(code, imp->clr_string_from_literal);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;

        case FRUITY_CHERRY:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_snapshot);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_BERRY:
          emit_peek_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_commit);
          break;
        case FRUITY_ROLLBACK:
          emit_peek_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->lux_rollback);
          break;

        case FRUITY_GRAPE:
          /* MVP: no-op */
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_LEMON:
          /* Ownership move: no-op */
          break;

        case FRUITY_LOAD_LOCAL: {
          u32int idx = instr->operand.value.index;
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, locals_offset + (idx * 8));
          wasm_emit_u8(code, 0x6A);
          wasm_emit_u8(code, WASM_OP_I64_LOAD);
          wasm_emit_uleb128(code, 3);
          wasm_emit_uleb128(code, 0);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        }
        case FRUITY_STORE_LOCAL: {
          u32int idx = instr->operand.value.index;
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, locals_offset + (idx * 8));
          wasm_emit_u8(code, 0x6A);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_STORE);
          wasm_emit_uleb128(code, 3);
          wasm_emit_uleb128(code, 0);
          break;
        }
        case FRUITY_LOAD_LOCAL_ADDR: {
          u32int idx = instr->operand.value.index;
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, locals_offset + (idx * 8));
          wasm_emit_u8(code, 0x6A);
          wasm_emit_u8(code, WASM_OP_I64_EXTEND_I32_U);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        }
        case FRUITY_LOAD_ARG: {
          u32int idx = instr->operand.value.index;
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, args_offset + (idx * 8));
          wasm_emit_u8(code, 0x6A);
          wasm_emit_u8(code, WASM_OP_I64_LOAD);
          wasm_emit_uleb128(code, 3);
          wasm_emit_uleb128(code, 0);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        }
        case FRUITY_STORE_ARG: {
          u32int idx = instr->operand.value.index;
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, args_offset + (idx * 8));
          wasm_emit_u8(code, 0x6A);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_STORE);
          wasm_emit_uleb128(code, 3);
          wasm_emit_uleb128(code, 0);
          break;
        }
        case FRUITY_LOAD_ARG_ADDR: {
          u32int idx = instr->operand.value.index;
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, args_offset + (idx * 8));
          wasm_emit_u8(code, 0x6A);
          wasm_emit_u8(code, WASM_OP_I64_EXTEND_I32_U);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        }

        case FRUITY_LOAD_FIELD:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, instr->operand.value.i32);
          emit_call_import(code, imp->clr_ptr_add);
          emit_call_import(code, imp->clr_load_i64);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_STORE_FIELD:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, instr->operand.value.i32);
          emit_call_import(code, imp->clr_ptr_add);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_call_import(code, imp->clr_store_i64);
          break;
        case FRUITY_LOAD_STATIC:
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_get_static_field);
          emit_call_import(code, imp->clr_load_i64);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_STORE_STATIC:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_get_static_field);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_store_i64);
          break;
        case FRUITY_LDFLDA:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, instr->operand.value.i32);
          emit_call_import(code, imp->clr_ptr_add);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_LOAD_IND:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_load_i64);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_STORE_IND:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_call_import(code, imp->clr_store_i64);
          break;
        case FRUITY_MEMCPY:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_c);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_c);
          emit_call_import(code, imp->clr_memmove);
          break;
        case FRUITY_MEMSET:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_c);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_c);
          emit_call_import(code, imp->clr_memset);
          break;

        case FRUITY_CASTCLASS:
          emit_peek_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_is_instance_of);
          wasm_emit_u8(code, WASM_OP_I32_EQZ);
          wasm_emit_u8(code, WASM_OP_IF);
          wasm_emit_u8(code, WASM_TYPE_EMPTY);
          emit_call_throw(code, imp);
          wasm_emit_u8(code, WASM_OP_END);
          break;
        case FRUITY_ISINST:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_is_instance_of);
          wasm_emit_u8(code, WASM_OP_IF);
          wasm_emit_u8(code, WASM_TYPE_I64);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_ELSE);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, 0);
          wasm_emit_u8(code, WASM_OP_END);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_BOX:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_box);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_UNBOX:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_unbox);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_UNBOX_ANY:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_unbox_any);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_INITOBJ:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_initobj);
          break;
        case FRUITY_CPOBJ:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          wasm_emit_u8(code, WASM_OP_I64_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_cpobj);
          break;
        case FRUITY_LDOBJ:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_ldobj);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_STOBJ:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_call_import(code, imp->clr_stobj);
          break;
        case FRUITY_NEWOBJ:
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_newobj);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_NEWARR:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_newarr);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_LDLEN:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_array_len);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_b);
          break;
        case FRUITY_LDELEM:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_call_import(code, imp->clr_array_get);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_c);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_c);
          break;
        case FRUITY_STELEM:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_c);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_c);
          emit_call_import(code, imp->clr_array_set);
          break;
        case FRUITY_LDELEMA:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_b);
          emit_call_import(code, imp->clr_array_elem_addr);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_c);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_c);
          break;

        case FRUITY_THROW:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_call_import(code, imp->clr_throw);
          wasm_emit_u8(code, WASM_OP_UNREACHABLE);
          break;
        case FRUITY_RETHROW:
          emit_call_throw(code, imp);
          wasm_emit_u8(code, WASM_OP_UNREACHABLE);
          break;

        case FRUITY_LEAVE:
        case FRUITY_JUMP:
          emit_set_target_and_jump(code, local_target,
                                   instr->operand.value.target
                                       ? instr->operand.value.target->block_id
                                       : 0,
                                   (u32int)(block_count - i + 1));
          break;

        case FRUITY_RET:
          /* Restore global stack top to frame base */
          if (func->return_type != CLR_VOID) {
            emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
            wasm_emit_u8(code, WASM_OP_LOCAL_GET);
            wasm_emit_uleb128(code, local_scratch_a);
          }
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, local_frame_base);
          wasm_emit_u8(code, WASM_OP_GLOBAL_SET);
          wasm_emit_uleb128(code, 0);
          wasm_emit_u8(code, WASM_OP_RETURN);
          break;

        case FRUITY_BTRUE:
        case FRUITY_BFALSE:
        case FRUITY_BEQ:
        case FRUITY_BNE:
        case FRUITY_BLT:
        case FRUITY_BLE:
        case FRUITY_BGT:
        case FRUITY_BGE: {
          u8int cmp_op = 0;
          int unary = 0;
          switch (instr->opcode) {
          case FRUITY_BTRUE:
            unary = 1;
            cmp_op = WASM_OP_I64_EQZ; /* invert later */
            break;
          case FRUITY_BFALSE:
            unary = 1;
            cmp_op = WASM_OP_I64_EQZ;
            break;
          case FRUITY_BEQ:
            cmp_op = WASM_OP_I64_EQ;
            break;
          case FRUITY_BNE:
            cmp_op = WASM_OP_I64_NE;
            break;
          case FRUITY_BLT:
            cmp_op = WASM_OP_I64_LT_S;
            break;
          case FRUITY_BLE:
            cmp_op = WASM_OP_I64_LE_S;
            break;
          case FRUITY_BGT:
            cmp_op = WASM_OP_I64_GT_S;
            break;
          case FRUITY_BGE:
            cmp_op = WASM_OP_I64_GE_S;
            break;
          default:
            break;
          }

          if (unary) {
            emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
            wasm_emit_u8(code, WASM_OP_LOCAL_GET);
            wasm_emit_uleb128(code, local_scratch_a);
            wasm_emit_u8(code, cmp_op);
            if (instr->opcode == FRUITY_BTRUE) {
              wasm_emit_u8(code, WASM_OP_I32_EQZ); /* invert */
            }
          } else {
            emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_b);
            emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
            wasm_emit_u8(code, WASM_OP_LOCAL_GET);
            wasm_emit_uleb128(code, local_scratch_a);
            wasm_emit_u8(code, WASM_OP_LOCAL_GET);
            wasm_emit_uleb128(code, local_scratch_b);
            wasm_emit_u8(code, cmp_op);
          }

          wasm_emit_u8(code, WASM_OP_IF);
          wasm_emit_u8(code, WASM_TYPE_EMPTY);
          emit_set_target_and_jump(code, local_target,
                                   instr->operand.value.target
                                       ? instr->operand.value.target->block_id
                                       : 0,
                                   (u32int)(block_count - i + 1));
          wasm_emit_u8(code, WASM_OP_END);
          break;
        }

        case FRUITY_SWITCH: {
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          fruity_switch_targets_t *targets = instr->operand.value.switch_targets;
          if (targets && targets->count > 0) {
            wasm_emit_u8(code, WASM_OP_BLOCK);
            wasm_emit_u8(code, WASM_TYPE_EMPTY);
            for (u32int t = 0; t < targets->count; t++) {
              fruity_basic_block_t *tgt = targets->targets[t];
              if (!tgt)
                continue;
              wasm_emit_u8(code, WASM_OP_LOCAL_GET);
              wasm_emit_uleb128(code, local_scratch_a);
              wasm_emit_u8(code, WASM_OP_I64_CONST);
              wasm_emit_sleb128(code, (s64int)t);
              wasm_emit_u8(code, WASM_OP_I64_EQ);
              wasm_emit_u8(code, WASM_OP_IF);
              wasm_emit_u8(code, WASM_TYPE_EMPTY);
              emit_set_target_and_jump(code, local_target, tgt->block_id,
                                       (u32int)(block_count - i + 1));
              wasm_emit_u8(code, WASM_OP_END);
            }
            wasm_emit_u8(code, WASM_OP_END);
          }
          break;
        }

        case FRUITY_CALL:
          emit_call_method(module, code, imp, local_stack_ptr, local_scratch_a,
                           local_scratch_b, local_call_base,
                           instr->operand.value.token);
          break;
        case FRUITY_CALLI:
          /* MVP: treat as direct call via token on stack */
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          emit_call_throw(code, imp);
          break;
        case FRUITY_LDFTN:
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a,
                              instr->operand.value.token);
          break;
        case FRUITY_LDVIRTFTN:
          emit_pop_i64_to_local(code, local_stack_ptr, local_scratch_a);
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a,
                              instr->operand.value.token);
          break;

        case FRUITY_SIZEOF:
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.token);
          emit_call_import(code, imp->clr_get_type_size);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, local_scratch_a);
          emit_push_i64_from_local(code, local_stack_ptr, local_scratch_a);
          break;
        case FRUITY_LDTOKEN:
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a,
                              instr->operand.value.token);
          break;
        case FRUITY_ARGLIST:
          emit_push_i64_const(code, local_stack_ptr, local_scratch_a, 0);
          break;
        case FRUITY_JMP:
        case FRUITY_CKFINITE:
        case FRUITY_ENDFINALLY:
        case FRUITY_PREFIX_CONSTRAINED:
        case FRUITY_PREFIX_READONLY:
        case FRUITY_PREFIX_NO:
        case FRUITY_PREFIX_TAIL:
        case FRUITY_PREFIX_UNALIGNED:
        case FRUITY_PREFIX_VOLATILE:
        case FRUITY_MKREFANY:
        case FRUITY_REFANYVAL:
        case FRUITY_REFANYTYPE:
          /* MVP: no-op */
          break;

        default:
          emit_call_throw(code, imp);
          wasm_emit_u8(code, WASM_OP_UNREACHABLE);
          break;
        }

        instr = instr->next;
      }
      bb = current_bb->next;
    }

    int needs_fallthrough = 1;
    if (current_bb && current_bb->instructions_tail) {
      switch (current_bb->instructions_tail->opcode) {
      case FRUITY_RET:
      case FRUITY_JUMP:
      case FRUITY_LEAVE:
        needs_fallthrough = 0;
        break;
      default:
        break;
      }
    }

    if (needs_fallthrough) {
      emit_set_target_and_jump(code, local_target, (u32int)(i + 1),
                               (u32int)(block_count - i + 1));
    }

    wasm_emit_u8(code, WASM_OP_END); /* Close block */
  }

  wasm_emit_u8(code, WASM_OP_END); /* Close Loop */
  wasm_emit_u8(code, WASM_OP_END); /* Close Wrapper Block */
  wasm_emit_u8(code, WASM_OP_END); /* Function end */

  return 0;
}

int fruity_compile_to_wasm(fruity_module_t *module,
                           fruity_wasm_result_t *result) {
  if (!module || !result)
    return -1;

  wasm_buffer_t main_buf;
  wasm_buf_init(&main_buf, 4096);

  /* 1. Header */
  wasm_emit_u32(&main_buf, 0x6D736100);
  wasm_emit_u32(&main_buf, 1);

  /* Count functions */
  ulong func_count = 0;
  for (fruity_function_t *f = module->functions_head; f; f = f->next)
    func_count++;
  print("WASM: compiling module with %d functions\n", (int)func_count);

  /* 2. Type Section */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 128);
    wasm_emit_vec_header(&sec, func_count);

    ulong seen = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen < func_count; f = f->next, seen++) {
      if (f->import_info.is_import && f->name) {
        print("WASM: import %s args=%d ret=%d", f->name, (int)f->arg_count,
              (int)f->return_type);
        for (ulong ai = 0; ai < f->arg_count && f->arg_types; ai++) {
          print(" %d", (int)f->arg_types[ai]);
        }
        print("\n");
      }
      wasm_emit_u8(&sec, WASM_TYPE_FUNC);
      if (f->arg_count > 0 && f->arg_types == nil) {
        print("WASM: missing arg_types for fn=%p, forcing 0 args\n", f);
        wasm_emit_vec_header(&sec, 0);
      } else {
        wasm_emit_vec_header(&sec, f->arg_count);
        for (ulong i = 0; i < f->arg_count; i++) {
          wasm_emit_u8(&sec, valtype_to_wasm(f->arg_types[i]));
        }
      }
      if (f->return_type == CLR_VOID) {
        wasm_emit_vec_header(&sec, 0);
      } else {
        wasm_emit_vec_header(&sec, 1);
        wasm_emit_u8(&sec, valtype_to_wasm(f->return_type));
      }
    }

    wasm_emit_u8(&main_buf, WASM_SEC_TYPE);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 3. Import Section */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 128);

    ulong import_count = 0;
    ulong seen = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen < func_count; f = f->next, seen++) {
      if (f->import_info.is_import)
        import_count++;
    }

    if (import_count > 0) {
      wasm_emit_vec_header(&sec, import_count);
      seen = 0;
      for (fruity_function_t *f = module->functions_head;
           f && seen < func_count; f = f->next, seen++) {
        if (!f->import_info.is_import)
          continue;
        wasm_emit_name(&sec, f->import_info.module_name
                                 ? f->import_info.module_name
                                 : "env");
        wasm_emit_name(&sec, f->import_info.function_name
                                 ? f->import_info.function_name
                                 : f->name);
        wasm_emit_u8(&sec, 0x00);
        ulong type_idx = 0;
        ulong seen_type = 0;
        for (fruity_function_t *tf = module->functions_head;
             tf && seen_type < func_count; tf = tf->next, seen_type++) {
          if (tf == f)
            break;
          type_idx++;
        }
        wasm_emit_uleb128(&sec, type_idx);
      }

      wasm_emit_u8(&main_buf, WASM_SEC_IMPORT);
      wasm_emit_uleb128(&main_buf, sec.size);
      wasm_emit_bytes(&main_buf, sec.data, sec.size);
    }
    wasm_buf_free(&sec);
  }

  /* 4. Function Section */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 64);

    ulong local_func_count = 0;
    ulong seen = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen < func_count; f = f->next, seen++) {
      if (!f->import_info.is_import)
        local_func_count++;
    }

    wasm_emit_vec_header(&sec, local_func_count);

    ulong idx = 0;
    seen = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen < func_count; f = f->next, seen++) {
      if (!f->import_info.is_import)
        wasm_emit_uleb128(&sec, idx);
      idx++;
    }

    wasm_emit_u8(&main_buf, WASM_SEC_FUNCTION);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 5. Memory Section */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 64);
    wasm_emit_vec_header(&sec, 1);
    wasm_emit_u8(&sec, 0);
    wasm_emit_uleb128(&sec, 2); /* 2 pages (128KB) */

    wasm_emit_u8(&main_buf, WASM_SEC_MEMORY);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 6. Global Section (stack top) */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 32);
    wasm_emit_vec_header(&sec, 1);
    wasm_emit_u8(&sec, WASM_TYPE_I32);
    wasm_emit_u8(&sec, 1); /* mutable */
    wasm_emit_u8(&sec, WASM_OP_I32_CONST);
    wasm_emit_sleb128(&sec, 0);
    wasm_emit_u8(&sec, WASM_OP_END);

    wasm_emit_u8(&main_buf, WASM_SEC_GLOBAL);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 7. Export Section */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 64);

    wasm_emit_vec_header(&sec, 1 + func_count);

    wasm_emit_name(&sec, "memory");
    wasm_emit_u8(&sec, 0x02);
    wasm_emit_uleb128(&sec, 0);

    ulong idx = 0;
    ulong seen = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen < func_count; f = f->next, seen++) {
      const char *name = f->name ? f->name : "MethodUnknown";
      wasm_emit_name(&sec, name);
      wasm_emit_u8(&sec, 0x00);
      wasm_emit_uleb128(&sec, idx++);
    }

    wasm_emit_u8(&main_buf, WASM_SEC_EXPORT);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 8. Code Section */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 2048);

    ulong local_func_count = 0;
    ulong seen = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen < func_count; f = f->next, seen++) {
      if (!f->import_info.is_import)
        local_func_count++;
    }

    wasm_emit_vec_header(&sec, local_func_count);

    runtime_imports_t imports;
    resolve_runtime_imports(module, &imports);

    ulong seen_code = 0;
    for (fruity_function_t *f = module->functions_head;
         f && seen_code < func_count; f = f->next, seen_code++) {
      if (f->import_info.is_import)
        continue;

      wasm_buffer_t body;
      wasm_buf_init(&body, 1024);
      compile_function_body(module, f, &imports, &body);
      wasm_emit_uleb128(&sec, body.size);
      wasm_emit_bytes(&sec, body.data, body.size);
      wasm_buf_free(&body);
    }

    wasm_emit_u8(&main_buf, WASM_SEC_CODE);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }
  print("WASM: code section done\n");

  if (main_buf.error) {
    wasm_buf_free(&main_buf);
    return -1;
  }

  result->wasm_binary = main_buf.data;
  result->wasm_size = main_buf.size;
  result->success = 0;

  return 0;
}

void fruity_wasm_result_free(fruity_wasm_result_t *result) {
  if (result->wasm_binary) {
    xfree(result->wasm_binary);
    result->wasm_binary = nil;
  }
}
