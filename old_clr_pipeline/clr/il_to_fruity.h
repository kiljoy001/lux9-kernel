/* il_to_fruity.h - IL → Fruity IR Converter
 *
 * Converts .NET IL bytecode (stack-based) to Fruity IR (explicit Pebble
 * operations). Handles:
 *   - Basic block identification
 *   - IL instruction decoding
 *   - Reference tracking (VANILLA/BURN insertion)
 *   - Control flow graph construction
 */

#ifndef IL_TO_FRUITY_H
#define IL_TO_FRUITY_H

#include "fruity/fruity_ir.h"
#include "il_parser.h"

/* Forward declarations */
typedef struct il_to_fruity_ctx il_to_fruity_ctx_t;

/* Error codes */
typedef enum {
  IL_TO_FRUITY_OK = 0,
  IL_TO_FRUITY_ERROR_INVALID_IL,
  IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE,
  IL_TO_FRUITY_ERROR_STACK_UNDERFLOW,
  IL_TO_FRUITY_ERROR_OUT_OF_MEMORY,
  IL_TO_FRUITY_ERROR_METADATA,
  IL_TO_FRUITY_ERROR_CFG,
} il_to_fruity_error_t;

/* Stack entry types (for reference tracking) */
typedef enum {
  STACK_VALUE,  /* Value type (int32, float, etc.) */
  STACK_REF,    /* Reference type (objects, strings) */
  STACK_UNKNOWN /* Unknown (conservative - treat as ref) */
} stack_entry_type_t;

/* Stack entry */
typedef struct {
  stack_entry_type_t type;
  uint32_t type_token; /* ECMA-335 metadata token (0 if unknown) */
} stack_entry_t;

/* ========== Public API ========== */

/* Convert IL method to Fruity function
 *
 * Parameters:
 *   assembly: The IL assembly containing the method
 *   method: The IL method to convert
 *   error: Output error code (can be NULL)
 *
 * Returns:
 *   Fruity function on success, NULL on error
 *
 * Note: Caller must free the returned function with fruity_free_function()
 */
fruity_function_t *il_to_fruity_convert_method(il_assembly_t *assembly,
                                               il_method_t *method,
                                               il_to_fruity_error_t *error);

/* Convert entire IL assembly to Fruity module
 *
 * Parameters:
 *   assembly: The IL assembly to convert
 *   error: Output error code (can be NULL)
 *
 * Returns:
 *   Fruity module on success, NULL on error
 *
 * Note: Caller must free the returned module with fruity_free_module()
 */
fruity_module_t *il_to_fruity_convert_assembly(il_assembly_t *assembly,
                                               il_to_fruity_error_t *error);

/* Free Fruity function */
void fruity_free_function(fruity_function_t *func);

/* Free Fruity module */
void fruity_free_module(fruity_module_t *mod);

/* Get error string */
const char *il_to_fruity_error_string(il_to_fruity_error_t error);

/* ========== IL Opcode Definitions (ECMA-335 Complete) ========== */

