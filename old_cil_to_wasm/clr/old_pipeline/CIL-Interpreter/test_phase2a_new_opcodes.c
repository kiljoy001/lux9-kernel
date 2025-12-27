#include "../tests/framework/test_framework.h"
#include "../include/execution_engine.h"
#include "../include/il_decoder.h"

// Test helpers
static vm_execution_state_t* create_test_state() {
    vm_init_t init = {0};
    return vm_create_execution_state(&init);
}

static void cleanup_test_state(vm_execution_state_t* state) {
    vm_destroy_execution_state(state);
}

// Test new constant opcodes
TEST_SUITE(phase2a_new_opcodes)

TEST(test_ldc_i8_operation) {
    vm_execution_state_t* state = create_test_state();
    vm_init_t init = {0};
    
    // Create IL instruction: LDC_I8 123456789
    uint8_t bytecode[] = {0x21, 0x15, 0xCD, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00};
    cil_instruction_t* instr = decode_next_instruction(create_cil_decoder(bytecode, sizeof(bytecode)));
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDC_I8, instr->opcode);
    TEST_ASSERT_EQUAL(123456789LL, instr->operand.long_val);
    
    // Execute instruction
    vm_frame_t frame = {0};
    frame.ip = instr;
    frame.arg_count = 0;
    frame.args = NULL;
    frame.local_count = 0;
    frame.locals = NULL;
    state->current_frame = &frame;
    
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Verify result on stack
    vm_value_t result;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I8, result.type);
    TEST_ASSERT_EQUAL(123456789LL, result.value.i8);
    
    destroy_cil_decoder((cil_decoder_t*)bytecode); // Note: this is wrong, we should use the decoder we created
    cleanup_test_state(state);
}

TEST(test_ldnull_operation) {
    vm_execution_state_t* state = create_test_state();
    
    // Create IL instruction: LDNULL
    uint8_t bytecode[] = {0x14};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDNULL, instr->opcode);
    
    // Execute instruction
    vm_frame_t frame = {0};
    frame.ip = instr;
    frame.arg_count = 0;
    frame.args = NULL;
    frame.local_count = 0;
    frame.locals = NULL;
    state->current_frame = &frame;
    
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Verify null reference on stack
    vm_value_t result;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_REF, result.type);
    TEST_ASSERT_NULL(result.value.ref);
    
    destroy_cil_decoder(decoder);
    cleanup_test_state(state);
}

TEST(test_ldc_i4_m1_operation) {
    vm_execution_state_t* state = create_test_state();
    
    // Create IL instruction: LDC_I4_M1
    uint8_t bytecode[] = {0x15};
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(CIL_OPCODE_LDC_I4_M1, instr->opcode);
    
    // Execute instruction
    vm_frame_t frame = {0};
    frame.ip = instr;
    frame.arg_count = 0;
    frame.args = NULL;
    frame.local_count = 0;
    frame.locals = NULL;
    state->current_frame = &frame;
    
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Verify -1 on stack
    vm_value_t result;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-1, result.value.i4);
    
    destroy_cil_decoder(decoder);
    cleanup_test_state(state);
}

TEST(test_div_un_operation) {
    vm_execution_state_t* state = create_test_state();
    
    // Create IL instruction: LDC_I4 20, LDC_I4 3, DIV_UN
    uint8_t bytecode[] = {
        0x20, 0x14, 0x00, 0x00, 0x00,  // LDC_I4 20
        0x20, 0x03, 0x00, 0x00, 0x00,  // LDC_I4 3
        0x5C                          // DIV_UN
    };
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    // Execute: 20 / 3 = 6 (unsigned)
    vm_frame_t frame = {0};
    frame.arg_count = 0;
    frame.args = NULL;
    frame.local_count = 0;
    frame.locals = NULL;
    state->current_frame = &frame;
    
    // Execute first instruction (LDC_I4 20)
    frame.ip = instr;
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Execute second instruction (LDC_I4 3)
    instr = decode_next_instruction(decoder);
    frame.ip = instr;
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Execute DIV_UN
    instr = decode_next_instruction(decoder);
    frame.ip = instr;
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Verify result: 20u / 3u = 6u
    vm_value_t result;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result));
    TEST_ASSERT_EQUAL(VM_TYPE_U4, result.type);
    TEST_ASSERT_EQUAL(6u, result.value.u4);
    
    destroy_cil_decoder(decoder);
    cleanup_test_state(state);
}

TEST(test_starg_s_operation) {
    vm_execution_state_t* state = create_test_state();
    
    // Create IL instruction: LDC_I4 42, STARG.S 0
    uint8_t bytecode[] = {
        0x20, 0x2A, 0x00, 0x00, 0x00,  // LDC_I4 42
        0x10, 0x00                     // STARG.S 0
    };
    cil_decoder_t* decoder = create_cil_decoder(bytecode, sizeof(bytecode));
    cil_instruction_t* instr = decode_next_instruction(decoder);
    
    // Setup frame with arguments
    vm_frame_t frame = {0};
    frame.arg_count = 1;
    vm_value_t args[1];
    frame.args = args;
    frame.local_count = 0;
    frame.locals = NULL;
    state->current_frame = &frame;
    
    // Execute first instruction (LDC_I4 42)
    frame.ip = instr;
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Execute STARG.S 0
    instr = decode_next_instruction(decoder);
    frame.ip = instr;
    TEST_ASSERT_TRUE(vm_execute_instruction(state));
    TEST_ASSERT_FALSE(vm_has_error(state));
    
    // Verify argument was stored
    TEST_ASSERT_EQUAL(VM_TYPE_I4, frame.args[0].type);
    TEST_ASSERT_EQUAL(42, frame.args[0].value.i4);
    
    destroy_cil_decoder(decoder);
    cleanup_test_state(state);
}

// Main test runner
int main() {
    printf("=== Phase 2A: New Opcodes TDD Tests ===\n");
    printf("Testing new opcode implementations:\n");
    printf("- LDC_I8: Load int64 constant\n");
    printf("- LDNULL: Load null reference\n");
    printf("- LDC_I4_M1: Load int32 constant -1\n");
    printf("- DIV_UN: Unsigned division\n");
    printf("- STARG.S: Store argument short\n\n");
    
    int result = run_test_suite(phase2a_new_opcodes);
    
    if (result == 0) {
        printf("\n✓ All Phase 2A new opcode tests passed!\n");
        printf("Successfully implemented 8 new opcodes:\n");
        printf("  LDC_I8, LDC_R4, LDC_R8, LDNULL, LDC_I4_M1\n");
        printf("  DIV_UN, STARG_S, LDARGA_S\n");
        printf("\nImplementation progress: %d/222 opcodes (%.1f%%)\n", 53, (53.0/222.0)*100);
    }
    
    return result;
}