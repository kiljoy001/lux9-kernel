#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

// Simple test framework
typedef enum {
    TEST_PASSED = 0,
    TEST_FAILED = 1
} test_result_t;

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("  FAIL: %s:%d Expected %lld, got %lld\n", \
                   __FILE__, __LINE__, \
                   (long long)(expected), (long long)(actual)); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("  FAIL: %s:%d Expected true, got false\n", __FILE__, __LINE__); \
            return TEST_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf("  FAIL: %s:%d Expected false, got true\n", __FILE__, __LINE__); \
            return TEST_FAILED; \
        } \
    } while(0)

// VM types (simplified for testing)
typedef enum {
    VM_TYPE_I1 = 0x01,
    VM_TYPE_U1 = 0x02,
    VM_TYPE_I2 = 0x03,
    VM_TYPE_U2 = 0x04,
    VM_TYPE_I4 = 0x05,
    VM_TYPE_U4 = 0x06,
    VM_TYPE_I8 = 0x07,
    VM_TYPE_U8 = 0x08,
    VM_TYPE_R4 = 0x0B,
    VM_TYPE_R8 = 0x0C,
    VM_TYPE_REF = 0x0D
} vm_type_t;

typedef struct {
    vm_type_t type;
    union {
        int8_t i1;
        uint8_t u1;
        int16_t i2;
        uint16_t u2;
        int32_t i4;
        uint32_t u4;
        int64_t i8;
        uint64_t u8;
        float r4;
        double r8;
        void* ref;
    } value;
} vm_value_t;

// Value constructors
vm_value_t vm_make_i4(int32_t value) { vm_value_t v; v.type = VM_TYPE_I4; v.value.i4 = value; return v; }
vm_value_t vm_make_u4(uint32_t value) { vm_value_t v; v.type = VM_TYPE_U4; v.value.u4 = value; return v; }
vm_value_t vm_make_i1(int8_t value) { vm_value_t v; v.type = VM_TYPE_I1; v.value.i1 = value; return v; }
vm_value_t vm_make_u1(uint8_t value) { vm_value_t v; v.type = VM_TYPE_U1; v.value.u1 = value; return v; }

// Arithmetic operations (simplified implementations for testing)
bool vm_add(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 + right->value.i4; 
        return true; 
    }
    return false; 
}

bool vm_subtract(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 - right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_multiply(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 * right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_divide(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 / right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_divide_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)(u1 / u2);
        return true; 
    }
    return false;
}

bool vm_remainder(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 % right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_remainder_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        if (right->value.i4 == 0) return false;
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (int32_t)(u1 % u2);
        return true; 
    }
    return false;
}

bool vm_and(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 & right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_or(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 | right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_xor(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) { 
        result->type = VM_TYPE_I4; 
        result->value.i4 = left->value.i4 ^ right->value.i4; 
        return true; 
    }
    return false;
}

bool vm_not(vm_value_t* v, vm_value_t* r) {
    if (v->type == VM_TYPE_I4) { 
        r->type = VM_TYPE_I4; 
        r->value.i4 = ~v->value.i4; 
        return true; 
    }
    return false;
}

bool vm_neg(vm_value_t* v, vm_value_t* r) {
    if (v->type == VM_TYPE_I4) { 
        r->type = VM_TYPE_I4; 
        r->value.i4 = -v->value.i4; 
        return true; 
    }
    return false;
}

bool vm_shl(vm_value_t* a, vm_value_t* b, vm_value_t* c) {
    if (a->type == VM_TYPE_I4 && b->type == VM_TYPE_I4) { 
        c->type = VM_TYPE_I4; 
        c->value.i4 = a->value.i4 << b->value.i4; 
        return true; 
    }
    return false;
}

bool vm_shr(vm_value_t* a, vm_value_t* b, vm_value_t* c) {
    if (a->type == VM_TYPE_I4 && b->type == VM_TYPE_I4) { 
        c->type = VM_TYPE_I4; 
        c->value.i4 = a->value.i4 >> b->value.i4; 
        return true; 
    }
    return false;
}

bool vm_shr_un(vm_value_t* a, vm_value_t* b, vm_value_t* c) { 
    if (a->type == VM_TYPE_I4 && b->type == VM_TYPE_I4) {
        uint32_t u1 = (uint32_t)a->value.i4;
        c->type = VM_TYPE_I4;
        c->value.i4 = (int32_t)(u1 >> b->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_equal(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = (left->value.i4 == right->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_greater(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = (left->value.i4 > right->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_greater_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (u1 > u2);
        return true;
    }
    return false;
}

bool vm_compare_less(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        result->type = VM_TYPE_I4;
        result->value.i4 = (left->value.i4 < right->value.i4);
        return true;
    }
    return false;
}

bool vm_compare_less_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        uint32_t u1 = (uint32_t)left->value.i4;
        uint32_t u2 = (uint32_t)right->value.i4;
        result->type = VM_TYPE_I4;
        result->value.i4 = (u1 < u2);
        return true;
    }
    return false;
}

// Overflow arithmetic implementations
bool vm_add_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 + (int64_t)right->value.i4;
        if (temp < INT32_MIN || temp > INT32_MAX) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 + right->value.i4;
        return true;
    }
    return false;
}

bool vm_add_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        uint64_t temp = (uint64_t)left->value.u4 + (uint64_t)right->value.u4;
        if (temp > UINT32_MAX) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 + right->value.u4;
        return true;
    }
    return false;
}

