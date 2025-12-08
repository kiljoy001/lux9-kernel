/* msil_reader.c - MSIL Bytecode Reader Implementation */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msil_reader.h"

/* Helper: Read uint8 from bytecode */
static uint8_t read_u8(uint8_t **ptr, uint8_t *end)
{
	if (*ptr >= end) return 0;
	return *(*ptr)++;
}

/* Helper: Read int8 from bytecode */
static int8_t read_i8(uint8_t **ptr, uint8_t *end)
{
	if (*ptr >= end) return 0;
	return (int8_t)*(*ptr)++;
}

/* Helper: Read uint16 from bytecode (little-endian) */
static uint16_t read_u16(uint8_t **ptr, uint8_t *end)
{
	if (*ptr + 2 > end) return 0;
	uint16_t val = (*ptr)[0] | ((*ptr)[1] << 8);
	*ptr += 2;
	return val;
}

/* Helper: Read int32 from bytecode (little-endian) */
static int32_t read_i32(uint8_t **ptr, uint8_t *end)
{
	if (*ptr + 4 > end) return 0;
	int32_t val = (*ptr)[0] | ((*ptr)[1] << 8) |
	              ((*ptr)[2] << 16) | ((*ptr)[3] << 24);
	*ptr += 4;
	return val;
}

/* Helper: Read int64 from bytecode (little-endian) */
static int64_t read_i64(uint8_t **ptr, uint8_t *end)
{
	if (*ptr + 8 > end) return 0;
	int64_t val = (int64_t)(*ptr)[0] | ((int64_t)(*ptr)[1] << 8) |
	              ((int64_t)(*ptr)[2] << 16) | ((int64_t)(*ptr)[3] << 24) |
	              ((int64_t)(*ptr)[4] << 32) | ((int64_t)(*ptr)[5] << 40) |
	              ((int64_t)(*ptr)[6] << 48) | ((int64_t)(*ptr)[7] << 56);
	*ptr += 8;
	return val;
}

/* Helper: Read float32 from bytecode */
static float read_r32(uint8_t **ptr, uint8_t *end)
{
	if (*ptr + 4 > end) return 0.0f;
	float val;
	memcpy(&val, *ptr, 4);
	*ptr += 4;
	return val;
}

/* Helper: Read float64 from bytecode */
static double read_r64(uint8_t **ptr, uint8_t *end)
{
	if (*ptr + 8 > end) return 0.0;
	double val;
	memcpy(&val, *ptr, 8);
	*ptr += 8;
	return val;
}

