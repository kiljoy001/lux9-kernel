/* il_disasm.c - IL Bytecode Disassembler
 *
 * Implements disassembly of CIL bytecode.
 * Based on ECMA-335 Partition III (CIL Instruction Set).
 */

#include "il_disasm.h"
#include <stdio.h>

/* IL Opcode definitions (ECMA-335 Partition III) */
#define IL_NOP          0x00
#define IL_BREAK        0x01
#define IL_LDARG_0      0x02
#define IL_LDARG_1      0x03
#define IL_LDARG_2      0x04
#define IL_LDARG_3      0x05
#define IL_LDLOC_0      0x06
#define IL_LDLOC_1      0x07
#define IL_LDLOC_2      0x08
#define IL_LDLOC_3      0x09
#define IL_STLOC_0      0x0A
#define IL_STLOC_1      0x0B
#define IL_STLOC_2      0x0C
#define IL_STLOC_3      0x0D
#define IL_LDARG_S      0x0E
#define IL_LDARGA_S     0x0F
#define IL_STARG_S      0x10
#define IL_LDLOC_S      0x11
#define IL_LDLOCA_S     0x12
#define IL_STLOC_S      0x13
#define IL_LDNULL       0x14
#define IL_LDC_I4_M1    0x15
#define IL_LDC_I4_0     0x16
#define IL_LDC_I4_1     0x17
#define IL_LDC_I4_2     0x18
#define IL_LDC_I4_3     0x19
#define IL_LDC_I4_4     0x1A
#define IL_LDC_I4_5     0x1B
#define IL_LDC_I4_6     0x1C
#define IL_LDC_I4_7     0x1D
#define IL_LDC_I4_8     0x1E
#define IL_LDC_I4_S     0x1F
#define IL_LDC_I4       0x20
#define IL_LDC_I8       0x21
#define IL_LDC_R4       0x22
#define IL_LDC_R8       0x23
#define IL_DUP          0x25
#define IL_POP          0x26
#define IL_CALL         0x28
#define IL_CALLI        0x29
#define IL_RET          0x2A
#define IL_BR_S         0x2B
#define IL_BRFALSE_S    0x2C
#define IL_BRTRUE_S     0x2D
#define IL_BEQ_S        0x2E
#define IL_BGE_S        0x2F
#define IL_BGT_S        0x30
#define IL_BLE_S        0x31
#define IL_BLT_S        0x32
#define IL_BNE_UN_S     0x33
#define IL_BGE_UN_S     0x34
#define IL_BGT_UN_S     0x35
#define IL_BLE_UN_S     0x36
#define IL_BLT_UN_S     0x37
#define IL_BR           0x38
#define IL_BRFALSE      0x39
#define IL_BRTRUE       0x3A
#define IL_BEQ          0x3B
#define IL_BGE          0x3C
#define IL_BGT          0x3D
#define IL_BLE          0x3E
#define IL_BLT          0x3F
#define IL_BNE_UN       0x40
#define IL_BGE_UN       0x41
#define IL_BGT_UN       0x42
#define IL_BLE_UN       0x43
#define IL_BLT_UN       0x44
#define IL_SWITCH       0x45
#define IL_ADD          0x58
#define IL_SUB          0x59
#define IL_MUL          0x5A
#define IL_DIV          0x5B
#define IL_DIV_UN       0x5C
#define IL_REM          0x5D
#define IL_REM_UN       0x5E
#define IL_AND          0x5F
#define IL_OR           0x60
#define IL_XOR          0x61
#define IL_SHL          0x62
#define IL_SHR          0x63
#define IL_SHR_UN       0x64
#define IL_NEG          0x65
#define IL_NOT          0x66
#define IL_CONV_I1      0x67
#define IL_CONV_I2      0x68
#define IL_CONV_I4      0x69
#define IL_CONV_I8      0x6A
#define IL_CONV_R4      0x6B
#define IL_CONV_R8      0x6C
#define IL_CONV_U4      0x6D
#define IL_CONV_U8      0x6E
#define IL_CALLVIRT     0x6F
#define IL_LDOBJ        0x71
#define IL_LDSTR        0x72
#define IL_NEWOBJ       0x73
#define IL_CASTCLASS    0x74
#define IL_ISINST       0x75
#define IL_CONV_R_UN    0x76
#define IL_UNBOX        0x79
#define IL_THROW        0x7A
#define IL_LDFLD        0x7B
#define IL_LDFLDA       0x7C
#define IL_STFLD        0x7D
#define IL_LDSFLD       0x7E
#define IL_LDSFLDA      0x7F
#define IL_STSFLD       0x80
#define IL_STOBJ        0x81
#define IL_BOX          0x8C
#define IL_NEWARR       0x8D
#define IL_LDLEN        0x8E
#define IL_LDELEMA      0x8F
#define IL_LDELEM_I1    0x90
#define IL_LDELEM_U1    0x91
#define IL_LDELEM_I2    0x92
#define IL_LDELEM_U2    0x93
#define IL_LDELEM_I4    0x94
#define IL_LDELEM_U4    0x95
#define IL_LDELEM_I8    0x96
#define IL_LDELEM_I     0x97
#define IL_LDELEM_R4    0x98
#define IL_LDELEM_R8    0x99
#define IL_LDELEM_REF   0x9A
#define IL_STELEM_I     0x9B
#define IL_STELEM_I1    0x9C
#define IL_STELEM_I2    0x9D
#define IL_STELEM_I4    0x9E
#define IL_STELEM_I8    0x9F
#define IL_STELEM_R4    0xA0
#define IL_STELEM_R8    0xA1
#define IL_STELEM_REF   0xA2
#define IL_LDELEM       0xA3
#define IL_STELEM       0xA4
#define IL_UNBOX_ANY    0xA5

