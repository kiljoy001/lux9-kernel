/* il_to_fruity.c - IL → Fruity IR Converter
 *
 * Converts .NET IL bytecode (stack-based) to Fruity IR (explicit Pebble operations).
 */

#include "il_to_fruity.h"
#include "fruity/fruity_ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Internal context for conversion */
typedef struct il_to_fruity_ctx {
	il_assembly_t *assembly;
	il_method_t *method;

	/* Basic block tracking */
	fruity_basic_block_t **blocks;
	size_t block_count;
	size_t block_capacity;

	/* Branch targets (IL offsets that start new blocks) */
	uint32_t *branch_targets;
	size_t branch_target_count;
	size_t branch_target_capacity;

	/* Current function being built */
	fruity_function_t *current_function;

	/* Error tracking */
	il_to_fruity_error_t last_error;
} il_to_fruity_ctx_t;

/* Helper: Error strings */
const char*
il_to_fruity_error_string(il_to_fruity_error_t error)
{
	switch(error){
	case IL_TO_FRUITY_OK:
		return "success";
	case IL_TO_FRUITY_ERROR_INVALID_IL:
		return "invalid IL bytecode";
	case IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE:
		return "unsupported IL opcode";
	case IL_TO_FRUITY_ERROR_STACK_UNDERFLOW:
		return "stack underflow";
	case IL_TO_FRUITY_ERROR_OUT_OF_MEMORY:
		return "out of memory";
	case IL_TO_FRUITY_ERROR_METADATA:
		return "metadata error";
	case IL_TO_FRUITY_ERROR_CFG:
		return "control flow graph error";
	default:
		return "unknown error";
	}
}

/* Helper: Add branch target */
static int
add_branch_target(il_to_fruity_ctx_t *ctx, uint32_t offset)
{
	/* Check if already exists */
	for(size_t i = 0; i < ctx->branch_target_count; i++){
		if(ctx->branch_targets[i] == offset)
			return 0;
	}

	/* Expand capacity if needed */
	if(ctx->branch_target_count >= ctx->branch_target_capacity){
		size_t new_capacity = ctx->branch_target_capacity ? ctx->branch_target_capacity * 2 : 16;
		uint32_t *new_targets = realloc(ctx->branch_targets, new_capacity * sizeof(uint32_t));
		if(new_targets == NULL)
			return -1;
		ctx->branch_targets = new_targets;
		ctx->branch_target_capacity = new_capacity;
	}

	ctx->branch_targets[ctx->branch_target_count++] = offset;
	return 0;
}

/* Helper: Is offset a branch target? */
static int
is_branch_target(il_to_fruity_ctx_t *ctx, uint32_t offset)
{
	for(size_t i = 0; i < ctx->branch_target_count; i++){
		if(ctx->branch_targets[i] == offset)
			return 1;
	}
	return 0;
}

