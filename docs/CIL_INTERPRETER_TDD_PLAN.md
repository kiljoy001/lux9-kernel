# CIL Interpreter TDD Implementation Plan
## Achieving Full ECMA-335 Compliance Through Test-Driven Development

### Executive Summary

This comprehensive TDD plan outlines the systematic implementation of a fully ECMA-335 compliant CIL (Common Intermediate Language) interpreter using Red-Green-Refactor cycles. The plan ensures correctness, maintainability, and complete specification compliance through rigorous test-driven development.

## TDD Framework Overview

### Core TDD Principles for CIL Interpreter

1. **Test-First Development**: Write failing test before implementation
2. **Minimal Implementation**: Write only what's needed to pass tests
3. **Continuous Refactoring**: Improve code while maintaining test compliance
4. **Specification-Driven**: All tests based directly on ECMA-335 standard
5. **Incremental Progress**: Small, verifiable steps toward full compliance
6. **Zero Tolerance for Regression**: All tests must pass continuously

### TDD Cycle Structure

```c
// Standard TDD Cycle Template
void test_opcode_[NAME](void) {
    // RED: Write failing test based on ECMA-335 spec
    CILInterpreter* interpreter = create_test_interpreter();
    uint8_t bytecode[] = {OP_[NAME], /* operand bytes */};
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, sizeof(bytecode));
    
    // Assertions based on ECMA-335 specification
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(expected_stack_size, interpreter->stack_size);
    TEST_ASSERT_EQUAL_INT32(expected_value, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}
```

## Test Categories Framework

### 1. Specification Compliance Tests
- **Purpose**: Direct mapping to ECMA-335 opcode semantics
- **Coverage**: All 200+ CIL opcodes defined in ECMA-335
- **Validation**: Stack effects, type system behavior, exception semantics
- **Standards**: ECMA-335 4th Edition specification compliance

### 2. Stack Effect Verification Tests
- **Pre-conditions**: Stack state before opcode execution
- **Post-conditions**: Stack state after opcode execution
- **Validation**: Push/pop behavior, stack depth changes
- **Edge Cases**: Stack underflow, overflow detection

### 3. Type System Compliance Tests
- **Type Safety**: Proper type conversions and checks
- **Reference vs Value Types**: Correct handling of different type categories
- **Array Types**: Multi-dimensional array operations
- **Pointer Types**: Managed pointer arithmetic

### 4. Exception Handling Tests
- **Exception Throwing**: Correct exception generation
- **Exception Catching**: Proper exception handling flow
- **Finally Blocks**: Cleanup code execution
- **Fault Handlers**: Fault handling semantics

### 5. Memory Safety Tests
- **Null Reference Detection**: Proper null checking
- **Array Bounds Checking**: Bounds validation
- **Memory Allocation**: Heap management correctness
- **Garbage Collection Integration**: GC safety guarantees

## Phase 1: Core Infrastructure & Stack Operations (TDD)
*Duration: 3 weeks | Target: 25 opcodes | ~200 test cases*

### 1.1 Test Infrastructure Setup (TDD Cycle 1-3)

**RED Phase - Core Framework Tests:**
```c
// test/test_framework_basic.c
void test_interpreter_creation_and_destruction(void) {
    CILInterpreter* interpreter = cil_interpreter_create();
    TEST_ASSERT_NOT_NULL(interpreter);
    TEST_ASSERT_EQUAL_INT(0, interpreter->ip);
    TEST_ASSERT_EQUAL_INT(0, interpreter->stack_size);
    TEST_ASSERT_NULL(interpreter->exception_handler);
    cil_interpreter_destroy(interpreter);
}

void test_empty_program_execution(void) {
    CILInterpreter* interpreter = cil_interpreter_create();
    uint8_t bytecode[] = {OP_RET};  // Minimal valid program
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 1);
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(0, interpreter->stack_size);
    
    cil_interpreter_destroy(interpreter);
}

void test_invalid_opcode_handling(void) {
    CILInterpreter* interpreter = cil_interpreter_create();
    uint8_t bytecode[] = {0xFF};  // Invalid opcode
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 1);
    TEST_ASSERT_EQUAL_INT(CIL_INVALID_OPCODE, result);
    
    cil_interpreter_destroy(interpreter);
}
```

**GREEN Phase - Minimal Implementation:**
```c
// cil_interpreter.c
#include "cil_interpreter.h"
#include "error.h"

CILInterpreter* cil_interpreter_create(void) {
    CILInterpreter* interpreter = malloc(sizeof(CILInterpreter));
    if (!interpreter) return NULL;
    
    memset(interpreter, 0, sizeof(CILInterpreter));
    interpreter->exception_handler = NULL;
    interpreter->max_stack_size = 1024;  // ECMA-335 minimum
    return interpreter;
}

void cil_interpreter_destroy(CILInterpreter* interpreter) {
    if (interpreter) {
        // Clean up any allocated resources
        if (interpreter->managed_heap.objects) {
            free(interpreter->managed_heap.objects);
        }
        free(interpreter);
    }
}

CILResult cil_interpreter_execute(CILInterpreter* interpreter,
                                  uint8_t* bytecode,
                                  size_t bytecode_len) {
    if (!interpreter || !bytecode) return CIL_INVALID_ARGUMENT;
    
    interpreter->ip = 0;
    
    while (interpreter->ip < bytecode_len) {
        if (interpreter->stack_size > interpreter->max_stack_size) {
            return CIL_STACK_OVERFLOW;
        }
        
        uint8_t opcode = bytecode[interpreter->ip++];
        
        switch (opcode) {
            case OP_RET:
                return CIL_SUCCESS;
            default:
                return CIL_INVALID_OPCODE;
        }
    }
    return CIL_SUCCESS;
}
```

