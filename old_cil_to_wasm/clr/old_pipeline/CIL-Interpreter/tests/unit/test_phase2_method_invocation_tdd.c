#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include "../../include/il_decoder.h"
#include <stdio.h>
#include <stdlib.h>

// Test suite for Phase 2: Method Invocation & Advanced Control Flow (TDD Red Phase)
TEST_SUITE(phase2_method_invocation);

static void phase2_method_invocation_setup(void) {
    // Setup code for method invocation tests
}

static void phase2_method_invocation_teardown(void) {
    // Teardown code for method invocation tests
}

// Test CALL (method invocation) - should FAIL initially
TEST(test_call_method) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: CALL method_token
    uint8_t bytecode[] = {
        0x28, 0x00, 0x00, 0x00, 0x01,  // CALL method_token (0x01000000)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because CALL is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test CALLI (indirect method invocation) - should FAIL initially  
TEST(test_calli_indirect) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: load method ptr, CALLI
    uint8_t bytecode[] = {
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (placeholder method ptr)
        0x29,                          // CALLI
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // Check if there was an error
    if (vm_has_error(state)) {
        const char* error_msg = vm_get_error(state);
        printf("CALLI test error: %s\n", error_msg);
    }
    
    // This should now PASS because CALLI is implemented
    if (!result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have passed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly passed as expected
}

// Test CGT_UN (compare greater than unsigned) - should FAIL initially
TEST(test_cgt_un_comparison) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: LDC_I4 -1, LDC_I4 1, CGT_UN
    // Should return true because -1 (0xFFFFFFFF) > 1 (unsigned)
    uint8_t bytecode[] = {
        0x20, 0xFF, 0xFF, 0xFF, 0xFF,  // LDC_I4 -1
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
        0x03, 0x01,                   // CGT_UN (0x103 opcode)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because CGT_UN is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test CLT_UN (compare less than unsigned) - should FAIL initially
TEST(test_clt_un_comparison) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: LDC_I4 1, LDC_I4 -1, CLT_UN
    // Should return true because 1 < -1 (0xFFFFFFFF) (unsigned)
    uint8_t bytecode[] = {
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1
        0x20, 0xFF, 0xFF, 0xFF, 0xFF,  // LDC_I4 -1
        0x05, 0x01,                   // CLT_UN (0x105 opcode)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because CLT_UN is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

int main(void) {
    printf("=== Phase 2: Method Invocation & Advanced Control Flow - TDD RED PHASE ===\n");
    printf("Running tests that should FAIL because opcodes are not implemented yet...\n\n");
    
    // Set current suite
    current_suite = &suite_phase2_method_invocation;
    
    // Test case declarations (following exact pattern from test_arithmetic_phase1a.c)
    static test_case_t test_call_method = {
        .suite_name = "phase2_method_invocation",
        .test_name = "test_call_method", 
        .test_func = test_call_method_wrapper,
        .next = NULL
    };
    
    static test_case_t test_calli_indirect = {
        .suite_name = "phase2_method_invocation",
        .test_name = "test_calli_indirect",
        .test_func = test_calli_indirect_wrapper,
        .next = NULL
    };
    
    static test_case_t test_cgt_un_comparison = {
        .suite_name = "phase2_method_invocation",
        .test_name = "test_cgt_un_comparison", 
        .test_func = test_cgt_un_comparison_wrapper,
        .next = NULL
    };
    
    static test_case_t test_clt_un_comparison = {
        .suite_name = "phase2_method_invocation",
        .test_name = "test_clt_un_comparison",
        .test_func = test_clt_un_comparison_wrapper,
        .next = NULL
    };
    
    // Link all test cases in order
    test_call_method.next = &test_calli_indirect;
    test_calli_indirect.next = &test_cgt_un_comparison;
    test_cgt_un_comparison.next = &test_clt_un_comparison;
    test_clt_un_comparison.next = NULL;
    
    // Set verbose mode
    test_set_verbose(true);
    
    // Register suite and first test case
    test_register_suite(&suite_phase2_method_invocation);
    test_register_case(&test_call_method);
    
    // Run tests
    test_run_all();
    
    printf("\n=== TDD RED PHASE RESULTS ===\n");
    printf("Expected: All tests should FAIL (opcodes not implemented)\n");
    printf("Actual results:\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u (these should be 0)\n", test_runner_state.passed_tests);
    printf("Failed: %u (these should equal total tests)\n", test_runner_state.failed_tests);
    
    if (test_runner_state.failed_tests == test_runner_state.total_tests) {
        printf("\n✓ PERFECT! All tests failed as expected.\n");
        printf("Ready to implement opcodes to make them pass (GREEN phase)\n");
    } else {
        printf("\n⚠ UNEXPECTED: Some tests passed when they should have failed!\n");
        printf("Check implementation - some opcodes may already be working\n");
    }
    
    return 0;
}