#define IL_NOP 0x00
#define IL_BREAK 0x01
#define IL_LDARG_0 0x02
#define IL_LDARG_1 0x03
#define IL_LDARG_2 0x04
#define IL_LDARG_3 0x05
#define IL_LDLOC_0 0x06
#define IL_LDLOC_1 0x07
#define IL_LDLOC_2 0x08
#define IL_LDLOC_3 0x09
#define IL_STLOC_0 0x0A
#define IL_STLOC_1 0x0B
#define IL_STLOC_2 0x0C
#define IL_STLOC_3 0x0D
#define IL_LDARG_S 0x0E
#define IL_LDARGA_S 0x0F
#define IL_STARG_S 0x10
#define IL_LDLOC_S 0x11
#define IL_LDLOCA_S 0x12
#define IL_STLOC_S 0x13
#define IL_LDNULL 0x14
#define IL_LDC_I4_M1 0x15
#define IL_LDC_I4_0 0x16
#define IL_LDC_I4_1 0x17
#define IL_LDC_I4_2 0x18
#define IL_LDC_I4_3 0x19
#define IL_LDC_I4_4 0x1A
#define IL_LDC_I4_5 0x1B
#define IL_LDC_I4_6 0x1C
#define IL_LDC_I4_7 0x1D
#define IL_LDC_I4_8 0x1E
#define IL_LDC_I4_S 0x1F
#define IL_LDC_I4 0x20
#define IL_LDC_I8 0x21
#define IL_LDC_R4 0x22
#define IL_LDC_R8 0x23
#define IL_DUP 0x25
#define IL_POP 0x26
#define IL_JMP 0x27
#define IL_CALL 0x28
#define IL_CALLI 0x29
#define IL_RET 0x2A
#define IL_BR_S 0x2B
#define IL_BRFALSE_S 0x2C
#define IL_BRTRUE_S 0x2D
#define IL_BEQ_S 0x2E
#define IL_BGE_S 0x2F
#define IL_BGT_S 0x30
#define IL_BLE_S 0x31
#define IL_BLT_S 0x32
#define IL_BNE_UN_S 0x33
#define IL_BR 0x38
#define IL_BRFALSE 0x39
#define IL_BRTRUE 0x3A
#define IL_BEQ 0x3B
#define IL_BGE 0x3C
#define IL_BGT 0x3D
#define IL_BLE 0x3E
#define IL_BLT 0x3F
#define IL_BNE_UN 0x40
#define IL_SWITCH 0x45
#define IL_LDIND_I1 0x46
#define IL_LDIND_U1 0x47
#define IL_LDIND_I2 0x48
#define IL_LDIND_U2 0x49
#define IL_LDIND_I4 0x4A
#define IL_LDIND_U4 0x4B
#define IL_LDIND_I8 0x4C
#define IL_LDIND_I 0x4D
#define IL_LDIND_R4 0x4E
#define IL_LDIND_R8 0x4F
#define IL_LDIND_REF 0x50
#define IL_STIND_REF 0x51
#define IL_STIND_I1 0x52
#define IL_STIND_I2 0x53
#define IL_STIND_I4 0x54
#define IL_STIND_I8 0x55
#define IL_STIND_R4 0x56
#define IL_STIND_R8 0x57
#define IL_ADD 0x58
#define IL_SUB 0x59
#define IL_MUL 0x5A
#define IL_DIV 0x5B
#define IL_DIV_UN 0x5C
#define IL_REM 0x5D
#define IL_REM_UN 0x5E
#define IL_AND 0x5F
#define IL_OR 0x60
#define IL_XOR 0x61
#define IL_SHL 0x62
#define IL_SHR 0x63
#define IL_SHR_UN 0x64
#define IL_NEG 0x65
#define IL_NOT 0x66
#define IL_CONV_I1 0x67
#define IL_CONV_I2 0x68
#define IL_CONV_I4 0x69
#define IL_CONV_I8 0x6A
#define IL_CONV_R4 0x6B
#define IL_CONV_R8 0x6C
#define IL_CONV_U4 0x6D
#define IL_CONV_U8 0x6E
#define IL_CALLVIRT 0x6F
#define IL_CPOBJ 0x70
#define IL_LDOBJ 0x71
#define IL_LDSTR 0x72
#define IL_NEWOBJ 0x73
#define IL_CASTCLASS 0x74
#define IL_ISINST 0x75
#define IL_CONV_R_UN 0x76
#define IL_UNBOX 0x79
#define IL_THROW 0x7A
#define IL_LDFLD 0x7B
#define IL_LDFLDA 0x7C
#define IL_STFLD 0x7D
#define IL_LDSFLD 0x7E
#define IL_LDSFLDA 0x7F
#define IL_STSFLD 0x80
#define IL_STOBJ 0x81
#define IL_CONV_OVF_I1_UN 0x82
#define IL_CONV_OVF_I2_UN 0x83
#define IL_CONV_OVF_I4_UN 0x84
#define IL_CONV_OVF_I8_UN 0x85
#define IL_CONV_OVF_U1_UN 0x86
#define IL_CONV_OVF_U2_UN 0x87
#define IL_CONV_OVF_U4_UN 0x88
#define IL_CONV_OVF_U8_UN 0x89
#define IL_CONV_OVF_I_UN 0x8A
#define IL_CONV_OVF_U_UN 0x8B
#define IL_BOX 0x8C
#define IL_NEWARR 0x8D
#define IL_LDLEN 0x8E
#define IL_LDELEMA 0x8F
#define IL_LDELEM_I1 0x90
#define IL_LDELEM_U1 0x91
#define IL_LDELEM_I2 0x92
#define IL_LDELEM_U2 0x93
#define IL_LDELEM_I4 0x94
#define IL_LDELEM_U4 0x95
#define IL_LDELEM_I8 0x96
#define IL_LDELEM_I 0x97
#define IL_LDELEM_R4 0x98
#define IL_LDELEM_R8 0x99
#define IL_LDELEM_REF 0x9A
#define IL_STELEM_I 0x9B
#define IL_STELEM_I1 0x9C
#define IL_STELEM_I2 0x9D
#define IL_STELEM_I4 0x9E
#define IL_STELEM_I8 0x9F
#define IL_STELEM_R4 0xA0
#define IL_STELEM_R8 0xA1
#define IL_STELEM_REF 0xA2
#define IL_LDELEM 0xA3
#define IL_STELEM 0xA4
#define IL_UNBOX_ANY 0xA5
#define IL_CONV_OVF_I1 0xB3
#define IL_CONV_OVF_U1 0xB4
#define IL_CONV_OVF_I2 0xB5
#define IL_CONV_OVF_U2 0xB6
#define IL_CONV_OVF_I4 0xB7
#define IL_CONV_OVF_U4 0xB8
#define IL_CONV_OVF_I8 0xB9
#define IL_CONV_OVF_U8 0xBA
#define IL_REFANYVAL 0xC2
#define IL_CKFINITE 0xC3
#define IL_MKREFANY 0xC6
#define IL_LDTOKEN 0xD0
#define IL_CONV_U2 0xD1
#define IL_CONV_U1 0xD2
#define IL_CONV_I 0xD3
#define IL_CONV_OVF_I 0xD4
#define IL_CONV_OVF_U 0xD5
#define IL_ADD_OVF 0xD6
#define IL_ADD_OVF_UN 0xD7
#define IL_MUL_OVF 0xD8
#define IL_MUL_OVF_UN 0xD9
#define IL_SUB_OVF 0xDA
#define IL_SUB_OVF_UN 0xDB
#define IL_ENDFINALLY 0xDC
#define IL_LEAVE 0xDD
#define IL_LEAVE_S 0xDE
#define IL_STIND_I 0xDF
#define IL_CONV_U 0xE0
#define IL_ARGLIST 0xFE00
#define IL_CEQ 0xFE01
#define IL_CGT 0xFE02
#define IL_CGT_UN 0xFE03
#define IL_CLT 0xFE04
#define IL_CLT_UN 0xFE05
#define IL_LDFTN 0xFE06
#define IL_LDVIRTFTN 0xFE07
#define IL_LDARG 0xFE09
#define IL_LDARGA 0xFE0A
#define IL_STARG 0xFE0B
#define IL_LDLOC 0xFE0C
#define IL_LDLOCA 0xFE0D
#define IL_STLOC 0xFE0E
#define IL_LOCALLOC 0xFE0F
#define IL_INITOBJ 0xFE15
#define IL_CONSTRAINED 0xFE16
#define IL_CPBLK 0xFE17
#define IL_INITBLK 0xFE18
#define IL_NO 0xFE19
#define IL_RETHROW 0xFE1A
#define IL_SIZEOF 0xFE1C
#define IL_REFANYTYPE 0xFE1D
#define IL_READONLY 0xFE1E

#endif /* IL_TO_FRUITY_H */
