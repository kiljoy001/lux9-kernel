# Detailed ECMA-335 to CIL Opcode Mapping

## Complete Opcode Inventory by ECMA-335 Section

### Section III.1: Introduction and Overview
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| NOP | 0x00 | ✅ | No-operation, implemented |
| BREAK | 0x01 | ❌ | Debugger breakpoint |

### Section III.3: Base Instructions

#### Stack Operations (III.3.33-37)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| DUP | 0x25 | ✅ | Duplicate top stack value |
| POP | 0x26 | ✅ | Remove top stack value |
| LDIND_I1 | 0x46 | ❌ | Load indirect int8 |
| LDIND_U1 | 0x47 | ❌ | Load indirect uint8 |
| LDIND_I2 | 0x48 | ❌ | Load indirect int16 |
| LDIND_U2 | 0x49 | ❌ | Load indirect uint16 |
| LDIND_I4 | 0x4A | ❌ | Load indirect int32 |
| LDIND_U4 | 0x4B | ❌ | Load indirect uint32 |
| LDIND_I8 | 0x4C | ❌ | Load indirect int64 |
| LDIND_I | 0x4D | ❌ | Load indirect native int |
| LDIND_R4 | 0x4E | ❌ | Load indirect float32 |
| LDIND_R8 | 0x4F | ❌ | Load indirect float64 |
| LDIND_REF | 0x50 | ❌ | Load indirect object reference |
| STIND_REF | 0x51 | ❌ | Store indirect object reference |
| STIND_I1 | 0x52 | ❌ | Store indirect int8 |
| STIND_I2 | 0x53 | ❌ | Store indirect int16 |
| STIND_I4 | 0x54 | ❌ | Store indirect int32 |
| STIND_I8 | 0x55 | ❌ | Store indirect int64 |
| STIND_R4 | 0x56 | ❌ | Store indirect float32 |
| STIND_R8 | 0x57 | ❌ | Store indirect float64 |

#### Arithmetic Operations (III.3.1-3)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| ADD | 0x58 | ✅ | Add values |
| ADD_OVF | 0xD6 | ❌ | Add with overflow check |
| ADD_OVF_UN | 0xD7 | ❌ | Add unsigned with overflow |
| SUB | 0x59 | ✅ | Subtract values |
| SUB_OVF | 0xDA | ❌ | Subtract with overflow check |
| SUB_OVF_UN | 0xDB | ❌ | Subtract unsigned with overflow |
| MUL | 0x5A | ✅ | Multiply values |
| MUL_OVF | 0xD8 | ❌ | Multiply with overflow check |
| MUL_OVF_UN | 0xD9 | ❌ | Multiply unsigned with overflow |
| DIV | 0x5B | ✅ | Divide values |
| DIV_UN | 0x5C | ✅ | Divide unsigned values |
| DIV_OVF | 0xDC | ❌ | Divide with overflow check |
| DIV_OVF_UN | 0xDD | ❌ | Divide unsigned with overflow |
| REM | 0x5D | ✅ | Remainder |
| REM_UN | 0x5E | ✅ | Remainder unsigned |

#### Bitwise Operations (III.3.4)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| AND | 0x5F | ✅ | Bitwise AND |
| OR | 0x60 | ✅ | Bitwise OR |
| XOR | 0x61 | ✅ | Bitwise XOR |
| SHL | 0x62 | ✅ | Shift left |
| SHR | 0x63 | ✅ | Shift right (signed) |
| SHR_UN | 0x64 | ✅ | Shift right (unsigned) |

#### Negation Operations (III.3.5)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| NEG | 0x65 | ✅ | Negate |
| NOT | 0x66 | ✅ | Bitwise NOT |

