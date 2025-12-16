/* fruity_lowering.c - MSIL to Fruity IR Lowering Implementation
 *
 * Transforms MSIL bytecode into Fruity IR with explicit Pebble operations.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fruity_lowering.h"

/* Create lowering context */
lowering_context_t* lowering_context_create(msil_method_t *msil_method)
{
	if (!msil_method)
		return NULL;

	lowering_context_t *ctx = calloc(1, sizeof(lowering_context_t));
	if (!ctx)
		return NULL;

	ctx->msil_method = msil_method;
	ctx->max_stack = msil_method->max_stack;

	/* Allocate local/arg type tracking arrays */
	if (msil_method->local_count > 0) {
		ctx->local_is_ref = calloc(msil_method->local_count, sizeof(int));
		if (!ctx->local_is_ref) {
			free(ctx);
			return NULL;
		}
	}

	if (msil_method->arg_count > 0) {
		ctx->arg_is_ref = calloc(msil_method->arg_count, sizeof(int));
		if (!ctx->arg_is_ref) {
			free(ctx->local_is_ref);
			free(ctx);
			return NULL;
		}
	}

	/* Allocate stack simulation array */
	if (ctx->max_stack > 0) {
		ctx->stack_is_ref = calloc(ctx->max_stack, sizeof(int));
		if (!ctx->stack_is_ref) {
			free(ctx->arg_is_ref);
			free(ctx->local_is_ref);
			free(ctx);
			return NULL;
		}
	}

	ctx->stack_depth = 0;

	/* Detect attributes */
	ctx->is_transactional = detect_transactional_attribute(msil_method);
	ctx->is_exchange = detect_exchange_attribute(msil_method);

	/* Create Fruity function */
	ctx->fruity_func = fruity_function_create(msil_method->name, msil_method->token);
	if (!ctx->fruity_func) {
		lowering_context_destroy(ctx);
		return NULL;
	}

	return ctx;
}

/* Destroy lowering context */
void lowering_context_destroy(lowering_context_t *ctx)
{
	if (!ctx)
		return;

	free(ctx->local_is_ref);
	free(ctx->arg_is_ref);
	free(ctx->stack_is_ref);

	/* Note: fruity_func is returned to caller, don't destroy */

	free(ctx);
}

/* Stack simulation for reference tracking */
void lowering_stack_push(lowering_context_t *ctx, int is_ref)
{
	if (ctx->stack_depth >= ctx->max_stack) {
		fprintf(stderr, "Stack overflow during lowering\n");
		return;
	}
	ctx->stack_is_ref[ctx->stack_depth++] = is_ref;
}

int lowering_stack_pop(lowering_context_t *ctx)
{
	if (ctx->stack_depth == 0) {
		fprintf(stderr, "Stack underflow during lowering\n");
		return 0;
	}
	return ctx->stack_is_ref[--ctx->stack_depth];
}

int lowering_stack_peek(lowering_context_t *ctx)
{
	if (ctx->stack_depth == 0)
		return 0;
	return ctx->stack_is_ref[ctx->stack_depth - 1];
}

/* Lower MSIL newobj → FRUITY_LIME */
int lower_newobj(lowering_context_t *ctx, msil_instruction_t *instr,
                 fruity_basic_block_t *block)
{
	/* newobj creates an object and calls constructor
	 * In Fruity IR: LIME allocates Black + first White token
	 */
	fruity_instruction_t *lime = fruity_instruction_create(FRUITY_LIME);
	fruity_instruction_set_operand_i32(lime, (int32_t)instr->operand.token);
	fruity_basic_block_add_instruction(block, lime);

	ctx->lime_count++;

	/* Mark stack top as reference */
	lowering_stack_push(ctx, 1);  /* Result is reference */

	return 0;
}