**REFACTOR Phase:**
- Extract error code definitions to separate header
- Add logging infrastructure
- Implement basic assertion framework

### 1.2 Stack Operation Tests (TDD Cycles 4-8)

**RED Phase - LDC_I4 Test:**
```c
void test_ldc_i4_positive_value(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 42
    uint8_t bytecode[] = {OP_LDC_I4, 42, OP_RET};
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 3);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    TEST_ASSERT_EQUAL_INT32(42, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_ldc_i4_negative_value(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 -123
    uint8_t bytecode[] = {OP_LDC_I4, 0x85, OP_RET};  // -123 in signed byte
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 3);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    TEST_ASSERT_EQUAL_INT32(-123, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_ldc_i4_stack_overflow(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    interpreter->max_stack_size = 0;  // Force overflow
    
    uint8_t bytecode[] = {OP_LDC_I4, 42, OP_RET};
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 3);
    
    TEST_ASSERT_EQUAL_INT(CIL_STACK_OVERFLOW, result);
    
    destroy_test_interpreter(interpreter);
}
```

**GREEN Phase - Stack Implementation:**
```c
// Stack operation implementation
case OP_LDC_I4:
    if (interpreter->stack_size >= interpreter->max_stack_size) {
        return CIL_STACK_OVERFLOW;
    }
    // Sign-extend 8-bit immediate to 32-bit
    int8_t signed_value = (int8_t)bytecode[interpreter->ip++];
    interpreter->stack[interpreter->stack_size++] = signed_value;
    break;

case OP_LDC_I4_0:
case OP_LDC_I4_1:
case OP_LDC_I4_2:
case OP_LDC_I4_3:
case OP_LDC_I4_4:
case OP_LDC_I4_5:
case OP_LDC_I4_6:
case OP_LDC_I4_7:
case OP_LDC_I4_8:
    if (interpreter->stack_size >= interpreter->max_stack_size) {
        return CIL_STACK_OVERFLOW;
    }
    interpreter->stack[interpreter->stack_size++] = opcode - OP_LDC_I4_0;
    break;

case OP_LDC_I4_M1:
    if (interpreter->stack_size >= interpreter->max_stack_size) {
        return CIL_STACK_OVERFLOW;
    }
    interpreter->stack[interpreter->stack_size++] = -1;
    break;
```

**TDD Pattern for Each Stack Opcode:**
1. **Basic Load Test**: Verify correct value loading
2. **Stack Effect Test**: Verify stack size changes
3. **Type Safety Test**: Verify proper type handling
4. **Edge Case Test**: Boundary value testing
5. **Error Handling Test**: Overflow/underflow scenarios

### 1.3 Arithmetic Operation Tests (TDD Cycles 9-15)

**RED Phase - ADD Operation:**
```c
void test_add_positive_integers(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 10, ldc.i4 32, add, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 10,
        OP_LDC_I4, 32,
        OP_ADD,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 7);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    TEST_ASSERT_EQUAL_INT32(42, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_add_overflow_detection(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 INT32_MAX, ldc.i4 1, add, ret
    uint8_t bytecode[] = {
        OP_LDC_I4_1,  // INT32_MAX
        OP_LDC_I4, 1,
        OP_ADD,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 6);
    
    TEST_ASSERT_EQUAL_INT(CIL_OVERFLOW, result);
    
    destroy_test_interpreter(interpreter);
}

void test_add_stack_underflow(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // add (insufficient operands)
    uint8_t bytecode[] = {OP_ADD, OP_RET};
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 2);
    
    TEST_ASSERT_EQUAL_INT(CIL_STACK_UNDERFLOW, result);
    
    destroy_test_interpreter(interpreter);
}
```

**GREEN Phase - Arithmetic Implementation:**
```c
case OP_ADD: {
    if (interpreter->stack_size < 2) {
        return CIL_STACK_UNDERFLOW;
    }
    
    int32_t right = interpreter->stack[--interpreter->stack_size];
    int32_t left = interpreter->stack[--interpreter->stack_size];
    
    // Check for overflow (ECMA-335 requires overflow detection)
    int64_t result = (int64_t)left + (int64_t)right;
    if (result > INT32_MAX || result < INT32_MIN) {
        return CIL_OVERFLOW;
    }
    
    interpreter->stack[interpreter->stack_size++] = (int32_t)result;
    break;
}

case OP_SUB: {
    if (interpreter->stack_size < 2) {
        return CIL_STACK_UNDERFLOW;
    }
    
    int32_t right = interpreter->stack[--interpreter->stack_size];
    int32_t left = interpreter->stack[--interpreter->stack_size];
    
    int64_t result = (int64_t)left - (int64_t)right;
    if (result > INT32_MAX || result < INT32_MIN) {
        return CIL_OVERFLOW;
    }
    
    interpreter->stack[interpreter->stack_size++] = (int32_t)result;
    break;
}

case OP_MUL: {
    if (interpreter->stack_size < 2) {
        return CIL_STACK_UNDERFLOW;
    }
    
    int32_t right = interpreter->stack[--interpreter->stack_size];
    int32_t left = interpreter->stack[--interpreter->stack_size];
    
    int64_t result = (int64_t)left * (int64_t)right;
    if (result > INT32_MAX || result < INT32_MIN) {
        return CIL_OVERFLOW;
    }
    
    interpreter->stack[interpreter->stack_size++] = (int32_t)result;
    break;
}
```

