#include "../framework/test_framework.h"
#include "../../include/execution_engine.h"
#include "../../include/il_decoder.h"
#include <stdio.h>
#include <stdlib.h>

// Test suite for Phase 3: Object Model & Advanced Operations (TDD Red Phase)
TEST_SUITE(phase3_object_model);

static void phase3_object_model_setup(void) {
    // Setup code for object model tests
}

static void phase3_object_model_teardown(void) {
    // Teardown code for object model tests
}

// Test CALLVIRT (virtual method call) - should FAIL initially
TEST(test_callvirt_virtual_call) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: CALLVIRT method_token
    uint8_t bytecode[] = {
        0x6F, 0x00, 0x00, 0x00, 0x01,  // CALLVIRT method_token (0x01000000)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because CALLVIRT is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test CONSTRAINED prefix - should FAIL initially
TEST(test_constrained_prefix) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: CONSTRAINED prefix with callvirt
    uint8_t bytecode[] = {
        0xFE, 0x16,                    // CONSTRAINED prefix
        0x6F, 0x00, 0x00, 0x00, 0x01,  // CALLVIRT method_token
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because CONSTRAINED is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test BREAK (debug breakpoint) - should FAIL initially
TEST(test_break_debug) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: BREAK instruction
    uint8_t bytecode[] = {
        0x01,                          // BREAK
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because BREAK is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test BRFALSE_S (short branch on false) - should FAIL initially
TEST(test_brfalse_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: LDC_I4 0, BRFALSE_S +3, LDC_I4 1
    // Should branch over LDC_I4 1 because 0 is false
    uint8_t bytecode[] = {
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (false)
        0x2C, 0x03,                    // BRFALSE_S +3 (skip next 3 bytes)
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1 (should be skipped)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because BRFALSE_S is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test BRTRUE_S (short branch on true) - should FAIL initially
TEST(test_brtrue_s_short_branch) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: LDC_I4 1, BRTRUE_S +3, LDC_I4 0
    // Should branch over LDC_I4 0 because 1 is true
    uint8_t bytecode[] = {
        0x20, 0x01, 0x00, 0x00, 0x00,  // LDC_I4 1 (true)
        0x2D, 0x03,                    // BRTRUE_S +3 (skip next 3 bytes)
        0x20, 0x00, 0x00, 0x00, 0x00,  // LDC_I4 0 (should be skipped)
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // This should FAIL initially because BRTRUE_S is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

// Test ARGLIST (get argument list) - should FAIL initially
TEST(test_arglist_get_args) {
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    if (!state) return TEST_FAILED;
    
    // Bytecode: ARGLIST
    uint8_t bytecode[] = {
        0xFE, 0x00,                          // ARGLIST
    };
    
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 1); // 1 local arg
    
    // This should FAIL initially because ARGLIST is not implemented
    if (result) {
        vm_destroy_execution_state(state);
        return TEST_FAILED; // Should have failed but didn't
    }
    
    vm_destroy_execution_state(state);
    return TEST_PASSED; // Correctly failed as expected
}

int main(void) {
    printf("=== Phase 3: Object Model & Advanced Operations - TDD RED PHASE ===\n");
    printf("Running tests that should FAIL because opcodes are not implemented yet...\n\n");
    
    // Set current suite
    current_suite = &suite_phase3_object_model;
    
    // Test case declarations (following exact pattern)
    static test_case_t test_callvirt_virtual_call = {
        .suite_name = "phase3_object_model",
        .test_name = "test_callvirt_virtual_call", 
        .test_func = test_callvirt_virtual_call_wrapper,
        .next = NULL
    };
    
    static test_case_t test_constrained_prefix = {
        .suite_name = "phase3_object_model",
        .test_name = "test_constrained_prefix",
        .test_func = test_constrained_prefix_wrapper,
        .next = NULL
    };
    
    static test_case_t test_break_debug = {
        .suite_name = "phase3_object_model",
        .test_name = "test_break_debug", 
        .test_func = test_break_debug_wrapper,
        .next = NULL
    };
    
    static test_case_t test_brfalse_s_short_branch = {
        .suite_name = "phase3_object_model",
        .test_name = "test_brfalse_s_short_branch",
        .test_func = test_brfalse_s_short_branch_wrapper,
        .next = NULL
    };
    
    static test_case_t test_brtrue_s_short_branch = {
        .suite_name = "phase3_object_model",
        .test_name = "test_brtrue_s_short_branch",
        .test_func = test_brtrue_s_short_branch_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arglist_get_args = {
        .suite_name = "phase3_object_model",
        .test_name = "test_arglist_get_args",
        .test_func = test_arglist_get_args_wrapper,
        .next = NULL
    };
    
    // Link all test cases in order
    test_callvirt_virtual_call.next = &test_constrained_prefix;
    test_constrained_prefix.next = &test_break_debug;
    test_break_debug.next = &test_brfalse_s_short_branch;
    test_brfalse_s_short_branch.next = &test_brtrue_s_short_branch;
    test_brtrue_s_short_branch.next = &test_arglist_get_args;
    test_arglist_get_args.next = NULL;
    
    // Set verbose mode
    test_set_verbose(true);
    
    // Register suite and first test case
    test_register_suite(&suite_phase3_object_model);
    test_register_case(&test_callvirt_virtual_call);
    
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