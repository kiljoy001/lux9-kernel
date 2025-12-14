# ECMA-335 CIL Opcode Implementation Plan

## Overview
This document provides a comprehensive mapping between ECMA-335 specification sections and CIL opcode implementation status, with a systematic approach for completing all missing opcodes.

## Phase Structure

### Phase 1: Foundation (✅ COMPLETE)
**ECMA-335 Sections**: III.1 (Introduction), Core III.3 (Base Instructions)
**Status**: Foundation established with basic arithmetic, logic, and control flow

**Implemented Opcodes**:
- Stack: NOP, DUP, POP
- Constants: LDC_I4_0, LDC_I4_1, LDC_I4_S, LDC_I4
- Arguments: LDARG_0-3, LDARG_S
- Locals: LDLOC_0-3, LDLOC_S, STLOC_0-3, STLOC_S
- Arithmetic: ADD, SUB, MUL, DIV, DIV_UN, REM, REM_UN
- Bitwise: AND, OR, XOR, NOT, NEG, SHL, SHR, SHR_UN
- Comparisons: CEQ, CGT, CLGT_UN, CLT, CLT_UN
- Branching: BR, BR_S, BEQ, BGE, BGT, BLE, BLT, BNE_UN, BRTRUE, BRFALSE, RET

### Phase 2: Data Conversion & Extended Arithmetic
**ECMA-335 Sections**: III.3.27-29 (Conv operations), III.3.2 (add.ovf), Overflow operations
**Target**: 25 opcodes

**Missing Opcodes**:
1. **Conversion Operations** (Conv.*):
   - CONV_I1, CONV_I2, CONV_I8, CONV_R4, CONV_R8, CONV_U4, CONV_U8
   - CONV_OVF_I1, CONV_OVF_I2, CONV_OVF_I4, CONV_OVF_I8, CONV_OVF_U1, CONV_OVF_U2, CONV_OVF_U4, CONV_OVF_U8
   - CONV_OVF_I1_UN, CONV_OVF_I2_UN, CONV_OVF_I4_UN, CONV_OVF_I8_UN, CONV_OVF_U1_UN, CONV_OVF_U2_UN, CONV_OVF_U4_UN, CONV_OVF_U8_UN
   - CONV_R_UN

2. **Overflow Arithmetic**:
   - ADD_OVF, ADD_OVF_UN, MUL_OVF, MUL_OVF_UN, SUB_OVF, SUB_OVF_UN
   - DIV_OVF, DIV_OVF_UN

**Implementation Strategy**:
- Start with simple conversions (I4 to I8, etc.)
- Add overflow detection logic
- Create comprehensive test cases for edge cases

### Phase 3: Memory Access & Indirect Operations
**ECMA-335 Sections**: III.3.33-37 (ldind/stind), III.3.47 (localloc)
**Target**: 15 opcodes

**Missing Opcodes**:
1. **Indirect Load** (LDIND.*):
   - LDIND_I1, LDIND_U1, LDIND_I2, LDIND_U2, LDIND_I4, LDIND_U4, LDIND_I8, LDIND_I
   - LDIND_R4, LDIND_R8, LDIND_REF

2. **Indirect Store** (STIND.*):
   - STIND_REF, STIND_I1, STIND_I2, STIND_I4, STIND_I8, STIND_R4, STIND_R8

3. **Local Allocation**:
   - LOCALLOC

**Implementation Strategy**:
- Implement pointer validation
- Add type safety checks
- Create memory management integration

### Phase 4: Object Model - Object Creation & Management
**ECMA-335 Sections**: III.4 (Object Model Instructions)
**Target**: 30 opcodes

**Missing Opcodes**:
1. **Object Creation**:
   - NEWOBJ, NEWARR
   - BOX, UNBOX, UNBOX_ANY
   - INITOBJ

2. **String Operations**:
   - LDSTR

3. **Type Operations**:
   - CASTCLASS, ISINST
   - LDTOKEN

4. **Field Operations**:
   - LDFLD, LDFLDA, STFLD
   - LDSFLD, LDSFLDA, STSFLD

5. **Array Operations**:
   - LDLEN
   - LDELEMA, LDELEM_I1, LDELEM_U1, LDELEM_I2, LDELEM_U2, LDELEM_I4, LDELEM_U4
   - LDELEM_I8, LDELEM_I, LDELEM_R4, LDELEM_R8, LDELEM_REF
   - STELEM_I, STELEM_I1, STELEM_I2, STELEM_I4, STELEM_I8, STELEM_R4, STELEM_R8, STELEM_REF
   - LDELEM_ANY, STELEM_ANY

**Implementation Strategy**:
- Start with simple object creation (NEWOBJ)
- Add array support progressively
- Implement type system integration

### Phase 5: Advanced Control Flow
**ECMA-335 Sections**: III.3.5-16 (Extended branching), SWITCH instruction
**Target**: 20 opcodes

