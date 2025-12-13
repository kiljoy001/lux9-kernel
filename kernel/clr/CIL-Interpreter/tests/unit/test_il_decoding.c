#include "../framework/test_framework.h"
#include "../../include/il_decoder.h"
#include <stdio.h>
#include <stdlib.h>

// Test suite for IL decoding
TEST_SUITE(il_decoding);

static void il_decoding_setup(void) {
    // Setup code if needed
}

static void il_decoding_teardown(void) {
    // Teardown code if needed
}

// Test that checks simple opcode decoding (no operands)
TEST(test_simple_opcodes) {
    // Create a simple bytecode with a few opcodes
    uint8_t bytecode[] = {
        0x00,  // NOP
        0x2A,  // RET
        0x58,  // ADD
        0x59,  // SUB
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Decode first instruction (NOP)
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_NOP, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("nop", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode second instruction (RET)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_RET, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("ret", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode third instruction (ADD)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_ADD, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("add", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode fourth instruction (SUB)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_SUB, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("sub", get_opcode_name(inst->opcode));
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

// Test that checks opcodes with byte operands
TEST(test_byte_operand_opcodes) {
    // Create bytecode with opcodes that have byte operands
    uint8_t bytecode[] = {
        0x1F, 0x05,  // LDC_I4_S 5
        0x0E, 0x03,  // LDARG_S 3
        0x11, 0x02,  // LDLOC_S 2
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Decode first instruction (LDC_I4_S)
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDC_I4_S, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_BYTE, inst->operand_type);
    TEST_ASSERT_EQUAL(2, inst->size);
    TEST_ASSERT_EQUAL(5, inst->operand.byte_val);
    TEST_ASSERT_STRING_EQUAL("ldc.i4.s", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode second instruction (LDARG_S)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDARG_S, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_BYTE, inst->operand_type);
    TEST_ASSERT_EQUAL(2, inst->size);
    TEST_ASSERT_EQUAL(3, inst->operand.byte_val);
    TEST_ASSERT_STRING_EQUAL("ldarg.s", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode third instruction (LDLOC_S)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDLOC_S, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_BYTE, inst->operand_type);
    TEST_ASSERT_EQUAL(2, inst->size);
    TEST_ASSERT_EQUAL(2, inst->operand.byte_val);
    TEST_ASSERT_STRING_EQUAL("ldloc.s", get_opcode_name(inst->opcode));
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

// Test that checks opcodes with int operands
TEST(test_int_operand_opcodes) {
    // Create bytecode with opcodes that have int operands
    uint8_t bytecode[] = {
        0x20, 0x05, 0x00, 0x00, 0x00,  // LDC_I4 5
        0x28, 0x10, 0x00, 0x00, 0x00,  // CALL token 0x10
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Decode first instruction (LDC_I4)
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDC_I4, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_INT, inst->operand_type);
    TEST_ASSERT_EQUAL(5, inst->size);
    TEST_ASSERT_EQUAL(5, inst->operand.int_val);
    TEST_ASSERT_STRING_EQUAL("ldc.i4", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode second instruction (CALL)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_CALL, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_INT, inst->operand_type);
    TEST_ASSERT_EQUAL(5, inst->size);
    TEST_ASSERT_EQUAL(0x10, inst->operand.int_val);
    TEST_ASSERT_STRING_EQUAL("call", get_opcode_name(inst->opcode));
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

// Test that checks branch opcodes
TEST(test_branch_opcodes) {
    // Create bytecode with branch opcodes
    uint8_t bytecode[] = {
        0x2B, 0x05,  // BR_S 5
        0x38, 0x0A, 0x00, 0x00, 0x00,  // BR 10
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Decode first instruction (BR_S)
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_BR_S, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_BRANCH_SHORT, inst->operand_type);
    TEST_ASSERT_EQUAL(2, inst->size);
    TEST_ASSERT_EQUAL(5, inst->operand.branch_offset_short);
    TEST_ASSERT_STRING_EQUAL("br.s", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode second instruction (BR)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_BR, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_BRANCH, inst->operand_type);
    TEST_ASSERT_EQUAL(5, inst->size);
    TEST_ASSERT_EQUAL(10, inst->operand.int_val);
    TEST_ASSERT_STRING_EQUAL("br", get_opcode_name(inst->opcode));
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

// Test that checks switch opcode
TEST(test_switch_opcode) {
    // Create bytecode with switch opcode
    uint8_t bytecode[] = {
        0x45,  // SWITCH
        0x02, 0x00, 0x00, 0x00,  // 2 targets
        0x05, 0x00, 0x00, 0x00,  // target 1
        0x0A, 0x00, 0x00, 0x00,  // target 2
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Decode switch instruction
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_SWITCH, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_SWITCH, inst->operand_type);
    TEST_ASSERT_EQUAL(13, inst->size);  // 1 + 4 + 4*2
    TEST_ASSERT_EQUAL(2, inst->operand.switch_table.num_targets);
    TEST_ASSERT_EQUAL(5, inst->operand.switch_table.targets[0]);
    TEST_ASSERT_EQUAL(10, inst->operand.switch_table.targets[1]);
    TEST_ASSERT_STRING_EQUAL("switch", get_opcode_name(inst->opcode));
    
    // Clean up switch targets
    free(inst->operand.switch_table.targets);
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

// Test extended opcodes
TEST(test_extended_opcodes) {
    // Create bytecode with extended opcodes
    uint8_t bytecode[] = {
        0xFE, 0x00,  // ARGLIST (no operand)
        0xFE, 0x06, 0x10, 0x00, 0x00, 0x00,  // LDFTN with token operand (4 bytes)
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Decode first instruction (ARGLIST)
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_ARGLIST, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(2, inst->size);
    TEST_ASSERT_STRING_EQUAL("arglist", get_opcode_name(inst->opcode));
    free(inst);
    
    // Decode second instruction (LDFTN)
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDFTN, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_TOKEN, inst->operand_type);
    TEST_ASSERT_EQUAL(6, inst->size); // 2 for opcode + 4 for token
    TEST_ASSERT_EQUAL(0x10, inst->operand.int_val);
    TEST_ASSERT_STRING_EQUAL("ldftn", get_opcode_name(inst->opcode));
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

// Test arithmetic overflow operations
TEST(test_arithmetic_overflow_opcodes) {
    // Create bytecode with arithmetic overflow operations
    uint8_t bytecode[] = {
        0xD6,  // ADD_OVF (no operand)
        0xD8,  // MUL_OVF (no operand) 
        0xDA,  // SUB_OVF (no operand)
        0xDC,  // DIV_OVF (no operand)
    };
    
    // Create decoder
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(decoder);
    
    // Test ADD_OVF
    cil_instruction_t* inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_ADD_OVF, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("add.ovf", get_opcode_name(inst->opcode));
    free(inst);
    
    // Test MUL_OVF
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_MUL_OVF, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("mul.ovf", get_opcode_name(inst->opcode));
    free(inst);
    
    // Test SUB_OVF
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_SUB_OVF, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("sub.ovf", get_opcode_name(inst->opcode));
    free(inst);
    
    // Test DIV_OVF
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(CIL_OPCODE_DIV_OVF, inst->opcode);
    TEST_ASSERT_EQUAL(CIL_OPERAND_NONE, inst->operand_type);
    TEST_ASSERT_EQUAL(1, inst->size);
    TEST_ASSERT_STRING_EQUAL("div.ovf", get_opcode_name(inst->opcode));
    free(inst);
    
    // Should be no more instructions
    inst = decode_next_instruction(decoder);
    TEST_ASSERT_NULL(inst);
    
    // Clean up
    destroy_cil_decoder(decoder);
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== IL Decoding Tests ===\n\n");
    
    // Register the test suite
    test_register_suite(&suite_il_decoding);
    
    // Set current suite
    current_suite = &suite_il_decoding;
    
    // Register test cases
    static test_case_t test_simple = {
        .suite_name = "il_decoding",
        .test_name = "test_simple_opcodes",
        .test_func = test_simple_opcodes_wrapper,
        .next = NULL
    };
    
    static test_case_t test_byte = {
        .suite_name = "il_decoding",
        .test_name = "test_byte_operand_opcodes",
        .test_func = test_byte_operand_opcodes_wrapper,
        .next = NULL
    };
    
    static test_case_t test_int = {
        .suite_name = "il_decoding",
        .test_name = "test_int_operand_opcodes",
        .test_func = test_int_operand_opcodes_wrapper,
        .next = NULL
    };
    
    static test_case_t test_branch = {
        .suite_name = "il_decoding",
        .test_name = "test_branch_opcodes",
        .test_func = test_branch_opcodes_wrapper,
        .next = NULL
    };
    
    static test_case_t test_switch = {
        .suite_name = "il_decoding",
        .test_name = "test_switch_opcode",
        .test_func = test_switch_opcode_wrapper,
        .next = NULL
    };
    
    static test_case_t test_extended = {
        .suite_name = "il_decoding",
        .test_name = "test_extended_opcodes",
        .test_func = test_extended_opcodes_wrapper,
        .next = NULL
    };
    
    static test_case_t test_arithmetic_overflow = {
        .suite_name = "il_decoding",
        .test_name = "test_arithmetic_overflow_opcodes",
        .test_func = test_arithmetic_overflow_opcodes_wrapper,
        .next = NULL
    };
    
    test_register_case(&test_simple);
    test_register_case(&test_byte);
    test_register_case(&test_int);
    test_register_case(&test_branch);
    test_register_case(&test_switch);
    test_register_case(&test_extended);
    test_register_case(&test_arithmetic_overflow);
    
    // Run tests
    test_set_verbose(1);
    test_run_suite("il_decoding");
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Total tests: %u\n", test_runner_state.total_tests);
    printf("Passed: %u\n", test_runner_state.passed_tests);
    printf("Failed: %u\n", test_runner_state.failed_tests);
    printf("Skipped: %u\n", test_runner_state.skipped_tests);
    
    if (test_runner_state.failed_tests == 0) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }
    
    return test_runner_state.failed_tests > 0 ? 1 : 0;
}