## Phase 2: Memory & Object Operations (TDD)
*Duration: 4 weeks | Target: 35 opcodes | ~280 test cases*

### 2.1 Object Creation Tests (TDD Cycles 16-20)

**RED Phase - NEWOBJ Basic Test:**
```c
void test_newobj_basic_allocation(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Simplified newobj test (actual ECMA-335 requires metadata)
    // newobj instance creation
    uint8_t bytecode[] = {
        OP_LDC_I4, 16,  // Object size
        OP_NEWOBJ, 0,   // Token for object type
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    TEST_ASSERT_NOT_NULL(interpreter->stack[0]); // Object reference
    
    // Verify object properties
    CILObject* obj = (CILObject*)interpreter->stack[0];
    TEST_ASSERT_EQUAL_INT32(16, obj->size);
    TEST_ASSERT_EQUAL_INT32(0, obj->vtable); // No methods for this test
    
    destroy_test_interpreter(interpreter);
}

void test_newobj_insufficient_memory(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    interpreter->managed_heap.max_objects = 0; // Force allocation failure
    
    uint8_t bytecode[] = {
        OP_LDC_I4, 16,
        OP_NEWOBJ, 0,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_OUT_OF_MEMORY, result);
    
    destroy_test_interpreter(interpreter);
}
```

**GREEN Phase - Object System:**
```c
// Object system implementation
typedef struct CILObject {
    struct CILObject* next;
    void* vtable;
    size_t size;
    uint32_t type_token;
    uint8_t data[];
} CILObject;

typedef struct {
    CILObject* objects;
    size_t object_count;
    size_t max_objects;
    Lock heap_lock;
} CILHeap;

case OP_NEWOBJ: {
    size_t object_size = bytecode[interpreter->ip++];
    uint32_t type_token = bytecode[interpreter->ip++];
    
    lock(&interpreter->managed_heap.heap_lock);
    
    if (interpreter->managed_heap.object_count >= interpreter->managed_heap.max_objects) {
        unlock(&interpreter->managed_heap.heap_lock);
        return CIL_OUT_OF_MEMORY;
    }
    
    CILObject* obj = malloc(sizeof(CILObject) + object_size);
    if (!obj) {
        unlock(&interpreter->managed_heap.heap_lock);
        return CIL_OUT_OF_MEMORY;
    }
    
    memset(obj, 0, sizeof(CILObject) + object_size);
    obj->size = object_size;
    obj->type_token = type_token;
    obj->vtable = NULL; // Simplified for this phase
    
    // Add to heap
    obj->next = interpreter->managed_heap.objects;
    interpreter->managed_heap.objects = obj;
    interpreter->managed_heap.object_count++;
    
    unlock(&interpreter->managed_heap.heap_lock);
    
    if (interpreter->stack_size >= interpreter->max_stack_size) {
        free(obj);
        return CIL_STACK_OVERFLOW;
    }
    
    interpreter->stack[interpreter->stack_size++] = (int32_t)obj;
    break;
}
```

### 2.2 Array Operation Tests (TDD Cycles 21-28)

**RED Phase - NEWARR Test:**
```c
void test_newarr_single_dimension(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 5, newarr int32, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 5,
        OP_NEWARR, TYPE_I4,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    
    CILArray* array = (CILArray*)interpreter->stack[0];
    TEST_ASSERT_NOT_NULL(array);
    TEST_ASSERT_EQUAL_INT32(5, array->length);
    TEST_ASSERT_EQUAL_INT32(TYPE_I4, array->element_type);
    
    destroy_test_interpreter(interpreter);
}

void test_newarr_negative_size(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 -1, newarr int32, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 0xFF,  // -1 as signed byte
        OP_NEWARR, TYPE_I4,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_ARGUMENT_OUT_OF_RANGE, result);
    
    destroy_test_interpreter(interpreter);
}

void test_ldelem_i4_access(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Create array and load element
    // ldc.i4 3, newarr int32, ldc.i4 1, ldc.i4 42, stelem.i4, ldc.i4 1, ldelem.i4, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 3,
        OP_NEWARR, TYPE_I4,
        OP_LDC_I4, 1,      // Index
        OP_LDC_I4, 42,     // Value
        OP_STELEM_I4,
        OP_LDC_I4, 1,      // Index to read
        OP_LDELEM_I4,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 15);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    TEST_ASSERT_EQUAL_INT32(42, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}
```

### 2.3 Field Operation Tests (TDD Cycles 29-35)

**Test Pattern for Field Operations:**
```c
void test_ldfld_loads_field_value(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Create object with fields
    // newobj with field layout, ldfld field_token, ret
    uint8_t bytecode[] = {
        OP_NEWOBJ, 0,      // Simple object
        OP_LDFLD, 0,       // Load first field
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 4);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    
    destroy_test_interpreter(interpreter);
}

void test_stfld_stores_field_value(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // newobj, ldc.i4 99, stfld field_token, ret
    uint8_t bytecode[] = {
        OP_NEWOBJ, 0,
        OP_LDC_I4, 99,
        OP_STFLD, 0,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    
    destroy_test_interpreter(interpreter);
}

void test_ldfld_null_reference(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 0 (null), ldfld field_token, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 0,      // null reference
        OP_LDFLD, 0,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 4);
    
    TEST_ASSERT_EQUAL_INT(CIL_NULL_REFERENCE, result);
    
    destroy_test_interpreter(interpreter);
}
```

