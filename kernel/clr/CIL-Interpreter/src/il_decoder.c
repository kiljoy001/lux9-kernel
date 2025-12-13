#include "../include/il_decoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Function to create a new CIL decoder
cil_decoder_t* create_cil_decoder(const uint8_t* bytecode, size_t size) {
    if (bytecode == NULL) {
        return NULL;
    }
    
    cil_decoder_t* decoder = (cil_decoder_t*)malloc(sizeof(cil_decoder_t));
    if (decoder == NULL) {
        return NULL;
    }
    
    decoder->bytecode = bytecode;
    decoder->bytecode_size = size;
    decoder->offset = 0;
    decoder->has_error = false;
    decoder->error_message = NULL;
    
    return decoder;
}

// Function to destroy a CIL decoder
void destroy_cil_decoder(cil_decoder_t* decoder) {
    if (decoder != NULL) {
        free(decoder);
    }
}

// Function to check if an opcode is an extended opcode (0xFE prefix)
bool is_extended_opcode(uint8_t opcode) {
    return opcode == 0xFE;
}

// Function to get the operand type for a given opcode
cil_operand_type_t get_opcode_operand_type(cil_opcode_t opcode) {
    switch (opcode) {
        // No operand opcodes
        case CIL_OPCODE_NOP:
        case CIL_OPCODE_BREAK:
        case CIL_OPCODE_LDARG_0:
        case CIL_OPCODE_LDARG_1:
        case CIL_OPCODE_LDARG_2:
        case CIL_OPCODE_LDARG_3:
        case CIL_OPCODE_LDLOC_0:
        case CIL_OPCODE_LDLOC_1:
        case CIL_OPCODE_LDLOC_2:
        case CIL_OPCODE_LDLOC_3:
        case CIL_OPCODE_STLOC_0:
        case CIL_OPCODE_STLOC_1:
        case CIL_OPCODE_STLOC_2:
        case CIL_OPCODE_STLOC_3:
        case CIL_OPCODE_LDNULL:
        case CIL_OPCODE_LDC_I4_M1:
        case CIL_OPCODE_LDC_I4_0:
        case CIL_OPCODE_LDC_I4_1:
        case CIL_OPCODE_LDC_I4_2:
        case CIL_OPCODE_LDC_I4_3:
        case CIL_OPCODE_LDC_I4_4:
        case CIL_OPCODE_LDC_I4_5:
        case CIL_OPCODE_LDC_I4_6:
        case CIL_OPCODE_LDC_I4_7:
        case CIL_OPCODE_LDC_I4_8:
        case CIL_OPCODE_DUP:
        case CIL_OPCODE_POP:
        case CIL_OPCODE_RET:
        case CIL_OPCODE_ADD:
        case CIL_OPCODE_SUB:
        case CIL_OPCODE_MUL:
        case CIL_OPCODE_DIV:
        case CIL_OPCODE_DIV_UN:
        case CIL_OPCODE_REM:
        case CIL_OPCODE_REM_UN:
        case CIL_OPCODE_AND:
        case CIL_OPCODE_OR:
        case CIL_OPCODE_XOR:
        case CIL_OPCODE_SHL:
        case CIL_OPCODE_SHR:
        case CIL_OPCODE_SHR_UN:
        case CIL_OPCODE_NEG:
        case CIL_OPCODE_NOT:
        case CIL_OPCODE_CONV_I1:
        case CIL_OPCODE_CONV_I2:
        case CIL_OPCODE_CONV_I4:
        case CIL_OPCODE_CONV_I8:
        case CIL_OPCODE_CONV_R4:
        case CIL_OPCODE_CONV_R8:
        case CIL_OPCODE_CONV_U4:
        case CIL_OPCODE_CONV_U8:
        case CIL_OPCODE_CONV_R_UN:
        case CIL_OPCODE_CONV_OVF_I1_UN:
        case CIL_OPCODE_CONV_OVF_I2_UN:
        case CIL_OPCODE_CONV_OVF_I4_UN:
        case CIL_OPCODE_CONV_OVF_I8_UN:
        case CIL_OPCODE_CONV_OVF_U1_UN:
        case CIL_OPCODE_CONV_OVF_U2_UN:
        case CIL_OPCODE_CONV_OVF_U4_UN:
        case CIL_OPCODE_CONV_OVF_U8_UN:
        case CIL_OPCODE_CONV_OVF_I_UN:
        case CIL_OPCODE_CONV_OVF_U_UN:
        case CIL_OPCODE_CONV_OVF_I1:
        case CIL_OPCODE_CONV_OVF_U1:
        case CIL_OPCODE_CONV_OVF_I2:
        case CIL_OPCODE_CONV_OVF_U2:
        case CIL_OPCODE_CONV_OVF_I4:
        case CIL_OPCODE_CONV_OVF_U4:
        case CIL_OPCODE_CONV_OVF_I8:
        case CIL_OPCODE_CONV_OVF_U8:
        case CIL_OPCODE_ARGLIST:
        case CIL_OPCODE_CEQ:
        case CIL_OPCODE_CGT:
        case CIL_OPCODE_CGT_UN:
        case CIL_OPCODE_CLT:
        case CIL_OPCODE_CLT_UN:
        case CIL_OPCODE_ENDFILTER:
        case CIL_OPCODE_RETHROW:
        
        // Arithmetic overflow operations
        case CIL_OPCODE_ADD_OVF:
        case CIL_OPCODE_ADD_OVF_UN:
        case CIL_OPCODE_MUL_OVF:
        case CIL_OPCODE_MUL_OVF_UN:
        case CIL_OPCODE_SUB_OVF:
        case CIL_OPCODE_SUB_OVF_UN:
        case CIL_OPCODE_DIV_OVF:
        case CIL_OPCODE_DIV_OVF_UN:
            return CIL_OPERAND_NONE;
            
        // Byte operand opcodes
        case CIL_OPCODE_LDARG_S:
        case CIL_OPCODE_LDARGA_S:
        case CIL_OPCODE_STARG_S:
        case CIL_OPCODE_LDLOC_S:
        case CIL_OPCODE_LDLOCA_S:
        case CIL_OPCODE_STLOC_S:
        case CIL_OPCODE_LDC_I4_S:
        case CIL_OPCODE_UNALIGNED:
            return CIL_OPERAND_BYTE;
            
        // Short operand opcodes (2 bytes)
        case CIL_OPCODE_LDARG:
        case CIL_OPCODE_LDARGA:
        case CIL_OPCODE_STARG:
        case CIL_OPCODE_LDLOC:
        case CIL_OPCODE_LDLOCA:
        case CIL_OPCODE_STLOC:
            return CIL_OPERAND_SHORT;
            
        // Int operand opcodes (4 bytes)
        case CIL_OPCODE_LDC_I4:
        case CIL_OPCODE_JMP:
        case CIL_OPCODE_CALL:
        case CIL_OPCODE_CALLI:
        case CIL_OPCODE_CALLVIRT:
        case CIL_OPCODE_LDVIRTFTN:
        case CIL_OPCODE_LOCALLOC:
            return CIL_OPERAND_INT;
            
        // Long operand opcodes (8 bytes)
        case CIL_OPCODE_LDC_I8:
            return CIL_OPERAND_LONG;
            
        // Float operand opcodes (4 bytes)
        case CIL_OPCODE_LDC_R4:
            return CIL_OPERAND_FLOAT;
            
        // Double operand opcodes (8 bytes)
        case CIL_OPCODE_LDC_R8:
            return CIL_OPERAND_DOUBLE;
            
        // Token operand opcodes (4 bytes)
        case CIL_OPCODE_LDSTR:
        case CIL_OPCODE_NEWOBJ:
        case CIL_OPCODE_CASTCLASS:
        case CIL_OPCODE_ISINST:
        case CIL_OPCODE_UNBOX:
        case CIL_OPCODE_LDFLD:
        case CIL_OPCODE_LDFLDA:
        case CIL_OPCODE_STFLD:
        case CIL_OPCODE_LDSFLD:
        case CIL_OPCODE_LDSFLDA:
        case CIL_OPCODE_STSFLD:
        case CIL_OPCODE_STOBJ:
        case CIL_OPCODE_BOX:
        case CIL_OPCODE_NEWARR:
        case CIL_OPCODE_LDELEMA:
        case CIL_OPCODE_LDELEM_I1:
        case CIL_OPCODE_LDELEM_U1:
        case CIL_OPCODE_LDELEM_I2:
        case CIL_OPCODE_LDELEM_U2:
        case CIL_OPCODE_LDELEM_I4:
        case CIL_OPCODE_LDELEM_U4:
        case CIL_OPCODE_LDELEM_I8:
        case CIL_OPCODE_LDELEM_I:
        case CIL_OPCODE_LDELEM_R4:
        case CIL_OPCODE_LDELEM_R8:
        case CIL_OPCODE_LDELEM_REF:
        case CIL_OPCODE_STELEM_I:
        case CIL_OPCODE_STELEM_I1:
        case CIL_OPCODE_STELEM_I2:
        case CIL_OPCODE_STELEM_I4:
        case CIL_OPCODE_STELEM_I8:
        case CIL_OPCODE_STELEM_R4:
        case CIL_OPCODE_STELEM_R8:
        case CIL_OPCODE_STELEM_REF:
        case CIL_OPCODE_LDELEM_ANY:
        case CIL_OPCODE_STELEM_ANY:
        case CIL_OPCODE_UNBOX_ANY:
        case CIL_OPCODE_INITOBJ:
        case CIL_OPCODE_SIZEOF:
        case CIL_OPCODE_REFANYTYPE:
        case CIL_OPCODE_LDFTN:
            return CIL_OPERAND_TOKEN;
            
        // Branch operand opcodes (4 bytes)
        case CIL_OPCODE_BR:
        case CIL_OPCODE_BRFALSE:
        case CIL_OPCODE_BRTRUE:
        case CIL_OPCODE_BEQ:
        case CIL_OPCODE_BGE:
        case CIL_OPCODE_BGT:
        case CIL_OPCODE_BLE:
        case CIL_OPCODE_BLT:
        case CIL_OPCODE_BNE_UN:
        case CIL_OPCODE_BGE_UN:
        case CIL_OPCODE_BGT_UN:
        case CIL_OPCODE_BLE_UN:
        case CIL_OPCODE_BLT_UN:
            return CIL_OPERAND_BRANCH;
            
        // Short branch operand opcodes (1 byte)
        case CIL_OPCODE_BR_S:
        case CIL_OPCODE_BRFALSE_S:
        case CIL_OPCODE_BRTRUE_S:
        case CIL_OPCODE_BEQ_S:
        case CIL_OPCODE_BGE_S:
        case CIL_OPCODE_BGT_S:
        case CIL_OPCODE_BLE_S:
        case CIL_OPCODE_BLT_S:
        case CIL_OPCODE_BNE_UN_S:
        case CIL_OPCODE_BGE_UN_S:
        case CIL_OPCODE_BGT_UN_S:
        case CIL_OPCODE_BLE_UN_S:
        case CIL_OPCODE_BLT_UN_S:
            return CIL_OPERAND_BRANCH_SHORT;
            
        // Switch table operand
        case CIL_OPCODE_SWITCH:
            return CIL_OPERAND_SWITCH;
            
        // Default case
        default:
            return CIL_OPERAND_NONE;
    }
}

