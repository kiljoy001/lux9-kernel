/*
 * CLR Runtime Test Suite
 * Tests the C implementation against the verified Coq properties
 */

#include "clr_runtime.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Test case counter */
static int test_count = 0;
static int passed_tests = 0;

#define TEST(name) \
    do { \
        printf("TEST %d: %s ... ", ++test_count, #name); \
        if (test_##name()) { \
            printf("PASS\n"); \
            passed_tests++; \
        } else { \
            printf("FAIL\n"); \
        } \
    } while(0)

/* ========== Individual Test Cases ========== */

/* Test NOP instruction (from NOP case proof) */
bool test_nop() {
    clr_state_t state;
    clr_state_init(&state);
    
    size_t initial_ip = state.ip;
    size_t initial_stack_size = state.stack_size;
    
    clr_instruction_t nop = { .opcode = CIL_NOP };
    clr_result_t result = clr_execute(&state, &nop);
    
    return (result == CLR_SUCCESS) && 
           (state.ip == initial_ip + 1) &&
           (state.stack_size == initial_stack_size);
}

/* Test DUP instruction (from DUP case proof) */
bool test_dup() {
    clr_state_t state;
    clr_state_init(&state);
    
    /* Push a value first */
    clr_value_t test_val = clr_make_int32(42);
    clr_push(&state, test_val);
    
    size_t initial_stack_size = state.stack_size;
    
    clr_instruction_t dup = { .opcode = CIL_DUP };
    clr_result_t result = clr_execute(&state, &dup);
    
    /* Verify DUP duplicated the top value */
    bool stack_grew = (state.stack_size == initial_stack_size + 1);
    bool top_duplicated = (state.stack[state.stack_size - 1].data.int32_val == 42) &&
                         (state.stack[state.stack_size - 2].data.int32_val == 42);
    
    return (result == CLR_SUCCESS) && stack_grew && top_duplicated;
}

/* Test POP instruction */
bool test_pop() {
    clr_state_t state;
    clr_state_init(&state);
    
    /* Push a value first */
    clr_push(&state, clr_make_int32(100));
    size_t initial_stack_size = state.stack_size;
    
    clr_instruction_t pop = { .opcode = CIL_POP };
    clr_result_t result = clr_execute(&state, &pop);
    
    return (result == CLR_SUCCESS) && 
           (state.stack_size == initial_stack_size - 1);
}

/* Test LDC_I4 instruction */
bool test_ldc_i4() {
    clr_state_t state;
    clr_state_init(&state);
    
    clr_instruction_t ldc = { .opcode = CIL_LDC_I4, .arg.int32_arg = 123 };
    clr_result_t result = clr_execute(&state, &ldc);
    
    bool pushed_correct_value = (state.stack_size == 1) &&
                               (state.stack[0].type == CLR_INT32) &&
                               (state.stack[0].data.int32_val == 123);
    
    return (result == CLR_SUCCESS) && pushed_correct_value;
}

/* Test ADD instruction (from ADD case proof) */
bool test_add() {
    clr_state_t state;
    clr_state_init(&state);
    
    /* Push two operands */
    clr_push(&state, clr_make_int32(15));
    clr_push(&state, clr_make_int32(27));
    
    clr_instruction_t add = { .opcode = CIL_ADD };
    clr_result_t result = clr_execute(&state, &add);
    
    bool correct_result = (state.stack_size == 1) &&
                         (state.stack[0].type == CLR_INT32) &&
                         (state.stack[0].data.int32_val == 42); /* 15 + 27 = 42 */
    
    return (result == CLR_SUCCESS) && correct_result;
}

/* Test SUB instruction */
bool test_sub() {
    clr_state_t state;
    clr_state_init(&state);
    
    /* Push two operands: second - first */
    clr_push(&state, clr_make_int32(10)); /* second operand */
    clr_push(&state, clr_make_int32(3));  /* first operand (top of stack) */
    
    clr_instruction_t sub = { .opcode = CIL_SUB };
    clr_result_t result = clr_execute(&state, &sub);
    
    bool correct_result = (state.stack_size == 1) &&
                         (state.stack[0].data.int32_val == 7); /* 10 - 3 = 7 */
    
    return (result == CLR_SUCCESS) && correct_result;
}

/* Test MUL instruction */
bool test_mul() {
    clr_state_t state;
    clr_state_init(&state);
    
    clr_push(&state, clr_make_int32(6));
    clr_push(&state, clr_make_int32(7));
    
    clr_instruction_t mul = { .opcode = CIL_MUL };
    clr_result_t result = clr_execute(&state, &mul);
    
    bool correct_result = (state.stack_size == 1) &&
                         (state.stack[0].data.int32_val == 42); /* 6 * 7 = 42 */
    
    return (result == CLR_SUCCESS) && correct_result;
}

/* Test LDLOC instruction */
bool test_ldloc() {
    clr_state_t state;
    clr_state_init(&state);
    
    /* Set a local variable */
    state.locals[5] = clr_make_int32(999);
    
    clr_instruction_t ldloc = { .opcode = CIL_LDLOC, .arg.index_arg = 5 };
    clr_result_t result = clr_execute(&state, &ldloc);
    
    bool loaded_correctly = (state.stack_size == 1) &&
                           (state.stack[0].data.int32_val == 999);
    
    return (result == CLR_SUCCESS) && loaded_correctly;
}

/* Test STLOC instruction (from verified stack_discipline proof) */
bool test_stloc() {
    clr_state_t state;
    clr_state_init(&state);
    
    /* Push value to store */
    clr_push(&state, clr_make_int32(777));
    size_t initial_stack_size = state.stack_size;
    
    clr_instruction_t stloc = { .opcode = CIL_STLOC, .arg.index_arg = 3 };
    clr_result_t result = clr_execute(&state, &stloc);
    
    /* Verify stack discipline: stack size decreased by 1 */
    bool stack_discipline = (state.stack_size == initial_stack_size - 1);
    
    /* Verify local variable was set */
    bool local_set = (state.locals[3].type == CLR_INT32) &&
                     (state.locals[3].data.int32_val == 777);
    
    return (result == CLR_SUCCESS) && stack_discipline && local_set;
}

/* Test stack effect calculations match verified definitions */
bool test_stack_effects() {
    clr_stack_effect_t effect;
    
    /* Test each opcode's stack effect matches the verified definition */
    effect = clr_get_stack_effect(CIL_NOP);
    if (effect.pops != 0 || effect.pushes != 0) return false;
    
    effect = clr_get_stack_effect(CIL_DUP);
    if (effect.pops != 0 || effect.pushes != 1) return false;
    
    effect = clr_get_stack_effect(CIL_POP);
    if (effect.pops != 1 || effect.pushes != 0) return false;
    
    effect = clr_get_stack_effect(CIL_LDC_I4);
    if (effect.pops != 0 || effect.pushes != 1) return false;
    
    effect = clr_get_stack_effect(CIL_ADD);
    if (effect.pops != 2 || effect.pushes != 1) return false;
    
    effect = clr_get_stack_effect(CIL_STLOC);
    if (effect.pops != 1 || effect.pushes != 0) return false; /* Verified in Coq */
    
    return true;
}

/* Test complex sequence matching verified execution patterns */
bool test_complex_sequence() {
    clr_state_t state;
    clr_state_init(&state);
    
    printf("\n  Executing: LDC 10, LDC 20, ADD, STLOC 0, LDLOC 0\n");
    clr_print_state(&state);
    
    /* LDC 10 */
    clr_instruction_t ldc1 = { .opcode = CIL_LDC_I4, .arg.int32_arg = 10 };
    if (clr_execute(&state, &ldc1) != CLR_SUCCESS) return false;
    clr_print_state(&state);
    
    /* LDC 20 */
    clr_instruction_t ldc2 = { .opcode = CIL_LDC_I4, .arg.int32_arg = 20 };
    if (clr_execute(&state, &ldc2) != CLR_SUCCESS) return false;
    clr_print_state(&state);
    
    /* ADD */
    clr_instruction_t add = { .opcode = CIL_ADD };
    if (clr_execute(&state, &add) != CLR_SUCCESS) return false;
    clr_print_state(&state);
    
    /* STLOC 0 */
    clr_instruction_t stloc = { .opcode = CIL_STLOC, .arg.index_arg = 0 };
    if (clr_execute(&state, &stloc) != CLR_SUCCESS) return false;
    clr_print_state(&state);
    
    /* LDLOC 0 */
    clr_instruction_t ldloc = { .opcode = CIL_LDLOC, .arg.index_arg = 0 };
    if (clr_execute(&state, &ldloc) != CLR_SUCCESS) return false;
    clr_print_state(&state);
    
    /* Verify final state: stack should have 30, local 0 should have 30 */
    bool final_state_correct = (state.stack_size == 1) &&
                              (state.stack[0].data.int32_val == 30) &&
                              (state.locals[0].data.int32_val == 30);
    
    return final_state_correct;
}

/* ========== Main Test Runner ========== */

int main() {
    printf("CLR Runtime Test Suite\n");
    printf("Testing C implementation against verified Coq properties\n");
    printf("=======================================================\n\n");
    
    /* Run all test cases */
    TEST(nop);
    TEST(dup);
    TEST(pop);
    TEST(ldc_i4);
    TEST(add);
    TEST(sub);
    TEST(mul);
    TEST(ldloc);
    TEST(stloc);
    TEST(stack_effects);
    TEST(complex_sequence);
    
    printf("\n=======================================================\n");
    printf("Test Results: %d/%d tests passed\n", passed_tests, test_count);
    
    if (passed_tests == test_count) {
        printf("SUCCESS: All tests passed! C implementation matches verified properties.\n");
        return 0;
    } else {
        printf("FAILURE: Some tests failed.\n");
        return 1;
    }
}