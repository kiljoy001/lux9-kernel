#include "../clr_capability.h"
#include "../il_parser.h"
#include "cil_domtree.h"
#include "cil_opcodes.h"

/*
 * analyze_cfg_security implementation
 */
void analyze_cfg_security(dt_cfg_t *cfg,
                          clr_monotonic_capability_t *method_cap) {
  if (!cfg)
    return;

  /* Explicitly unused parameter for initial implementation */
  (void)method_cap;

  /* Iterate over all basic blocks */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *block = &cfg->blocks[i];

    /* Reset security metadata */
    block->required_permissions = 0;
    block->requires_validation = 0;
    block->is_sensitive_op = 0;

    /* Iterate over instructions in the block */
    u32int offset = block->start_offset;
    u32int end = block->end_offset;

    /* Note: dt_basic_block offsets are [start, end) covering the instructions
     */

    while (offset < end) {
      /* Get instruction size using helper */
      int size = cil_get_instruction_size(cfg->il, offset, cfg->il_size);

      if (size <= 0) {
        /* Should not happen on valid CFG, bail to avoid infinite loop */
        break;
      }

      u8int op = cfg->il[offset];
      u16int full_op = op;

      /* Handle 2-byte opcodes */
      if (op == 0xFE && offset + 1 < cfg->il_size) {
        full_op = (op << 8) | cfg->il[offset + 1];
      }

      /*
       * Security Policy:
       * 1. CALL / CALLVIRT / CALLI / NEWOBJ: Require EXEC permission.
       *    This ensures code cannot transfer control to arbitrary methods
       *    without having the right to execute code initiated from this
       * context.
       *
       * 2. JMP: Tail call, also requires EXEC.
       */

      int is_call =
          (full_op == IL_CALL || full_op == IL_CALLVIRT ||
           full_op == IL_CALLI || full_op == IL_NEWOBJ || full_op == IL_JMP);

      if (is_call) {
        block->is_sensitive_op = 1;
        block->required_permissions |= CAP_PERM_EXEC;
        block->requires_validation = 1;
      }

      /* Advance offset */
      offset += size;
    }
  }
}
