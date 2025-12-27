#include <stdio.h>
#include <stdlib.h>
#include "include/execution_engine.h"

int main() {
    printf("=== Testing SHR Operation Execution ===\n");
    
    // Create VM state
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    // Create bytecode: LDC_I4 16, LDC_I4 2, SHR
    uint8_t bytecode[] = {
        0x20, 0x10, 0x00, 0x00, 0x00,  // LDC_I4 16
        0x20, 0x02, 0x00, 0x00, 0x00,  // LDC_I4 2
        0x63,                          // SHR
    };
    
    printf("Executing: 16 LDC_I4, 2 LDC_I4, SHR\n");
    printf("Expected: 16 >> 2 = 4\n");
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    if (result) {
        printf("Execution: SUCCESS\n");
        
        // Check result
        vm_value_t top;
        if (vm_stack_pop(state, &top)) {
            printf("Result on stack: %d (type %d)\n", top.value.i4, top.type);
            if (top.value.i4 == 4) {
                printf("✓ SHR operation works correctly!\n");
            } else {
                printf("✗ SHR operation result incorrect (expected 4, got %d)\n", top.value.i4);
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