## Phase 3: Control Flow Operations (TDD)
*Duration: 3 weeks | Target: 30 opcodes | ~240 test cases*

### 3.1 Branch Operation Tests (TDD Cycles 36-45)

**RED Phase - Conditional Branch Tests:**
```c
void test_brtrue_positive_condition(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 1, brtrue target, ldc.i4 0, ret, target: ldc.i4 42, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 1,      // true condition
        OP_BRTRUE, 5,      // Skip next instruction (offset)
        OP_LDC_I4, 0,
        OP_RET,
        OP_LDC_I4, 42,     // Target
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 8);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT32(42, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_brtrue_negative_condition(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 0, brtrue target, ldc.i4 42, ret, target: ldc.i4 99, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 0,      // false condition
        OP_BRTRUE, 5,      // Don't skip
        OP_LDC_I4, 42,
        OP_RET,
        OP_LDC_I4, 99,     // Unreachable
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 8);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT32(42, interpreter->stack[0]);
    
    destroy_test);
}

void test_interpreter(interpreter_beq_equal_values(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 42, ldc.i4 42, beq target, ldc.i4 0, ret, target: ldc.i4 1, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 42,
        OP_LDC_I4, 42,
        OP_BEQ, 6,         // Equal, take branch
        OP_LDC_I4, 0,
        OP_RET,
        OP_LDC_I4, 1,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 9);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT32(1, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}
```

**GREEN Phase - Branch Implementation:**
```c
case OP_BRTRUE: {
    if (interpreter->stack_size < 1) {
        return CIL_STACK_UNDERFLOW;
    }
    
    int32_t condition = interpreter->stack[--interpreter->stack_size];
    int32_t offset = (int8_t)bytecode[interpreter->ip++];
    
    if (condition != 0) {
        interpreter->ip += offset;
    }
    break;
}

case OP_BRFALSE: {
    if (interpreter->stack_size < 1) {
        return CIL_STACK_UNDERFLOW;
    }
    
    int32_t condition = interpreter->stack[--interpreter->stack_size];
    int32_t offset = (int8_t)bytecode[interpreter->ip++];
    
    if (condition == 0) {
        interpreter->ip += offset;
    }
    break;
}

case OP_BEQ:
case OP_BGE:
case OP_BGT:
case OP_BLE:
case OP_BLT:
case OP_BNE_UN: {
    if (interpreter->stack_size < 2) {
        return CIL_STACK_UNDERFLOW;
    }
    
    int32_t right = interpreter->stack[--interpreter->stack_size];
    int32_t left = interpreter->stack[--interpreter->stack_size];
    int32_t offset = (int8_t)bytecode[interpreter->ip++];
    
    int32_t branch_taken = 0;
    
    switch (opcode) {
        case OP_BEQ:
            branch_taken = (left == right);
            break;
        case OP_BGE:
            branch_taken = (left >= right);
            break;
        case OP_BGT:
            branch_taken = (left > right);
            break;
        case OP_BLE:
            branch_taken = (left <= right);
            break;
        case OP_BLT:
            branch_taken = (left < right);
            break;
        case OP_BNE_UN:
            branch_taken = (left != right);
            break;
    }
    
    if (branch_taken) {
        interpreter->ip += offset;
    }
    break;
}
```

### 3.2 Exception Handling Tests (TDD Cycles 46-55)

**RED Phase - Try-Catch Tests:**
```c
void test_try_catch_basic_exception(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Setup exception handler
    CILEHClause eh_clause = {
        .try_start = 0,
        .try_end = 4,
        .handler_start = 5,
        .handler_end = 7,
        .exception_type = CIL_NULL_REFERENCE,
        .catch_type = 0  // Catch any
    };
    
    cil_interpreter_add_exception_handler(interpreter, &eh_clause);
    
    // Code that throws null reference exception
    uint8_t bytecode[] = {
        OP_LDC_I4, 0,      // null reference
        OP_LDFLD, 0,       // This throws NullReferenceException
        OP_RET,
        OP_LDC_I4, 99,     // Exception handler
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 7);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT32(99, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_try_finally_basic(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Setup finally handler
    CILEHClause eh_clause = {
        .try_start = 0,
        .try_end = 3,
        .handler_start = 4,
        .handler_end = 6,
        .exception_type = CIL_FINALLY,
        .handler_type = CIL_FINALLY
    };
    
    cil_interpreter_add_exception_handler(interpreter, &eh_clause);
    
    uint8_t bytecode[] = {
        OP_LDC_I4, 42,
        OP_RET,
        OP_LDC_I4, 99,     // Finally block
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 6);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    // Finally block should execute even on normal return
    TEST_ASSERT_EQUAL_INT32(99, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}
```

### 3.3 Loop Operation Tests (TDD Cycles 56-65)

**Test Pattern for Loops:**
```c
void test_simple_for_loop(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Simulated for loop: i = 0; i < 5; i++
    // ldc.i4 0, loop_start: dup, ldc.i4 5, blt loop_body, ret, loop_body: ...
    uint8_t bytecode[] = {
        OP_LDC_I4, 0,      // i = 0
        OP_BR, 2,          // Skip increment initially
        // increment: (would be handled by caller)
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 6);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    
    destroy_test_interpreter(interpreter);
}
```

## Phase 4: Method Invocation & Advanced Features (TDD)
*Duration: 4 weeks | Target: 40 opcodes | ~320 test cases*

### 4.1 Method Invocation Tests (TDD Cycles 66-75)