#### Conversion Operations (III.3.27-29)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CONV_I1 | 0x67 | ❌ | Convert to int8 |
| CONV_I2 | 0x68 | ❌ | Convert to int16 |
| CONV_I4 | 0x69 | ❌ | Convert to int32 |
| CONV_I8 | 0x6A | ❌ | Convert to int64 |
| CONV_R4 | 0x6B | ❌ | Convert to float32 |
| CONV_R8 | 0x6C | ❌ | Convert to float64 |
| CONV_U4 | 0x6D | ❌ | Convert to uint32 |
| CONV_U8 | 0x6E | ❌ | Convert to uint64 |
| CONV_R_UN | 0x76 | ❌ | Convert unsigned to float |
| CONV_OVF_I1 | 0xB3 | ❌ | Convert to int8 with overflow |
| CONV_OVF_I2 | 0xB5 | ❌ | Convert to int16 with overflow |
| CONV_OVF_I4 | 0xB7 | ❌ | Convert to int32 with overflow |
| CONV_OVF_I8 | 0xB9 | ❌ | Convert to int64 with overflow |
| CONV_OVF_U1 | 0xB4 | ❌ | Convert to uint8 with overflow |
| CONV_OVF_U2 | 0xB6 | ❌ | Convert to uint16 with overflow |
| CONV_OVF_U4 | 0xB8 | ❌ | Convert to uint32 with overflow |
| CONV_OVF_U8 | 0xBA | ❌ | Convert to uint64 with overflow |
| CONV_OVF_I1_UN | 0x82 | ❌ | Convert unsigned to int8 with overflow |
| CONV_OVF_I2_UN | 0x83 | ❌ | Convert unsigned to int16 with overflow |
| CONV_OVF_I4_UN | 0x84 | ❌ | Convert unsigned to int32 with overflow |
| CONV_OVF_I8_UN | 0x85 | ❌ | Convert unsigned to int64 with overflow |
| CONV_OVF_U1_UN | 0x86 | ❌ | Convert to uint8 with overflow |
| CONV_OVF_U2_UN | 0x87 | ❌ | Convert to uint16 with overflow |
| CONV_OVF_U4_UN | 0x88 | ❌ | Convert to uint32 with overflow |
| CONV_OVF_U8_UN | 0x89 | ❌ | Convert to uint64 with overflow |
| CONV_OVF_I_UN | 0x8A | ❌ | Convert unsigned to native int with overflow |
| CONV_OVF_U_UN | 0x8B | ❌ | Convert to native uint with overflow |

#### Comparison Operations (III.3.21-26)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CEQ | 0x101 | ✅ | Compare equal |
| CGT | 0x102 | ✅ | Compare greater than |
| CGT_UN | 0x103 | ❌ | Compare greater than unsigned |
| CLT | 0x104 | ✅ | Compare less than |
| CLT_UN | 0x105 | ❌ | Compare less than unsigned |

#### Branch Instructions (III.3.5-16)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| BR | 0x38 | ✅ | Branch (4-byte) |
| BR_S | 0x2B | ✅ | Branch (1-byte) |
| BEQ | 0x3B | ✅ | Branch if equal (4-byte) |
| BEQ_S | 0x2E | ❌ | Branch if equal (1-byte) |
| BGE | 0x3C | ❌ | Branch if greater/equal (4-byte) |
| BGE_S | 0x2F | ❌ | Branch if greater/equal (1-byte) |
| BGT | 0x3D | ❌ | Branch if greater (4-byte) |
| BGT_S | 0x30 | ❌ | Branch if greater (1-byte) |
| BLE | 0x3E | ❌ | Branch if less/equal (4-byte) |
| BLE_S | 0x31 | ❌ | Branch if less/equal (1-byte) |
| BLT | 0x3F | ❌ | Branch if less (4-byte) |
| BLT_S | 0x32 | ❌ | Branch if less (1-byte) |
| BNE_UN | 0x40 | ❌ | Branch if not equal unsigned (4-byte) |
| BNE_UN_S | 0x33 | ❌ | Branch if not equal unsigned (1-byte) |
| BGE_UN | 0x41 | ❌ | Branch if greater/equal unsigned (4-byte) |
| BGE_UN_S | 0x34 | ❌ | Branch if greater/equal unsigned (1-byte) |
| BGT_UN | 0x42 | ❌ | Branch if greater unsigned (4-byte) |
| BGT_UN_S | 0x35 | ❌ | Branch if greater unsigned (1-byte) |
| BLE_UN | 0x43 | ❌ | Branch if less/equal unsigned (4-byte) |
| BLE_UN_S | 0x36 | ❌ | Branch if less/equal unsigned (1-byte) |
| BLT_UN | 0x44 | ❌ | Branch if less unsigned (4-byte) |
| BLT_UN_S | 0x37 | ❌ | Branch if less unsigned (1-byte) |
| BRTRUE | 0x3A | ✅ | Branch if true (4-byte) |
| BRTRUE_S | 0x2D | ❌ | Branch if true (1-byte) |
| BRFALSE | 0x39 | ✅ | Branch if false (4-byte) |
| BRFALSE_S | 0x2C | ❌ | Branch if false (1-byte) |
| SWITCH | 0x45 | ❌ | Table switch |

