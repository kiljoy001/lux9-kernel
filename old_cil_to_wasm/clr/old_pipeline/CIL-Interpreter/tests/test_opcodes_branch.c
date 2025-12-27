#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/execution_engine.h"

vm_init_t mock_init = {0};

int main() {
    printf("Testing Branching BRTRUE...\n");
    vm_execution_state_t* state = vm_create_execution_state(&mock_init);
    
    vm_frame_t* frame = calloc(1, sizeof(vm_frame_t));
    
    // 0: LDC 1
    // 1: BRTRUE 2 (Jump to 3)
    // 2: LDC 99
    // 3: LDC 42
    cil_instruction_t* insts = calloc(4, sizeof(cil_instruction_t));
    
    insts[0].opcode = CIL_OPCODE_LDC_I4;
    insts[0].operand.int_val = 1;
    
    insts[1].opcode = CIL_OPCODE_BRTRUE;
    insts[1].operand.branch_offset = 2;
    
    insts[2].opcode = CIL_OPCODE_LDC_I4;
    insts[2].operand.int_val = 99;
    
    insts[3].opcode = CIL_OPCODE_LDC_I4;
    insts[3].operand.int_val = 42;
    
    frame->ip = &insts[0];
    state->current_frame = frame;
    
    // Exec 0
    if (!vm_execute_instruction(state)) return 1;
    frame->ip++;
    
    // Exec 1: BRTRUE
    // This should fail if BRTRUE is not implemented
    if (!vm_execute_instruction(state)) {
        printf("EXPECTED FAILURE: BRTRUE not implemented\n");
        return 1;
    }
    
    // If it implemented, check logic
    // We expect implementation to do: ip += (offset - 1)
    // So ip points to index 1 + (2-1) = 2.
    // Then loop (manual here) does ip++. Result: 3.
    frame->ip++;
    
    if (state->current_frame->ip != &insts[3]) {
        printf("FAILED: IP target mismatch\n");
        return 1;
    }
    
    printf("PASS: BRTRUE jumped\n");
    return 0;
}