**RED Phase - CALL Tests:**
```c
void test_call_basic_method(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Setup method metadata
    CILMethod simple_method = {
        .name = "AddTwoNumbers",
        .signature = "(int32, int32)int32",
        .code = create_add_method(),
        .code_length = 10,
        .parameter_count = 2,
        .locals_count = 0,
        .max_stack = 4
    };
    
    cil_interpreter_register_method(interpreter, &simple_method);
    
    // ldc.i4 10, ldc.i4 32, call AddTwoNumbers, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 10,
        OP_LDC_I4, 32,
        OP_CALL, METHOD_TOKEN_ADD,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 7);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT32(42, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_call_stack_overflow(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    interpreter->call_stack_max_depth = 2; // Limit recursion
    
    // Recursive call that exceeds limit
    uint8_t bytecode[] = {
        OP_LDC_I4, 1,
        OP_CALL, METHOD_TOKEN_RECURSE,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_STACK_OVERFLOW, result);
    
    destroy_test_interpreter(interpreter);
}
```

### 4.2 Advanced Type Operation Tests (TDD Cycles 76-85)

**RED Phase - Type Conversion Tests:**
```c
void test_conv_i2_i4(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i2 1000, conv.i4, ret
    uint8_t bytecode[] = {
        OP_LDC_I2, 0xE8, 0x03,  // 1000 as int16
        OP_CONV_I4,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 5);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT32(1000, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}

void test_conv_ovf_i2_i4_overflow(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // ldc.i4 40000, conv.ovf.i2 (should overflow)
    uint8_t bytecode[] = {
        OP_LDC_I4, 0x40, 0x9C, 0x00, 0x00,  // 40000 as int32
        OP_CONV_OVF_I2,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 7);
    
    TEST_ASSERT_EQUAL_INT(CIL_OVERFLOW, result);
    
    destroy_test_interpreter(interpreter);
}

void test_isinst_valid_cast(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Create object of type A, test if it's of type B (compatible)
    uint8_t bytecode[] = {
        OP_NEWOBJ, TYPE_TOKEN_A,
        OP_ISINST, TYPE_TOKEN_B,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 4);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    // Should succeed (assuming inheritance relationship)
    
    destroy_test_interpreter(interpreter);
}
```

### 4.3 Array Advanced Tests (TDD Cycles 86-95)

**Multi-dimensional Array Tests:**
```c
void test_newarr_multidimensional(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // newarr [3,4] int32
    uint8_t bytecode[] = {
        OP_LDC_I4, 3,
        OP_LDC_I4, 4,
        OP_NEWARR, TYPE_I4, 0x02,  // 2D array
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 6);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    
    CILArray* array = (CILArray*)interpreter->stack[0];
    TEST_ASSERT_EQUAL_INT32(2, array->rank);
    TEST_ASSERT_EQUAL_INT32(3, array->dimensions[0]);
    TEST_ASSERT_EQUAL_INT32(4, array->dimensions[1]);
    
    destroy_test_interpreter(interpreter);
}

void test_ldelema_address_calculation(void) {
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Create 2D array and get element address
    // newarr [3,4], ldc.i4 1, ldc.i4 2, ldelema int32, ret
    uint8_t bytecode[] = {
        OP_LDC_I4, 3,
        OP_LDC_I4, 4,
        OP_NEWARR, TYPE_I4, 0x02,
        OP_LDC_I4, 1,
        OP_LDC_I4, 2,
        OP_LDELEMA, TYPE_I4,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter, bytecode, 11);
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, interpreter->stack_size);
    
    // Verify address calculation for 2D array [3,4]
    // Element [1,2] should be at offset 1*4 + 2 = 6
    int32_t expected_address = /* base_address */ + (6 * sizeof(int32_t));
    TEST_ASSERT_EQUAL_INT32(expected_address, interpreter->stack[0]);
    
    destroy_test_interpreter(interpreter);
}
```

## Phase 5: Complete ECMA-335 Compliance (TDD)
*Duration: 4 weeks | Target: All remaining opcodes | ~400 test cases*

### 5.1 Complete Opcode Coverage Tests (TDD Cycles 96-150)

**Comprehensive Test Suite for All Opcodes:**
```c
void test_all_load_opcodes(void) {
    // Test all LDC variants
    test_ldc_i4_opcodes();
    test_ldc_i8_opcodes();
    test_ldc_r4_opcodes();
    test_ldc_r8_opcodes();
    test_ldlen_opcodes();
    test_ldarg_opcodes();
    test_ldloc_opcodes();
}

void test_all_store_opcodes(void) {
    // Test all ST variants
    test_stloc_opcodes();
    test_starg_opcodes();
    test_stelem_opcodes();
    test_stfld_opcodes();
}

void test_all_arithmetic_opcodes(void) {
    // Test all arithmetic operations
    test_add_variants();
    test_sub_variants();
    test_mul_variants();
    test_div_variants();
    test_rem_variants();
    test_neg_opcodes();
}

void test_all_conversion_opcodes(void) {
    // Test all conversion operations
    test_conv_opcodes();
    test_conv_ovf_opcodes();
    test_conv_un_opcodes();
}
```

### 5.2 ECMA-335 Compliance Validation Tests (TDD Cycles 151-175)