#### Method Call Instructions (III.3.19-21)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CALL | 0x28 | ❌ | Call method |
| CALLI | 0x29 | ❌ | Indirect method call |
| CALLVIRT | 0x6F | ❌ | Call virtual method |
| JMP | 0x27 | ❌ | Jump to method |

#### Return Instructions (III.3.57)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| RET | 0x2A | ✅ | Return from method |

#### Constant Loading (III.3.38-49)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDC_I4_M1 | 0x15 | ❌ | Load int32 constant -1 |
| LDC_I4_0 | 0x16 | ✅ | Load int32 constant 0 |
| LDC_I4_1 | 0x17 | ✅ | Load int32 constant 1 |
| LDC_I4_2 | 0x18 | ❌ | Load int32 constant 2 |
| LDC_I4_3 | 0x19 | ❌ | Load int32 constant 3 |
| LDC_I4_4 | 0x1A | ❌ | Load int32 constant 4 |
| LDC_I4_5 | 0x1B | ❌ | Load int32 constant 5 |
| LDC_I4_6 | 0x1C | ❌ | Load int32 constant 6 |
| LDC_I4_7 | 0x1D | ❌ | Load int32 constant 7 |
| LDC_I4_8 | 0x1E | ❌ | Load int32 constant 8 |
| LDC_I4_S | 0x1F | ✅ | Load int32 constant (short) |
| LDC_I4 | 0x20 | ✅ | Load int32 constant |
| LDC_I8 | 0x21 | ❌ | Load int64 constant |
| LDC_R4 | 0x22 | ❌ | Load float32 constant |
| LDC_R8 | 0x23 | ❌ | Load float64 constant |
| LDNULL | 0x14 | ❌ | Load null reference |

#### Argument Loading (III.3.38-39)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDARG_0 | 0x02 | ✅ | Load argument 0 |
| LDARG_1 | 0x03 | ✅ | Load argument 1 |
| LDARG_2 | 0x04 | ✅ | Load argument 2 |
| LDARG_3 | 0x05 | ✅ | Load argument 3 |
| LDARG_S | 0x0E | ✅ | Load argument (short) |
| LDARG | 0x109 | ❌ | Load argument |
| LDARGA_S | 0x0F | ❌ | Load argument address (short) |
| LDARGA | 0x10A | ❌ | Load argument address |
| STARG_S | 0x10 | ❌ | Store argument (short) |
| STARG | 0x10B | ❌ | Store argument |

#### Local Loading (III.3.40-41)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDLOC_0 | 0x06 | ✅ | Load local 0 |
| LDLOC_1 | 0x07 | ✅ | Load local 1 |
| LDLOC_2 | 0x08 | ✅ | Load local 2 |
| LDLOC_3 | 0x09 | ✅ | Load local 3 |
| LDLOC_S | 0x11 | ✅ | Load local (short) |
| LDLOC | 0x10C | ❌ | Load local |
| LDLOCA_S | 0x12 | ❌ | Load local address (short) |
| LDLOCA | 0x10D | ❌ | Load local address |
| STLOC_0 | 0x0A | ✅ | Store local 0 |
| STLOC_1 | 0x0B | ✅ | Store local 1 |
| STLOC_2 | 0x0C | ✅ | Store local 2 |
| STLOC_3 | 0x0D | ✅ | Store local 3 |
| STLOC_S | 0x13 | ✅ | Store local (short) |
| STLOC | 0x10E | ❌ | Store local |

