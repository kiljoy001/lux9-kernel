# C# Init System Test Suite

## Overview

This test suite provides practical, implementable tests for the C# init system that validate the complete ETL pipeline: **F# → .NET DLL → CIL → Fruity IR → QBE IL → x86-64**.

## Quick Start (5-15 minutes)

```bash
# Build and test individual components
make smoke          # Basic operations (5 min)
make collections    # Collections test (5 min) 
make etl           # ETL pipeline test (10 min)

# Or build everything at once
make all

# Run all tests
make test
```

## Test Categories

### 1. Smoke Tests (5 minutes)
**File:** `test_cs_init_smoke.cs`

**Validates:**
- Basic console output via `Write()` external call
- String concatenation and manipulation
- Integer arithmetic (addition, subtraction, multiplication, division, modulus)
- Boolean logic (AND, OR, NOT operations)
- Method compilation and calling

**Implementation Time:** 5 minutes to build and run
**Success Criteria:** All basic operations execute without runtime errors

### 2. Collections Test (5 minutes)
**File:** `test_cs_init_collections.cs`

**Validates:**
- `List<string>` operations (Add, Count, foreach iteration)
- `List<int>` operations and aggregation
- `Dictionary<string, int>` operations
- Generic type compilation and instantiation
- Collection enumeration performance

**Implementation Time:** 5 minutes to build and run
**Success Criteria:** All collections operations work correctly

### 3. ETL Pipeline Test (10 minutes)
**File:** `test_cs_init_etl.cs`

**Validates:**
- Complete ETL pipeline: F# data structures → C# business logic → transformation → aggregation
- Object-oriented programming (classes, constructors, properties)
- Complex data transformations
- LINQ-like operations on collections
- Memory management for complex objects

**Implementation Time:** 10 minutes to build and run
**Success Criteria:** ETL pipeline completes with correct data transformations

### 4. Control Flow Test (5 minutes)
**File:** `test_cs_init_controlflow.cs`

**Validates:**
- If/else conditional statements
- For loops and iteration
- While loops and termination conditions
- Switch statements and case handling
- Nested control structures

**Implementation Time:** 5 minutes to build and run
**Success Criteria:** All control flow constructs execute correctly

### 5. Boot Sequence Test (5 minutes)
**File:** `test_cs_init_boot.cs`

**Validates:**
- Multi-stage initialization process
- Method calling and parameter passing
- String formatting and output
- Simulated system boot sequence
- Error handling during initialization

**Implementation Time:** 5 minutes to build and run
**Success Criteria:** Boot sequence completes without errors

### 6. Integration Test (10 minutes)
**File:** `test_cs_init_integration.cs`

**Validates:**
- Integration with existing Lux9 systems:
  - 9P file system operations
  - AHCI storage controller
  - Cryptographic service
- Cross-system data flow
- Service initialization and communication
- Mock object patterns for system integration

**Implementation Time:** 10 minutes to build and run
**Success Criteria:** Integration with Lux9 systems works correctly

### 7. Error Handling Test (5 minutes)
**File:** `test_cs_init_error.cs`

**Validates:**
- Null reference handling
- Array bounds checking
- Division by zero protection
- String operation edge cases
- Loop boundary conditions
- Type conversion safety

**Implementation Time:** 5 minutes to build and run
**Success Criteria:** Error conditions handled gracefully

### 8. Performance Baseline Test (10 minutes)
**File:** `test_cs_init_performance.cs`

**Validates:**
- String concatenation performance
- Loop operation efficiency
- Array operation speed
- List operation throughput
- Method call overhead
- Memory allocation patterns

**Implementation Time:** 10 minutes to build and run
**Success Criteria:** Performance meets expected baselines

### 9. Master Test Runner (5 minutes)
**File:** `test_cs_init_runner.cs`

**Validates:**
- Comprehensive test orchestration
- Exception handling and reporting
- Test result aggregation
- System health monitoring
- Automated validation