**Specification Compliance Test Generator:**
```c
typedef struct {
    OpCode opcode;
    ECMA335StackEffect expected_stack_effect;
    ECMA335TypeSystem expected_type_requirements;
    ECMA335ExceptionBehavior expected_exceptions;
    uint8_t test_bytecode[10];
    size_t test_bytecode_length;
} ECMA335ComplianceTest;

void generate_ecma335_compliance_tests(void) {
    // Load ECMA-335 specification data
    ECMA335SpecData* spec = load_ecma335_specification();
    
    for (int i = 0; i < spec->opcode_count; i++) {
        ECMA335OpcodeSpec* opcode_spec = &spec->opcodes[i];
        
        // Generate test for each opcode based on specification
        ECMA335ComplianceTest* test = generate_compliance_test(opcode_spec);
        
        // Run compliance test
        run_compliance_test(test);
        
        // Verify specification compliance
        verify_stack_effect(test, opcode_spec->stack_effect);
        verify_type_requirements(test, opcode_spec->type_requirements);
        verify_exception_behavior(test, opcode_spec->exception_behavior);
    }
}
```

### 5.3 Performance Benchmarking Tests (TDD Cycles 176-185)

**Performance Test Framework:**
```c
void test_interpreter_performance_benchmarks(void) {
    uint64_t start_time, end_time;
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Fibonacci performance test
    uint8_t fibonacci_program[] = {
        // Optimized Fibonacci implementation
        OP_LDC_I4, 30,
        OP_CALL, METHOD_TOKEN_FIBONACCI,
        OP_RET
    };
    
    start_time = get_ticks();
    CILResult result = cil_interpreter_execute(interpreter, 
                                              fibonacci_program, 
                                              sizeof(fibonacci_program));
    end_time = get_ticks();
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    
    uint64_t execution_time = end_time - start_time;
    // Performance target: < 1ms for Fibonacci(30)
    TEST_ASSERT_LESS_THAN_UINT64(1000000, execution_time);
    
    destroy_test_interpreter(interpreter);
}

void test_stack_operation_performance(void) {
    // Test stack operation throughput
    CILInterpreter* interpreter = create_test_interpreter();
    
    uint8_t stack_test_program[] = {
        OP_LDC_I4, 0,  // Initialize counter
        OP_BR, 2,      // Skip increment initially
        // Loop body (would be implemented with proper branching)
        OP_RET
    };
    
    uint64_t start_time = get_ticks();
    CILResult result = cil_interpreter_execute(interpreter,
                                              stack_test_program,
                                              sizeof(stack_test_program));
    uint64_t end_time = get_ticks();
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    
    // Verify acceptable performance for stack operations
    uint64_t execution_time = end_time - start_time;
    TEST_ASSERT_LESS_THAN_UINT64(100000, execution_time); // < 100us
    
    destroy_test_interpreter(interpreter);
}
```

### 5.4 Integration and System Tests (TDD Cycles 186-200)

**End-to-End Test Scenarios:**
```c
void test_complete_cil_program_execution(void) {
    // Test complete CIL program with multiple features
    // This would include:
    // - Class definition
    // - Method implementation
    // - Exception handling
    // - Array operations
    // - Type conversions
    // - Control flow
    
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Load complete CIL assembly
    uint8_t complete_program[] = {
        // .class definition would be parsed and loaded
        // Method implementations
        // Exception handlers
        // etc.
    };
    
    CILResult result = cil_interpreter_execute(interpreter,
                                              complete_program,
                                              sizeof(complete_program));
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    
    // Verify program execution results
    verify_program_output(interpreter);
    
    destroy_test_interpreter(interpreter);
}

void test_garbage_collection_safety(void) {
    // Test GC safety guarantees
    CILInterpreter* interpreter = create_test_interpreter();
    
    // Create object, create reference, remove direct reference
    // Verify object is still accessible through reference
    uint8_t gc_test_program[] = {
        OP_NEWOBJ, TYPE_SIMPLE,
        OP_DUP,
        OP_STLOC, 0,     // Store in local
        OP_POP,          // Remove from stack
        OP_LDLOC, 0,     // Reload from local
        OP_CALL, METHOD_USING_OBJECT,
        OP_RET
    };
    
    CILResult result = cil_interpreter_execute(interpreter,
                                              gc_test_program,
                                              sizeof(gc_test_program));
    
    TEST_ASSERT_EQUAL_INT(CIL_SUCCESS, result);
    // Verify object was still accessible (not garbage collected)
    
    destroy_test_interpreter(interpreter);
}
```

## TDD Infrastructure Requirements

### 1. Test Framework Implementation

**Unity Test Framework Integration:**
```c
// test/test_framework.c
#include "unity.h"
#include "cil_interpreter.h"

// Test group registration
TEST_GROUP(CoreStackTests);
TEST_GROUP(ArithmeticTests);
TEST_GROUP(ObjectTests);
TEST_GROUP(ControlFlowTests);
TEST_GROUP(MethodInvocationTests);
TEST_GROUP(AdvancedTypeTests);
TEST_GROUP(ComplianceTests);

// Standard test setup/teardown
void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

// Test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST_GROUP(CoreStackTests);
    RUN_TEST_GROUP(ArithmeticTests);
    RUN_TEST_GROUP(ObjectTests);
    RUN_TEST_GROUP(ControlFlowTests);
    RUN_TEST_GROUP(MethodInvocationTests);
    RUN_TEST_GROUP(AdvancedTypeTests);
    RUN_TEST_GROUP(ComplianceTests);
    
    return UNITY_END();
}
```

### 2. ECMA-335 Specification Parser