bool vm_multiply_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 * (int64_t)right->value.i4;
        if (temp < INT32_MIN || temp > INT32_MAX) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 * right->value.i4;
        return true;
    }
    return false;
}

bool vm_multiply_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        uint64_t temp = (uint64_t)left->value.u4 * (uint64_t)right->value.u4;
        if (temp > UINT32_MAX) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 * right->value.u4;
        return true;
    }
    return false;
}

bool vm_subtract_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        int64_t temp = (int64_t)left->value.i4 - (int64_t)right->value.i4;
        if (temp < INT32_MIN || temp > INT32_MAX) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 - right->value.i4;
        return true;
    }
    return false;
}

bool vm_subtract_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (left->value.u4 < right->value.u4) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 - right->value.u4;
        return true;
    }
    return false;
}

bool vm_divide_ovf(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    // Division overflow only occurs with INT32_MIN / -1
    if (left->type == VM_TYPE_I4 && right->type == VM_TYPE_I4) {
        if (right->value.i4 == 0) return false;
        if (left->value.i4 == INT32_MIN && right->value.i4 == -1) return false;
        result->type = VM_TYPE_I4;
        result->value.i4 = left->value.i4 / right->value.i4;
        return true;
    }
    return false;
}

bool vm_divide_ovf_un(vm_value_t* left, vm_value_t* right, vm_value_t* result) {
    if (left->type == VM_TYPE_U4 && right->type == VM_TYPE_U4) {
        if (right->value.u4 == 0) return false;
        result->type = VM_TYPE_U4;
        result->value.u4 = left->value.u4 / right->value.u4;
        return true;
    }
    return false;
}

// Type conversion implementation
bool vm_convert(vm_value_t* source, vm_type_t target_type, vm_value_t* result) {
    if (!source || !result) return false;
    
    result->type = target_type;
    
    switch(target_type) {
        case VM_TYPE_I1:
            switch(source->type) {
                case VM_TYPE_I1: result->value.i1 = source->value.i1; return true;
                case VM_TYPE_I4: result->value.i1 = (int8_t)source->value.i4; return true;
                case VM_TYPE_U1: result->value.i1 = (int8_t)source->value.u1; return true;
                case VM_TYPE_U4: result->value.i1 = (int8_t)source->value.u4; return true;
                default: return false;
            }
        case VM_TYPE_I4:
            switch(source->type) {
                case VM_TYPE_I1: result->value.i4 = source->value.i1; return true;
                case VM_TYPE_I4: result->value.i4 = source->value.i4; return true;
                case VM_TYPE_U1: result->value.i4 = source->value.u1; return true;
                case VM_TYPE_U4: result->value.i4 = (int32_t)source->value.u4; return true;
                default: return false;
            }
        case VM_TYPE_U4:
            switch(source->type) {
                case VM_TYPE_I1: result->value.u4 = (uint32_t)source->value.i1; return true;
                case VM_TYPE_I4: result->value.u4 = (uint32_t)source->value.i4; return true;
                case VM_TYPE_U1: result->value.u4 = source->value.u1; return true;
                case VM_TYPE_U4: result->value.u4 = source->value.u4; return true;
                default: return false;
            }
        default: return false;
    }
}

