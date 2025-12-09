/* test_il_to_fruity.c - Test IL → Fruity IR converter
 *
 * Compile:
 *   gcc -o test_il_to_fruity test_il_to_fruity.c il_to_fruity.c il_parser.c il_disasm.c -I. -I./fruity
 *
 * Test:
 *   fsc test_hello.fs
 *   ./test_il_to_fruity test_hello.dll
 */

#include "il_parser.h"
#include "il_to_fruity.h"
#include "il_disasm.h"
#include "fruity/fruity_ir.h"
#include <stdio.h>

/* Print Fruity instruction */
void print_fruity_instruction(fruity_instruction_t *instr)
{
	printf("  [IL_%04x] ", instr->msil_offset);

	switch(instr->opcode){
	case FRUITY_NOP: printf("FRUITY_NOP"); break;
	case FRUITY_LDC_I4: printf("FRUITY_LDC_I4 %d", instr->operand.value.i32); break;
	case FRUITY_LDC_I8: printf("FRUITY_LDC_I8 %ld", instr->operand.value.i64); break;
	case FRUITY_LDNULL: printf("FRUITY_LDNULL"); break;
	case FRUITY_DUP: printf("FRUITY_DUP"); break;
	case FRUITY_POP: printf("FRUITY_POP"); break;
	case FRUITY_LOAD_LOCAL: printf("FRUITY_LOAD_LOCAL %u", instr->operand.value.index); break;
	case FRUITY_STORE_LOCAL: printf("FRUITY_STORE_LOCAL %u", instr->operand.value.index); break;
	case FRUITY_LOAD_ARG: printf("FRUITY_LOAD_ARG %u", instr->operand.value.index); break;
	case FRUITY_ADD: printf("FRUITY_ADD"); break;
	case FRUITY_SUB: printf("FRUITY_SUB"); break;
	case FRUITY_MUL: printf("FRUITY_MUL"); break;
	case FRUITY_DIV: printf("FRUITY_DIV"); break;
	case FRUITY_REM: printf("FRUITY_REM"); break;
	case FRUITY_AND: printf("FRUITY_AND"); break;
	case FRUITY_OR: printf("FRUITY_OR"); break;
	case FRUITY_XOR: printf("FRUITY_XOR"); break;
	case FRUITY_NOT: printf("FRUITY_NOT"); break;
	case FRUITY_NEG: printf("FRUITY_NEG"); break;
	case FRUITY_CALL: printf("FRUITY_CALL 0x%08x", instr->operand.value.token); break;
	case FRUITY_RET: printf("FRUITY_RET"); break;
	case FRUITY_LIME: printf("FRUITY_LIME 0x%08x", instr->operand.value.token); break;
	case FRUITY_JUMP: printf("FRUITY_JUMP"); break;
	default: printf("UNKNOWN(%d)", instr->opcode); break;
	}
	printf("\n");
}

/* Print Fruity basic block */
void print_fruity_block(fruity_basic_block_t *block)
{
	printf("\nBlock %u: (%lu instructions)\n", block->block_id, block->instruction_count);
	for(fruity_instruction_t *instr = block->instructions_head; instr != NULL; instr = instr->next){
		print_fruity_instruction(instr);
	}
}

/* Print Fruity function */
void print_fruity_function(fruity_function_t *func)
{
	printf("\n=== Fruity Function: %s ===\n", func->name);
	printf("Method token: 0x%08x\n", func->method_token);
	printf("Block count: %lu\n", func->block_count);

	for(fruity_basic_block_t *block = func->blocks_head; block != NULL; block = block->next){
		print_fruity_block(block);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2){
		fprintf(stderr, "Usage: %s <assembly.dll>\n", argv[0]);
		return 1;
	}

	const char *path = argv[1];

	printf("=== IL Parser Test ===\n");
	printf("Parsing: %s\n\n", path);

	/* Parse IL assembly */
	il_error_t il_error;
	il_assembly_t *assembly = il_parse_assembly(path, &il_error);
	if(assembly == NULL){
		fprintf(stderr, "Error parsing assembly: %s\n", il_error_string(il_error));
		return 1;
	}

	printf("Assembly parsed successfully!\n");
	printf("PE sections: %u\n", assembly->section_count);
	printf("CLI version: %u.%u\n", assembly->cli_header.major_runtime_version,
	       assembly->cli_header.minor_runtime_version);
	printf("Entry point token: 0x%08x\n\n", assembly->cli_header.entry_point_token);

	/* Get entry point method */
	il_method_t *method = NULL;
	if(assembly->cli_header.entry_point_token != 0){
		method = il_get_method_by_token(assembly, assembly->cli_header.entry_point_token);
		if(method == NULL){
			/* Try getting main method by name */
			method = il_get_method(assembly, "main");
		}
	}

	if(method == NULL){
		fprintf(stderr, "Could not find entry point method\n");
		il_free_assembly(assembly);
		return 1;
	}

	printf("=== Entry Point Method ===\n");
	printf("Name: %s\n", method->name ? method->name : "(null)");
	printf("Max stack: %u\n", method->max_stack);
	printf("IL code size: %lu bytes\n", method->il_code_size);
	printf("Flags: 0x%02x\n\n", method->flags);

	/* Disassemble IL */
	printf("=== IL Disassembly ===\n");
	il_disassemble_method(method);

	/* Convert to Fruity IR */
	printf("\n=== Converting to Fruity IR ===\n");
	il_to_fruity_error_t fruity_error;
	fruity_function_t *fruity_func = il_to_fruity_convert_method(assembly, method, &fruity_error);

	if(fruity_func == NULL){
		fprintf(stderr, "Error converting to Fruity IR: %s\n", il_to_fruity_error_string(fruity_error));
		il_free_method(method);
		il_free_assembly(assembly);
		return 1;
	}

	printf("Conversion successful!\n");

	/* Print Fruity IR */
	print_fruity_function(fruity_func);

	/* Cleanup */
	fruity_free_function(fruity_func);
	il_free_method(method);
	il_free_assembly(assembly);

	printf("\n=== Test Complete ===\n");
	return 0;
}