**Specification Data Structure:**
```c
typedef struct {
    char* opcode_name;
    OpCode opcode;
    uint8_t operand_count;
    ECMA335OperandType* operand_types;
    ECMA335StackEffect stack_effect;
    ECMA335TypeSystem type_system;
    ECMA335ExceptionBehavior exception_behavior;
    char* description;
} ECMA335OpcodeSpec;

typedef struct {
    ECMA335OpcodeSpec* opcodes;
    size_t opcode_count;
    HashTable* opcode_by_name;
    HashTable* opcode_by_code;
} ECMA335SpecData;

ECMA335SpecData* load_ecma335_specification(const char* spec_file) {
    // Parse ECMA-335 specification from JSON/XML
    // Create structured data for test generation
}
```

### 3. Automated Test Generation

**Test Generator Implementation:**
```c
void generate_opcode_tests(ECMA335OpcodeSpec* spec) {
    // Generate comprehensive test cases for each opcode
    
    // Basic functionality test
    generate_basic_test(spec);
    
    // Edge case tests
    generate_edge_case_tests(spec);
    
    // Type safety tests
    generate_type_safety_tests(spec);
    
    // Stack effect tests
    generate_stack_effect_tests(spec);
    
    // Exception handling tests
    generate_exception_tests(spec);
    
    // Performance tests
    generate_performance_tests(spec);
}
```

### 4. Continuous Integration Setup

**Automated TDD Workflow:**
```bash
#!/bin/sh
# run_tdd_cycle.sh

OPCODE=$1
PHASE=$2

echo "=== TDD Cycle for $OPCODE (Phase $PHASE) ==="

echo "=== RED PHASE: Writing Failing Tests ==="
make test_opcode_${OPCODE} 2>&1 | tee test_output.log
if [ $? -eq 0 ]; then
    echo "ERROR: Test should have failed but passed!"
    exit 1
fi

echo "=== GREEN PHASE: Implementing Minimum Code ==="
# Apply minimal implementation changes
apply_implementation_${OPCODE}()

make test_opcode_${OPCODE} 2>&1 | tee -a test_output.log
if [ $? -ne 0 ]; then
    echo "ERROR: Implementation doesn't pass tests!"
    exit 1
fi

echo "=== REFACTOR PHASE: Code Improvement ==="
# Refactor while maintaining green state
refactor_opcode_${OPCODE}()

make test_opcode_${OPCODE} 2>&1 | tee -a test_output.log
if [ $? -ne 0 ]; then
    echo "ERROR: Refactoring broke functionality!"
    exit 1
fi

echo "=== REGRESSION TEST: Ensuring No Breakage ==="
make test_full_suite 2>&1 | tee -a test_output.log
if [ $? -ne 0 ]; then
    echo "ERROR: Regression detected!"
    exit 1
fi

echo "SUCCESS: TDD cycle completed for $OPCODE"
```

### 5. Code Coverage Analysis

**Coverage Measurement:**
```c
typedef struct {
    uint32_t total_opcodes;
    uint32_t implemented_opcodes;
    uint32_t tested_opcodes;
    uint32_t fully_tested_opcodes;
    float coverage_percentage;
    HashSet* tested_opcode_set;
    HashSet* fully_tested_opcode_set;
} CoverageReport;

CoverageReport generate_coverage_report(void) {
    CoverageReport report = {0};
    
    // Count implemented opcodes
    report.implemented_opcodes = count_implemented_opcodes();
    
    // Count tested opcodes
    report.tested_opcodes = count_tested_opcodes();
    
    // Count fully tested opcodes (all test categories pass)
    report.fully_tested_opcodes = count_fully_tested_opcodes();
    
    // Calculate coverage percentage
    if (report.implemented_opcodes > 0) {
        report.coverage_percentage = 
            ((float)report.fully_tested_opcodes / report.implemented_opcodes) * 100.0f;
    }
    
    return report;
}

void print_coverage_report(CoverageReport* report) {
    printf("CIL Interpreter Coverage Report\n");
    printf("================================\n");
    printf("Total Opcodes: %u\n", report->total_opcodes);
    printf("Implemented: %u\n", report->implemented_opcodes);
    printf("Tested: %u\n", report->tested_opcodes);
    printf("Fully Tested: %u\n", report->fully_tested_opcodes);
    printf("Coverage: %.2f%%\n", report->coverage_percentage);
    
    if (report->coverage_percentage < 100.0f) {
        printf("\nMissing full test coverage for:\n");
        list_opcodes_needing_tests(report);
    }
}
```

## Success Metrics & Validation

### 1. Test Coverage Targets

| Phase | Opcodes | Test Cases | Coverage Target | Duration |
|-------|---------|------------|-----------------|----------|
| Phase 1 | 25 | ~200 | 100% | 3 weeks |
| Phase 2 | 35 | ~280 | 100% | 4 weeks |
| Phase 3 | 30 | ~240 | 100% | 3 weeks |
| Phase 4 | 40 | ~320 | 100% | 4 weeks |
| Phase 5 | 70+ | ~400 | 100% | 4 weeks |
| **Total** | **200+** | **~1440** | **100%** | **18 weeks** |

### 2. Quality Metrics

**Code Quality Targets:**
- **Cyclomatic Complexity**: < 10 per function
- **Test Coverage**: 100% of implemented opcodes
- **Code Duplication**: < 3% (using automated tools)
- **Technical Debt Ratio**: < 5%

**Performance Targets:**
- **Interpreter Overhead**: < 2x vs reference implementation
- **Memory Usage**: < 10MB for typical programs
- **Startup Time**: < 100ms for basic programs
- **Throughput**: > 1M opcodes/second

### 3. ECMA-335 Compliance Validation