**Missing Opcodes**:
1. **Extended Branch Instructions**:
   - BEQ, BGE, BGT, BLE, BLT (4-byte variants)
   - BGE_UN, BGT_UN, BLE_UN, BLT_UN, BNE_UN (both short and long)
   - BR, BRTRUE, BRFALSE (4-byte variants)

2. **Switch Instruction**:
   - SWITCH

**Implementation Strategy**:
- Extend existing branch logic for 4-byte offsets
- Implement switch table dispatch

### Phase 6: Method Calls & Advanced Features
**ECMA-335 Sections**: III.3.19-21 (call instructions), III.3.41-43 (function pointers)
**Target**: 15 opcodes

**Missing Opcodes**:
1. **Method Calls**:
   - CALL, CALLI, CALLVIRT
   - JMP

2. **Function Pointers**:
   - LDFTN, LDVIRTFTN

3. **Advanced Stack**:
   - ARGLIST
   - MKREFANY, REFANYVAL, REFANYTYPE

4. **Exception Handling**:
   - THROW, RETHROW
   - ENDFILTER

**Implementation Strategy**:
- Integrate with metadata system
- Add exception handling framework

### Phase 7: Memory Management & Advanced Types
**ECMA-335 Sections**: III.3.55-57 (cpblk, initblk), advanced type operations
**Target**: 10 opcodes

**Missing Opcodes**:
1. **Memory Block Operations**:
   - CPBLK, INITBLK

2. **Size Operations**:
   - SIZEOF

3. **Prefix Instructions**:
   - CONSTRAINED, READONLY
   - VOLATILE, UNALIGNED, TAIL

4. **Special**:
   - BREAK

**Implementation Strategy**:
- Add platform-specific optimizations
- Implement security checks

### Phase 8: Symbolic Computing Extensions
**Custom Extensions**: Beyond ECMA-335
**Target**: 8 opcodes

**Missing Custom Opcodes**:
1. **Symbolic Operations** (0xE0-0xE7):
   - SYM_CREATE, SYM_EXPR, SYM_DIFF, SYM_INTEGRATE
   - SYM_SIMPLIFY, SYM_EVAL, SYM_MATCH, SYM_REWRITE

**Implementation Strategy**:
- Extend symbolic expression system
- Add calculus operations

## Implementation Order Strategy

### Within Each Phase:
1. **Start with simplest opcodes** (single-byte, no operands)
2. **Progress to complex opcodes** (multi-byte, complex logic)
3. **Add comprehensive tests** before moving to next opcode
4. **Validate against ECMA-335 specification**

### Prerequisites for Each Phase:
- **Phase 2**: Type conversion utilities, overflow detection
- **Phase 3**: Memory management system, pointer validation
- **Phase 4**: Object model, type system integration
- **Phase 5**: Extended branch logic, jump table support
- **Phase 6**: Metadata system, exception framework
- **Phase 7**: Advanced memory management, security
- **Phase 8**: Symbolic math library

## Quality Assurance Strategy

### Testing Approach:
1. **Unit Tests**: Each opcode individually
2. **Integration Tests**: Opcode combinations
3. **Compliance Tests**: ECMA-335 specification conformance
4. **Performance Tests**: Benchmark critical opcodes
5. **Security Tests**: Verify safe execution

### Validation Against ECMA-335:
1. **Opcode Encoding**: Verify byte-level encoding matches spec
2. **Stack Effects**: Ensure stack transitions match specification
3. **Type Safety**: Validate type system compliance
4. **Exception Behavior**: Match specified exception throwing
5. **Verification Rules**: Support verifiable CIL subset

## Progress Tracking

### Metrics:
- **Total Opcodes**: 179 (standard) + 8 (custom) = 187
- **Implemented**: ~25 (13.4%)
- **Remaining**: 162 (86.6%)

### Milestones:
- **Phase 2 Complete**: 50 opcodes (26.7%)
- **Phase 3 Complete**: 65 opcodes (34.8%)
- **Phase 4 Complete**: 95 opcodes (50.8%)
- **Phase 5 Complete**: 115 opcodes (61.5%)
- **Phase 6 Complete**: 130 opcodes (69.5%)
- **Phase 7 Complete**: 140 opcodes (74.9%)
- **Phase 8 Complete**: 148 opcodes (79.1%)
- **All Standard Opcodes**: 179 opcodes (95.7%)

## Documentation-Driven TDD Workflow

### For Each Opcode:
1. **Read ECMA-335 specification section**
2. **Create test cases based on specification**
3. **Implement opcode to pass tests**
4. **Validate against specification requirements**
5. **Document implementation decisions**
6. **Add to integration test suite**

### Specification Requirements Capture:
- Stack effect (before → after)
- Operand types and constraints
- Exception conditions
- Type safety requirements
- Verification rules
- Encoding format

This systematic approach ensures full compliance with ECMA-335 while maintaining high code quality and test coverage throughout the implementation process.