/* Lower MSIL dup → FRUITY_VANILLA (if reference type) */
int lower_dup(lowering_context_t *ctx, fruity_basic_block_t *block)
{
	int is_ref = lowering_stack_peek(ctx);

	if (is_ref) {
		/* Duplicating a reference requires issuing a new white token */
		fruity_instruction_t *vanilla = fruity_instruction_create(FRUITY_VANILLA);
		fruity_basic_block_add_instruction(block, vanilla);
		ctx->vanilla_count++;
	} else {
		/* Duplicating a value type - just copy */
		fruity_instruction_t *dup = fruity_instruction_create(FRUITY_DUP);
		fruity_basic_block_add_instruction(block, dup);
	}

	/* Push same type to stack */
	lowering_stack_push(ctx, is_ref);

	return 0;
}

/* Lower MSIL pop → FRUITY_BURN (if reference type) */
int lower_pop(lowering_context_t *ctx, fruity_basic_block_t *block)
{
	int is_ref = lowering_stack_pop(ctx);

	if (is_ref) {
		/* Popping a reference requires burning the white token */
		fruity_instruction_t *burn = fruity_instruction_create(FRUITY_BURN);
		fruity_basic_block_add_instruction(block, burn);
		ctx->burn_count++;
	} else {
		/* Popping a value type - just discard */
		fruity_instruction_t *pop = fruity_instruction_create(FRUITY_POP);
		fruity_basic_block_add_instruction(block, pop);
	}

	return 0;
}

/* Lower MSIL ldloc → FRUITY_VANILLA (if reference type) */
int lower_ldloc(lowering_context_t *ctx, uint32_t index,
                fruity_basic_block_t *block)
{
	int is_ref = (index < ctx->msil_method->local_count) ?
	             ctx->local_is_ref[index] : 0;

	/* Load local to stack */
	fruity_instruction_t *ldloc = fruity_instruction_create(FRUITY_LOAD_LOCAL);
	fruity_instruction_set_operand_i32(ldloc, (int32_t)index);
	fruity_basic_block_add_instruction(block, ldloc);

	if (is_ref) {
		/* Loading a reference requires issuing a white token for the stack */
		fruity_instruction_t *vanilla = fruity_instruction_create(FRUITY_VANILLA);
		fruity_basic_block_add_instruction(block, vanilla);
		ctx->vanilla_count++;
	}

	lowering_stack_push(ctx, is_ref);

	return 0;
}

/* Lower MSIL stloc → FRUITY_BURN old + FRUITY_VANILLA new (if reference) */
int lower_stloc(lowering_context_t *ctx, uint32_t index,
                fruity_basic_block_t *block)
{
	int is_ref = lowering_stack_pop(ctx);

	if (is_ref && index < ctx->msil_method->local_count) {
		/* If local already holds a reference, burn the old token */
		if (ctx->local_is_ref[index]) {
			fruity_instruction_t *burn_old = fruity_instruction_create(FRUITY_BURN);
			fruity_basic_block_add_instruction(block, burn_old);
			ctx->burn_count++;
		}

		/* Store new reference */
		fruity_instruction_t *stloc = fruity_instruction_create(FRUITY_STORE_LOCAL);
		fruity_instruction_set_operand_i32(stloc, (int32_t)index);
		fruity_basic_block_add_instruction(block, stloc);

		/* Issue white token for new local reference */
		fruity_instruction_t *vanilla = fruity_instruction_create(FRUITY_VANILLA);
		fruity_basic_block_add_instruction(block, vanilla);
		ctx->vanilla_count++;

		/* Mark local as holding reference */
		ctx->local_is_ref[index] = 1;
	} else {
		/* Value type - simple store */
		fruity_instruction_t *stloc = fruity_instruction_create(FRUITY_STORE_LOCAL);
		fruity_instruction_set_operand_i32(stloc, (int32_t)index);
		fruity_basic_block_add_instruction(block, stloc);
	}

	return 0;
}

/* Lower MSIL call */
int lower_call(lowering_context_t *ctx, msil_instruction_t *instr,
               fruity_basic_block_t *block)
{
	/* TODO: Parse method signature to determine argument/return types */
	/* For now, generate basic CALL */
	(void)ctx;  /* Unused for now */

	fruity_instruction_t *call = fruity_instruction_create(FRUITY_CALL);
	fruity_instruction_set_operand_i32(call, (int32_t)instr->operand.token);
	fruity_basic_block_add_instruction(block, call);

	/* TODO: Adjust stack based on signature */
	/* Placeholder: assume no return value */

	return 0;
}

