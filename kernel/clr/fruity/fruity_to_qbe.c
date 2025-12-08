/* fruity_to_qbe.c - Fruity IR to QBE IL Translator
 *
 * Converts in-memory Fruity IR to textual QBE IL format.
 * Zero-copy I/O via exchange_fprintf() - just pointer arithmetic.
 *
 * Translation Strategy:
 *   - Fruity IR: In-memory linked lists (fruity_module_t, fruity_function_t, etc.)
 *   - QBE IL: Textual SSA format written directly to exchange page
 *   - Performance: exchange_fprintf() is just movb - 5-10 cycles per char
 */

#include "fruity_to_qbe.h"
#include "../qbe/exchange_io.h"

/* Declare only the kernel functions we need - avoid pulling in full headers */
extern int vsnprint(char *buf, int len, char *fmt, va_list args);
extern int snprint(char *buf, int len, char *fmt, ...);

/* Global state for error handling */
static char *error_buf = NULL;
static size_t error_buf_size = 0;

/* Helper: Set error message */
static void
set_error(const char *fmt, ...)
{
	va_list ap;

	if (error_buf && error_buf_size > 0) {
		va_start(ap, fmt);
		vsnprint(error_buf, error_buf_size, (char*)fmt, ap);
		va_end(ap);
	}
}

/* Map CLR type to QBE type
 * Returns: QBE type character ('w', 'l', 's', 'd', or 0 for error)
 */
static char
qbe_type(clr_value_type_t clr_type)
{
	switch (clr_type) {
	case CLR_INT32:
	case CLR_BOOL:
		return 'w';  /* word (32-bit) */
	case CLR_INT64:
	case CLR_REF:
	case CLR_NULL:
		return 'l';  /* long (64-bit) */
	default:
		return 0;    /* Unknown type */
	}
}

/* Emit QBE IL header with runtime ABI declarations
 *
 * Runtime Functions:
 *   $lux_alloc(w size, w type) → l ptr
 *   $lux_token_mint(l ptr) → void
 *   $lux_token_burn(l ptr) → void
 *   $lux_snapshot(l ptr) → void
 *   $lux_commit(l ptr) → void
 *   $lux_rollback(l ptr) → void
 *   $lux_exchange_send(l token, w channel) → void
 */
static void
emit_qbe_header(ExchangeFILE *out)
{
	exchange_fprintf(out, "# QBE IL generated from Fruity IR\n");
	exchange_fprintf(out, "# AMD64 SysV ABI calling convention\n\n");

	/* Runtime ABI declarations */
	exchange_fprintf(out, "# Pebble Runtime ABI\n");
	exchange_fprintf(out, "# LIME: Allocate Black Pebble + first White Token\n");
	exchange_fprintf(out, "export function l $lux_alloc(w %%size, w %%type) { @start ret 0 }\n\n");

	exchange_fprintf(out, "# VANILLA: Issue White Token (addref)\n");
	exchange_fprintf(out, "export function $lux_token_mint(l %%ptr) { @start ret }\n\n");

	exchange_fprintf(out, "# BURN: Release White Token (may free)\n");
	exchange_fprintf(out, "export function $lux_token_burn(l %%ptr) { @start ret }\n\n");

	exchange_fprintf(out, "# CHERRY: Create Red snapshot\n");
	exchange_fprintf(out, "export function $lux_snapshot(l %%ptr) { @start ret }\n\n");

	exchange_fprintf(out, "# BERRY: Commit transaction\n");
	exchange_fprintf(out, "export function $lux_commit(l %%ptr) { @start ret }\n\n");

	exchange_fprintf(out, "# ROLLBACK: Restore from Red\n");
	exchange_fprintf(out, "export function $lux_rollback(l %%ptr) { @start ret }\n\n");

	exchange_fprintf(out, "# GRAPE: Zero-copy IPC transfer\n");
	exchange_fprintf(out, "export function $lux_exchange_send(l %%token, w %%channel) { @start ret }\n\n");
}

/* Emit a single Fruity instruction as QBE IL
 *
 * Translation examples:
 *   FRUITY_LIME → call $lux_alloc(w %size, w %type)
 *   FRUITY_VANILLA → call $lux_token_mint(l %ptr)
 *   FRUITY_ADD → %result =w add %a, %b
 */