#### Advanced Stack Operations (III.3.42-43)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| ARGLIST | 0x100 | ❌ | Get argument list |
| LOCALLOC | 0x10F | ❌ | Allocate local memory |

#### Memory Operations (III.3.55-57)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CPBLK | 0x117 | ❌ | Copy block of memory |
| INITBLK | 0x118 | ❌ | Initialize block of memory |

#### Exception Handling (III.3.58-62)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| THROW | 0x7A | ❌ | Throw exception |
| RETHROW | 0x11A | ❌ | Rethrow exception |
| ENDFILTER | 0x111 | ❌ | End filter clause |

#### Type Operations (III.3.63-65)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| SIZEOF | 0x11C | ❌ | Get size of type |
| REFANYTYPE | 0x11D | ❌ | Get type of typed reference |
| LDTOKEN | 0xD0 | ❌ | Load runtime token |

#### Function Pointer Operations (III.3.41-43)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDFTN | 0x106 | ❌ | Load function pointer |
| LDVIRTFTN | 0x107 | ❌ | Load virtual function pointer |

### Section III.4: Object Model Instructions

#### Object Creation (III.4.1, III.4.20-21)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| NEWOBJ | 0x73 | ❌ | Create new object |
| NEWARR | 0x8D | ❌ | Create new array |
| INITOBJ | 0x115 | ❌ | Initialize object |

#### Boxing Operations (III.4.1)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| BOX | 0x8C | ❌ | Box value type |
| UNBOX | 0x79 | ❌ | Unbox value type |
| UNBOX_ANY | 0xA5 | ❌ | Unbox any type |

#### String Operations (III.4.16)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDSTR | 0x72 | ❌ | Load string literal |

#### Type Conversion (III.4.3, III.4.6)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CASTCLASS | 0x74 | ❌ | Cast object to type |
| ISINST | 0x75 | ❌ | Test if object is type |

#### Field Operations (III.4.10-15)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDFLD | 0x7B | ❌ | Load instance field |
| LDFLDA | 0x7C | ❌ | Load instance field address |
| STFLD | 0x7D | ❌ | Store instance field |
| LDSFLD | 0x7E | ❌ | Load static field |
| LDSFLDA | 0x7F | ❌ | Load static field address |
| STSFLD | 0x80 | ❌ | Store static field |

#### Array Operations (III.4.8-9, III.4.12-13, III.4.26-28)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| LDLEN | 0x8E | ❌ | Load array length |
| LDELEMA | 0x8F | ❌ | Load array element address |
| LDELEM_I1 | 0x90 | ❌ | Load array element int8 |
| LDELEM_U1 | 0x91 | ❌ | Load array element uint8 |
| LDELEM_I2 | 0x92 | ❌ | Load array element int16 |
| LDELEM_U2 | 0x93 | ❌ | Load array element uint16 |
| LDELEM_I4 | 0x94 | ❌ | Load array element int32 |
| LDELEM_U4 | 0x95 | ❌ | Load array element uint32 |
| LDELEM_I8 | 0x96 | ❌ | Load array element int64 |
| LDELEM_I | 0x97 | ❌ | Load array element native int |
| LDELEM_R4 | 0x98 | ❌ | Load array element float32 |
| LDELEM_R8 | 0x99 | ❌ | Load array element float64 |
| LDELEM_REF | 0x9A | ❌ | Load array element reference |
| LDELEM_ANY | 0xA3 | ❌ | Load array element any type |
| STELEM_I | 0x9B | ❌ | Store array element int |
| STELEM_I1 | 0x9C | ❌ | Store array element int8 |
| STELEM_I2 | 0x9D | ❌ | Store array element int16 |
| STELEM_I4 | 0x9E | ❌ | Store array element int32 |
| STELEM_I8 | 0x9F | ❌ | Store array element int64 |
| STELEM_R4 | 0xA0 | ❌ | Store array element float32 |
| STELEM_R8 | 0xA1 | ❌ | Store array element float64 |
| STELEM_REF | 0xA2 | ❌ | Store array element reference |
| STELEM_ANY | 0xA4 | ❌ | Store array element any type |