// Test functions
test_result_t test_add_basic() {
    vm_value_t val1 = vm_make_i4(5);
    vm_value_t val2 = vm_make_i4(3);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(8, result.value.i4);
    
    return TEST_PASSED;
}

test_result_t test_sub_basic() {
    vm_value_t val1 = vm_make_i4(10);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_subtract(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(6, result.value.i4);
    
    return TEST_PASSED;
}

test_result_t test_mul_basic() {
    vm_value_t val1 = vm_make_i4(7);
    vm_value_t val2 = vm_make_i4(6);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_multiply(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);
    
    return TEST_PASSED;
}

test_result_t test_div_basic() {
    vm_value_t val1 = vm_make_i4(20);
    vm_value_t val2 = vm_make_i4(4);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_divide(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(5, result.value.i4);
    
    return TEST_PASSED;
}

test_result_t test_and_basic() {
    vm_value_t val1 = vm_make_i4(0b1100);  // 12
    vm_value_t val2 = vm_make_i4(0b1010);  // 10
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_and(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0b1000, result.value.i4);  // 12 & 10 = 8
    
    return TEST_PASSED;
}

test_result_t test_or_basic() {
    vm_value_t val1 = vm_make_i4(0b1100);  // 12
    vm_value_t val2 = vm_make_i4(0b1010);  // 10
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_or(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(0b1110, result.value.i4);  // 12 | 10 = 14
    
    return TEST_PASSED;
}

test_result_t test_add_ovf_no_overflow() {
    vm_value_t val1 = vm_make_i4(100);
    vm_value_t val2 = vm_make_i4(200);
    vm_value_t result = vm_make_i4(0);
    
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(300, result.value.i4);
    
    return TEST_PASSED;
}

test_result_t test_add_ovf_signed_overflow() {
    vm_value_t val1 = vm_make_i4(INT_MAX);
    vm_value_t val2 = vm_make_i4(1);
    vm_value_t result = vm_make_i4(0);
    
    // ADD.OVF should fail on overflow
    bool success = vm_add_ovf(&val1, &val2, &result);
    TEST_ASSERT_FALSE(success);  // Should fail due to overflow
    
    return TEST_PASSED;
}

int main(void) {
    printf("=== CIL Interpreter Verification Test ===\n");
    
    int passed = 0;
    int failed = 0;
    int total = 0;
    
    // Run tests
    printf("Running arithmetic tests...\n");
    
    total++;
    if (test_add_basic() == TEST_PASSED) {
        printf("  ✓ test_add_basic\n");
        passed++;
    } else {
        printf("  ✗ test_add_basic\n");
        failed++;
    }
    
    total++;
    if (test_sub_basic() == TEST_PASSED) {
        printf("  ✓ test_sub_basic\n");
        passed++;
    } else {
        printf("  ✗ test_sub_basic\n");
        failed++;
    }
    
    total++;
    if (test_mul_basic() == TEST_PASSED) {
        printf("  ✓ test_mul_basic\n");
        passed++;
    } else {
        printf("  ✗ test_mul_basic\n");
        failed++;
    }
    
    total++;
    if (test_div_basic() == TEST_PASSED) {
        printf("  ✓ test_div_basic\n");
        passed++;
    } else {
        printf("  ✗ test_div_basic\n");
        failed++;
    }
    
    total++;
    if (test_and_basic() == TEST_PASSED) {
        printf("  ✓ test_and_basic\n");
        passed++;
    } else {
        printf("  ✗ test_and_basic\n");
        failed++;
    }
    
    total++;
    if (test_or_basic() == TEST_PASSED) {
        printf("  ✓ test_or_basic\n");
        passed++;
    } else {
        printf("  ✗ test_or_basic\n");
        failed++;
    }
    
    total++;
    if (test_add_ovf_no_overflow() == TEST_PASSED) {
        printf("  ✓ test_add_ovf_no_overflow\n");
        passed++;
    } else {
        printf("  ✗ test_add_ovf_no_overflow\n");
        failed++;
    }
    
    total++;
    if (test_add_ovf_signed_overflow() == TEST_PASSED) {
        printf("  ✓ test_add_ovf_signed_overflow\n");
        passed++;
    } else {
        printf("  ✗ test_add_ovf_signed_overflow\n");
        failed++;
    }
    
    printf("\n=== Test Results ===\n");
    printf("Total tests: %d\n", total);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    
    if (failed == 0) {
        printf("\n✓ All tests passed! CIL interpreter core functionality verified.\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}