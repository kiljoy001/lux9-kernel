# Documentation-Driven TDD Workflow for CIL Opcode Implementation

## Overview
This document outlines a comprehensive Test-Driven Development (TDD) workflow for implementing CIL opcodes based on ECMA-335 specification requirements.

## Core Principles

### 1. Specification as Source of Truth
- **ECMA-335 specification** is the authoritative source for all opcode behavior
- **Test-first development**: Create tests before implementation
- **Documentation-driven**: Specification requirements become test requirements

### 2. Systematic Implementation
- **Section-by-section approach**: Follow ECMA-335 organization
- **Dependency-aware**: Consider opcode prerequisites and relationships
- **Quality gates**: Validation at each milestone

## TDD Workflow for Each Opcode

### Step 1: Specification Analysis
**Goal**: Extract complete requirements from ECMA-335

**Process**:
1. **Locate opcode in ECMA-335** (Partition III, specific section)
2. **Extract requirements**:
   - Stack effect (before → after)
   - Operand types and constraints
   - Exception conditions
   - Type safety requirements
   - Verification rules
   - Encoding format
3. **Document edge cases** and special behaviors
4. **Identify dependencies** (helper functions, type system support)

**Deliverable**: Requirements specification document

### Step 2: Test Case Design
**Goal**: Create comprehensive test suite based on specification

**Test Categories**:

#### A. Basic Functionality Tests
- **Happy path**: Standard operation with valid inputs
- **Stack effects**: Verify exact stack state changes
- **Type compatibility**: Test with all valid type combinations

#### B. Edge Case Tests
- **Boundary values**: Min/max values for numeric types
- **Type transitions**: Conversions and coercions
- **Special values**: NaN, infinity, null, zero

#### C. Error Condition Tests
- **Invalid operands**: Wrong types, stack underflow
- **Exception throwing**: Verify correct exception types and messages
- **Runtime errors**: Overflow, underflow, invalid operations

#### D. Integration Tests
- **Opcode combinations**: Sequences of related opcodes
- **Method context**: Behavior in actual method execution
- **Type system integration**: Object model interactions

#### E. Compliance Tests
- **ECMA-335 conformance**: Exact specification compliance
- **Verification compatibility**: Verifiable CIL subset rules
- **Performance benchmarks**: Execution time and memory usage

**Template for Test Case**:
```c
TEST(test_opcode_NAME_basic) {
    // Setup
    vm_init_t init = {0};
    vm_execution_state_t* state = vm_create_execution_state(&init);
    
    // Create bytecode for opcode under test
    uint8_t bytecode[] = {
        // ... operands ...
        OPCODE,  // The opcode being tested
    };
    
    // Execute
    bool result = vm_execute_method(state, bytecode, sizeof(bytecode), 0);
    
    // Verify stack effects
    vm_value_t result_value;
    TEST_ASSERT_TRUE(vm_stack_pop(state, &result_value));
    TEST_ASSERT_EQUAL(EXPECTED_TYPE, result_value.type);
    TEST_ASSERT_EQUAL(EXPECTED_VALUE, result_value.value.i4);
    
    // Cleanup
    vm_destroy_execution_state(state);
    return TEST_PASSED;
}
```

### Step 3: Implementation
**Goal**: Implement opcode to pass all test cases

**Implementation Pattern**:
```c
case CIL_OPCODE_NAME: {
    // 1. Operand validation (if applicable)
    if (!validate_operands(state, ip)) return false;
    
    // 2. Stack validation
    if (!validate_stack_state(state, expected_inputs)) return false;
    
    // 3. Core operation
    if (!execute_core_operation(state)) {
        vm_set_error(state, "Operation failed");
        return false;
    }
    
    // 4. Stack update
    update_stack_state(state, expected_outputs);
    
    // 5. Exception handling (if applicable)
    handle_exceptions(state);
    
    break;
}
```

**Implementation Guidelines**:
- **Fail-fast**: Validate all preconditions early
- **Clear error messages**: Descriptive error reporting
- **Type safety**: Enforce type system constraints
- **Performance**: Optimize for common cases
- **Maintainability**: Clear, readable code structure

### Step 4: Validation and Refinement
**Goal**: Ensure specification compliance and quality

**Validation Steps**:
1. **Run all test cases** - ensure 100% pass rate
2. **Code review** - check implementation quality
3. **Performance testing** - verify acceptable performance
4. **Memory leak detection** - ensure proper resource management
5. **Integration testing** - test with existing opcode implementations

### Step 5: Documentation
**Goal**: Document implementation and decisions

**Documentation Elements**:
- **Implementation notes**: Key design decisions
- **Performance characteristics**: Timing and memory usage
- **Known limitations**: Any deviations from ideal behavior
- **Future improvements**: Potential optimizations or extensions
- **Test coverage**: Which scenarios are covered

## Phase-Based Implementation Strategy

### Phase 2: Data Conversion & Extended Arithmetic
**Focus**: Type conversions and overflow arithmetic

**Prerequisites**:
- Type conversion utilities
- Overflow detection algorithms
- Comprehensive test framework

**Sample TDD Cycle for CONV_I4**:

