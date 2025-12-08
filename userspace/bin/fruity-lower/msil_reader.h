/* msil_reader.h - MSIL Bytecode Reader
 *
 * Reads .NET assemblies and parses MSIL bytecode.
 * Does NOT parse full PE/COFF format - just extracts method bodies.
 */

#ifndef MSIL_READER_H
#define MSIL_READER_H

#include <stdint.h>
#include <stdio.h>
#include "msil_opcodes.h"

/* MSIL instruction (parsed from bytecode) */
typedef struct msil_instruction {
	uint32_t offset;        /* Offset in bytecode */
	msil_opcode_t opcode;

	/* Operand (varies by instruction) */
	union {
		int32_t i32;
		int64_t i64;
		float r32;
		double r64;
		uint32_t token;     /* Metadata token */
		uint32_t index;     /* Local/arg index */
		int32_t branch;     /* Branch offset */
	} operand;

	int has_operand;

	struct msil_instruction *next;
	struct msil_instruction *prev;
} msil_instruction_t;

/* MSIL method body */
typedef struct {
	char *name;
	uint32_t token;

	/* Method signature */
	uint32_t local_count;
	uint32_t arg_count;
	uint32_t max_stack;

	/* Bytecode */
	uint8_t *code;
	uint32_t code_size;

	/* Parsed instructions */
	msil_instruction_t *instructions_head;
	msil_instruction_t *instructions_tail;
	uint32_t instruction_count;

} msil_method_t;

/* Simple assembly reader (simplified - doesn't parse full PE format) */
typedef struct {
	char *filename;
	FILE *file;

	/* Methods */
	msil_method_t **methods;
	uint32_t method_count;

} msil_assembly_t;

/* Assembly reading */
msil_assembly_t* msil_assembly_open(const char *filename);
void msil_assembly_close(msil_assembly_t *assembly);
msil_method_t* msil_assembly_get_method(msil_assembly_t *assembly, const char *name);

/* Method parsing */
msil_method_t* msil_method_create(const char *name);
void msil_method_destroy(msil_method_t *method);
int msil_method_parse_bytecode(msil_method_t *method, uint8_t *code, uint32_t size);

/* Instruction parsing */
msil_instruction_t* msil_read_instruction(uint8_t **code_ptr, uint8_t *code_end);
void msil_instruction_destroy(msil_instruction_t *instr);

/* Debug */
void msil_method_print(msil_method_t *method);
void msil_instruction_print(msil_instruction_t *instr);

#endif /* MSIL_READER_H */
