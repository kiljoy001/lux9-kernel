/* msil_opcodes.h - MSIL (CIL) Opcode Definitions
 *
 * Based on ECMA-335 Common Intermediate Language (CIL) specification.
 * These are the standard .NET bytecode instructions we need to lower
 * to Fruity IR.
 */

#ifndef MSIL_OPCODES_H
#define MSIL_OPCODES_H

/* MSIL Opcode enumeration (ECMA-335 standard) */
typedef enum {
	/* Stack operations */
	MSIL_NOP        = 0x00,
	MSIL_BREAK      = 0x01,

	/* Load constants */
	MSIL_LDNULL     = 0x14,
	MSIL_LDC_I4_M1  = 0x15,  /* Load -1 */
	MSIL_LDC_I4_0   = 0x16,
	MSIL_LDC_I4_1   = 0x17,
	MSIL_LDC_I4_2   = 0x18,
	MSIL_LDC_I4_3   = 0x19,
	MSIL_LDC_I4_4   = 0x1A,
	MSIL_LDC_I4_5   = 0x1B,
	MSIL_LDC_I4_6   = 0x1C,
	MSIL_LDC_I4_7   = 0x1D,
	MSIL_LDC_I4_8   = 0x1E,
	MSIL_LDC_I4_S   = 0x1F,  /* Load int8 */
	MSIL_LDC_I4     = 0x20,  /* Load int32 */
	MSIL_LDC_I8     = 0x21,  /* Load int64 */
	MSIL_LDC_R4     = 0x22,  /* Load float32 */
	MSIL_LDC_R8     = 0x23,  /* Load float64 */

	/* Stack operations */
	MSIL_DUP        = 0x25,  /* Duplicate top of stack */
	MSIL_POP        = 0x26,  /* Pop top of stack */

	/* Control flow */
	MSIL_CALL       = 0x28,
	MSIL_CALLI      = 0x29,
	MSIL_RET        = 0x2A,
	MSIL_BR_S       = 0x2B,  /* Branch short */
	MSIL_BRFALSE_S  = 0x2C,
	MSIL_BRTRUE_S   = 0x2D,
	MSIL_BEQ_S      = 0x2E,
	MSIL_BGE_S      = 0x2F,
	MSIL_BGT_S      = 0x30,
	MSIL_BLE_S      = 0x31,
	MSIL_BLT_S      = 0x32,
	MSIL_BNE_UN_S   = 0x33,
	MSIL_BGE_UN_S   = 0x34,
	MSIL_BGT_UN_S   = 0x35,
	MSIL_BLE_UN_S   = 0x36,
	MSIL_BLT_UN_S   = 0x37,
	MSIL_BR         = 0x38,  /* Branch */
	MSIL_BRFALSE    = 0x39,
	MSIL_BRTRUE     = 0x3A,
	MSIL_BEQ        = 0x3B,
	MSIL_BGE        = 0x3C,
	MSIL_BGT        = 0x3D,
	MSIL_BLE        = 0x3E,
	MSIL_BLT        = 0x3F,
	MSIL_BNE_UN     = 0x40,
	MSIL_BGE_UN     = 0x41,
	MSIL_BGT_UN     = 0x42,
	MSIL_BLE_UN     = 0x43,
	MSIL_BLT_UN     = 0x44,
	MSIL_SWITCH     = 0x45,

	/* Arithmetic */
	MSIL_ADD        = 0x58,
	MSIL_SUB        = 0x59,
	MSIL_MUL        = 0x5A,
	MSIL_DIV        = 0x5B,
	MSIL_DIV_UN     = 0x5C,
	MSIL_REM        = 0x5D,
	MSIL_REM_UN     = 0x5E,
	MSIL_AND        = 0x5F,
	MSIL_OR         = 0x60,
	MSIL_XOR        = 0x61,
	MSIL_SHL        = 0x62,
	MSIL_SHR        = 0x63,
	MSIL_SHR_UN     = 0x64,
	MSIL_NEG        = 0x65,
	MSIL_NOT        = 0x66,

	/* Comparison */
	MSIL_CEQ        = 0xFE01,
	MSIL_CGT        = 0xFE02,
	MSIL_CGT_UN     = 0xFE03,
	MSIL_CLT        = 0xFE04,
	MSIL_CLT_UN     = 0xFE05,

	/* Conversion */
	MSIL_CONV_I1    = 0x67,
	MSIL_CONV_I2    = 0x68,
	MSIL_CONV_I4    = 0x69,
	MSIL_CONV_I8    = 0x6A,
	MSIL_CONV_R4    = 0x6B,
	MSIL_CONV_R8    = 0x6C,
	MSIL_CONV_U4    = 0x6D,
	MSIL_CONV_U8    = 0x6E,

	/* Local variables */
	MSIL_LDLOC_0    = 0x06,
	MSIL_LDLOC_1    = 0x07,
	MSIL_LDLOC_2    = 0x08,
	MSIL_LDLOC_3    = 0x09,
	MSIL_STLOC_0    = 0x0A,
	MSIL_STLOC_1    = 0x0B,
	MSIL_STLOC_2    = 0x0C,
	MSIL_STLOC_3    = 0x0D,
	MSIL_LDLOC_S    = 0x11,  /* Load local short */
	MSIL_LDLOCA_S   = 0x12,  /* Load local address */
	MSIL_STLOC_S    = 0x13,  /* Store local short */
	MSIL_LDLOC      = 0xFE0C,
	MSIL_LDLOCA     = 0xFE0D,
	MSIL_STLOC      = 0xFE0E,

	/* Arguments */
	MSIL_LDARG_0    = 0x02,
	MSIL_LDARG_1    = 0x03,
	MSIL_LDARG_2    = 0x04,
	MSIL_LDARG_3    = 0x05,
	MSIL_LDARG_S    = 0x0E,
	MSIL_LDARGA_S   = 0x0F,
	MSIL_STARG_S    = 0x10,
	MSIL_LDARG      = 0xFE09,
	MSIL_LDARGA     = 0xFE0A,
	MSIL_STARG      = 0xFE0B,

	/* Object model */
	MSIL_NEWOBJ     = 0x73,  /* Create object - KEY for LIME */
	MSIL_CASTCLASS  = 0x74,
	MSIL_ISINST     = 0x75,
	MSIL_UNBOX      = 0x79,
	MSIL_THROW      = 0x7A,
	MSIL_LDFLD      = 0x7B,  /* Load field */
	MSIL_LDFLDA     = 0x7C,  /* Load field address */
	MSIL_STFLD      = 0x7D,  /* Store field */
	MSIL_LDSFLD     = 0x7E,  /* Load static field */
	MSIL_LDSFLDA    = 0x7F,  /* Load static field address */
	MSIL_STSFLD     = 0x80,  /* Store static field */
	MSIL_STOBJ      = 0x81,
	MSIL_BOX        = 0x8C,
	MSIL_NEWARR     = 0x8D,  /* New array */
	MSIL_LDLEN      = 0x8E,  /* Array length */
	MSIL_LDELEMA    = 0x8F,  /* Load array element address */
	MSIL_LDELEM_I1  = 0x90,
	MSIL_LDELEM_U1  = 0x91,
	MSIL_LDELEM_I2  = 0x92,
	MSIL_LDELEM_U2  = 0x93,
	MSIL_LDELEM_I4  = 0x94,
	MSIL_LDELEM_U4  = 0x95,
	MSIL_LDELEM_I8  = 0x96,
	MSIL_LDELEM_I   = 0x97,
	MSIL_LDELEM_R4  = 0x98,
	MSIL_LDELEM_R8  = 0x99,
	MSIL_LDELEM_REF = 0x9A,
	MSIL_STELEM_I   = 0x9B,
	MSIL_STELEM_I1  = 0x9C,
	MSIL_STELEM_I2  = 0x9D,
	MSIL_STELEM_I4  = 0x9E,
	MSIL_STELEM_I8  = 0x9F,
	MSIL_STELEM_R4  = 0xA0,
	MSIL_STELEM_R8  = 0xA1,
	MSIL_STELEM_REF = 0xA2,
	MSIL_LDELEM     = 0xA3,
	MSIL_STELEM     = 0xA4,
	MSIL_UNBOX_ANY  = 0xA5,

} msil_opcode_t;

/* MSIL instruction metadata */
typedef struct {
	msil_opcode_t opcode;
	const char *name;
	int operand_bytes;  /* How many bytes follow the opcode */
	int stack_pop;      /* How many values popped */
	int stack_push;     /* How many values pushed */
	int is_branch;
	int is_call;
	int is_return;
} msil_opcode_info_t;

/* Get opcode info */
const msil_opcode_info_t* msil_get_opcode_info(msil_opcode_t opcode);
const char* msil_opcode_name(msil_opcode_t opcode);

#endif /* MSIL_OPCODES_H */
