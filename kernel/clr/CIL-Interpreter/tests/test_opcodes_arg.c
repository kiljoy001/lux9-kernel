#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/execution_engine.h"

/* Mock VM Init */
vm_init_t mock_init = {0};

int main() {
    printf("Testing LDARG_0...\n");
    vm_execution_state_t* state = vm_create_execution_state(&mock_init);
    
    // Manual Frame Setup
    vm_frame_t* frame = calloc(1, sizeof(vm_frame_t));
    frame->arg_count = 1;
    frame->args = calloc(1, sizeof(vm_value_t));
    frame->args[0] = vm_make_i4(42); // Argument 0 is 42
    
    // Setup instruction pointer manually
    cil_instruction_t inst;
    inst.opcode = CIL_OPCODE_LDARG_0; // 0x02
    frame->ip = &inst;
    
    state->current_frame = frame;
    
    // Execute
    // This should fail if LDARG_0 is not implemented
    if (!vm_execute_instruction(state)) {
        printf("EXPECTED FAILURE: vm_execute_instruction returned false (Opcode not implemented)\n");
        // Check error message
        const char* err = vm_get_error(state);
        printf("Error: %s\n", err ? err : "None");
        return 1; 
    }
    
    // Verify Stack
    vm_value_t result;
    if (!vm_stack_pop(state, &result)) {
        printf("FAILED: Stack empty\n");
        return 1;
    }
    
    if (result.value.i4 != 42) {
        printf("FAILED: Expected 42, got %d\n", result.value.i4);
        return 1;
    }
    
    printf("PASS: LDARG_0 loaded argument 0 correctly.\n");
    return 0;
}