/* Helper: Create new basic block */
static fruity_basic_block_t*
create_basic_block(il_to_fruity_ctx_t *ctx, uint32_t block_id)
{
	fruity_basic_block_t *block = calloc(1, sizeof(fruity_basic_block_t));
	if(block == NULL){
		ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	block->block_id = block_id;
	block->instructions_head = NULL;
	block->instructions_tail = NULL;
	block->instruction_count = 0;

	/* Expand blocks array if needed */
	if(ctx->block_count >= ctx->block_capacity){
		size_t new_capacity = ctx->block_capacity ? ctx->block_capacity * 2 : 16;
		fruity_basic_block_t **new_blocks = realloc(ctx->blocks, new_capacity * sizeof(fruity_basic_block_t*));
		if(new_blocks == NULL){
			free(block);
			ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
			return NULL;
		}
		ctx->blocks = new_blocks;
		ctx->block_capacity = new_capacity;
	}

	ctx->blocks[ctx->block_count++] = block;
	return block;
}

/* Helper: Add instruction to block */
static int
add_instruction_to_block(fruity_basic_block_t *block, fruity_instruction_t *instr)
{
	instr->next = NULL;
	instr->prev = block->instructions_tail;

	if(block->instructions_tail)
		block->instructions_tail->next = instr;
	else
		block->instructions_head = instr;

	block->instructions_tail = instr;
	block->instruction_count++;
	return 0;
}

/* Helper: Create Fruity instruction */
static fruity_instruction_t*
create_fruity_instruction(fruity_opcode_t opcode, fruity_operand_t operand, uint32_t il_offset)
{
	fruity_instruction_t *instr = calloc(1, sizeof(fruity_instruction_t));
	if(instr == NULL)
		return NULL;

	instr->opcode = opcode;
	instr->operand = operand;
	instr->msil_offset = il_offset;
	return instr;
}

/* Phase 1: Identify basic block boundaries */
static int
identify_basic_blocks(il_to_fruity_ctx_t *ctx)
{
	const uint8_t *il = ctx->method->il_code;
	size_t il_size = ctx->method->il_code_size;
	size_t offset = 0;

	/* Offset 0 is always a block start */
	if(add_branch_target(ctx, 0) != 0)
		return -1;

	/* Scan IL to find branch targets */
	while(offset < il_size){
		uint8_t opcode = il[offset];

		/* Handle branch instructions */
		switch(opcode){
		case IL_BR_S:
		case IL_BRFALSE_S:
		case IL_BRTRUE_S:
		case IL_BEQ_S:
		case IL_BGE_S:
		case IL_BGT_S:
		case IL_BLE_S:
		case IL_BLT_S:
		case IL_BNE_UN_S:
			/* Short branch: 1-byte offset */
			if(offset + 2 > il_size){
				ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
				return -1;
			}
			int8_t short_offset = (int8_t)il[offset + 1];
			uint32_t target = offset + 2 + short_offset;
			if(add_branch_target(ctx, target) != 0)
				return -1;
			/* Instruction after branch is also a block start */
			if(add_branch_target(ctx, offset + 2) != 0)
				return -1;
			offset += 2;
			break;

		case IL_BR:
		case IL_BRFALSE:
		case IL_BRTRUE:
		case IL_BEQ:
		case IL_BGE:
		case IL_BGT:
		case IL_BLE:
		case IL_BLT:
		case IL_BNE_UN:
			/* Long branch: 4-byte offset */
			if(offset + 5 > il_size){
				ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
				return -1;
			}
			int32_t long_offset = *(int32_t*)&il[offset + 1];
			uint32_t long_target = offset + 5 + long_offset;
			if(add_branch_target(ctx, long_target) != 0)
				return -1;
			/* Instruction after branch is also a block start */
			if(add_branch_target(ctx, offset + 5) != 0)
				return -1;
			offset += 5;
			break;

		case IL_RET:
			/* Return ends a block; next instruction (if any) starts new block */
			if(offset + 1 < il_size){
				if(add_branch_target(ctx, offset + 1) != 0)
					return -1;
			}
			offset += 1;
			break;

		/* Single-byte instructions */
		case IL_NOP:
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
		case IL_ADD:
		case IL_SUB:
		case IL_MUL:
		case IL_DIV:
		case IL_REM:
		case IL_AND:
		case IL_OR:
		case IL_XOR:
		case IL_SHL:
		case IL_SHR:
		case IL_NEG:
		case IL_NOT:
			offset += 1;
			break;

		/* Two-byte instructions */
		case IL_LDARG_S:
		case IL_LDLOC_S:
		case IL_STLOC_S:
		case IL_LDC_I4_S:
			offset += 2;
			break;

		/* Five-byte instructions */
		case IL_LDC_I4:
		case IL_CALL:
		case IL_LDSTR:
		case IL_NEWOBJ:
		case IL_LDFLD:
		case IL_STFLD:
		case IL_NEWARR:
		case IL_LDELEMA:
			offset += 5;
			break;

		/* Nine-byte instructions */
		case IL_LDC_I8:
			offset += 9;
			break;

		/* Two-byte opcode prefix */
		case 0xFE:
			if(offset + 2 > il_size){
				ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
				return -1;
			}
			offset += 2;
			break;

		default:
			/* Unknown/unsupported opcode */
			ctx->last_error = IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE;
			return -1;
		}
	}

	return 0;
}

/* Phase 2: Translate IL instructions to Fruity IR */
static int
translate_instruction(il_to_fruity_ctx_t *ctx, fruity_basic_block_t *block,
                      const uint8_t *il, size_t *offset_ptr, size_t il_size)
{
	size_t offset = *offset_ptr;
	uint8_t opcode = il[offset];
	fruity_instruction_t *instr = NULL;
	fruity_operand_t operand;

	memset(&operand, 0, sizeof(operand));
	operand.type = FRUITY_OP_NONE;

	switch(opcode){
	case IL_NOP:
		instr = create_fruity_instruction(FRUITY_NOP, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_LDC_I4_M1:
		operand.type = FRUITY_OP_IMM_I32;
		operand.value.i32 = -1;
		instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_LDC_I4_0:
	case IL_LDC_I4_1:
	case IL_LDC_I4_2:
	case IL_LDC_I4_3:
	case IL_LDC_I4_4:
	case IL_LDC_I4_5:
	case IL_LDC_I4_6:
	case IL_LDC_I4_7:
	case IL_LDC_I4_8:
		operand.type = FRUITY_OP_IMM_I32;
		operand.value.i32 = opcode - IL_LDC_I4_0;
		instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_LDC_I4_S:
		operand.type = FRUITY_OP_IMM_I32;
		operand.value.i32 = (int8_t)il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
		*offset_ptr += 2;
		break;

	case IL_LDC_I4:
		operand.type = FRUITY_OP_IMM_I32;
		operand.value.i32 = *(int32_t*)&il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
		*offset_ptr += 5;
		break;

	case IL_LDC_I8:
		operand.type = FRUITY_OP_IMM_I64;
		operand.value.i64 = *(int64_t*)&il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LDC_I8, operand, offset);
		*offset_ptr += 9;
		break;

	case IL_LDNULL:
		instr = create_fruity_instruction(FRUITY_LDNULL, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_DUP:
		instr = create_fruity_instruction(FRUITY_DUP, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_POP:
		instr = create_fruity_instruction(FRUITY_POP, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_LDLOC_0:
	case IL_LDLOC_1:
	case IL_LDLOC_2:
	case IL_LDLOC_3:
		operand.type = FRUITY_OP_LOCAL;
		operand.value.index = opcode - IL_LDLOC_0;
		instr = create_fruity_instruction(FRUITY_LOAD_LOCAL, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_LDLOC_S:
		operand.type = FRUITY_OP_LOCAL;
		operand.value.index = il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LOAD_LOCAL, operand, offset);
		*offset_ptr += 2;
		break;

	case IL_STLOC_0:
	case IL_STLOC_1:
	case IL_STLOC_2:
	case IL_STLOC_3:
		operand.type = FRUITY_OP_LOCAL;
		operand.value.index = opcode - IL_STLOC_0;
		instr = create_fruity_instruction(FRUITY_STORE_LOCAL, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_STLOC_S:
		operand.type = FRUITY_OP_LOCAL;
		operand.value.index = il[offset + 1];
		instr = create_fruity_instruction(FRUITY_STORE_LOCAL, operand, offset);
		*offset_ptr += 2;
		break;

	case IL_LDARG_0:
	case IL_LDARG_1:
	case IL_LDARG_2:
	case IL_LDARG_3:
		operand.type = FRUITY_OP_ARG;
		operand.value.index = opcode - IL_LDARG_0;
		instr = create_fruity_instruction(FRUITY_LOAD_ARG, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_LDARG_S:
		operand.type = FRUITY_OP_ARG;
		operand.value.index = il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LOAD_ARG, operand, offset);
		*offset_ptr += 2;
		break;

	case IL_ADD:
		instr = create_fruity_instruction(FRUITY_ADD, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_SUB:
		instr = create_fruity_instruction(FRUITY_SUB, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_MUL:
		instr = create_fruity_instruction(FRUITY_MUL, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_DIV:
		instr = create_fruity_instruction(FRUITY_DIV, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_REM:
		instr = create_fruity_instruction(FRUITY_REM, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_AND:
		instr = create_fruity_instruction(FRUITY_AND, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_OR:
		instr = create_fruity_instruction(FRUITY_OR, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_XOR:
		instr = create_fruity_instruction(FRUITY_XOR, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_NOT:
		instr = create_fruity_instruction(FRUITY_NOT, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_NEG:
		instr = create_fruity_instruction(FRUITY_NEG, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_CALL:
		operand.type = FRUITY_OP_METHOD;
		operand.value.token = *(uint32_t*)&il[offset + 1];
		instr = create_fruity_instruction(FRUITY_CALL, operand, offset);
		*offset_ptr += 5;
		break;

	case IL_RET:
		instr = create_fruity_instruction(FRUITY_RET, operand, offset);
		*offset_ptr += 1;
		break;

	case IL_NEWOBJ:
	case IL_NEWARR:
		operand.type = FRUITY_OP_TYPE;
		operand.value.token = *(uint32_t*)&il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LIME, operand, offset);
		*offset_ptr += 5;
		break;

	case IL_LDSTR:
		/* TODO: Handle string loading properly */
		operand.type = FRUITY_OP_TYPE;
		operand.value.token = *(uint32_t*)&il[offset + 1];
		instr = create_fruity_instruction(FRUITY_LIME, operand, offset);
		*offset_ptr += 5;
		break;

	/* Branch instructions - TODO: Link to target blocks */
	case IL_BR_S:
	case IL_BRFALSE_S:
	case IL_BRTRUE_S:
		/* For now, treat as jump placeholder */
		instr = create_fruity_instruction(FRUITY_JUMP, operand, offset);
		*offset_ptr += 2;
		break;

	case IL_BR:
	case IL_BRFALSE:
	case IL_BRTRUE:
		/* For now, treat as jump placeholder */
		instr = create_fruity_instruction(FRUITY_JUMP, operand, offset);
		*offset_ptr += 5;
		break;

	default:
		ctx->last_error = IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE;
		return -1;
	}

	if(instr == NULL){
		ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
		return -1;
	}

	if(add_instruction_to_block(block, instr) != 0){
		free(instr);
		ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
		return -1;
	}

	return 0;
}

/* Convert IL method to Fruity function */
fruity_function_t*
il_to_fruity_convert_method(il_assembly_t *assembly, il_method_t *method,
                            il_to_fruity_error_t *error)
{
	il_to_fruity_ctx_t ctx;
	memset(&ctx, 0, sizeof(ctx));

	ctx.assembly = assembly;
	ctx.method = method;
	ctx.last_error = IL_TO_FRUITY_OK;

	/* Phase 1: Identify basic blocks */
	if(identify_basic_blocks(&ctx) != 0){
		if(error) *error = ctx.last_error;
		goto cleanup;
	}

	/* Create function */
	fruity_function_t *func = calloc(1, sizeof(fruity_function_t));
	if(func == NULL){
		if(error) *error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
		goto cleanup;
	}

	func->name = strdup(method->name ? method->name : "unnamed");
	func->method_token = 0; /* TODO: Get from method */
	ctx.current_function = func;

	/* Phase 2: Build basic blocks and translate instructions */
	size_t offset = 0;
	fruity_basic_block_t *current_block = NULL;
	uint32_t block_id = 0;

	while(offset < method->il_code_size){
		/* Start new block if at branch target */
		if(is_branch_target(&ctx, offset)){
			current_block = create_basic_block(&ctx, block_id++);
			if(current_block == NULL){
				if(error) *error = ctx.last_error;
				goto cleanup;
			}
		}

		/* Ensure we have a current block */
		if(current_block == NULL){
			current_block = create_basic_block(&ctx, block_id++);
			if(current_block == NULL){
				if(error) *error = ctx.last_error;
				goto cleanup;
			}
		}

		/* Translate instruction */
		if(translate_instruction(&ctx, current_block, method->il_code, &offset, method->il_code_size) != 0){
			if(error) *error = ctx.last_error;
			goto cleanup;
		}
	}

	/* Link blocks into function */
	func->block_count = ctx.block_count;
	for(size_t i = 0; i < ctx.block_count; i++){
		fruity_basic_block_t *block = ctx.blocks[i];
		block->next = (i + 1 < ctx.block_count) ? ctx.blocks[i + 1] : NULL;
		block->prev = (i > 0) ? ctx.blocks[i - 1] : NULL;
	}
	if(ctx.block_count > 0){
		func->blocks_head = ctx.blocks[0];
		func->blocks_tail = ctx.blocks[ctx.block_count - 1];
	}

	if(error) *error = IL_TO_FRUITY_OK;

cleanup:
	if(ctx.branch_targets) free(ctx.branch_targets);
	if(ctx.blocks) free(ctx.blocks);

	return func;
}

/* Convert entire assembly to Fruity module */
fruity_module_t*
il_to_fruity_convert_assembly(il_assembly_t *assembly, il_to_fruity_error_t *error)
{
	/* TODO: Implement full assembly conversion */
	if(error) *error = IL_TO_FRUITY_ERROR_METADATA;
	return NULL;
}

/* Free Fruity function */
void
fruity_free_function(fruity_function_t *func)
{
	if(func == NULL) return;

	/* Free blocks and instructions */
	for(fruity_basic_block_t *block = func->blocks_head; block != NULL;){
		fruity_basic_block_t *next_block = block->next;

		for(fruity_instruction_t *instr = block->instructions_head; instr != NULL;){
			fruity_instruction_t *next_instr = instr->next;
			free(instr);
			instr = next_instr;
		}

		free(block);
		block = next_block;
	}

	if(func->name) free(func->name);
	free(func);
}

/* Free Fruity module */
void
fruity_free_module(fruity_module_t *mod)
{
	if(mod == NULL) return;

	/* Free functions */
	for(fruity_function_t *func = mod->functions_head; func != NULL;){
		fruity_function_t *next = func->next;
		fruity_free_function(func);
		func = next;
	}

	if(mod->name) free(mod->name);
	free(mod);
}