/* Get opcode name */
const char* il_opcode_name(uint8_t opcode) {
    switch (opcode) {
        case IL_NOP: return "nop";
        case IL_BREAK: return "break";
        case IL_LDARG_0: return "ldarg.0";
        case IL_LDARG_1: return "ldarg.1";
        case IL_LDARG_2: return "ldarg.2";
        case IL_LDARG_3: return "ldarg.3";
        case IL_LDLOC_0: return "ldloc.0";
        case IL_LDLOC_1: return "ldloc.1";
        case IL_LDLOC_2: return "ldloc.2";
        case IL_LDLOC_3: return "ldloc.3";
        case IL_STLOC_0: return "stloc.0";
        case IL_STLOC_1: return "stloc.1";
        case IL_STLOC_2: return "stloc.2";
        case IL_STLOC_3: return "stloc.3";
        case IL_LDARG_S: return "ldarg.s";
        case IL_LDARGA_S: return "ldarga.s";
        case IL_STARG_S: return "starg.s";
        case IL_LDLOC_S: return "ldloc.s";
        case IL_LDLOCA_S: return "ldloca.s";
        case IL_STLOC_S: return "stloc.s";
        case IL_LDNULL: return "ldnull";
        case IL_LDC_I4_M1: return "ldc.i4.m1";
        case IL_LDC_I4_0: return "ldc.i4.0";
        case IL_LDC_I4_1: return "ldc.i4.1";
        case IL_LDC_I4_2: return "ldc.i4.2";
        case IL_LDC_I4_3: return "ldc.i4.3";
        case IL_LDC_I4_4: return "ldc.i4.4";
        case IL_LDC_I4_5: return "ldc.i4.5";
        case IL_LDC_I4_6: return "ldc.i4.6";
        case IL_LDC_I4_7: return "ldc.i4.7";
        case IL_LDC_I4_8: return "ldc.i4.8";
        case IL_LDC_I4_S: return "ldc.i4.s";
        case IL_LDC_I4: return "ldc.i4";
        case IL_LDC_I8: return "ldc.i8";
        case IL_LDC_R4: return "ldc.r4";
        case IL_LDC_R8: return "ldc.r8";
        case IL_DUP: return "dup";
        case IL_POP: return "pop";
        case IL_CALL: return "call";
        case IL_CALLI: return "calli";
        case IL_RET: return "ret";
        case IL_BR_S: return "br.s";
        case IL_BRFALSE_S: return "brfalse.s";
        case IL_BRTRUE_S: return "brtrue.s";
        case IL_BEQ_S: return "beq.s";
        case IL_BGE_S: return "bge.s";
        case IL_BGT_S: return "bgt.s";
        case IL_BLE_S: return "ble.s";
        case IL_BLT_S: return "blt.s";
        case IL_BNE_UN_S: return "bne.un.s";
        case IL_BGE_UN_S: return "bge.un.s";
        case IL_BGT_UN_S: return "bgt.un.s";
        case IL_BLE_UN_S: return "ble.un.s";
        case IL_BLT_UN_S: return "blt.un.s";
        case IL_BR: return "br";
        case IL_BRFALSE: return "brfalse";
        case IL_BRTRUE: return "brtrue";
        case IL_BEQ: return "beq";
        case IL_BGE: return "bge";
        case IL_BGT: return "bgt";
        case IL_BLE: return "ble";
        case IL_BLT: return "blt";
        case IL_BNE_UN: return "bne.un";
        case IL_BGE_UN: return "bge.un";
        case IL_BGT_UN: return "bgt.un";
        case IL_BLE_UN: return "ble.un";
        case IL_BLT_UN: return "blt.un";
        case IL_SWITCH: return "switch";
        case IL_ADD: return "add";
        case IL_SUB: return "sub";
        case IL_MUL: return "mul";
        case IL_DIV: return "div";
        case IL_DIV_UN: return "div.un";
        case IL_REM: return "rem";
        case IL_REM_UN: return "rem.un";
        case IL_AND: return "and";
        case IL_OR: return "or";
        case IL_XOR: return "xor";
        case IL_SHL: return "shl";
        case IL_SHR: return "shr";
        case IL_SHR_UN: return "shr.un";
        case IL_NEG: return "neg";
        case IL_NOT: return "not";
        case IL_CONV_I1: return "conv.i1";
        case IL_CONV_I2: return "conv.i2";
        case IL_CONV_I4: return "conv.i4";
        case IL_CONV_I8: return "conv.i8";
        case IL_CONV_R4: return "conv.r4";
        case IL_CONV_R8: return "conv.r8";
        case IL_CONV_U4: return "conv.u4";
        case IL_CONV_U8: return "conv.u8";
        case IL_CALLVIRT: return "callvirt";
        case IL_LDOBJ: return "ldobj";
        case IL_LDSTR: return "ldstr";
        case IL_NEWOBJ: return "newobj";
        case IL_CASTCLASS: return "castclass";
        case IL_ISINST: return "isinst";
        case IL_CONV_R_UN: return "conv.r.un";
        case IL_UNBOX: return "unbox";
        case IL_THROW: return "throw";
        case IL_LDFLD: return "ldfld";
        case IL_LDFLDA: return "ldflda";
        case IL_STFLD: return "stfld";
        case IL_LDSFLD: return "ldsfld";
        case IL_LDSFLDA: return "ldsflda";
        case IL_STSFLD: return "stsfld";
        case IL_STOBJ: return "stobj";
        case IL_BOX: return "box";
        case IL_NEWARR: return "newarr";
        case IL_LDLEN: return "ldlen";
        case IL_LDELEMA: return "ldelema";
        case IL_LDELEM_I1: return "ldelem.i1";
        case IL_LDELEM_U1: return "ldelem.u1";
        case IL_LDELEM_I2: return "ldelem.i2";
        case IL_LDELEM_U2: return "ldelem.u2";
        case IL_LDELEM_I4: return "ldelem.i4";
        case IL_LDELEM_U4: return "ldelem.u4";
        case IL_LDELEM_I8: return "ldelem.i8";
        case IL_LDELEM_I: return "ldelem.i";
        case IL_LDELEM_R4: return "ldelem.r4";
        case IL_LDELEM_R8: return "ldelem.r8";
        case IL_LDELEM_REF: return "ldelem.ref";
        case IL_STELEM_I: return "stelem.i";
        case IL_STELEM_I1: return "stelem.i1";
        case IL_STELEM_I2: return "stelem.i2";
        case IL_STELEM_I4: return "stelem.i4";
        case IL_STELEM_I8: return "stelem.i8";
        case IL_STELEM_R4: return "stelem.r4";
        case IL_STELEM_R8: return "stelem.r8";
        case IL_STELEM_REF: return "stelem.ref";
        case IL_LDELEM: return "ldelem";
        case IL_STELEM: return "stelem";
        case IL_UNBOX_ANY: return "unbox.any";
        default: return "unknown";
    }
}