/* Lower MSIL ret */
int lower_ret(lowering_context_t *ctx, fruity_basic_block_t *block)
{
	/* Check if method returns reference */
	int returns_ref = 0;  /* TODO: Parse method signature */

	if (returns_ref) {
		/* Returning reference - caller owns the white token */
		int is_ref = lowering_stack_pop(ctx);
		if (!is_ref) {
			fprintf(stderr, "Type mismatch: expected reference on stack\n");
		}
	}

	fruity_instruction_t *ret = fruity_instruction_create(FRUITY_RET);
	fruity_basic_block_add_instruction(block, ret);

	return 0;
}

/* Lower arithmetic/comparison (no pebble operations) */
int lower_arithmetic(lowering_context_t *ctx, msil_opcode_t msil_op,
                     fruity_basic_block_t *block)
{
	fruity_opcode_t fruity_op;

	/* Map MSIL arithmetic to Fruity arithmetic */
	switch (msil_op) {
	case MSIL_ADD:     fruity_op = FRUITY_ADD; break;
	case MSIL_SUB:     fruity_op = FRUITY_SUB; break;
	case MSIL_MUL:     fruity_op = FRUITY_MUL; break;
	case MSIL_DIV:     fruity_op = FRUITY_DIV; break;
	case MSIL_REM:     fruity_op = FRUITY_REM; break;
	case MSIL_AND:     fruity_op = FRUITY_AND; break;
	case MSIL_OR:      fruity_op = FRUITY_OR; break;
	case MSIL_XOR:     fruity_op = FRUITY_XOR; break;
	case MSIL_SHL:     fruity_op = FRUITY_SHL; break;
	case MSIL_SHR:     fruity_op = FRUITY_SHR; break;
	case MSIL_NEG:     fruity_op = FRUITY_NEG; break;
	case MSIL_NOT:     fruity_op = FRUITY_NOT; break;
	case MSIL_CEQ:     fruity_op = FRUITY_CEQ; break;
	case MSIL_CGT:     fruity_op = FRUITY_CGT; break;
	case MSIL_CLT:     fruity_op = FRUITY_CLT; break;
	default:
		fprintf(stderr, "Unknown arithmetic opcode: %d\n", msil_op);
		return -1;
	}

	fruity_instruction_t *arith = fruity_instruction_create(fruity_op);
	fruity_basic_block_add_instruction(block, arith);

	/* Adjust stack simulation */
	const msil_opcode_info_t *info = msil_get_opcode_info(msil_op);
	if (info) {
		for (int i = 0; i < info->stack_pop; i++)
			lowering_stack_pop(ctx);
		for (int i = 0; i < info->stack_push; i++)
			lowering_stack_push(ctx, 0);  /* Value types */
	}

	return 0;
}

/* Lower branch instructions */
int lower_branch(lowering_context_t *ctx, msil_instruction_t *instr,
                 fruity_basic_block_t *block)
{
	fruity_opcode_t fruity_op;

	/* Map MSIL branches to Fruity branches */
	switch (instr->opcode) {
	case MSIL_BR:
	case MSIL_BR_S:        fruity_op = FRUITY_JUMP; break;
	case MSIL_BRFALSE:
	case MSIL_BRFALSE_S:   fruity_op = FRUITY_BFALSE; break;
	case MSIL_BRTRUE:
	case MSIL_BRTRUE_S:    fruity_op = FRUITY_BTRUE; break;
	case MSIL_BEQ:
	case MSIL_BEQ_S:       fruity_op = FRUITY_BEQ; break;
	case MSIL_BNE_UN:
	case MSIL_BNE_UN_S:    fruity_op = FRUITY_BNE; break;
	case MSIL_BGE:
	case MSIL_BGE_S:       fruity_op = FRUITY_BGE; break;
	case MSIL_BGT:
	case MSIL_BGT_S:       fruity_op = FRUITY_BGT; break;
	case MSIL_BLE:
	case MSIL_BLE_S:       fruity_op = FRUITY_BLE; break;
	case MSIL_BLT:
	case MSIL_BLT_S:       fruity_op = FRUITY_BLT; break;
	default:
		fprintf(stderr, "Unknown branch opcode: 0x%02X\n", instr->opcode);
		return -1;
	}

	fruity_instruction_t *branch = fruity_instruction_create(fruity_op);
	fruity_instruction_set_operand_i32(branch, instr->operand.branch);
	fruity_basic_block_add_instruction(block, branch);

	/* Adjust stack for conditional branches */
	const msil_opcode_info_t *info = msil_get_opcode_info(instr->opcode);
	if (info) {
		for (int i = 0; i < info->stack_pop; i++)
			lowering_stack_pop(ctx);
	}

	return 0;
}

