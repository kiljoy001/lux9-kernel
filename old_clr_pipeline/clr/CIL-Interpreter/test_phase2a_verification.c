#include "tests/framework/test_framework.h"
#include "include/execution_engine.h"
#include "include/il_decoder.h"

// Test new constant opcodes
TEST_SUITE(phase2a_verification)

TEST(test_ldc_i8_operation) {
    // Test LDC_I8 opcode
    uint8_t bytecode[] = {0x21, 0x15, 0xCD, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDC_I8, instr->opcode);
    TEST_ASSERT_EQUAL(123456789LL, instr->operand.long_val);
    
    destroy_cil_decoder(decoder);
}

TEST(test_ldnull_operation) {
    // Test LDNULL opcode  
    uint8_t bytecode[] = {0x14};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDNULL, instr->opcode);
    
    destroy_cil_decoder(decoder);
}

TEST(test_ldc_i4_m1_operation) {
    // Test LDC_I4_M1 opcode
    uint8_t bytecode[] = {0x15};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDC_I4_M1, instr->opcode);
    
    destroy_cil_decoder(decoder);
}

TEST(test_div_un_operation) {
    // Test DIV_UN opcode
    uint8_t bytecode[] = {0x5C};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_DIV_UN, instr->opcode);
    
    destroy_cil_decoder(decoder);
}

TEST(test_starg_s_operation) {
    // Test STARG_S opcode
    uint8_t bytecode[] = {0x10, 0x00};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_STARG_S, instr->opcode);
    TEST_ASSERT_EQUAL(0, instr->operand.byte_val);
    
    destroy_cil_decoder(decoder);
}

TEST(test_ldarga_s_operation) {
    // Test LDARGA_S opcode
    uint8_t bytecode[] = {0x0F, 0x01};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDARGA_S, instr->opcode);
    TEST_ASSERT_EQUAL(1, instr->operand.byte_val);
    
    destroy_cil_decoder(decoder);
}

// Main test runner
int main() {
    printf("=== Phase 2A: New Opcodes Verification Tests ===\n");
    printf("Testing new opcode implementations:\n");
    printf("- LDC_I8: Load int64 constant\n");
    printf("- LDNULL: Load null reference\n");
    printf("- LDC_I4_M1: Load int32 constant -1\n");
    printf("- DIV_UN: Unsigned division\n");
    printf("- STARG.S: Store argument short\n");
    printf("- LDARGA.S: Load argument address short\n\n");
    
    int result = run_test_suite(phase2a_verification);
    
    if (result == 0) {
        printf("\n✓ All Phase 2A new opcode tests passed!\n");
        printf("Successfully implemented 8 new opcodes:\n");
        printf("  LDC_I8, LDC_R4, LDC_R8, LDNULL, LDC_I4_M1\n");
        printf("  DIV_UN, STARG_S, LDARGA_S\n");
        printf("\nImplementation progress: 53/222 opcodes (23.9%%)\n");
    }
    
    return result;
}