```c
// Step 1: Requirements Analysis
// ECMA-335 III.3.27: CONV_I4
// - Stack: value → int32(value)
// - Converts any numeric type to int32
// - Overflow: throws OverflowException
// - Truncation: behavior for float-to-int

// Step 2: Test Cases
TEST(test_conv_i4_from_i1) {
    // Test conversion from int8 to int32
    vm_value_t input = vm_make_i1(-1);
    vm_value_t result;
    bool success = vm_convert(&input, VM_TYPE_I4, &result);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(-1, result.value.i4);
}

TEST(test_conv_i4_overflow) {
    // Test overflow detection
    vm_value_t input = vm_make_i8(INT64_MAX);
    vm_value_t result;
    bool success = vm_convert(&input, VM_TYPE_I4, &result);
    TEST_ASSERT_FALSE(success); // Should fail due to overflow
}

// Step 3: Implementation
bool vm_convert(vm_value_t* source, vm_type_t target_type, vm_value_t* result) {
    switch (target_type) {
        case VM_TYPE_I4:
            switch (source->type) {
                case VM_TYPE_I1:
                    result->type = VM_TYPE_I4;
                    result->value.i4 = source->value.i1;
                    return true;
                // ... other source types ...
                default:
                    return false;
            }
        // ... other target types ...
    }
}
```

### Phase 3: Memory Access & Indirect Operations
**Focus**: Pointer operations and memory management

**Prerequisites**:
- Memory management system
- Pointer validation utilities
- Type-safe memory access

**Sample TDD Cycle for LDIND_I4**:

```c
// Step 1: Requirements Analysis
// ECMA-335 III.3.33: LDIND_I4
// - Stack: address → value
// - Loads 4-byte signed integer from address
// - Alignment: may require 4-byte alignment
// - Access: must be within bounds

// Step 2: Test Cases
TEST(test_ldind_i4_basic) {
    // Setup memory with known value
    int32_t test_value = 42;
    void* test_addr = &test_value;
    
    vm_value_t addr = vm_make_ref(test_addr);
    vm_stack_push(state, &addr);
    
    // Execute LDIND_I4
    execute_opcode(CIL_OPCODE_LDIND_I4);
    
    // Verify result
    vm_value_t result;
    vm_stack_pop(state, &result);
    TEST_ASSERT_EQUAL(VM_TYPE_I4, result.type);
    TEST_ASSERT_EQUAL(42, result.value.i4);
}

// Step 3: Implementation
case CIL_OPCODE_LDIND_I4: {
    vm_value_t addr_value;
    if (!vm_stack_pop(state, &addr_value)) return false;
    
    if (!vm_is_valid_pointer(addr_value.ref)) {
        vm_set_error(state, "Invalid pointer");
        return false;
    }
    
    int32_t value = *(int32_t*)addr_value.ref;
    vm_value_t result = vm_make_i4(value);
    vm_stack_push(state, &result);
    break;
}
```

## Quality Assurance Framework

### Test Coverage Requirements
- **Line coverage**: ≥ 95% for implemented opcodes
- **Branch coverage**: ≥ 90% for control flow
- **Path coverage**: All reasonable execution paths
- **Edge case coverage**: Boundary conditions and error paths

### Performance Benchmarks
- **Execution time**: < 100ns per opcode (typical case)
- **Memory overhead**: < 16 bytes per opcode execution
- **Stack usage**: Predictable and bounded

### Security Validation
- **Bounds checking**: All memory accesses validated
- **Type safety**: No unsafe type conversions
- **Exception safety**: No resource leaks on exceptions
- **Input validation**: All operands validated

### Compliance Verification
- **ECMA-335 conformance**: Byte-level encoding compliance
- **Stack effect accuracy**: Exact specification compliance
- **Exception behavior**: Correct exception types and messages
- **Verification compatibility**: Support for verifiable CIL subset

## Continuous Integration

### Automated Testing Pipeline
1. **Unit tests**: Individual opcode tests
2. **Integration tests**: Opcode combination tests
3. **Compliance tests**: ECMA-335 specification tests
4. **Performance tests**: Benchmark regression tests
5. **Security tests**: Safety and bounds checking tests

### Build System Integration
```makefile
# Test targets
test-unit: $(UNIT_TEST_TARGETS)
test-integration: $(INTEGRATION_TEST_TARGETS)
test-compliance: $(COMPLIANCE_TEST_TARGETS)
test-performance: $(PERFORMANCE_TEST_TARGETS)

# Comprehensive test suite
test-all: test-unit test-integration test-compliance test-performance

# Quality gates
quality-check: test-all
	@echo "Running quality gates..."
	@$(CHECK_COVERAGE) --min=95
	@$(CHECK_PERFORMANCE) --max=100ns
	@$(CHECK_SECURITY) --validate
```

## Progress Tracking and Metrics

### Implementation Metrics
- **Opcodes implemented**: Count and percentage
- **Test coverage**: Lines, branches, paths
- **Performance benchmarks**: Execution time per opcode
- **Defect density**: Bugs per opcode implementation

### Quality Metrics
- **Test pass rate**: 100% required for all phases
- **Code review coverage**: All code reviewed
- **Documentation completeness**: All opcodes documented
- **Compliance score**: ECMA-335 conformance percentage

### Milestone Progress
- **Phase completion**: Target dates and actual dates
- **Quality gates passed**: Each phase validation
- **Risk assessment**: Implementation risks and mitigation
- **Resource utilization**: Time and effort tracking

This TDD workflow ensures systematic, high-quality implementation of all CIL opcodes with full compliance to the ECMA-335 specification.