**Implementation Time:** 5 minutes to build and run
**Success Criteria:** All tests pass with proper reporting

## ETL Pipeline Validation

Each test validates a specific part of the ETL pipeline:

```
F# Source → .NET DLL → CIL → Fruity IR → QBE IL → x86-64 → Runtime
    ↓           ↓       ↓         ↓         ↓         ↓         ↓
  Data      Assembly  Bytecode  IR Nodes   LLVM-like  Machine   Execution
Structure  Format    Format    Format     IL         Code      Results
```

### Pipeline Validation Points:

1. **F# → .NET DLL**: Object creation and generic types
2. **.NET DLL → CIL**: Method compilation and external calls
3. **CIL → Fruity IR**: Control flow and type conversions
4. **Fruity IR → QBE IL**: Memory layout and function calls
5. **QBE IL → x86-64**: Machine code generation
6. **x86-64 → Runtime**: External call binding (clr_write, clr_newobj, clr_string_from_literal)

## Build Requirements

- .NET 9.0 SDK or later
- Linux x64 environment
- AOT (Ahead-of-Time) compilation support

## Runtime Requirements

The compiled AOT binaries require the Lux9 runtime shim with:
- `clr_write(StringObject*)` - Console output
- `clr_newobj(uint32_t)` - Object allocation
- `clr_string_from_literal(int)` - String literal creation
- `lux_alloc(size_t, uint32_t)` - Memory allocation

## Expected Output

### Successful Test Execution:
```
=== C# INIT SMOKE TEST 1: Basic Operations ===
Test 1: String concatenation...
Hello World
Test 2: Math operations...
a = 42, b = 8
a + b = 50
a - b = 34
[... more output ...]
SMOKE TEST 1: PASSED
```

### Master Test Runner Output:
```
=== C# INIT SYSTEM TEST RUNNER ===
Running comprehensive test suite...

TEST 1/6: Running smoke test...
PASS: Smoke test completed

TEST 2/6: Running collections test...
PASS: Collections test completed

[... more tests ...]

=== TEST RUNNER SUMMARY ===
Tests passed: 6/6
ALL TESTS PASSED! C# Init system is working correctly.
```

## Integration with Existing Systems

These tests integrate with existing Lux9 components:

- **9P File System**: Tests file operations via mock P9FileSystem class
- **AHCI Storage**: Tests storage operations via mock AHCIStorage class  
- **Crypto Service**: Tests cryptographic operations via mock CryptoService class
- **Memory Management**: Tests allocation via lux_alloc
- **Console Output**: Tests output via clr_write

## Troubleshooting

### Common Issues:

1. **Build Failures**: Ensure .NET 9.0 SDK is installed
2. **Runtime Errors**: Check that runtime shim is properly linked
3. **String Literal Errors**: Verify clr_string_from_literal mappings
4. **Memory Issues**: Ensure lux_alloc is properly implemented

### Debug Steps:

1. Build individual tests: `make smoke`
2. Check compilation output for errors
3. Verify runtime host implementation
4. Test with host runtime: `gcc runtime_host.c && ./a.out`

## Performance Expectations

- **Build Time**: 5-15 minutes per test category
- **Runtime**: < 1 second for basic operations
- **Memory**: < 10MB for test execution
- **Success Rate**: 100% for all implemented features

## Future Extensions

- Network service integration tests
- Multi-threading validation
- Real hardware interaction tests
- Stress testing and memory leak detection
- Benchmark comparison with native C code

## Success Criteria

A test category passes if:
1. **Builds successfully** without compilation errors
2. **Runs to completion** without runtime crashes  
3. **Produces expected output** matching test specifications
4. **Integrates properly** with Lux9 runtime environment
5. **Performance meets baselines** for the specific operation type

The complete test suite passes when all 8 categories complete successfully and the master test runner reports 100% success rate.
