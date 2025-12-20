#include <stdio.h>
#include <stdlib.h>
#include "include/execution_engine.h"

int main() {
    printf("=== Testing REM_UN Operation Execution ===\n");
    
    // Create VM state
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    // Create bytecode: LDC_U4 10, LDC_U4 3, REM_UN
    uint8_t bytecode[] = {
        0x22, 0x0A, 0x00, 0x00, 0x00,  // LDC_U4 10
        0x22, 0x03, 0x00, 0x00, 0x00,  // LDC_U4 3
        0x5E,                          // REM_UN
    };
    
    printf("Executing: 10 LDC_U4, 3 LDC_U4, REM_UN\n");
    printf("Expected: 10 % 3 = 1 (unsigned)\n");
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    if (result) {
        printf("Execution: SUCCESS\n");
        
        // Check result
        vm_value_t top;
        if (vm_stack_pop(state, &top)) {
            printf("Result on stack: %u (type %d)\n", top.value.u4, top.type);
            if (top.value.u4 == 1) {
                printf("✓ REM_UN operation works correctly!\n");
            } else {
                printf("✗ REM_UN operation result incorrect (expected 1, got %u)\n", top.value.u4);
            }
        } else {
            printf("✗ Failed to pop result from stack\n");
        }
    } else {
        printf("Execution: FAILED\n");
        const char* error = vm_get_error(state);
        if (error) {
            printf("Error: %s\n", error);
        }
    }
    
    vm_destroy_execution_state(state);
    return 0;
}