// Function to get the name of an opcode
const char* get_opcode_name(cil_opcode_t opcode) {
    switch (opcode) {
        case CIL_OPCODE_NOP: return "nop";
        case CIL_OPCODE_BREAK: return "break";
        case CIL_OPCODE_LDARG_0: return "ldarg.0";
        case CIL_OPCODE_LDARG_1: return "ldarg.1";
        case CIL_OPCODE_LDARG_2: return "ldarg.2";
        case CIL_OPCODE_LDARG_3: return "ldarg.3";
        case CIL_OPCODE_LDLOC_0: return "ldloc.0";
        case CIL_OPCODE_LDLOC_1: return "ldloc.1";
        case CIL_OPCODE_LDLOC_2: return "ldloc.2";
        case CIL_OPCODE_LDLOC_3: return "ldloc.3";
        case CIL_OPCODE_STLOC_0: return "stloc.0";
        case CIL_OPCODE_STLOC_1: return "stloc.1";
        case CIL_OPCODE_STLOC_2: return "stloc.2";
        case CIL_OPCODE_STLOC_3: return "stloc.3";
        case CIL_OPCODE_LDARG_S: return "ldarg.s";
        case CIL_OPCODE_LDARGA_S: return "ldarga.s";
        case CIL_OPCODE_STARG_S: return "starg.s";
        case CIL_OPCODE_LDLOC_S: return "ldloc.s";
        case CIL_OPCODE_LDLOCA_S: return "ldloca.s";
        case CIL_OPCODE_STLOC_S: return "stloc.s";
        case CIL_OPCODE_LDNULL: return "ldnull";
        case CIL_OPCODE_LDC_I4_M1: return "ldc.i4.m1";
        case CIL_OPCODE_LDC_I4_0: return "ldc.i4.0";
        case CIL_OPCODE_LDC_I4_1: return "ldc.i4.1";
        case CIL_OPCODE_LDC_I4_2: return "ldc.i4.2";
        case CIL_OPCODE_LDC_I4_3: return "ldc.i4.3";
        case CIL_OPCODE_LDC_I4_4: return "ldc.i4.4";
        case CIL_OPCODE_LDC_I4_5: return "ldc.i4.5";
        case CIL_OPCODE_LDC_I4_6: return "ldc.i4.6";
        case CIL_OPCODE_LDC_I4_7: return "ldc.i4.7";
        case CIL_OPCODE_LDC_I4_8: return "ldc.i4.8";
        case CIL_OPCODE_LDC_I4_S: return "ldc.i4.s";
        case CIL_OPCODE_LDC_I4: return "ldc.i4";
        case CIL_OPCODE_LDC_I8: return "ldc.i8";
        case CIL_OPCODE_LDC_R4: return "ldc.r4";
        case CIL_OPCODE_LDC_R8: return "ldc.r8";
        case CIL_OPCODE_DUP: return "dup";
        case CIL_OPCODE_POP: return "pop";
        case CIL_OPCODE_JMP: return "jmp";
        case CIL_OPCODE_CALL: return "call";
        case CIL_OPCODE_CALLI: return "calli";
        case CIL_OPCODE_RET: return "ret";
        case CIL_OPCODE_BR_S: return "br.s";
        case CIL_OPCODE_BRFALSE_S: return "brfalse.s";
        case CIL_OPCODE_BRTRUE_S: return "brtrue.s";
        case CIL_OPCODE_BEQ_S: return "beq.s";
        case CIL_OPCODE_BGE_S: return "bge.s";
        case CIL_OPCODE_BGT_S: return "bgt.s";
        case CIL_OPCODE_BLE_S: return "ble.s";
        case CIL_OPCODE_BLT_S: return "blt.s";
        case CIL_OPCODE_BNE_UN_S: return "bne.un.s";
        case CIL_OPCODE_BGE_UN_S: return "bge.un.s";
        case CIL_OPCODE_BGT_UN_S: return "bgt.un.s";
        case CIL_OPCODE_BLE_UN_S: return "ble.un.s";
        case CIL_OPCODE_BLT_UN_S: return "blt.un.s";
        case CIL_OPCODE_BR: return "br";
        case CIL_OPCODE_BRFALSE: return "brfalse";
        case CIL_OPCODE_BRTRUE: return "brtrue";
        case CIL_OPCODE_BEQ: return "beq";
        case CIL_OPCODE_BGE: return "bge";
        case CIL_OPCODE_BGT: return "bgt";
        case CIL_OPCODE_BLE: return "ble";
        case CIL_OPCODE_BLT: return "blt";
        case CIL_OPCODE_BNE_UN: return "bne.un";
        case CIL_OPCODE_BGE_UN: return "bge.un";
        case CIL_OPCODE_BGT_UN: return "bgt.un";
        case CIL_OPCODE_BLE_UN: return "ble.un";
        case CIL_OPCODE_BLT_UN: return "blt.un";
        case CIL_OPCODE_SWITCH: return "switch";
        case CIL_OPCODE_LDIND_I1: return "ldind.i1";
        case CIL_OPCODE_LDIND_U1: return "ldind.u1";
        case CIL_OPCODE_LDIND_I2: return "ldind.i2";
        case CIL_OPCODE_LDIND_U2: return "ldind.u2";
        case CIL_OPCODE_LDIND_I4: return "ldind.i4";
        case CIL_OPCODE_LDIND_U4: return "ldind.u4";
        case CIL_OPCODE_LDIND_I8: return "ldind.i8";
        case CIL_OPCODE_LDIND_I: return "ldind.i";
        case CIL_OPCODE_LDIND_R4: return "ldind.r4";
        case CIL_OPCODE_LDIND_R8: return "ldind.r8";
        case CIL_OPCODE_LDIND_REF: return "ldind.ref";
        case CIL_OPCODE_STIND_REF: return "stind.ref";
        case CIL_OPCODE_STIND_I1: return "stind.i1";
        case CIL_OPCODE_STIND_I2: return "stind.i2";
        case CIL_OPCODE_STIND_I4: return "stind.i4";
        case CIL_OPCODE_STIND_I8: return "stind.i8";
        case CIL_OPCODE_STIND_R4: return "stind.r4";
        case CIL_OPCODE_STIND_R8: return "stind.r8";
        case CIL_OPCODE_ADD: return "add";
        case CIL_OPCODE_SUB: return "sub";
        case CIL_OPCODE_MUL: return "mul";
        case CIL_OPCODE_DIV: return "div";
        case CIL_OPCODE_DIV_UN: return "div.un";
        case CIL_OPCODE_REM: return "rem";
        case CIL_OPCODE_REM_UN: return "rem.un";
        case CIL_OPCODE_AND: return "and";
        case CIL_OPCODE_OR: return "or";
        case CIL_OPCODE_XOR: return "xor";
        case CIL_OPCODE_SHL: return "shl";
        case CIL_OPCODE_SHR: return "shr";
        case CIL_OPCODE_SHR_UN: return "shr.un";
        case CIL_OPCODE_NEG: return "neg";
        case CIL_OPCODE_NOT: return "not";
        case CIL_OPCODE_CONV_I1: return "conv.i1";
        case CIL_OPCODE_CONV_I2: return "conv.i2";
        case CIL_OPCODE_CONV_I4: return "conv.i4";
        case CIL_OPCODE_CONV_I8: return "conv.i8";
        case CIL_OPCODE_CONV_R4: return "conv.r4";
        case CIL_OPCODE_CONV_R8: return "conv.r8";
        case CIL_OPCODE_CONV_U4: return "conv.u4";
        case CIL_OPCODE_CONV_U8: return "conv.u8";
        case CIL_OPCODE_CALLVIRT: return "callvirt";
        case CIL_OPCODE_CPOBJ: return "cpobj";
        case CIL_OPCODE_LDOBJ: return "ldobj";
        case CIL_OPCODE_LDSTR: return "ldstr";
        case CIL_OPCODE_NEWOBJ: return "newobj";
        case CIL_OPCODE_CASTCLASS: return "castclass";
        case CIL_OPCODE_ISINST: return "isinst";
        case CIL_OPCODE_CONV_R_UN: return "conv.r.un";
        case CIL_OPCODE_UNBOX: return "unbox";
        case CIL_OPCODE_THROW: return "throw";
        case CIL_OPCODE_LDFLD: return "ldfld";
        case CIL_OPCODE_LDFLDA: return "ldflda";
        case CIL_OPCODE_STFLD: return "stfld";
        case CIL_OPCODE_LDSFLD: return "ldsfld";
        case CIL_OPCODE_LDSFLDA: return "ldsflda";
        case CIL_OPCODE_STSFLD: return "stsfld";
        case CIL_OPCODE_STOBJ: return "stobj";
        case CIL_OPCODE_CONV_OVF_I1_UN: return "conv.ovf.i1.un";
        case CIL_OPCODE_CONV_OVF_I2_UN: return "conv.ovf.i2.un";
        case CIL_OPCODE_CONV_OVF_I4_UN: return "conv.ovf.i4.un";
        case CIL_OPCODE_CONV_OVF_I8_UN: return "conv.ovf.i8.un";
        case CIL_OPCODE_CONV_OVF_U1_UN: return "conv.ovf.u1.un";
        case CIL_OPCODE_CONV_OVF_U2_UN: return "conv.ovf.u2.un";
        case CIL_OPCODE_CONV_OVF_U4_UN: return "conv.ovf.u4.un";
        case CIL_OPCODE_CONV_OVF_U8_UN: return "conv.ovf.u8.un";
        case CIL_OPCODE_CONV_OVF_I_UN: return "conv.ovf.i.un";
        case CIL_OPCODE_CONV_OVF_U_UN: return "conv.ovf.u.un";
        case CIL_OPCODE_BOX: return "box";
        case CIL_OPCODE_NEWARR: return "newarr";
        case CIL_OPCODE_LDLEN: return "ldlen";
        case CIL_OPCODE_LDELEMA: return "ldelema";
        case CIL_OPCODE_LDELEM_I1: return "ldelem.i1";
        case CIL_OPCODE_LDELEM_U1: return "ldelem.u1";
        case CIL_OPCODE_LDELEM_I2: return "ldelem.i2";
        case CIL_OPCODE_LDELEM_U2: return "ldelem.u2";
        case CIL_OPCODE_LDELEM_I4: return "ldelem.i4";
        case CIL_OPCODE_LDELEM_U4: return "ldelem.u4";
        case CIL_OPCODE_LDELEM_I8: return "ldelem.i8";
        case CIL_OPCODE_LDELEM_I: return "ldelem.i";
        case CIL_OPCODE_LDELEM_R4: return "ldelem.r4";
        case CIL_OPCODE_LDELEM_R8: return "ldelem.r8";
        case CIL_OPCODE_LDELEM_REF: return "ldelem.ref";
        case CIL_OPCODE_STELEM_I: return "stelem.i";
        case CIL_OPCODE_STELEM_I1: return "stelem.i1";
        case CIL_OPCODE_STELEM_I2: return "stelem.i2";
        case CIL_OPCODE_STELEM_I4: return "stelem.i4";
        case CIL_OPCODE_STELEM_I8: return "stelem.i8";
        case CIL_OPCODE_STELEM_R4: return "stelem.r4";
        case CIL_OPCODE_STELEM_R8: return "stelem.r8";
        case CIL_OPCODE_STELEM_REF: return "stelem.ref";
        case CIL_OPCODE_LDELEM_ANY: return "ldelem.any";
        case CIL_OPCODE_STELEM_ANY: return "stelem.any";
        case CIL_OPCODE_UNBOX_ANY: return "unbox.any";
        case CIL_OPCODE_CONV_OVF_I1: return "conv.ovf.i1";
        case CIL_OPCODE_CONV_OVF_U1: return "conv.ovf.u1";
        case CIL_OPCODE_CONV_OVF_I2: return "conv.ovf.i2";
        case CIL_OPCODE_CONV_OVF_U2: return "conv.ovf.u2";
        case CIL_OPCODE_CONV_OVF_I4: return "conv.ovf.i4";
        case CIL_OPCODE_CONV_OVF_U4: return "conv.ovf.u4";
        case CIL_OPCODE_CONV_OVF_I8: return "conv.ovf.i8";
        case CIL_OPCODE_CONV_OVF_U8: return "conv.ovf.u8";
        case CIL_OPCODE_ARGLIST: return "arglist";
        case CIL_OPCODE_CEQ: return "ceq";
        case CIL_OPCODE_CGT: return "cgt";
        case CIL_OPCODE_CGT_UN: return "cgt.un";
        case CIL_OPCODE_CLT: return "clt";
        case CIL_OPCODE_CLT_UN: return "clt.un";
        case CIL_OPCODE_LDFTN: return "ldftn";
        case CIL_OPCODE_LDVIRTFTN: return "ldvirtftn";
        case CIL_OPCODE_LDARG: return "ldarg";
        case CIL_OPCODE_LDARGA: return "ldarga";
        case CIL_OPCODE_STARG: return "starg";
        case CIL_OPCODE_LDLOC: return "ldloc";
        case CIL_OPCODE_LDLOCA: return "ldloca";
        case CIL_OPCODE_STLOC: return "stloc";
        case CIL_OPCODE_LOCALLOC: return "localloc";
        case CIL_OPCODE_ENDFILTER: return "endfilter";
        case CIL_OPCODE_UNALIGNED: return "unaligned.";
        case CIL_OPCODE_VOLATILE: return "volatile.";
        case CIL_OPCODE_TAIL: return "tail.";
        case CIL_OPCODE_INITOBJ: return "initobj";
        case CIL_OPCODE_CONSTRAINED: return "constrained.";
        case CIL_OPCODE_CPBLK: return "cpblk";
        case CIL_OPCODE_INITBLK: return "initblk";
        case CIL_OPCODE_RETHROW: return "rethrow";
        case CIL_OPCODE_SIZEOF: return "sizeof";
        case CIL_OPCODE_REFANYTYPE: return "refanytype";
        case CIL_OPCODE_READONLY: return "readonly.";
        case CIL_OPCODE_PREFIX7: return "prefix7";
        case CIL_OPCODE_PREFIX6: return "prefix6";
        case CIL_OPCODE_PREFIX5: return "prefix5";
        case CIL_OPCODE_PREFIX4: return "prefix4";
        case CIL_OPCODE_PREFIX3: return "prefix3";
        case CIL_OPCODE_PREFIX2: return "prefix2";
        case CIL_OPCODE_PREFIX1: return "prefix1";
        case CIL_OPCODE_PREFIXREF: return "prefixref";
        
        // Arithmetic overflow operations
        case CIL_OPCODE_ADD_OVF: return "add.ovf";
        case CIL_OPCODE_ADD_OVF_UN: return "add.ovf.un";
        case CIL_OPCODE_MUL_OVF: return "mul.ovf";
        case CIL_OPCODE_MUL_OVF_UN: return "mul.ovf.un";
        case CIL_OPCODE_SUB_OVF: return "sub.ovf";
        case CIL_OPCODE_SUB_OVF_UN: return "sub.ovf.un";
        case CIL_OPCODE_DIV_OVF: return "div.ovf";
        case CIL_OPCODE_DIV_OVF_UN: return "div.ovf.un";
        
        default: return "unknown";
    }
}

