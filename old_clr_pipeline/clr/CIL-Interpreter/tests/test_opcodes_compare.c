#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/execution_engine.h"

vm_init_t mock_init = {0};

int main() {
    printf("Testing Comparison CGT...\n");
    vm_execution_state_t* state = vm_create_execution_state(&mock_init);
    
    // Stack: 10, 5 -> CGT -> 1
    // Note: CIL comparison pops val2, then val1. 
    // val1 > val2 ? 
    // So push 10 (val1), push 5 (val2).
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(5);
    vm_stack_push(state, &val1);
    vm_stack_push(state, &val2);
    
    // Frame setup
    vm_frame_t* frame = calloc(1, sizeof(vm_frame_t));
    cil_instruction_t inst;
    inst.opcode = CIL_OPCODE_CGT;
    
    frame->ip = &inst;
    state->current_frame = frame;
    
    // Execute
    if (!vm_execute_instruction(state)) {
        printf("EXPECTED FAILURE: CGT not implemented\n");
        return 1;
    }
    
    // Check Result
    vm_value_t result;
    vm_stack_pop(state, &result);
    if (result.value.i4 != 1) {
        printf("FAILED: Expected 1, got %d\n", result.value.i4);
        return 1;
    }
    
    printf("PASS: CGT opcode works\n");
    return 0;
}
