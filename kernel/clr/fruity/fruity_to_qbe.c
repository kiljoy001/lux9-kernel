/* fruity_to_qbe.c - Fruity IR to QBE IL Translator
 *
 * Simplified translator: just emit QBE stubs for now
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "fruity_to_qbe.h"
#include "qbe_buffer.h"
#include "exchange.h"
#include "blind_ledger.h"
#include "hhdm.h"

/* Emit QBE IL header */
static void
emit_header(QBEBuffer *buf)
{
	qbe_buffer_printf(buf, "# QBE IL generated from Fruity IR\n\n");
	qbe_buffer_printf(buf, "# Pebble Runtime ABI\n");
	qbe_buffer_printf(buf, "export function l $lux_alloc(w %%size, w %%type) { @start ret 0 }\n");
	qbe_buffer_printf(buf, "export function $lux_token_mint(l %%ptr) { @start ret }\n");
	qbe_buffer_printf(buf, "export function $lux_token_burn(l %%ptr) { @start ret }\n\n");
}

/* Emit a function as QBE IL */
static int
emit_function(QBEBuffer *buf, fruity_function_t *func)
{
	fruity_basic_block_t *bb;
	fruity_instruction_t *instr;

	qbe_buffer_printf(buf, "export function w $%s() {\n", func->name);
	qbe_buffer_printf(buf, "@start\n");

	/* Just emit a simple stub - full translation would be complex */
	for(bb = func->blocks_head; bb != nil; bb = bb->next){
		if(bb != func->blocks_head)
			qbe_buffer_printf(buf, "@bb%d\n", bb->block_id);

		for(instr = bb->instructions_head; instr != nil; instr = instr->next){
			/* Translate each Fruity opcode to QBE */
			switch(instr->opcode){
			case FRUITY_LIME:
				qbe_buffer_printf(buf, "    # LIME: allocate\n");
				break;
			case FRUITY_VANILLA:
				qbe_buffer_printf(buf, "    # VANILLA: addref\n");
				break;
			case FRUITY_BURN:
				qbe_buffer_printf(buf, "    # BURN: release\n");
				break;
			case FRUITY_RET:
				qbe_buffer_printf(buf, "    ret 0\n");
				goto done_block;
			default:
				qbe_buffer_printf(buf, "    # opcode %d\n", instr->opcode);
				break;
			}
		}
done_block:
		;
	}

	qbe_buffer_printf(buf, "}\n\n");
	return 0;
}

/* Main translation function */
int
fruity_to_qbe(fruity_module_t *module, uintptr out_handle, char *errorbuf, usize errorbuf_size)
{
	QBEBuffer buf;
	fruity_function_t *func;
	void *vaddr;
	usize copy_len;

	if(module == nil){
		if(errorbuf && errorbuf_size > 0)
			snprint(errorbuf, errorbuf_size, "null module");
		return -1;
	}

	/* Initialize buffer */
	qbe_buffer_init(&buf);

	/* Emit header */
	emit_header(&buf);

	/* Emit all functions */
	for(func = module->functions_head; func != nil; func = func->next){
		if(emit_function(&buf, func) < 0){
			if(errorbuf && errorbuf_size > 0)
				snprint(errorbuf, errorbuf_size, "Failed to emit function");
			qbe_buffer_free(&buf);
			return -1;
		}
	}

	/* out_handle is a physical address - convert to kernel virtual */
	vaddr = KADDR(out_handle);
	copy_len = qbe_buffer_len(&buf);
	if(copy_len > 4096){
		if(errorbuf && errorbuf_size > 0)
			snprint(errorbuf, errorbuf_size, "Output too large: %lud bytes", copy_len);
		qbe_buffer_free(&buf);
		return -1;
	}

	/* Copy to page */
	memmove(vaddr, qbe_buffer_data(&buf), copy_len);
	((char*)vaddr)[copy_len] = 0;

	qbe_buffer_free(&buf);
	return 0;
}