static int
emit_instruction(ExchangeFILE *out, fruity_instruction_t *instr, int *tmp_counter)
{
	int tmp_id = (*tmp_counter)++;

	switch (instr->opcode) {
	/* ===== Pebble Memory Operations ===== */
	case FRUITY_LIME:
		/* Allocate: %ptr =l call $lux_alloc(w %size, w %type) */
		if (instr->operand.type == FRUITY_OP_IMM_I32) {
			exchange_fprintf(out, "    %%t%d =l call $lux_alloc(w %d, w 0)\n",
			                tmp_id, instr->operand.value.i32);
		} else {
			exchange_fprintf(out, "    %%t%d =l call $lux_alloc(w %%size, w 0)\n",
			                tmp_id);
		}
		break;

	case FRUITY_VANILLA:
		/* Share reference: call $lux_token_mint(l %ptr) */
		exchange_fprintf(out, "    call $lux_token_mint(l %%t%d)\n", tmp_id - 1);
		break;

	case FRUITY_BURN:
		/* Release reference: call $lux_token_burn(l %ptr) */
		exchange_fprintf(out, "    call $lux_token_burn(l %%t%d)\n", tmp_id - 1);
		break;

	/* ===== Transactional Operations ===== */
	case FRUITY_CHERRY:
		exchange_fprintf(out, "    call $lux_snapshot(l %%t%d)\n", tmp_id - 1);
		break;

	case FRUITY_BERRY:
		exchange_fprintf(out, "    call $lux_commit(l %%t%d)\n", tmp_id - 1);
		break;

	case FRUITY_ROLLBACK:
		exchange_fprintf(out, "    call $lux_rollback(l %%t%d)\n", tmp_id - 1);
		break;

	/* ===== IPC Operations ===== */
	case FRUITY_GRAPE:
		/* Zero-copy transfer: call $lux_exchange_send(l %token, w %channel) */
		if (instr->operand.type == FRUITY_OP_TASKLET) {
			exchange_fprintf(out, "    call $lux_exchange_send(l %%t%d, w %u)\n",
			                tmp_id - 1, instr->operand.value.tasklet);
		} else {
			exchange_fprintf(out, "    call $lux_exchange_send(l %%t%d, w %%channel)\n",
			                tmp_id - 1);
		}
		break;

	/* ===== Arithmetic Operations ===== */
	case FRUITY_ADD:
		exchange_fprintf(out, "    %%t%d =w add %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_SUB:
		exchange_fprintf(out, "    %%t%d =w sub %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_MUL:
		exchange_fprintf(out, "    %%t%d =w mul %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_DIV:
		exchange_fprintf(out, "    %%t%d =w div %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_REM:
		exchange_fprintf(out, "    %%t%d =w rem %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_NEG:
		exchange_fprintf(out, "    %%t%d =w neg %%t%d\n",
		                tmp_id, tmp_id - 1);
		break;

	/* ===== Bitwise Operations ===== */
	case FRUITY_AND:
		exchange_fprintf(out, "    %%t%d =w and %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_OR:
		exchange_fprintf(out, "    %%t%d =w or %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_XOR:
		exchange_fprintf(out, "    %%t%d =w xor %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_SHL:
		exchange_fprintf(out, "    %%t%d =w shl %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_SHR:
		exchange_fprintf(out, "    %%t%d =w shr %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	/* ===== Constants ===== */
	case FRUITY_LDC_I4:
		exchange_fprintf(out, "    %%t%d =w copy %d\n",
		                tmp_id, instr->operand.value.i32);
		break;

	case FRUITY_LDC_I8:
		exchange_fprintf(out, "    %%t%d =l copy %lld\n",
		                tmp_id, instr->operand.value.i64);
		break;

	case FRUITY_LDNULL:
		exchange_fprintf(out, "    %%t%d =l copy 0\n", tmp_id);
		break;

	/* ===== Comparison Operations ===== */
	case FRUITY_CEQ:
		exchange_fprintf(out, "    %%t%d =w ceqw %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_CNE:
		exchange_fprintf(out, "    %%t%d =w cnew %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_CLT:
		exchange_fprintf(out, "    %%t%d =w csltw %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_CLE:
		exchange_fprintf(out, "    %%t%d =w cslew %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_CGT:
		exchange_fprintf(out, "    %%t%d =w csgtw %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	case FRUITY_CGE:
		exchange_fprintf(out, "    %%t%d =w csgew %%t%d, %%t%d\n",
		                tmp_id, tmp_id - 2, tmp_id - 1);
		break;

	/* ===== Stack Operations ===== */
	case FRUITY_DUP:
		/* Duplicate top of stack - just reference same temp */
		exchange_fprintf(out, "    %%t%d =w copy %%t%d\n",
		                tmp_id, tmp_id - 1);
		break;

	case FRUITY_POP:
		/* Pop - no-op in SSA form, just don't reference it */
		break;

	/* ===== Local Variables ===== */
	case FRUITY_LOAD_LOCAL:
		if (instr->operand.type == FRUITY_OP_LOCAL) {
			exchange_fprintf(out, "    %%t%d =w loadw %%local%u\n",
			                tmp_id, instr->operand.value.index);
		}
		break;

	case FRUITY_STORE_LOCAL:
		if (instr->operand.type == FRUITY_OP_LOCAL) {
			exchange_fprintf(out, "    storew %%t%d, %%local%u\n",
			                tmp_id - 1, instr->operand.value.index);
		}
		break;

	case FRUITY_LOAD_ARG:
		if (instr->operand.type == FRUITY_OP_ARG) {
			exchange_fprintf(out, "    %%t%d =w copy %%arg%u\n",
			                tmp_id, instr->operand.value.index);
		}
		break;

	/* ===== Control Flow ===== */
	case FRUITY_RET:
		exchange_fprintf(out, "    ret %%t%d\n", tmp_id - 1);
		break;

	case FRUITY_JUMP:
		if (instr->operand.type == FRUITY_OP_BRANCH) {
			exchange_fprintf(out, "    jmp @block%u\n",
			                instr->operand.value.target->block_id);
		}
		break;

	case FRUITY_BTRUE:
		if (instr->operand.type == FRUITY_OP_BRANCH) {
			exchange_fprintf(out, "    jnz %%t%d, @block%u, @fallthrough\n",
			                tmp_id - 1, instr->operand.value.target->block_id);
		}
		break;

	case FRUITY_BFALSE:
		if (instr->operand.type == FRUITY_OP_BRANCH) {
			exchange_fprintf(out, "    jnz %%t%d, @fallthrough, @block%u\n",
			                tmp_id - 1, instr->operand.value.target->block_id);
		}
		break;

	case FRUITY_BEQ:
		if (instr->operand.type == FRUITY_OP_BRANCH) {
			exchange_fprintf(out, "    %%cmp =w ceqw %%t%d, %%t%d\n",
			                tmp_id - 2, tmp_id - 1);
			exchange_fprintf(out, "    jnz %%cmp, @block%u, @fallthrough\n",
			                instr->operand.value.target->block_id);
		}
		break;

	case FRUITY_BNE:
		if (instr->operand.type == FRUITY_OP_BRANCH) {
			exchange_fprintf(out, "    %%cmp =w cnew %%t%d, %%t%d\n",
			                tmp_id - 2, tmp_id - 1);
			exchange_fprintf(out, "    jnz %%cmp, @block%u, @fallthrough\n",
			                instr->operand.value.target->block_id);
		}
		break;

	/* ===== Not Yet Implemented ===== */
	case FRUITY_NOP:
		exchange_fprintf(out, "    # nop\n");
		break;

	default:
		set_error("Unsupported opcode: 0x%x", instr->opcode);
		return -1;
	}

	return 0;
}

/* Emit a single Fruity function as QBE IL */
static int
emit_function(ExchangeFILE *out, fruity_function_t *func)
{
	fruity_basic_block_t *block;
	fruity_instruction_t *instr;
	int tmp_counter = 0;
	char ret_type;

	/* Function signature */
	ret_type = qbe_type(func->return_type);
	if (ret_type == 0) {
		set_error("Unsupported return type in function %s", func->name);
		return -1;
	}

	exchange_fprintf(out, "export function ");
	if (ret_type != 'w' || func->return_type != CLR_INT32) {
		exchange_fprintf(out, "%c ", ret_type);
	}
	exchange_fprintf(out, "$%s(", func->name);

	/* Arguments */
	for (ulong i = 0; i < func->arg_count; i++) {
		char arg_type = qbe_type(func->arg_types[i]);
		if (arg_type == 0) {
			set_error("Unsupported arg type in function %s", func->name);
			return -1;
		}
		if (i > 0)
			exchange_fprintf(out, ", ");
		exchange_fprintf(out, "%c %%arg%lu", arg_type, i);
	}

	exchange_fprintf(out, ") {\n");

	/* Emit all basic blocks */
	for (block = func->blocks_head; block != NULL; block = block->next) {
		/* Block label */
		if (block == func->entry_block)
			exchange_fprintf(out, "@start\n");
		else
			exchange_fprintf(out, "@block%u\n", block->block_id);

		/* Emit all instructions in block */
		for (instr = block->instructions_head; instr != NULL; instr = instr->next) {
			if (emit_instruction(out, instr, &tmp_counter) < 0)
				return -1;
		}
	}

	exchange_fprintf(out, "}\n\n");
	return 0;
}

/*
 * Main entry point: Translate Fruity IR module to QBE IL
 */
int
fruity_to_qbe(fruity_module_t *module,
              uintptr out_handle,
              char *errorbuf,
              size_t errorbuf_size)
{
	ExchangeFILE *out;
	fruity_function_t *func;
	int ret = 0;

	/* Validate inputs */
	if (!module || !out_handle) {
		if (errorbuf && errorbuf_size > 0)
			snprint(errorbuf, errorbuf_size, "Invalid arguments");
		return -1;
	}

	/* Set up error handling */
	error_buf = errorbuf;
	error_buf_size = errorbuf_size;

	/* Open output exchange page */
	out = exchange_fmemopen_handle(out_handle, "w");
	if (!out) {
		set_error("Failed to open output exchange page");
		return -1;
	}

	/* Emit QBE header with runtime ABI */
	emit_qbe_header(out);

	/* Emit all functions */
	for (func = module->functions_head; func != NULL; func = func->next) {
		if (emit_function(out, func) < 0) {
			ret = -1;
			goto cleanup;
		}
	}

	/* Success */
	exchange_fprintf(out, "# End of QBE IL\n");

cleanup:
	exchange_fclose(out);
	error_buf = NULL;
	error_buf_size = 0;
	return ret;
}
