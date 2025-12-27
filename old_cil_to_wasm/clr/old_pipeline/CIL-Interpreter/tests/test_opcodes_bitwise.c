#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/execution_engine.h"

vm_init_t mock_init = {0};

int main() {
    printf("Testing Bitwise AND...\n");
    vm_execution_state_t* state = vm_create_execution_state(&mock_init);
    
    // Setup stack: 0x0F (15) AND 0x03 (3) -> 0x03
    vm_value_t val1 = vm_make_i4(0x0F);
    vm_value_t val2 = vm_make_i4(0x03);
    vm_stack_push(state, &val1);
    vm_stack_push(state, &val2);
    
    // Manual Frame
    vm_frame_t* frame = calloc(1, sizeof(vm_frame_t));
    cil_instruction_t inst;
    inst.opcode = CIL_OPCODE_AND; // 0x5F
    frame->ip = &inst;
    state->current_frame = frame;
    
    // Execute
    if (!vm_execute_instruction(state)) {
        printf("EXPECTED FAILURE: AND not implemented\n");
        return 1;
    }
    
    // Check Result
    vm_value_t result;
    if (!vm_stack_pop(state, &result)) {
        printf("FAILED: Stack empty\n");
        return 1;
    }
    
    if (result.value.i4 != 0x03) {
        printf("FAILED: Expected 3, got %d\n", result.value.i4);
        return 1;
    }
    
    printf("PASS: AND opcode works\n");
    return 0;
}
