/* qbe_kernel_wrapper.c - High-level QBE compilation wrapper
 *
 * Provides kernel-safe wrapper around QBE compiler that:
 * - Reads input from exchange pages
 * - Writes output to exchange pages
 * - Catches errors via setjmp/longjmp (instead of panic)
 * - Returns clean error codes
 */

#include "qbe_kernel_wrapper.h"
#include "exchange_io.h"
#include "kernel_compat.h"
#include "all.h"  /* QBE internal headers */

/* Global state for compilation */
static ExchangeFILE *output_file = NULL;
static jmp_buf error_jmpbuf;
static int error_active = 0;
static char *error_buffer = NULL;
static size_t error_buffer_size = 0;

/* Override die_ to use longjmp instead of panic */
void
die_(char *file, char *s, ...)
{
	va_list ap;

	if (error_active && error_buffer && error_buffer_size > 0) {
		/* Capture error message */
		int n;
		va_start(ap, s);
		n = vsnprintf(error_buffer, error_buffer_size, s, ap);
		va_end(ap);

		/* Add file info if there's room */
		if (n > 0 && (size_t)n < error_buffer_size - 20) {
			snprintf(error_buffer + n, error_buffer_size - n,
			         " (in %s)", file);
		}

		/* Jump back to error handler */
		longjmp(error_jmpbuf, 1);
	}

	/* Fallback - should never reach here in normal operation */
	extern void panic(const char *fmt, ...);
	va_start(ap, s);
	panic("QBE error in %s: %s", file, s);
	va_end(ap);
}

/* Callback for data sections */
static void
emit_data(Dat *d)
{
	if (!output_file)
		return;
	gasemitdat(d, output_file);
	if (d->type == DEnd) {
		exchange_fprintf(output_file, "/* end data */\n\n");
		freeall();
	}
}

/* Callback for functions */
static void
emit_func(Fn *fn)
{
	if (!output_file)
		return;

	/* QBE compilation pipeline (from main.c) */
	fillrpo(fn);
	fillpreds(fn);
	filluse(fn);
	memopt(fn);
	filluse(fn);
	ssa(fn);
	filluse(fn);
	ssacheck(fn);
	fillalias(fn);
	loadopt(fn);
	filluse(fn);
	ssacheck(fn);
	copy(fn);
	filluse(fn);
	fold(fn);
	T.abi(fn);
	fillpreds(fn);
	filluse(fn);
	T.isel(fn);
	fillrpo(fn);
	filllive(fn);
	fillloop(fn);
	fillcost(fn);
	spill(fn);
	rega(fn);
	fillrpo(fn);
	simpljmp(fn);
	fillpreds(fn);
	fillrpo(fn);

	/* Build block linked list */
	for (uint n = 0; n < fn->nblk; n++) {
		if (n == fn->nblk - 1)
			fn->rpo[n]->link = NULL;
		else
			fn->rpo[n]->link = fn->rpo[n + 1];
	}

	/* Emit function */
	T.emitfn(fn, output_file);
	exchange_fprintf(output_file, "/* end function %s */\n\n", fn->name);
	freeall();
}

/*
 * Compile QBE IL from input exchange page to machine code in output exchange page
 */
int
qbe_compile_page(uintptr input, uintptr output,
                 char *errorbuf, size_t errorbuf_size)
{
	ExchangeFILE *in_fp = NULL;
	extern Target T_amd64_sysv;

	/* Validate parameters */
	if (!input || !output) {
		if (errorbuf && errorbuf_size > 0)
			snprintf(errorbuf, errorbuf_size,
			         "Invalid exchange handles");
		return -1;
	}

	/* Set up error handling */
	error_active = 1;
	error_buffer = errorbuf;
	error_buffer_size = errorbuf_size;

	if (setjmp(error_jmpbuf) != 0) {
		/* Error occurred - clean up and return */
		if (in_fp)
			exchange_fclose(in_fp);
		if (output_file)
			exchange_fclose(output_file);
		output_file = NULL;
		error_active = 0;
		return -1;
	}

	/* Open input exchange page for reading */
	in_fp = exchange_fmemopen_handle(input, "r");
	if (!in_fp) {
		if (errorbuf && errorbuf_size > 0)
			snprintf(errorbuf, errorbuf_size,
			         "Failed to open input exchange page");
		error_active = 0;
		return -1;
	}

	/* Open output exchange page for writing */
	output_file = exchange_fmemopen_handle(output, "w");
	if (!output_file) {
		if (errorbuf && errorbuf_size > 0)
			snprintf(errorbuf, errorbuf_size,
			         "Failed to open output exchange page");
		exchange_fclose(in_fp);
		error_active = 0;
		return -1;
	}

	/* Set target to AMD64 SysV ABI */
	T = T_amd64_sysv;

	/* Set up gas flavor for ELF */
	extern char *gasloc, *gassym;
	gasloc = ".L";
	gassym = "";

	/* Parse and compile - callbacks will be invoked for each function/data */
	parse(in_fp, "<exchange-page>", emit_data, emit_func);

	/* Emit finalizer */
	gasemitfin(output_file);
	exchange_fprintf(output_file, ".section .note.GNU-stack,\"\",@progbits\n");

	/* Clean up */
	exchange_fclose(in_fp);
	exchange_fclose(output_file);
	output_file = NULL;
	error_active = 0;

	return 0;
}