#### Value Type Operations (III.4.4-5, III.4.25, III.4.30)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CPOBJ | 0x70 | ❌ | Copy value type object |
| LDOBJ | 0x71 | ❌ | Load value type object |
| STOBJ | 0x81 | ❌ | Store value type object |

#### Typed Reference Operations (III.4.19, III.4.22-23)
| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| MKREFANY | 0xC6 | ❌ | Make typed reference |
| REFANYVAL | 0xC7 | ❌ | Get typed reference value |
| REFANYTYPE | 0x11D | ❌ | Get typed reference type |

### Section III.2: Prefix Instructions

| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| CONSTRAINED | 0x116 | ❌ | Constrain type before call |
| READONLY | 0x11E | ❌ | Readonly prefix |
| VOLATILE | 0x113 | ❌ | Volatile prefix |
| UNALIGNED | 0x112 | ❌ | Unaligned prefix |
| TAIL | 0x114 | ❌ | Tail call prefix |

### Extended Opcodes (0xFE00+)

| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| BREAK | 0x01 | ❌ | Debugger breakpoint (duplicate) |
| ENDFILTER | 0x111 | ❌ | End filter clause |
| UNALIGNED | 0x112 | ❌ | Unaligned prefix |
| VOLATILE | 0x113 | ❌ | Volatile prefix |
| TAIL | 0x114 | ❌ | Tail call prefix |
| INITOBJ | 0x115 | ❌ | Initialize object |
| CONSTRAINED | 0x116 | ❌ | Constrain type |
| CPBLK | 0x117 | ❌ | Copy block |
| INITBLK | 0x118 | ❌ | Initialize block |
| RETHROW | 0x11A | ❌ | Rethrow exception |
| SIZEOF | 0x11C | ❌ | Get size of type |
| REFANYTYPE | 0x11D | ❌ | Get type of typed reference |
| READONLY | 0x11E | ❌ | Readonly prefix |

### Custom Symbolic Computing Extensions (0xE0+)

| Opcode | Hex | Status | Implementation Notes |
|--------|-----|--------|---------------------|
| SYM_CREATE | 0xE0 | ❌ | Create symbolic variable |
| SYM_EXPR | 0xE1 | ❌ | Build symbolic expression |
| SYM_DIFF | 0xE2 | ❌ | Symbolic differentiation |
| SYM_INTEGRATE | 0xE3 | ❌ | Symbolic integration |
| SYM_SIMPLIFY | 0xE4 | ❌ | Expression simplification |
| SYM_EVAL | 0xE5 | ❌ | Evaluate symbolic expression |
| SYM_MATCH | 0xE6 | ❌ | Pattern matching |
| SYM_REWRITE | 0xE7 | ❌ | Term rewriting |

## Implementation Statistics

### By Category:
- **Base Instructions**: 45 implemented / 105 total (42.9%)
- **Object Model**: 0 implemented / 38 total (0.0%)
- **Prefix Instructions**: 0 implemented / 6 total (0.0%)
- **Extended Opcodes**: 1 implemented / 16 total (6.3%)
- **Custom Extensions**: 0 implemented / 8 total (0.0%)

### Overall Progress:
- **Total Opcodes**: 187 (179 standard + 8 custom)
- **Implemented**: 46 (24.6%)
- **Remaining**: 141 (75.4%)

### Implementation Complexity Distribution:
- **Simple** (no operands): 12 implemented / 15 total (80.0%)
- **Complex** (with operands): 34 implemented / 172 total (19.8%)

This detailed mapping provides the foundation for systematic implementation following the ECMA-335 specification section by section.