/* Lower single MSIL instruction to Fruity IR */
int lower_instruction(lowering_context_t *ctx,
                      msil_instruction_t *msil_instr,
                      fruity_basic_block_t *block)
{
	switch (msil_instr->opcode) {
	/* Object creation */
	case MSIL_NEWOBJ:
		return lower_newobj(ctx, msil_instr, block);

	/* Stack operations */
	case MSIL_DUP:
		return lower_dup(ctx, block);

	case MSIL_POP:
		return lower_pop(ctx, block);

	/* Local variables */
	case MSIL_LDLOC_0:
		return lower_ldloc(ctx, 0, block);
	case MSIL_LDLOC_1:
		return lower_ldloc(ctx, 1, block);
	case MSIL_LDLOC_2:
		return lower_ldloc(ctx, 2, block);
	case MSIL_LDLOC_3:
		return lower_ldloc(ctx, 3, block);
	case MSIL_LDLOC_S:
	case MSIL_LDLOC:
		return lower_ldloc(ctx, msil_instr->operand.index, block);

	case MSIL_STLOC_0:
		return lower_stloc(ctx, 0, block);
	case MSIL_STLOC_1:
		return lower_stloc(ctx, 1, block);
	case MSIL_STLOC_2:
		return lower_stloc(ctx, 2, block);
	case MSIL_STLOC_3:
		return lower_stloc(ctx, 3, block);
	case MSIL_STLOC_S:
	case MSIL_STLOC:
		return lower_stloc(ctx, msil_instr->operand.index, block);

	/* Control flow */
	case MSIL_CALL:
		return lower_call(ctx, msil_instr, block);

	case MSIL_RET:
		return lower_ret(ctx, block);

	/* Branches */
	case MSIL_BR:
	case MSIL_BR_S:
	case MSIL_BRFALSE:
	case MSIL_BRFALSE_S:
	case MSIL_BRTRUE:
	case MSIL_BRTRUE_S:
	case MSIL_BEQ:
	case MSIL_BEQ_S:
	case MSIL_BNE_UN:
	case MSIL_BNE_UN_S:
	case MSIL_BGE:
	case MSIL_BGE_S:
	case MSIL_BGT:
	case MSIL_BGT_S:
	case MSIL_BLE:
	case MSIL_BLE_S:
	case MSIL_BLT:
	case MSIL_BLT_S:
		return lower_branch(ctx, msil_instr, block);

	/* Arithmetic */
	case MSIL_ADD:
	case MSIL_SUB:
	case MSIL_MUL:
	case MSIL_DIV:
	case MSIL_REM:
	case MSIL_AND:
	case MSIL_OR:
	case MSIL_XOR:
	case MSIL_SHL:
	case MSIL_SHR:
	case MSIL_NEG:
	case MSIL_NOT:
	case MSIL_CEQ:
	case MSIL_CGT:
	case MSIL_CLT:
		return lower_arithmetic(ctx, msil_instr->opcode, block);

	/* Constants */
	case MSIL_LDNULL:
	case MSIL_LDC_I4_M1:
	case MSIL_LDC_I4_0:
	case MSIL_LDC_I4_1:
	case MSIL_LDC_I4_2:
	case MSIL_LDC_I4_3:
	case MSIL_LDC_I4_4:
	case MSIL_LDC_I4_5:
	case MSIL_LDC_I4_6:
	case MSIL_LDC_I4_7:
	case MSIL_LDC_I4_8:
	case MSIL_LDC_I4_S:
	case MSIL_LDC_I4:
	case MSIL_LDC_I8:
	case MSIL_LDC_R4:
	case MSIL_LDC_R8:
	{
		/* Direct mapping to Fruity constants */
		fruity_instruction_t *ldc = fruity_instruction_create(FRUITY_LDC_I4);

		if (msil_instr->opcode >= MSIL_LDC_I4_M1 && msil_instr->opcode <= MSIL_LDC_I4_8) {
			/* Inline constants */
			int val = msil_instr->opcode - MSIL_LDC_I4_0;
			if (msil_instr->opcode == MSIL_LDC_I4_M1) val = -1;
			fruity_instruction_set_operand_i32(ldc, val);
		} else {
			fruity_instruction_set_operand_i32(ldc, msil_instr->operand.i32);
		}

		fruity_basic_block_add_instruction(block, ldc);
		lowering_stack_push(ctx, 0);  /* Value type */
		return 0;
	}

	case MSIL_NOP:
		/* No-op - skip */
		return 0;

	default:
		fprintf(stderr, "Unimplemented MSIL opcode: %s (0x%02X)\n",
		        msil_opcode_name(msil_instr->opcode), msil_instr->opcode);
		return -1;
	}
}

