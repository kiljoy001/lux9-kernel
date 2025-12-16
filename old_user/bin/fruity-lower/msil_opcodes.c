/* msil_opcodes.c - MSIL Opcode Metadata Implementation */

#include <stdio.h>
#include <string.h>
#include "msil_opcodes.h"

/* MSIL opcode metadata table */
static const msil_opcode_info_t opcode_table[] = {
	/* Stack operations */
	{MSIL_NOP,        "nop",        0, 0, 0, 0, 0, 0},
	{MSIL_BREAK,      "break",      0, 0, 0, 0, 0, 0},
	{MSIL_DUP,        "dup",        0, 1, 2, 0, 0, 0},
	{MSIL_POP,        "pop",        0, 1, 0, 0, 0, 0},

	/* Load constants */
	{MSIL_LDNULL,     "ldnull",     0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_M1,  "ldc.i4.m1",  0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_0,   "ldc.i4.0",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_1,   "ldc.i4.1",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_2,   "ldc.i4.2",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_3,   "ldc.i4.3",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_4,   "ldc.i4.4",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_5,   "ldc.i4.5",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_6,   "ldc.i4.6",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_7,   "ldc.i4.7",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_8,   "ldc.i4.8",   0, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4_S,   "ldc.i4.s",   1, 0, 1, 0, 0, 0},
	{MSIL_LDC_I4,     "ldc.i4",     4, 0, 1, 0, 0, 0},
	{MSIL_LDC_I8,     "ldc.i8",     8, 0, 1, 0, 0, 0},
	{MSIL_LDC_R4,     "ldc.r4",     4, 0, 1, 0, 0, 0},
	{MSIL_LDC_R8,     "ldc.r8",     8, 0, 1, 0, 0, 0},

	/* Control flow */
	{MSIL_CALL,       "call",       4, 0, 0, 0, 1, 0},  /* Variable pop/push */
	{MSIL_CALLI,      "calli",      4, 0, 0, 0, 1, 0},
	{MSIL_RET,        "ret",        0, 0, 0, 0, 0, 1},
	{MSIL_BR_S,       "br.s",       1, 0, 0, 1, 0, 0},
	{MSIL_BRFALSE_S,  "brfalse.s",  1, 1, 0, 1, 0, 0},
	{MSIL_BRTRUE_S,   "brtrue.s",   1, 1, 0, 1, 0, 0},
	{MSIL_BEQ_S,      "beq.s",      1, 2, 0, 1, 0, 0},
	{MSIL_BGE_S,      "bge.s",      1, 2, 0, 1, 0, 0},
	{MSIL_BGT_S,      "bgt.s",      1, 2, 0, 1, 0, 0},
	{MSIL_BLE_S,      "ble.s",      1, 2, 0, 1, 0, 0},
	{MSIL_BLT_S,      "blt.s",      1, 2, 0, 1, 0, 0},
	{MSIL_BNE_UN_S,   "bne.un.s",   1, 2, 0, 1, 0, 0},
	{MSIL_BGE_UN_S,   "bge.un.s",   1, 2, 0, 1, 0, 0},
	{MSIL_BGT_UN_S,   "bgt.un.s",   1, 2, 0, 1, 0, 0},
	{MSIL_BLE_UN_S,   "ble.un.s",   1, 2, 0, 1, 0, 0},
	{MSIL_BLT_UN_S,   "blt.un.s",   1, 2, 0, 1, 0, 0},
	{MSIL_BR,         "br",         4, 0, 0, 1, 0, 0},
	{MSIL_BRFALSE,    "brfalse",    4, 1, 0, 1, 0, 0},
	{MSIL_BRTRUE,     "brtrue",     4, 1, 0, 1, 0, 0},
	{MSIL_BEQ,        "beq",        4, 2, 0, 1, 0, 0},
	{MSIL_BGE,        "bge",        4, 2, 0, 1, 0, 0},
	{MSIL_BGT,        "bgt",        4, 2, 0, 1, 0, 0},
	{MSIL_BLE,        "ble",        4, 2, 0, 1, 0, 0},
	{MSIL_BLT,        "blt",        4, 2, 0, 1, 0, 0},
	{MSIL_BNE_UN,     "bne.un",     4, 2, 0, 1, 0, 0},
	{MSIL_BGE_UN,     "bge.un",     4, 2, 0, 1, 0, 0},
	{MSIL_BGT_UN,     "bgt.un",     4, 2, 0, 1, 0, 0},
	{MSIL_BLE_UN,     "ble.un",     4, 2, 0, 1, 0, 0},
	{MSIL_BLT_UN,     "blt.un",     4, 2, 0, 1, 0, 0},
	{MSIL_SWITCH,     "switch",     4, 1, 0, 1, 0, 0},  /* Variable operand */

	/* Arithmetic */
	{MSIL_ADD,        "add",        0, 2, 1, 0, 0, 0},
	{MSIL_SUB,        "sub",        0, 2, 1, 0, 0, 0},
	{MSIL_MUL,        "mul",        0, 2, 1, 0, 0, 0},
	{MSIL_DIV,        "div",        0, 2, 1, 0, 0, 0},
	{MSIL_DIV_UN,     "div.un",     0, 2, 1, 0, 0, 0},
	{MSIL_REM,        "rem",        0, 2, 1, 0, 0, 0},
	{MSIL_REM_UN,     "rem.un",     0, 2, 1, 0, 0, 0},
	{MSIL_AND,        "and",        0, 2, 1, 0, 0, 0},
	{MSIL_OR,         "or",         0, 2, 1, 0, 0, 0},
	{MSIL_XOR,        "xor",        0, 2, 1, 0, 0, 0},
	{MSIL_SHL,        "shl",        0, 2, 1, 0, 0, 0},
	{MSIL_SHR,        "shr",        0, 2, 1, 0, 0, 0},
	{MSIL_SHR_UN,     "shr.un",     0, 2, 1, 0, 0, 0},
	{MSIL_NEG,        "neg",        0, 1, 1, 0, 0, 0},
	{MSIL_NOT,        "not",        0, 1, 1, 0, 0, 0},

	/* Comparison */
	{MSIL_CEQ,        "ceq",        0, 2, 1, 0, 0, 0},
	{MSIL_CGT,        "cgt",        0, 2, 1, 0, 0, 0},
	{MSIL_CGT_UN,     "cgt.un",     0, 2, 1, 0, 0, 0},
	{MSIL_CLT,        "clt",        0, 2, 1, 0, 0, 0},
	{MSIL_CLT_UN,     "clt.un",     0, 2, 1, 0, 0, 0},

	/* Conversion */
	{MSIL_CONV_I1,    "conv.i1",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_I2,    "conv.i2",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_I4,    "conv.i4",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_I8,    "conv.i8",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_R4,    "conv.r4",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_R8,    "conv.r8",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_U4,    "conv.u4",    0, 1, 1, 0, 0, 0},
	{MSIL_CONV_U8,    "conv.u8",    0, 1, 1, 0, 0, 0},

	/* Local variables */
	{MSIL_LDLOC_0,    "ldloc.0",    0, 0, 1, 0, 0, 0},
	{MSIL_LDLOC_1,    "ldloc.1",    0, 0, 1, 0, 0, 0},
	{MSIL_LDLOC_2,    "ldloc.2",    0, 0, 1, 0, 0, 0},
	{MSIL_LDLOC_3,    "ldloc.3",    0, 0, 1, 0, 0, 0},
	{MSIL_STLOC_0,    "stloc.0",    0, 1, 0, 0, 0, 0},
	{MSIL_STLOC_1,    "stloc.1",    0, 1, 0, 0, 0, 0},
	{MSIL_STLOC_2,    "stloc.2",    0, 1, 0, 0, 0, 0},
	{MSIL_STLOC_3,    "stloc.3",    0, 1, 0, 0, 0, 0},
	{MSIL_LDLOC_S,    "ldloc.s",    1, 0, 1, 0, 0, 0},
	{MSIL_LDLOCA_S,   "ldloca.s",   1, 0, 1, 0, 0, 0},
	{MSIL_STLOC_S,    "stloc.s",    1, 1, 0, 0, 0, 0},
	{MSIL_LDLOC,      "ldloc",      2, 0, 1, 0, 0, 0},
	{MSIL_LDLOCA,     "ldloca",     2, 0, 1, 0, 0, 0},
	{MSIL_STLOC,      "stloc",      2, 1, 0, 0, 0, 0},

	/* Arguments */
	{MSIL_LDARG_0,    "ldarg.0",    0, 0, 1, 0, 0, 0},
	{MSIL_LDARG_1,    "ldarg.1",    0, 0, 1, 0, 0, 0},
	{MSIL_LDARG_2,    "ldarg.2",    0, 0, 1, 0, 0, 0},
	{MSIL_LDARG_3,    "ldarg.3",    0, 0, 1, 0, 0, 0},
	{MSIL_LDARG_S,    "ldarg.s",    1, 0, 1, 0, 0, 0},
	{MSIL_LDARGA_S,   "ldarga.s",   1, 0, 1, 0, 0, 0},
	{MSIL_STARG_S,    "starg.s",    1, 1, 0, 0, 0, 0},
	{MSIL_LDARG,      "ldarg",      2, 0, 1, 0, 0, 0},
	{MSIL_LDARGA,     "ldarga",     2, 0, 1, 0, 0, 0},
	{MSIL_STARG,      "starg",      2, 1, 0, 0, 0, 0},

	/* Object model */
	{MSIL_NEWOBJ,     "newobj",     4, 0, 1, 0, 1, 0},  /* KEY: → LIME */
	{MSIL_CASTCLASS,  "castclass",  4, 1, 1, 0, 0, 0},
	{MSIL_ISINST,     "isinst",     4, 1, 1, 0, 0, 0},
	{MSIL_UNBOX,      "unbox",      4, 1, 1, 0, 0, 0},
	{MSIL_THROW,      "throw",      0, 1, 0, 0, 0, 0},
	{MSIL_LDFLD,      "ldfld",      4, 1, 1, 0, 0, 0},
	{MSIL_LDFLDA,     "ldflda",     4, 1, 1, 0, 0, 0},
	{MSIL_STFLD,      "stfld",      4, 2, 0, 0, 0, 0},
	{MSIL_LDSFLD,     "ldsfld",     4, 0, 1, 0, 0, 0},
	{MSIL_LDSFLDA,    "ldsflda",    4, 0, 1, 0, 0, 0},
	{MSIL_STSFLD,     "stsfld",     4, 1, 0, 0, 0, 0},
	{MSIL_STOBJ,      "stobj",      4, 2, 0, 0, 0, 0},
	{MSIL_BOX,        "box",        4, 1, 1, 0, 0, 0},
	{MSIL_NEWARR,     "newarr",     4, 1, 1, 0, 0, 0},
	{MSIL_LDLEN,      "ldlen",      0, 1, 1, 0, 0, 0},
	{MSIL_LDELEMA,    "ldelema",    4, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_I1,  "ldelem.i1",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_U1,  "ldelem.u1",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_I2,  "ldelem.i2",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_U2,  "ldelem.u2",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_I4,  "ldelem.i4",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_U4,  "ldelem.u4",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_I8,  "ldelem.i8",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_I,   "ldelem.i",   0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_R4,  "ldelem.r4",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_R8,  "ldelem.r8",  0, 2, 1, 0, 0, 0},
	{MSIL_LDELEM_REF, "ldelem.ref", 0, 2, 1, 0, 0, 0},
	{MSIL_STELEM_I,   "stelem.i",   0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_I1,  "stelem.i1",  0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_I2,  "stelem.i2",  0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_I4,  "stelem.i4",  0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_I8,  "stelem.i8",  0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_R4,  "stelem.r4",  0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_R8,  "stelem.r8",  0, 3, 0, 0, 0, 0},
	{MSIL_STELEM_REF, "stelem.ref", 0, 3, 0, 0, 0, 0},
	{MSIL_LDELEM,     "ldelem",     4, 2, 1, 0, 0, 0},
	{MSIL_STELEM,     "stelem",     4, 3, 0, 0, 0, 0},
	{MSIL_UNBOX_ANY,  "unbox.any",  4, 1, 1, 0, 0, 0},

	/* Sentinel */
	{0, NULL, 0, 0, 0, 0, 0, 0}
};

const msil_opcode_info_t* msil_get_opcode_info(msil_opcode_t opcode)
{
	for (int i = 0; opcode_table[i].name != NULL; i++) {
		if (opcode_table[i].opcode == opcode)
			return &opcode_table[i];
	}
	return NULL;
}

const char* msil_opcode_name(msil_opcode_t opcode)
{
	const msil_opcode_info_t *info = msil_get_opcode_info(opcode);
	return info ? info->name : "unknown";
}