// Helper function to read a 32-bit integer from bytecode (little-endian)
static int32_t read_int32(const uint8_t* data, size_t offset) {
    return (int32_t)(data[offset] | 
                    (data[offset + 1] << 8) | 
                    (data[offset + 2] << 16) | 
                    (data[offset + 3] << 24));
}

// Helper function to read a 64-bit integer from bytecode (little-endian)
static int64_t read_int64(const uint8_t* data, size_t offset) {
    uint32_t low = (uint32_t)(data[offset] | 
                             (data[offset + 1] << 8) | 
                             (data[offset + 2] << 16) | 
                             (data[offset + 3] << 24));
    uint32_t high = (uint32_t)(data[offset + 4] | 
                              (data[offset + 5] << 8) | 
                              (data[offset + 6] << 16) | 
                              (data[offset + 7] << 24));
    return ((int64_t)high << 32) | low;
}

// Helper function to read a 32-bit unsigned integer from bytecode (little-endian)
static uint32_t read_uint32(const uint8_t* data, size_t offset) {
    return (uint32_t)(data[offset] | 
                     (data[offset + 1] << 8) | 
                     (data[offset + 2] << 16) | 
                     (data[offset + 3] << 24));
}

// Function to check if there are more instructions to decode
bool has_more_instructions(cil_decoder_t* decoder) {
    if (decoder == NULL) {
        return false;
    }
    return decoder->offset < decoder->bytecode_size && !decoder->has_error;
}

