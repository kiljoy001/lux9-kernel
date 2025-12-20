/* test_clr_compile.c - Test sys_clr_compile syscall
 * 
 * Creates a minimal Fruity IR module (function that returns 42)
 * and tests the kernel compilation pipeline.
 */

#include "u.h"
#include "libc.h"
#include "portlib.h"

/* Syscall number */
#define CLR_COMPILE 65

/* Minimal Fruity IR structures for testing */
typedef enum {
	FRUITY_LDC_I4 = 0x20,  /* Load constant int32 */
	FRUITY_RET = 0x2A,      /* Return */
} fruity_opcode_t;

typedef struct {
	uint opcode;
	int immediate;  /* For LDC_I4 */
} fruity_instruction_t;

typedef struct {
	fruity_instruction_t *instructions;
	ulong instruction_count;
} fruity_basic_block_t;

typedef struct {
	char *name;
	fruity_basic_block_t *blocks;
	ulong block_count;
	ulong arg_count;
	uint return_type;
} fruity_function_t;

typedef struct {
	char *name;
	fruity_function_t *functions;
	ulong function_count;
} fruity_module_t;

/* Create minimal module: int return42() { return 42; } */
fruity_module_t*
create_test_module(void)
{
	fruity_module_t *mod;
	fruity_function_t *func;
	fruity_basic_block_t *block;
	fruity_instruction_t *instrs;

	/* Allocate module */
	mod = mallocz(sizeof(fruity_module_t), 1);
	if(mod == nil)
		return nil;

	mod->name = strdup("test_module");
	mod->function_count = 1;
	mod->functions = mallocz(sizeof(fruity_function_t), 1);
	if(mod->functions == nil){
		free(mod);
		return nil;
	}

	/* Create function */
	func = &mod->functions[0];
	func->name = strdup("return42");
	func->arg_count = 0;
	func->return_type = 0x08; /* I4 - int32 */
	func->block_count = 1;
	func->blocks = mallocz(sizeof(fruity_basic_block_t), 1);
	if(func->blocks == nil){
		free(mod->functions);
		free(mod);
		return nil;
	}

	/* Create basic block */
	block = &func->blocks[0];
	block->instruction_count = 2;
	block->instructions = mallocz(sizeof(fruity_instruction_t) * 2, 1);
	if(block->instructions == nil){
		free(func->blocks);
		free(mod->functions);
		free(mod);
		return nil;
	}

	/* Instruction 0: LDC.I4 42 */
	instrs = block->instructions;
	instrs[0].opcode = FRUITY_LDC_I4;
	instrs[0].immediate = 42;

	/* Instruction 1: RET */
	instrs[1].opcode = FRUITY_RET;
	instrs[1].immediate = 0;

	return mod;
}

void
destroy_test_module(fruity_module_t *mod)
{
	if(mod == nil)
		return;

	if(mod->functions != nil){
		if(mod->functions[0].blocks != nil){
			if(mod->functions[0].blocks[0].instructions != nil)
				free(mod->functions[0].blocks[0].instructions);
			free(mod->functions[0].blocks);
		}
		if(mod->functions[0].name != nil)
			free(mod->functions[0].name);
		free(mod->functions);
	}
	if(mod->name != nil)
		free(mod->name);
	free(mod);
}

void
main(void)
{
	fruity_module_t *mod;
	int fd;
	char errorbuf[256];
	int ret;

	print("Creating minimal Fruity IR module...\n");

	mod = create_test_module();
	if(mod == nil){
		print("FAIL: Could not create test module\n");
		exits("create");
	}

	print("Module: %s\n", mod->name);
	print("  Function: %s (args=%ld, ret_type=0x%x)\n",
	      mod->functions[0].name,
	      mod->functions[0].arg_count,
	      mod->functions[0].return_type);
	print("  Instructions: %ld\n", mod->functions[0].blocks[0].instruction_count);

	/* For now, just verify we can create the module */
	/* TODO: Open /dev/clr and call sys_clr_compile when devclr is ready */

	print("SUCCESS: Test module created\n");
	print("TODO: Implement /dev/clr write and sys_clr_compile call\n");

	destroy_test_module(mod);
	exits(nil);
}
