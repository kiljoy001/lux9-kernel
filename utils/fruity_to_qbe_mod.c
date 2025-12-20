#include "compat.h"
/* fruity_to_qbe.c - Fruity IR to QBE IL Translator
 *
 * Simplified translator: just emit QBE stubs for now
 */

/* include removed */
/* include removed */
/* include removed */
/* include removed */
/* include removed */

/* include removed */
/* include removed */
/* include removed */
#include "../kernel/clr/fruity/fruity_to_qbe.h"
#include "qbe_buffer.h"

#include "qbe_buffer.h"

/* Emit QBE IL header */
static void emit_header(QBEBuffer *buf) {
  qbe_buffer_printf(buf, "# QBE IL generated from Fruity IR\n\n");
  qbe_buffer_printf(
      buf,
      "# Allocator and Token stubs removed (implemented in runtime.c)\n\n");
}

/* Emit a function as QBE IL */
static int emit_function(QBEBuffer *buf, fruity_function_t *func) {
  fruity_basic_block_t *bb;
  fruity_instruction_t *instr;
  int tmp_counter = 0;

  /* Emit function */
  qbe_buffer_printf(buf, "export function w $%s(", func->name);
  for (ulong i = 0; i < func->arg_count; i++) {
    qbe_buffer_printf(buf, "%s %%arg%lu", i == 0 ? "l" : ", l", i);
  }
  qbe_buffer_printf(buf, ") {\n");

  /* Allocate storage for locals (simplified: all 8 bytes) */
  qbe_buffer_printf(buf, "@%s_start\n", func->name);

  /* Allocate storage for locals (simplified: all 8 bytes) */
  for (ulong i = 0; i < func->local_count; i++) {
    qbe_buffer_printf(buf, "    %%loc%lu =l alloc8 8\n", i);
  }

  for (bb = func->blocks_head; bb != nil; bb = bb->next) {
    if (bb != func->blocks_head)
      qbe_buffer_printf(buf, "@%s_bb%d\n", func->name, bb->block_id);

    int is_terminated = 0;
    for (instr = bb->instructions_head; instr != nil; instr = instr->next) {
      /* Check for terminator */
      if (instr->opcode == FRUITY_RET || instr->opcode == FRUITY_JUMP ||
          instr->opcode == FRUITY_SWITCH || instr->opcode == FRUITY_BTRUE ||
          instr->opcode == FRUITY_BFALSE) {
        is_terminated = 1;
      }

      /* Translate each Fruity opcode to QBE */
      switch (instr->opcode) {
      case FRUITY_LDC_I4:
        qbe_buffer_printf(buf, "    %%t%d =l copy %d\n", ++tmp_counter,
                          (int)instr->operand.value.i64);
        break;

      case FRUITY_ADD:
        qbe_buffer_printf(buf, "    %%t%d =l add %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_SUB:
        qbe_buffer_printf(buf, "    %%t%d =l sub %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_MUL:
        qbe_buffer_printf(buf, "    %%t%d =l mul %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_DIV:
        qbe_buffer_printf(buf, "    %%t%d =l div %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_REM:
        qbe_buffer_printf(buf, "    %%t%d =l rem %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_AND:
        qbe_buffer_printf(buf, "    %%t%d =l and %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_OR:
        qbe_buffer_printf(buf, "    %%t%d =l or %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_XOR:
        qbe_buffer_printf(buf, "    %%t%d =l xor %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_CALL: {
        /* Emit call with result */
        /* For now, generate calls without explicit args - QBE handles variadics
         */
        int result_reg = ++tmp_counter;
        qbe_buffer_printf(buf, "    %%t%d =l call $method_%u()\n", result_reg,
                          instr->operand.value.token);
        /* Note: Proper arg handling requires tracking call signature metadata
         */
        /* This is a simplified version that works for nullary functions */
      } break;

      /* Variable Access */
      case FRUITY_LOAD_LOCAL:
        qbe_buffer_printf(buf, "    %%t%d =l loadl %%loc%d\n", ++tmp_counter,
                          instr->operand.value.index);
        break;
      case FRUITY_STORE_LOCAL:
        qbe_buffer_printf(buf, "    storel %%t%d, %%loc%d\n", tmp_counter--,
                          instr->operand.value.index);
        break;
      case FRUITY_LOAD_ARG:
        qbe_buffer_printf(buf, "    %%t%d =l copy %%arg%d\n", ++tmp_counter,
                          instr->operand.value.index);
        break;

      /* Control Flow */
      case FRUITY_SWITCH: {
        fruity_switch_targets_t *targets = instr->operand.value.switch_targets;
        int val_reg = tmp_counter--; /* Value to switch on */

        if (!targets)
          break;

        for (u32int i = 0; i < targets->count; i++) {
          fruity_basic_block_t *target = targets->targets[i];
          u32int target_id = target ? target->block_id : 0;
          int cmp_reg = ++tmp_counter;

          /* Check if val == i */
          qbe_buffer_printf(buf, "    %%t%d =l ceqw %%t%d, %d\n", cmp_reg,
                            val_reg, i);

          /* If match, jump to target. Else jump to next check (local label) */
          if (i < targets->count - 1) {
            qbe_buffer_printf(buf, "    jnz %%t%d, @%s_bb%u, @sw_%u_%u\n",
                              cmp_reg, func->name, target_id, bb->block_id,
                              i + 1);
            qbe_buffer_printf(buf, "@sw_%u_%u\n", bb->block_id, i + 1);
          } else {
            /* Last check. If match, jump target. Else fallthrough (default) */
            u32int next_id = bb->next ? bb->next->block_id : 0;
            qbe_buffer_printf(buf, "    jnz %%t%d, @%s_bb%u, @%s_bb%u\n",
                              cmp_reg, func->name, target_id, func->name,
                              next_id);
          }
          tmp_counter--; /* Consume cmp_reg */
        }
      } break;

      case FRUITY_JUMP:
        if (instr->operand.value.target)
          qbe_buffer_printf(buf, "    jmp @%s_bb%d\n", func->name,
                            instr->operand.value.target->block_id);
        break;

      case FRUITY_BTRUE:
      case FRUITY_BFALSE: {
        int cond_temp = tmp_counter--;
        u32int target_id = instr->operand.value.target
                               ? instr->operand.value.target->block_id
                               : 0;
        u32int next_id = bb->next ? bb->next->block_id : 0; // Fallthrough

        /* QBE jnz: jnz %val, @true, @false */
        if (instr->opcode == FRUITY_BTRUE)
          qbe_buffer_printf(buf, "    jnz %%t%d, @%s_bb%u, @%s_bb%u\n",
                            cond_temp, func->name, target_id, func->name,
                            next_id);
        else
          qbe_buffer_printf(buf, "    jnz %%t%d, @%s_bb%u, @%s_bb%u\n",
                            cond_temp, func->name, next_id, func->name,
                            target_id); /* Swap for FALSE */
      } break;

      case FRUITY_LOAD_STRING:
        qbe_buffer_printf(buf,
                          "    %%t%d =l call $clr_string_from_literal(w %d)\n",
                          ++tmp_counter, instr->operand.value.i32);
        break;

      case FRUITY_NEWOBJ:
        /* Object allocation - simplified without args */
        qbe_buffer_printf(buf, "    %%t%d =l call $clr_newobj(w %d)\n",
                          ++tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_NEWARR:
        /* Array allocation - size on stack */
        qbe_buffer_printf(buf, "    %%t%d =l call $clr_newarr(w %d, w %%t%d)\n",
                          tmp_counter, instr->operand.value.token, tmp_counter);
        break;

      case FRUITY_LIME:
        /* call l $lux_alloc(w %size, w %type) */
        /* Assume size is on top of stack */
        qbe_buffer_printf(buf, "    %%ptr%d =l call $lux_alloc(w %%t%d, w 0)\n",
                          tmp_counter, tmp_counter);
        /* Result replaces stack top */
        break;

      case FRUITY_RET:
        /* Return top of stack */
        qbe_buffer_printf(buf, "    ret %%t%d\n", tmp_counter);
        goto done_block;

      default:
        qbe_buffer_printf(buf, "    # opcode %d (unimplemented)\n",
                          instr->opcode);
        break;
      }
    }
  done_block:;
    if (!is_terminated) {
      if (bb->next)
        qbe_buffer_printf(buf, "    jmp @%s_bb%d\n", func->name,
                          bb->next->block_id);
      else
        qbe_buffer_printf(buf, "    ret 0\n");
    }
  }

  qbe_buffer_printf(buf, "}\n\n");
  return 0;
}

/* Main translation function */
int fruity_to_qbe(fruity_module_t *module, uintptr out_handle, char *errorbuf,
                  usize errorbuf_size) {
  QBEBuffer buf;
  fruity_function_t *func;
  void *vaddr;
  usize copy_len;

  if (module == nil) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "null module");
    return -1;
  }

  /* Initialize buffer */
  qbe_buffer_init(&buf);

  /* Emit header */
  emit_header(&buf);

  /* Emit all functions */
  for (func = module->functions_head; func != nil; func = func->next) {
    if (emit_function(&buf, func) < 0) {
      if (errorbuf && errorbuf_size > 0)
        snprint(errorbuf, (int)errorbuf_size, "Failed to emit function");
      qbe_buffer_free(&buf);
      return -1;
    }
  }

  /* out_handle is a physical address - convert to kernel virtual */
  vaddr = KADDR(out_handle);
  copy_len = qbe_buffer_len(&buf);
  if (copy_len > 16777216) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "Output too large: %lud bytes",
              copy_len);
    qbe_buffer_free(&buf);
    return -1;
  }

  /* Copy to page */
  memmove(vaddr, qbe_buffer_data(&buf), copy_len);
  ((char *)vaddr)[copy_len] = 0;

  qbe_buffer_free(&buf);
  return 0;
}