// Function to decode the next instruction
cil_instruction_t* decode_next_instruction(cil_decoder_t* decoder) {
    if (decoder == NULL || decoder->has_error || decoder->offset >= decoder->bytecode_size) {
        return NULL;
    }
    
    cil_instruction_t* instruction = (cil_instruction_t*)malloc(sizeof(cil_instruction_t));
    if (instruction == NULL) {
        decoder->has_error = true;
        decoder->error_message = "Memory allocation failed";
        return NULL;
    }
    
    // Initialize instruction
    memset(instruction, 0, sizeof(cil_instruction_t));
    
    // Read the opcode
    uint8_t opcode_byte = decoder->bytecode[decoder->offset];
    instruction->size = 1;
    
    // Check if it's an extended opcode
    if (is_extended_opcode(opcode_byte)) {
        if (decoder->offset + 1 >= decoder->bytecode_size) {
            decoder->has_error = true;
            decoder->error_message = "Unexpected end of bytecode";
            free(instruction);
            return NULL;
        }
        
        uint8_t extended_opcode = decoder->bytecode[decoder->offset + 1];
        instruction->opcode = (cil_opcode_t)(0x100 + extended_opcode); // Extended opcodes start at 0x100
        instruction->size = 2;
        decoder->offset += 2;
    } else {
        instruction->opcode = (cil_opcode_t)opcode_byte;
        decoder->offset += 1;
    }
    
    // Get operand type
    instruction->operand_type = get_opcode_operand_type(instruction->opcode);
    
    // Decode operand based on type
    switch (instruction->operand_type) {
        case CIL_OPERAND_BYTE:
            if (decoder->offset >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            instruction->operand.byte_val = decoder->bytecode[decoder->offset];
            instruction->size += 1;
            decoder->offset += 1;
            break;
            
        case CIL_OPERAND_SHORT:
            if (decoder->offset + 1 >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            instruction->operand.short_val = (int16_t)read_int32(decoder->bytecode, decoder->offset);
            instruction->size += 2;
            decoder->offset += 2;
            break;
            
        case CIL_OPERAND_INT:
        case CIL_OPERAND_TOKEN:
        case CIL_OPERAND_BRANCH:
            if (decoder->offset + 3 >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            instruction->operand.int_val = read_int32(decoder->bytecode, decoder->offset);
            instruction->size += 4;
            decoder->offset += 4;
            break;
            
        case CIL_OPERAND_LONG:
            if (decoder->offset + 7 >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            instruction->operand.long_val = read_int64(decoder->bytecode, decoder->offset);
            instruction->size += 8;
            decoder->offset += 8;
            break;
            
        case CIL_OPERAND_FLOAT:
            if (decoder->offset + 3 >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            // For simplicity, we're treating float as uint32_t
            instruction->operand.token = read_uint32(decoder->bytecode, decoder->offset);
            instruction->size += 4;
            decoder->offset += 4;
            break;
            
        case CIL_OPERAND_DOUBLE:
            if (decoder->offset + 7 >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            // For simplicity, we're treating double as uint64_t
            instruction->operand.long_val = read_int64(decoder->bytecode, decoder->offset);
            instruction->size += 8;
            decoder->offset += 8;
            break;
            
        case CIL_OPERAND_BRANCH_SHORT:
            if (decoder->offset >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            instruction->operand.branch_offset_short = decoder->bytecode[decoder->offset];
            instruction->size += 1;
            decoder->offset += 1;
            break;
            
        case CIL_OPERAND_SWITCH: {
            // Read the number of targets
            if (decoder->offset + 3 >= decoder->bytecode_size) {
                decoder->has_error = true;
                decoder->error_message = "Unexpected end of bytecode";
                free(instruction);
                return NULL;
            }
            
            uint32_t num_targets = read_uint32(decoder->bytecode, decoder->offset);
            instruction->operand.switch_table.num_targets = num_targets;
            instruction->size += 4;
            decoder->offset += 4;
            
            // Allocate memory for targets
            instruction->operand.switch_table.targets = (int32_t*)malloc(num_targets * sizeof(int32_t));
            if (instruction->operand.switch_table.targets == NULL) {
                decoder->has_error = true;
                decoder->error_message = "Memory allocation failed";
                free(instruction);
                return NULL;
            }
            
            // Read targets
            for (uint32_t i = 0; i < num_targets; i++) {
                if (decoder->offset + 3 >= decoder->bytecode_size) {
                    decoder->has_error = true;
                    decoder->error_message = "Unexpected end of bytecode";
                    free(instruction->operand.switch_table.targets);
                    free(instruction);
                    return NULL;
                }
                instruction->operand.switch_table.targets[i] = read_int32(decoder->bytecode, decoder->offset);
                instruction->size += 4;
                decoder->offset += 4;
            }
            break;
        }
        
        case CIL_OPERAND_NONE:
        default:
            // No operand to read
            break;
    }
    
    return instruction;
}