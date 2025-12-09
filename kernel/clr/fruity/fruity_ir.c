/* fruity_ir.c - Fruity IR Implementation
 *
 * Core IR manipulation functions using intrusive linked lists.
 * All allocations use xalloc() from the kernel.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "fruity_ir.h"

/* Opcode metadata table */
const fruity_opcode_metadata_t fruity_opcode_table[] = {
	/* Standard operations */
	{FRUITY_NOP, "nop", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{FRUITY_ADD, "add", 0, 0, 0, 0, 2, 1, 0, 0, 0, 0},
	{FRUITY_SUB, "sub", 0, 0, 0, 0, 2, 1, 0, 0, 0, 0},
	{FRUITY_MUL, "mul", 0, 0, 0, 0, 2, 1, 0, 0, 0, 0},

	/* Pebble operations */
	{FRUITY_LIME, "lime", 1, 0, 0, 0, 1, 1, 0, 0, 0, 0},
	{FRUITY_VANILLA, "vanilla", 1, 0, 0, 0, 1, 2, 0, 0, 0, 0},
	{FRUITY_BURN, "burn", 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
	{FRUITY_DUP, "dup", 0, 0, 0, 0, 1, 2, 0, 0, 0, 0},
	{FRUITY_POP, "pop", 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},

	/* Transactional */
	{FRUITY_CHERRY, "cherry", 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
	{FRUITY_BERRY, "berry", 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
	{FRUITY_ROLLBACK, "rollback", 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},

	/* IPC */
	{FRUITY_GRAPE, "grape", 0, 1, 0, 0, 2, 0, 0, 0, 0, 0},
	{FRUITY_LEMON, "lemon", 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},

	/* Control flow */
	{FRUITY_CALL, "call", 0, 0, 0, 0, -1, -1, 0, 1, 0, 0},
	{FRUITY_RET, "ret", 0, 0, 0, 0, -1, 0, 0, 0, 1, 1},
	{FRUITY_JUMP, "jump", 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
	{FRUITY_BEQ, "beq", 0, 0, 0, 0, 2, 0, 1, 0, 0, 1},
	{FRUITY_BNE, "bne", 0, 0, 0, 0, 2, 0, 1, 0, 0, 1},

	/* Stack/locals */
	{FRUITY_LOAD_LOCAL, "ldloc", 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
	{FRUITY_STORE_LOCAL, "stloc", 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},

	/* Terminator */
	{0, nil, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/* ===== Opcode Queries ===== */

const char*
fruity_opcode_name(fruity_opcode_t opcode)
{
	for(int i = 0; fruity_opcode_table[i].name != nil; i++) {
		if(fruity_opcode_table[i].opcode == opcode)
			return fruity_opcode_table[i].name;
	}
	return "unknown";
}

int
fruity_opcode_creates_white(fruity_opcode_t opcode)
{
	for(int i = 0; fruity_opcode_table[i].name != nil; i++) {
		if(fruity_opcode_table[i].opcode == opcode)
			return fruity_opcode_table[i].creates_white;
	}
	return 0;
}

int
fruity_opcode_burns_white(fruity_opcode_t opcode)
{
	for(int i = 0; fruity_opcode_table[i].name != nil; i++) {
		if(fruity_opcode_table[i].opcode == opcode)
			return fruity_opcode_table[i].burns_white;
	}
	return 0;
}

int
fruity_opcode_may_free(fruity_opcode_t opcode)
{
	for(int i = 0; fruity_opcode_table[i].name != nil; i++) {
		if(fruity_opcode_table[i].opcode == opcode)
			return fruity_opcode_table[i].may_free;
	}
	return 0;
}

int
fruity_opcode_is_terminator(fruity_opcode_t opcode)
{
	for(int i = 0; fruity_opcode_table[i].name != nil; i++) {
		if(fruity_opcode_table[i].opcode == opcode)
			return fruity_opcode_table[i].is_terminator;
	}
	return 0;
}

/* ===== Module Management ===== */

fruity_module_t*
fruity_module_create(const char *name)
{
	fruity_module_t *module = xallocz(sizeof(fruity_module_t), 1);
	if(module == nil)
		return nil;

	module->name = xalloc(strlen(name) + 1);
	if(module->name == nil) {
		xfree(module);
		return nil;
	}
	strcpy(module->name, name);

	module->version = 1;
	module->functions_head = nil;
	module->functions_tail = nil;
	module->function_count = 0;

	return module;
}

void
fruity_module_destroy(fruity_module_t *module)
{
	if(module == nil)
		return;

	/* Free all functions */
	fruity_function_t *func = module->functions_head;
	while(func != nil) {
		fruity_function_t *next = func->next;
		fruity_function_destroy(func);
		func = next;
	}

	/* Free constants */
	if(module->constants.strings != nil) {
		for(ulong i = 0; i < module->constants.string_count; i++)
			xfree(module->constants.strings[i]);
		xfree(module->constants.strings);
	}
	if(module->constants.blob_data != nil)
		xfree(module->constants.blob_data);

	xfree(module->name);
	xfree(module);
}

fruity_function_t*
fruity_module_add_function(fruity_module_t *module, const char *name,
                           u32int method_token)
{
	if(module == nil)
		return nil;

	fruity_function_t *func = fruity_function_create(name, method_token);
	if(func == nil)
		return nil;

	/* Add to tail of intrusive list */
	func->next = nil;
	func->prev = module->functions_tail;

	if(module->functions_tail != nil)
		module->functions_tail->next = func;
	else
		module->functions_head = func;

	module->functions_tail = func;
	module->function_count++;

	return func;
}

fruity_function_t*
fruity_module_find_function(fruity_module_t *module, const char *name)
{
	if(module == nil || name == nil)
		return nil;

	for(fruity_function_t *func = module->functions_head;
	    func != nil;
	    func = func->next) {
		if(strcmp(func->name, name) == 0)
			return func;
	}

	return nil;
}

/* ===== Function Management ===== */

fruity_function_t*
fruity_function_create(const char *name, u32int method_token)
{
	fruity_function_t *func = xallocz(sizeof(fruity_function_t), 1);
	if(func == nil)
		return nil;

	func->name = xalloc(strlen(name) + 1);
	if(func->name == nil) {
		xfree(func);
		return nil;
	}
	strcpy(func->name, name);

	func->method_token = method_token;
	func->blocks_head = nil;
	func->blocks_tail = nil;
	func->block_count = 0;
	func->entry_block = nil;
	func->exit_block = nil;

	return func;
}

void
fruity_function_destroy(fruity_function_t *func)
{
	if(func == nil)
		return;

	/* Free all blocks */
	fruity_basic_block_t *block = func->blocks_head;
	while(block != nil) {
		fruity_basic_block_t *next = block->next;
		fruity_basic_block_destroy(block);
		block = next;
	}

	/* Free locals and args */
	if(func->local_types != nil)
		xfree(func->local_types);
	if(func->arg_types != nil)
		xfree(func->arg_types);

	xfree(func->name);
	if(func->signature != nil)
		xfree(func->signature);
	xfree(func);
}

fruity_basic_block_t*
fruity_function_add_block(fruity_function_t *func)
{
	if(func == nil)
		return nil;

	u32int block_id = func->block_count;
	fruity_basic_block_t *block = fruity_basic_block_create(block_id);
	if(block == nil)
		return nil;

	/* Add to tail of intrusive list */
	block->next = nil;
	block->prev = func->blocks_tail;

	if(func->blocks_tail != nil)
		func->blocks_tail->next = block;
	else
		func->blocks_head = block;

	func->blocks_tail = block;
	func->block_count++;

	/* First block is entry */
	if(func->entry_block == nil)
		func->entry_block = block;

	return block;
}

fruity_basic_block_t*
fruity_function_find_block(fruity_function_t *func, u32int block_id)
{
	if(func == nil)
		return nil;

	for(fruity_basic_block_t *block = func->blocks_head;
	    block != nil;
	    block = block->next) {
		if(block->block_id == block_id)
			return block;
	}

	return nil;
}

/* ===== Basic Block Management ===== */

fruity_basic_block_t*
fruity_basic_block_create(u32int block_id)
{
	fruity_basic_block_t *block = xallocz(sizeof(fruity_basic_block_t), 1);
	if(block == nil)
		return nil;

	block->block_id = block_id;
	block->instructions_head = nil;
	block->instructions_tail = nil;
	block->instruction_count = 0;

	block->successors = nil;
	block->predecessors = nil;
	block->successor_count = 0;
	block->predecessor_count = 0;
	block->successor_capacity = 0;
	block->predecessor_capacity = 0;

	return block;
}

void
fruity_basic_block_destroy(fruity_basic_block_t *block)
{
	if(block == nil)
		return;

	/* Free all instructions */
	fruity_instruction_t *instr = block->instructions_head;
	while(instr != nil) {
		fruity_instruction_t *next = instr->next;
		fruity_instruction_destroy(instr);
		instr = next;
	}

	/* Free CFG arrays */
	if(block->successors != nil)
		xfree(block->successors);
	if(block->predecessors != nil)
		xfree(block->predecessors);
	if(block->dominance_frontier != nil)
		xfree(block->dominance_frontier);

	xfree(block);
}

void
fruity_basic_block_add_instruction(fruity_basic_block_t *block,
                                   fruity_instruction_t *instr)
{
	if(block == nil || instr == nil)
		return;

	/* Add to tail of intrusive list */
	instr->next = nil;
	instr->prev = block->instructions_tail;

	if(block->instructions_tail != nil)
		block->instructions_tail->next = instr;
	else
		block->instructions_head = instr;

	block->instructions_tail = instr;
	block->instruction_count++;
}

void
fruity_basic_block_add_successor(fruity_basic_block_t *block,
                                 fruity_basic_block_t *successor)
{
	if(block == nil || successor == nil)
		return;

	/* Grow array if needed */
	if(block->successor_count >= block->successor_capacity) {
		ulong new_cap = block->successor_capacity == 0 ? 2 : block->successor_capacity * 2;
		fruity_basic_block_t **new_arr = xalloc(new_cap * sizeof(fruity_basic_block_t*));
		if(block->successors != nil) {
			memmove(new_arr, block->successors,
			        block->successor_count * sizeof(fruity_basic_block_t*));
			xfree(block->successors);
		}
		block->successors = new_arr;
		block->successor_capacity = new_cap;
	}

	block->successors[block->successor_count++] = successor;
}

void
fruity_basic_block_add_predecessor(fruity_basic_block_t *block,
                                   fruity_basic_block_t *predecessor)
{
	if(block == nil || predecessor == nil)
		return;

	/* Grow array if needed */
	if(block->predecessor_count >= block->predecessor_capacity) {
		ulong new_cap = block->predecessor_capacity == 0 ? 2 : block->predecessor_capacity * 2;
		fruity_basic_block_t **new_arr = xalloc(new_cap * sizeof(fruity_basic_block_t*));
		if(block->predecessors != nil) {
			memmove(new_arr, block->predecessors,
			        block->predecessor_count * sizeof(fruity_basic_block_t*));
			xfree(block->predecessors);
		}
		block->predecessors = new_arr;
		block->predecessor_capacity = new_cap;
	}

	block->predecessors[block->predecessor_count++] = predecessor;
}

/* ===== Instruction Construction ===== */

fruity_instruction_t*
fruity_instruction_create(fruity_opcode_t opcode)
{
	fruity_instruction_t *instr = xallocz(sizeof(fruity_instruction_t), 1);
	if(instr == nil)
		return nil;

	instr->opcode = opcode;
	instr->operand.type = FRUITY_OP_NONE;

	/* Set pebble effects from opcode table */
	instr->pebble_effects.creates_white = fruity_opcode_creates_white(opcode);
	instr->pebble_effects.burns_white = fruity_opcode_burns_white(opcode);
	instr->pebble_effects.may_free = fruity_opcode_may_free(opcode);
	instr->pebble_effects.is_speculative = (opcode == FRUITY_CHERRY);

	return instr;
}

void
fruity_instruction_destroy(fruity_instruction_t *instr)
{
	if(instr == nil)
		return;
	xfree(instr);
}

void
fruity_instruction_set_operand_i32(fruity_instruction_t *instr, s32int val)
{
	if(instr == nil)
		return;
	instr->operand.type = FRUITY_OP_IMM_I32;
	instr->operand.value.i32 = val;
}

void
fruity_instruction_set_operand_i64(fruity_instruction_t *instr, s64int val)
{
	if(instr == nil)
		return;
	instr->operand.type = FRUITY_OP_IMM_I64;
	instr->operand.value.i64 = val;
}

void
fruity_instruction_set_operand_local(fruity_instruction_t *instr, u32int idx)
{
	if(instr == nil)
		return;
	instr->operand.type = FRUITY_OP_LOCAL;
	instr->operand.value.index = idx;
}

void
fruity_instruction_set_operand_branch(fruity_instruction_t *instr,
                                      fruity_basic_block_t *target)
{
	if(instr == nil)
		return;
	instr->operand.type = FRUITY_OP_BRANCH;
	instr->operand.value.target = target;
}

void
fruity_instruction_set_operand_tasklet(fruity_instruction_t *instr,
                                       tasklet_id_t tasklet)
{
	if(instr == nil)
		return;
	instr->operand.type = FRUITY_OP_TASKLET;
	instr->operand.value.tasklet = tasklet;
}

/* ===== Verification Stubs (to be implemented) ===== */

int
fruity_module_verify(fruity_module_t *module)
{
	/* TODO: Implement module verification */
	return 0;
}

int
fruity_function_verify(fruity_function_t *func)
{
	/* TODO: Implement function verification */
	return 0;
}

int
fruity_basic_block_verify(fruity_basic_block_t *block)
{
	/* TODO: Implement block verification */
	return 0;
}

/* ===== Traversal ===== */

void
fruity_module_foreach_function(fruity_module_t *module,
                                fruity_function_visitor_t visitor,
                                void *ctx)
{
	if(module == nil || visitor == nil)
		return;

	for(fruity_function_t *func = module->functions_head;
	    func != nil;
	    func = func->next) {
		if(visitor(func, ctx) != 0)
			break;
	}
}

void
fruity_function_foreach_block(fruity_function_t *func,
                               fruity_basic_block_visitor_t visitor,
                               void *ctx)
{
	if(func == nil || visitor == nil)
		return;

	for(fruity_basic_block_t *block = func->blocks_head;
	    block != nil;
	    block = block->next) {
		if(visitor(block, ctx) != 0)
			break;
	}
}

void
fruity_basic_block_foreach_instruction(fruity_basic_block_t *block,
                                       fruity_instruction_visitor_t visitor,
                                       void *ctx)
{
	if(block == nil || visitor == nil)
		return;

	for(fruity_instruction_t *instr = block->instructions_head;
	    instr != nil;
	    instr = instr->next) {
		if(visitor(instr, ctx) != 0)
			break;
	}
}

/* ===== Debugging ===== */

void
fruity_instruction_print(fruity_instruction_t *instr)
{
	if(instr == nil)
		return;

	print("  %s", fruity_opcode_name(instr->opcode));

	switch(instr->operand.type) {
	case FRUITY_OP_IMM_I32:
		print(" %d", instr->operand.value.i32);
		break;
	case FRUITY_OP_IMM_I64:
		print(" %lld", instr->operand.value.i64);
		break;
	case FRUITY_OP_LOCAL:
		print(" local.%ud", instr->operand.value.index);
		break;
	case FRUITY_OP_BRANCH:
		print(" bb.%ud", instr->operand.value.target->block_id);
		break;
	default:
		break;
	}

	print("\n");
}

void
fruity_basic_block_print(fruity_basic_block_t *block)
{
	if(block == nil)
		return;

	print("bb.%ud:\n", block->block_id);

	for(fruity_instruction_t *instr = block->instructions_head;
	    instr != nil;
	    instr = instr->next) {
		fruity_instruction_print(instr);
	}
}

void
fruity_function_print(fruity_function_t *func)
{
	if(func == nil)
		return;

	print("function %s:\n", func->name);

	for(fruity_basic_block_t *block = func->blocks_head;
	    block != nil;
	    block = block->next) {
		fruity_basic_block_print(block);
	}
}

void
fruity_module_print(fruity_module_t *module)
{
	if(module == nil)
		return;

	print("module %s (version %ud):\n", module->name, module->version);
	print("  functions: %lud\n", module->function_count);

	for(fruity_function_t *func = module->functions_head;
	    func != nil;
	    func = func->next) {
		print("\n");
		fruity_function_print(func);
	}
}