/* Main lowering entry point */
fruity_function_t* lower_msil_to_fruity(msil_method_t *msil_method)
{
	if (!msil_method)
		return NULL;

	/* Create lowering context */
	lowering_context_t *ctx = lowering_context_create(msil_method);
	if (!ctx)
		return NULL;

	/* Create single basic block (TODO: proper CFG construction) */
	fruity_basic_block_t *block = fruity_function_add_block(ctx->fruity_func);
	if (!block) {
		lowering_context_destroy(ctx);
		return NULL;
	}

	/* Lower each MSIL instruction */
	msil_instruction_t *msil_instr = msil_method->instructions_head;
	while (msil_instr) {
		if (lower_instruction(ctx, msil_instr, block) < 0) {
			fprintf(stderr, "Failed to lower instruction at IL_%04X\n",
			        msil_instr->offset);
		}
		msil_instr = msil_instr->next;
	}

	/* Print statistics */
	lowering_context_print_stats(ctx);

	fruity_function_t *result = ctx->fruity_func;
	ctx->fruity_func = NULL;  /* Prevent destruction */
	lowering_context_destroy(ctx);

	return result;
}

/* Attribute detection (placeholder - would parse custom attributes) */
int detect_transactional_attribute(msil_method_t *method)
{
	/* TODO: Parse custom attribute table for [Transactional] */
	(void)method;
	return 0;
}

int detect_exchange_attribute(msil_method_t *method)
{
	/* TODO: Parse custom attribute table for [Exchange] */
	(void)method;
	return 0;
}

/* Print lowering statistics */
void lowering_context_print_stats(lowering_context_t *ctx)
{
	if (!ctx)
		return;

	printf("\n=== Fruity Lowering Statistics ===\n");
	printf("Method: %s\n", ctx->msil_method->name);
	printf("LIME opcodes (allocations):     %u\n", ctx->lime_count);
	printf("VANILLA opcodes (white tokens): %u\n", ctx->vanilla_count);
	printf("BURN opcodes (releases):        %u\n", ctx->burn_count);
	printf("CHERRY opcodes (snapshots):     %u\n", ctx->cherry_count);
	printf("GRAPE opcodes (transfers):      %u\n", ctx->grape_count);
	printf("Transactional: %s\n", ctx->is_transactional ? "yes" : "no");
	printf("Exchange:      %s\n", ctx->is_exchange ? "yes" : "no");
	printf("==================================\n\n");
}
