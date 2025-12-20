#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include "../../include/il_decoder.h"

// Test REM operation - should initially FAIL (RED phase)
TEST(test_rem_operation_impl) {
    // Create a simple execution state
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    TEST_ASSERT_NOT_NULL(state);
    
    // Create bytecode: LDC_I4 10, LDC_I4 3, REM
    uint8_t bytecode[] = {
        0x20, 0x0A, 0x00, 0x00, 0x00,  // LDC_I4 10
        0x20, 0x03, 0x00, 0x00, 0x00,  // LDC_I4 3
        0x5D,                          // REM
    };
    
    // Execute the method
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should initially FAIL because REM is not implemented
    // After implementation, it should succeed
    TEST_ASSERT_TRUE(result);
    
    // Verify stack has remainder (10 % 3 = 1)
    vm_value_t top;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &top));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, top.type);
    TEST_ASSERT_EQUAL(1, top.value.i4);
    
    // Clean up
    vm_destroy_execution_state(state);
    
    return TEST_PASSED;
}

// Main test runner
int main(void) {
    printf("=== REM Operation TDD Test ===\n");
    printf("RED Phase: Testing REM operation (should initially fail)\n\n");
    
    test_register_case(&test_rem_operation);
    test_run_all();
    
    return 0;
}