/* Read single MSIL instruction from bytecode stream */
msil_instruction_t* msil_read_instruction(uint8_t **code_ptr, uint8_t *code_end)
{
	if (*code_ptr >= code_end)
		return NULL;

	msil_instruction_t *instr = calloc(1, sizeof(msil_instruction_t));
	if (!instr)
		return NULL;

	instr->offset = (uint32_t)(*code_ptr - (code_end - (*code_ptr)));

	/* Read opcode (single or double byte) */
	uint8_t byte1 = read_u8(code_ptr, code_end);

	if (byte1 == 0xFE) {
		/* Two-byte opcode */
		uint8_t byte2 = read_u8(code_ptr, code_end);
		instr->opcode = (msil_opcode_t)((byte1 << 8) | byte2);
	} else {
		/* Single-byte opcode */
		instr->opcode = (msil_opcode_t)byte1;
	}

	/* Get opcode info for operand parsing */
	const msil_opcode_info_t *info = msil_get_opcode_info(instr->opcode);
	if (!info) {
		fprintf(stderr, "Unknown MSIL opcode: 0x%02X\n", instr->opcode);
		free(instr);
		return NULL;
	}

	/* Parse operand based on opcode */
	instr->has_operand = (info->operand_bytes > 0);

	if (instr->has_operand) {
		switch (instr->opcode) {
		/* Inline i8 */
		case MSIL_LDC_I4_S:
			instr->operand.i32 = (int32_t)read_i8(code_ptr, code_end);
			break;

		/* Inline i32 */
		case MSIL_LDC_I4:
			instr->operand.i32 = read_i32(code_ptr, code_end);
			break;

		/* Inline i64 */
		case MSIL_LDC_I8:
			instr->operand.i64 = read_i64(code_ptr, code_end);
			break;

		/* Inline r32 */
		case MSIL_LDC_R4:
			instr->operand.r32 = read_r32(code_ptr, code_end);
			break;

		/* Inline r64 */
		case MSIL_LDC_R8:
			instr->operand.r64 = read_r64(code_ptr, code_end);
			break;

		/* Short branch (int8) */
		case MSIL_BR_S:
		case MSIL_BRFALSE_S:
		case MSIL_BRTRUE_S:
		case MSIL_BEQ_S:
		case MSIL_BGE_S:
		case MSIL_BGT_S:
		case MSIL_BLE_S:
		case MSIL_BLT_S:
		case MSIL_BNE_UN_S:
		case MSIL_BGE_UN_S:
		case MSIL_BGT_UN_S:
		case MSIL_BLE_UN_S:
		case MSIL_BLT_UN_S:
			instr->operand.branch = (int32_t)read_i8(code_ptr, code_end);
			break;

		/* Long branch (int32) */
		case MSIL_BR:
		case MSIL_BRFALSE:
		case MSIL_BRTRUE:
		case MSIL_BEQ:
		case MSIL_BGE:
		case MSIL_BGT:
		case MSIL_BLE:
		case MSIL_BLT:
		case MSIL_BNE_UN:
		case MSIL_BGE_UN:
		case MSIL_BGT_UN:
		case MSIL_BLE_UN:
		case MSIL_BLT_UN:
			instr->operand.branch = read_i32(code_ptr, code_end);
			break;

		/* Local/arg index (uint8) */
		case MSIL_LDLOC_S:
		case MSIL_LDLOCA_S:
		case MSIL_STLOC_S:
		case MSIL_LDARG_S:
		case MSIL_LDARGA_S:
		case MSIL_STARG_S:
			instr->operand.index = (uint32_t)read_u8(code_ptr, code_end);
			break;

		/* Local/arg index (uint16) */
		case MSIL_LDLOC:
		case MSIL_LDLOCA:
		case MSIL_STLOC:
		case MSIL_LDARG:
		case MSIL_LDARGA:
		case MSIL_STARG:
			instr->operand.index = (uint32_t)read_u16(code_ptr, code_end);
			break;

		/* Metadata token (uint32) */
		case MSIL_CALL:
		case MSIL_CALLI:
		case MSIL_NEWOBJ:
		case MSIL_CASTCLASS:
		case MSIL_ISINST:
		case MSIL_UNBOX:
		case MSIL_LDFLD:
		case MSIL_LDFLDA:
		case MSIL_STFLD:
		case MSIL_LDSFLD:
		case MSIL_LDSFLDA:
		case MSIL_STSFLD:
		case MSIL_STOBJ:
		case MSIL_BOX:
		case MSIL_NEWARR:
		case MSIL_LDELEMA:
		case MSIL_LDELEM:
		case MSIL_STELEM:
		case MSIL_UNBOX_ANY:
			instr->operand.token = (uint32_t)read_i32(code_ptr, code_end);
			break;

		/* Switch (variable length - just read count for now) */
		case MSIL_SWITCH:
			instr->operand.i32 = read_i32(code_ptr, code_end);  /* Jump table size */
			/* Skip jump table entries */
			*code_ptr += instr->operand.i32 * 4;
			break;

		default:
			/* Generic operand parsing based on size */
			if (info->operand_bytes == 1)
				instr->operand.i32 = (int32_t)read_u8(code_ptr, code_end);
			else if (info->operand_bytes == 2)
				instr->operand.i32 = (int32_t)read_u16(code_ptr, code_end);
			else if (info->operand_bytes == 4)
				instr->operand.i32 = read_i32(code_ptr, code_end);
			else if (info->operand_bytes == 8)
				instr->operand.i64 = read_i64(code_ptr, code_end);
			break;
		}
	}

	return instr;
}

/* Parse entire method bytecode into instruction list */
int msil_method_parse_bytecode(msil_method_t *method, uint8_t *code, uint32_t size)
{
	if (!method || !code || size == 0)
		return -1;

	method->code = malloc(size);
	if (!method->code)
		return -1;
	memcpy(method->code, code, size);
	method->code_size = size;

	uint8_t *ptr = method->code;
	uint8_t *end = method->code + size;

	method->instruction_count = 0;
	method->instructions_head = NULL;
	method->instructions_tail = NULL;

	/* Parse all instructions */
	while (ptr < end) {
		msil_instruction_t *instr = msil_read_instruction(&ptr, end);
		if (!instr) {
			fprintf(stderr, "Failed to parse instruction at offset %ld\n",
			        ptr - method->code);
			break;
		}

		/* Add to linked list */
		if (!method->instructions_head) {
			method->instructions_head = instr;
			method->instructions_tail = instr;
		} else {
			method->instructions_tail->next = instr;
			instr->prev = method->instructions_tail;
			method->instructions_tail = instr;
		}

		method->instruction_count++;
	}

	return 0;
}