/* Disassemble single instruction */
int il_disassemble_instruction(const uint8_t *il, size_t offset, size_t max_offset) {
    if (offset >= max_offset) {
        return 0;
    }

    uint8_t opcode = il[offset];
    printf("IL_%04zx: %-12s", offset, il_opcode_name(opcode));

    int consumed = 1;

    switch (opcode) {
        // No operands
        case IL_NOP:
        case IL_BREAK:
        case IL_LDARG_0:
        case IL_LDARG_1:
        case IL_LDARG_2:
        case IL_LDARG_3:
        case IL_LDLOC_0:
        case IL_LDLOC_1:
        case IL_LDLOC_2:
        case IL_LDLOC_3:
        case IL_STLOC_0:
        case IL_STLOC_1:
        case IL_STLOC_2:
        case IL_STLOC_3:
        case IL_LDNULL:
        case IL_LDC_I4_M1:
        case IL_LDC_I4_0:
        case IL_LDC_I4_1:
        case IL_LDC_I4_2:
        case IL_LDC_I4_3:
        case IL_LDC_I4_4:
        case IL_LDC_I4_5:
        case IL_LDC_I4_6:
        case IL_LDC_I4_7:
        case IL_LDC_I4_8:
        case IL_DUP:
        case IL_POP:
        case IL_RET:
        case IL_ADD:
        case IL_SUB:
        case IL_MUL:
        case IL_DIV:
        case IL_DIV_UN:
        case IL_REM:
        case IL_REM_UN:
        case IL_AND:
        case IL_OR:
        case IL_XOR:
        case IL_SHL:
        case IL_SHR:
        case IL_SHR_UN:
        case IL_NEG:
        case IL_NOT:
        case IL_LDLEN:
        case IL_THROW:
            printf("\n");
            break;

        // 1-byte operand
        case IL_LDARG_S:
        case IL_LDARGA_S:
        case IL_STARG_S:
        case IL_LDLOC_S:
        case IL_LDLOCA_S:
        case IL_STLOC_S:
        case IL_LDC_I4_S:
        case IL_BR_S:
        case IL_BRFALSE_S:
        case IL_BRTRUE_S:
        case IL_BEQ_S:
        case IL_BGE_S:
        case IL_BGT_S:
        case IL_BLE_S:
        case IL_BLT_S:
        case IL_BNE_UN_S:
        case IL_BGE_UN_S:
        case IL_BGT_UN_S:
        case IL_BLE_UN_S:
        case IL_BLT_UN_S:
            if (offset + 1 < max_offset) {
                printf("%d\n", (int8_t)il[offset + 1]);
                consumed = 2;
            } else {
                printf("<truncated>\n");
            }
            break;

        // 4-byte operand (token or int32)
        case IL_LDC_I4:
        case IL_CALL:
        case IL_CALLI:
        case IL_CALLVIRT:
        case IL_LDOBJ:
        case IL_LDSTR:
        case IL_NEWOBJ:
        case IL_CASTCLASS:
        case IL_ISINST:
        case IL_UNBOX:
        case IL_LDFLD:
        case IL_LDFLDA:
        case IL_STFLD:
        case IL_LDSFLD:
        case IL_LDSFLDA:
        case IL_STSFLD:
        case IL_STOBJ:
        case IL_BOX:
        case IL_NEWARR:
        case IL_LDELEMA:
        case IL_LDELEM:
        case IL_STELEM:
        case IL_UNBOX_ANY:
        case IL_BR:
        case IL_BRFALSE:
        case IL_BRTRUE:
        case IL_BEQ:
        case IL_BGE:
        case IL_BGT:
        case IL_BLE:
        case IL_BLT:
        case IL_BNE_UN:
        case IL_BGE_UN:
        case IL_BGT_UN:
        case IL_BLE_UN:
        case IL_BLT_UN:
            if (offset + 4 < max_offset) {
                uint32_t operand = *(uint32_t*)&il[offset + 1];
                printf("0x%08x\n", operand);
                consumed = 5;
            } else {
                printf("<truncated>\n");
            }
            break;

        // 8-byte operand
        case IL_LDC_I8:
        case IL_LDC_R8:
            if (offset + 8 < max_offset) {
                uint64_t operand = *(uint64_t*)&il[offset + 1];
                printf("0x%016llx\n", (unsigned long long)operand);
                consumed = 9;
            } else {
                printf("<truncated>\n");
            }
            break;

        // Switch (variable length)
        case IL_SWITCH:
            if (offset + 4 < max_offset) {
                uint32_t n = *(uint32_t*)&il[offset + 1];
                printf("%u cases\n", n);
                consumed = 5 + (n * 4);
            } else {
                printf("<truncated>\n");
            }
            break;

        default:
            printf("(unknown opcode 0x%02x)\n", opcode);
            break;
    }

    return consumed;
}

/* Disassemble entire method */
void il_disassemble_method(il_method_t *method) {
    if (method == NULL || method->il_code == NULL) {
        printf("No IL code to disassemble\n");
        return;
    }

    printf("=== IL Disassembly: %s ===\n", method->name ? method->name : "<unknown>");
    printf("Max stack: %u, Code size: %zu bytes\n\n",
           method->max_stack, method->il_code_size);

    size_t offset = 0;
    while (offset < method->il_code_size) {
        int consumed = il_disassemble_instruction(method->il_code, offset, method->il_code_size);
        if (consumed == 0) {
            break;  // Error or end of code
        }
        offset += consumed;
    }

    printf("\n");
}