**Compliance Checklist:**
- [ ] All 200+ opcodes implemented
- [ ] Stack semantics match specification exactly
- [ ] Exception handling follows ECMA-335 rules
- [ ] Type system behavior is specification-compliant
- [ ] Memory safety guarantees are maintained
- [ ] Performance characteristics are acceptable

### 4. Integration Success Criteria

**System Integration Tests:**
- [ ] Complete CIL assemblies execute successfully
- [ ] Interoperability with existing .NET programs
- [ ] Garbage collection safety is maintained
- [ ] Thread safety guarantees are provided
- [ ] Debugging and profiling support is available

## Risk Mitigation Strategies

### 1. Specification Ambiguity
**Risk**: ECMA-335 interpretation differences between implementations
**Mitigation**:
- Cross-reference multiple .NET implementations (Mono, CoreCLR)
- Participate in ECMA-335 community discussions
- Implement comprehensive specification compliance tests
- Use official Microsoft .NET Framework as reference

### 2. Performance Degradation
**Risk**: TDD focus on correctness may lead to performance issues
**Mitigation**:
- Include performance tests in each TDD cycle
- Profile critical paths regularly
- Implement performance regression detection
- Balance correctness with performance throughout development

### 3. Scope Creep
**Risk**: Adding non-standard features that deviate from ECMA-335
**Mitigation**:
- Strict adherence to ECMA-335 specification
- Separate experimental features from compliance implementation
- Regular specification compliance audits
- Clear boundaries between core interpreter and extensions

### 4. Test Maintenance Overhead
**Risk**: Brittle tests that break with minor implementation changes
**Mitigation**:
- Focus on behavior-based testing rather than implementation details
- Use specification-driven test generation where possible
- Minimize coupling between tests and internal implementation
- Regular test refactoring during TDD cycles

## Implementation Timeline

### Phase 1: Core Infrastructure (Weeks 1-3)
**Week 1**: Test infrastructure setup and basic interpreter framework
**Week 2**: Stack operations implementation with full test coverage
**Week 3**: Arithmetic operations and overflow detection

### Phase 2: Memory & Object Operations (Weeks 4-7)
**Week 4**: Object creation and basic memory management
**Week 5**: Array operations and bounds checking
**Week 6**: Field operations and object structure handling
**Week 7**: Memory safety and garbage collection integration

### Phase 3: Control Flow (Weeks 8-10)
**Week 8**: Branch operations and condition evaluation
**Week 9**: Exception handling implementation
**Week 10**: Advanced control flow (loops, switch statements)

### Phase 4: Advanced Features (Weeks 11-14)
**Week 11**: Method invocation and call stack management
**Week 12**: Advanced type operations and conversions
**Week 13**: Multi-dimensional arrays and complex data structures
**Week 14**: Performance optimization and profiling integration

### Phase 5: Complete Compliance (Weeks 15-18)
**Week 15**: Remaining opcode implementation and testing
**Week 16**: Comprehensive compliance validation
**Week 17**: System integration testing and bug fixes
**Week 18**: Final optimization and documentation

## Tools and Environment Setup

### 1. Development Environment
```bash
# Required tools
- C compiler with C99 support (gcc/clang)
- Unit testing framework (Unity)
- Code coverage tool (gcov/lcov)
- Performance profiler (perf/valgrind)
- ECMA-335 specification parser
```

### 2. Build System Integration
```makefile
# Makefile integration
.PHONY: test test-opcode-% test-phase-% coverage compliance

test: test-phase-1 test-phase-2 test-phase-3 test-phase-4 test-phase-5
	@echo "All TDD tests completed"

test-phase-%:
	@echo "Running Phase $* tests"
	$(MAKE) test-opcode-$(PHASE_$*_OPCODES)

coverage:
	$(MAKE) test
	gcov cil_interpreter.c
	lcov --capture --directory . --output-file coverage.info

compliance: test
	./tools/validate_ecma335_compliance
```

### 3. Continuous Integration Pipeline
```yaml
# .github/workflows/tdd.yml
name: CIL Interpreter TDD
on: [push, pull_request]

jobs:
  tdd-cycle:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        phase: [1, 2, 3, 4, 5]
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Setup build environment
      run: |
        sudo apt-get update
        sudo apt-get install build-essential lcov
    
    - name: Run T
      run:DD cycle for phase |
        make test-phase-${{ matrix.phase }}
        make coverage
    
    - name: Upload coverage reports
      uses: codecov/codecov-action@v1
```

## Conclusion

This comprehensive TDD implementation plan provides a structured, rigorous approach to building a fully ECMA-335 compliant CIL interpreter. By following Red-Green-Refactor cycles for every opcode and maintaining 100% test coverage, we ensure both correctness and maintainability.

The plan's success depends on:
1. **Discipline**: Strict adherence to TDD methodology
2. **Quality**: High-quality tests based on ECMA-335 specification
3. **Incremental Progress**: Small, verifiable steps forward
4. **Continuous Validation**: Ongoing compliance verification
5. **Performance Awareness**: Balancing correctness with efficiency

The 18-week timeline provides realistic milestones for achieving full compliance while maintaining code quality throughout the development process. The comprehensive test suite (1400+ test cases) ensures robust coverage of all CIL operations and edge cases.

Success metrics and risk mitigation strategies are built into each phase, providing clear validation criteria and proactive problem resolution. The final result will be a production-ready CIL interpreter that meets all ECMA-335 requirements and can serve as a foundation for .NET compatibility on the Plan 9 platform.