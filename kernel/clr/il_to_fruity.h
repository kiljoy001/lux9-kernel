/* il_to_fruity.h - IL → Fruity IR Converter
 *
 * Converts .NET IL bytecode (stack-based) to Fruity IR (explicit Pebble operations).
 * Handles:
 *   - Basic block identification
 *   - IL instruction decoding
 *   - Reference tracking (VANILLA/BURN insertion)
 *   - Control flow graph construction
 */

#ifndef IL_TO_FRUITY_H
#define IL_TO_FRUITY_H

#include "il_parser.h"
#include "fruity/fruity_ir.h"

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
    STACK_VALUE,    /* Value type (int32, float, etc.) */
    STACK_REF,      /* Reference type (objects, strings) */
    STACK_UNKNOWN   /* Unknown (conservative - treat as ref) */
} stack_entry_type_t;

/* Stack entry */
typedef struct {
    stack_entry_type_t type;
    uint32_t type_token;  /* ECMA-335 metadata token (0 if unknown) */
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
fruity_function_t* il_to_fruity_convert_method(
    il_assembly_t *assembly,
    il_method_t *method,
    il_to_fruity_error_t *error
);

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
fruity_module_t* il_to_fruity_convert_assembly(
    il_assembly_t *assembly,
    il_to_fruity_error_t *error
);

/* Free Fruity function */
void fruity_free_function(fruity_function_t *func);

/* Free Fruity module */
void fruity_free_module(fruity_module_t *mod);

/* Get error string */
const char* il_to_fruity_error_string(il_to_fruity_error_t error);

/* ========== IL Opcode Definitions ========== */

/* IL opcodes (subset we support) */
#define IL_NOP          0x00
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
#define IL_LDLOC_S      0x11
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
#define IL_BR           0x38
#define IL_BRFALSE      0x39
#define IL_BRTRUE       0x3A
#define IL_BEQ          0x3B
#define IL_BGE          0x3C
#define IL_BGT          0x3D
#define IL_BLE          0x3E
#define IL_BLT          0x3F
#define IL_BNE_UN       0x40
#define IL_LDSTR        0x72
#define IL_NEWOBJ       0x73
#define IL_LDFLD        0x7B
#define IL_STFLD        0x7D
#define IL_NEWARR       0x8D
#define IL_LDLEN        0x8E
#define IL_LDELEMA      0x8F
#define IL_LDELEM_I4    0x94
#define IL_STELEM_I4    0x9E
#define IL_CONV_I4      0x69
#define IL_CONV_I8      0x6A
#define IL_CONV_R4      0x6B
#define IL_CONV_R8      0x6C
#define IL_ADD          0x58
#define IL_SUB          0x59
#define IL_MUL          0x5A
#define IL_DIV          0x5B
#define IL_REM          0x5D
#define IL_AND          0x5F
#define IL_OR           0x60
#define IL_XOR          0x61
#define IL_SHL          0x62
#define IL_SHR          0x63
#define IL_NEG          0x65
#define IL_NOT          0x66
#define IL_CEQ          0xFE01
#define IL_CGT          0xFE02
#define IL_CLT          0xFE04

#endif /* IL_TO_FRUITY_H */