/* Create method structure */
msil_method_t* msil_method_create(const char *name)
{
	msil_method_t *method = calloc(1, sizeof(msil_method_t));
	if (!method)
		return NULL;

	if (name) {
		method->name = strdup(name);
		if (!method->name) {
			free(method);
			return NULL;
		}
	}

	return method;
}

/* Destroy method structure */
void msil_method_destroy(msil_method_t *method)
{
	if (!method)
		return;

	free(method->name);
	free(method->code);

	/* Free instruction list */
	msil_instruction_t *instr = method->instructions_head;
	while (instr) {
		msil_instruction_t *next = instr->next;
		msil_instruction_destroy(instr);
		instr = next;
	}

	free(method);
}

/* Destroy instruction */
void msil_instruction_destroy(msil_instruction_t *instr)
{
	free(instr);
}

/* Print instruction for debugging */
void msil_instruction_print(msil_instruction_t *instr)
{
	if (!instr)
		return;

	printf("  IL_%04X: %-12s", instr->offset, msil_opcode_name(instr->opcode));

	if (instr->has_operand) {
		const msil_opcode_info_t *info = msil_get_opcode_info(instr->opcode);
		if (info && info->is_branch)
			printf(" -> IL_%04X", instr->offset + (int32_t)instr->operand.branch);
		else if (info && (info->is_call || instr->opcode == MSIL_NEWOBJ))
			printf(" 0x%08X", instr->operand.token);
		else if (instr->opcode >= MSIL_LDLOC_S && instr->opcode <= MSIL_STARG)
			printf(" %u", instr->operand.index);
		else if (instr->opcode == MSIL_LDC_I4 || instr->opcode == MSIL_LDC_I4_S)
			printf(" %d", instr->operand.i32);
		else if (instr->opcode == MSIL_LDC_I8)
			printf(" %ld", instr->operand.i64);
		else if (instr->opcode == MSIL_LDC_R4)
			printf(" %f", instr->operand.r32);
		else if (instr->opcode == MSIL_LDC_R8)
			printf(" %f", instr->operand.r64);
	}

	printf("\n");
}

/* Print method for debugging */
void msil_method_print(msil_method_t *method)
{
	if (!method)
		return;

	printf(".method %s (token=0x%08X)\n", method->name, method->token);
	printf("  .maxstack %u\n", method->max_stack);
	printf("  .locals (%u)\n", method->local_count);
	printf("{\n");

	msil_instruction_t *instr = method->instructions_head;
	while (instr) {
		msil_instruction_print(instr);
		instr = instr->next;
	}

	printf("}\n");
}

/* Assembly operations (simplified - not full PE/COFF parser) */
msil_assembly_t* msil_assembly_open(const char *filename)
{
	msil_assembly_t *assembly = calloc(1, sizeof(msil_assembly_t));
	if (!assembly)
		return NULL;

	assembly->filename = strdup(filename);
	if (!assembly->filename) {
		free(assembly);
		return NULL;
	}

	assembly->file = fopen(filename, "rb");
	if (!assembly->file) {
		free(assembly->filename);
		free(assembly);
		return NULL;
	}

	/* TODO: Parse PE/COFF headers and metadata tables */
	/* For now, this is a placeholder */

	return assembly;
}

void msil_assembly_close(msil_assembly_t *assembly)
{
	if (!assembly)
		return;

	if (assembly->file)
		fclose(assembly->file);

	free(assembly->filename);

	/* Free methods */
	for (uint32_t i = 0; i < assembly->method_count; i++) {
		msil_method_destroy(assembly->methods[i]);
	}
	free(assembly->methods);

	free(assembly);
}

msil_method_t* msil_assembly_get_method(msil_assembly_t *assembly, const char *name)
{
	if (!assembly || !name)
		return NULL;

	/* TODO: Search metadata tables for method by name */
	/* For now, placeholder */

	for (uint32_t i = 0; i < assembly->method_count; i++) {
		if (assembly->methods[i]->name &&
		    strcmp(assembly->methods[i]->name, name) == 0) {
			return assembly->methods[i];
		}
	}

	return NULL;
}
