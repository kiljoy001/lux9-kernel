/* fruity_lowering.h - MSIL to Fruity IR Lowering
 *
 * Transforms MSIL bytecode into Fruity IR with explicit Pebble operations.
 *
 * Key transformations:
 *   newobj    → FRUITY_LIME  (Allocate Black + White)
 *   dup (ref) → FRUITY_VANILLA (Issue White Token)
 *   pop (ref) → FRUITY_BURN (Release White Token)
 *   stloc     → FRUITY_BURN old + FRUITY_VANILLA new (for refs)
 *   ldloc     → FRUITY_VANILLA (Issue White for stack)
 */

#ifndef FRUITY_LOWERING_H
#define FRUITY_LOWERING_H

#include "msil_reader.h"
#include "fruity_userspace.h"

/* Lowering context - tracks state during transformation */
typedef struct {
	msil_method_t *msil_method;
	fruity_function_t *fruity_func;

	/* Type information for locals (to track references) */
	int *local_is_ref;     /* 1 if local is reference type */
	int *arg_is_ref;       /* 1 if arg is reference type */

	/* Stack simulation for reference tracking */
	int *stack_is_ref;     /* 1 if stack slot holds reference */
	int stack_depth;
	int max_stack;

	/* Attributes detected */
	int is_transactional;  /* Method has [Transactional] */
	int is_exchange;       /* Method has [Exchange] */

	/* Statistics */
	uint32_t lime_count;     /* LIME opcodes generated */
	uint32_t vanilla_count;  /* VANILLA opcodes generated */
	uint32_t burn_count;     /* BURN opcodes generated */
	uint32_t cherry_count;   /* CHERRY opcodes generated */
	uint32_t grape_count;    /* GRAPE opcodes generated */

} lowering_context_t;

/* Main lowering function */
fruity_function_t* lower_msil_to_fruity(msil_method_t *msil_method);

/* Lowering context management */
lowering_context_t* lowering_context_create(msil_method_t *msil_method);
void lowering_context_destroy(lowering_context_t *ctx);

/* Stack simulation (for reference tracking) */
void lowering_stack_push(lowering_context_t *ctx, int is_ref);
int lowering_stack_pop(lowering_context_t *ctx);  /* Returns: is_ref */
int lowering_stack_peek(lowering_context_t *ctx); /* Returns: is_ref */

/* Individual instruction lowering */
int lower_instruction(lowering_context_t *ctx,
                      msil_instruction_t *msil_instr,
                      fruity_basic_block_t *block);

/* Specific lowering rules */
int lower_newobj(lowering_context_t *ctx, msil_instruction_t *instr,
                 fruity_basic_block_t *block);
int lower_dup(lowering_context_t *ctx, fruity_basic_block_t *block);
int lower_pop(lowering_context_t *ctx, fruity_basic_block_t *block);
int lower_ldloc(lowering_context_t *ctx, uint32_t index,
                fruity_basic_block_t *block);
int lower_stloc(lowering_context_t *ctx, uint32_t index,
                fruity_basic_block_t *block);
int lower_call(lowering_context_t *ctx, msil_instruction_t *instr,
               fruity_basic_block_t *block);
int lower_ret(lowering_context_t *ctx, fruity_basic_block_t *block);

/* Arithmetic/comparison (no pebble operations) */
int lower_arithmetic(lowering_context_t *ctx, msil_opcode_t msil_op,
                     fruity_basic_block_t *block);

/* Branch lowering */
int lower_branch(lowering_context_t *ctx, msil_instruction_t *instr,
                 fruity_basic_block_t *block);

/* Attribute detection */
int detect_transactional_attribute(msil_method_t *method);
int detect_exchange_attribute(msil_method_t *method);

/* Debug */
void lowering_context_print_stats(lowering_context_t *ctx);

#endif /* FRUITY_LOWERING_H */
