/* qbe_kernel_wrapper.c - High-level QBE compilation wrapper
 *
 * Provides kernel-safe wrapper around QBE compiler that:
 * - Reads input from exchange pages
 * - Writes output to exchange pages
 * - Catches errors via setjmp/longjmp (instead of panic)
 * - Returns clean error codes
 */

#include "qbe_kernel_wrapper.h"
#include "all.h" /* QBE internal headers */
#include "exchange_io.h"
#include "kernel_compat.h"

/* Global state for compilation */
static ExchangeFILE *output_file = NULL;
static jmp_buf error_jmpbuf;
static int error_active = 0;
static char *error_buffer = NULL;
static size_t error_buffer_size = 0;

/* Override die_ to use longjmp instead of panic */
void die_(char *file, char *s, ...) {
  va_list ap;
  extern void uartputs(char *, int);
  char debug_buf[256];

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: die_() CALLED file=%s error_active=%d\n", file, error_active);
  uartputs(debug_buf, strlen(debug_buf));

  if (error_active && error_buffer && error_buffer_size > 0) {
    /* Capture error message */
    int n;
    va_start(ap, s);
    n = vsnprintf(error_buffer, error_buffer_size, s, ap);
    va_end(ap);

    /* Add file info if there's room */
    if (n > 0 && (size_t)n < error_buffer_size - 20) {
      snprintf(error_buffer + n, (size_t)(error_buffer_size - (size_t)n),
               " (in %s)", file);
    }

    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: die_() calling longjmp, errorbuf=%s, error_jmpbuf=%p, "
            "error_jmpbuf[0]=%p\n",
            error_buffer, (void *)&error_jmpbuf, ((void **)error_jmpbuf)[0]);
    uartputs(debug_buf, strlen(debug_buf));

    /* Jump back to error handler */
    longjmp(error_jmpbuf, 1);
  }

  snprint(debug_buf, sizeof(debug_buf), "DEBUG: die_() FALLBACK panic path\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Fallback - should never reach here in normal operation */
  extern void panic(const char *fmt, ...);
  va_start(ap, s);
  panic("QBE error in %s: %s", file, s);
  va_end(ap);
  __builtin_unreachable();
}

/* Callback for data sections */
static void emit_data(Dat *d) {
  if (!output_file)
    return;
  gasemitdat(d, output_file);
  if (d->type == DEnd) {
    exchange_fprintf(output_file, "/* end data */\n\n");
    freeall();
  }
}

/* Callback for functions */
static void emit_func(Fn *fn) {
  extern void uartputs(char *, int);
  char debug_buf[128];

  if (!output_file)
    return;

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: emit_func START fn=%s ntmp=%d\n", fn->name, fn->ntmp);
  uartputs(debug_buf, strlen(debug_buf));

  /* QBE compilation pipeline (from main.c) */
  uartputs("DEBUG: emit_func fillrpo\n", 25);
  fillrpo(fn);
  uartputs("DEBUG: emit_func fillpreds\n", 27);
  fillpreds(fn);
  uartputs("DEBUG: emit_func filluse\n", 25);
  filluse(fn);
  uartputs("DEBUG: emit_func memopt\n", 24);
  memopt(fn);
  filluse(fn);
  uartputs("DEBUG: emit_func ssa\n", 21);
  ssa(fn);
  filluse(fn);
  uartputs("DEBUG: emit_func ssacheck\n", 26);
  ssacheck(fn);
  uartputs("DEBUG: emit_func fillalias\n", 27);
  fillalias(fn);
  uartputs("DEBUG: emit_func loadopt\n", 25);
  loadopt(fn);
  filluse(fn);
  ssacheck(fn);
  uartputs("DEBUG: emit_func copy\n", 22);
  copy(fn);
  filluse(fn);
  uartputs("DEBUG: emit_func fold\n", 22);
  fold(fn);
  uartputs("DEBUG: emit_func T.abi\n", 23);
  T.abi(fn);
  fillpreds(fn);
  filluse(fn);
  uartputs("DEBUG: emit_func T.isel\n", 24);
  T.isel(fn);
  uartputs("DEBUG: emit_func fillrpo2\n", 26);
  fillrpo(fn);
  uartputs("DEBUG: emit_func filllive\n", 26);
  filllive(fn);
  uartputs("DEBUG: emit_func fillloop\n", 26);
  fillloop(fn);
  uartputs("DEBUG: emit_func fillcost\n", 26);
  fillcost(fn);
  uartputs("DEBUG: emit_func spill\n", 23);
  spill(fn);
  uartputs("DEBUG: emit_func rega\n", 22);
  rega(fn);
  uartputs("DEBUG: emit_func fillrpo3\n", 26);
  fillrpo(fn);
  uartputs("DEBUG: emit_func simpljmp\n", 26);
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

  uartputs("DEBUG: emit_func T.emitfn\n", 26);
  /* Emit function */
  T.emitfn(fn, output_file);
  exchange_fprintf(output_file, "/* end function %s */\n\n", fn->name);
  freeall();
}

/*
 * Compile QBE IL from input exchange page to machine code in output exchange
 * page
 */
int qbe_compile_page(uintptr input, uintptr output, char *errorbuf,
                     size_t errorbuf_size) {
  ExchangeFILE *in_fp = NULL;
  extern Target T_amd64_sysv;
  extern void uartputs(char *, int);
  char debug_buf[128];

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: qbe_compile_page ENTERED input=%#p output=%#p\n", input,
          output);
  uartputs(debug_buf, strlen(debug_buf));

  /* Validate parameters */
  if (!input || !output) {
    if (errorbuf && errorbuf_size > 0)
      snprintf(errorbuf, errorbuf_size, "Invalid exchange handles");
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: qbe_compile_page INVALID HANDLES\n");
    uartputs(debug_buf, strlen(debug_buf));
    return -1;
  }

  /* Set up error handling */
  error_active = 1;
  error_buffer = errorbuf;
  error_buffer_size = errorbuf_size;

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: qbe_compile_page setting up setjmp error_jmpbuf=%p\n",
          (void *)&error_jmpbuf);
  uartputs(debug_buf, strlen(debug_buf));

  int setjmp_ret = setjmp(error_jmpbuf);
  uintptr *jbuf = (uintptr *)error_jmpbuf;
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: setjmp returned %d\n"
          "  jbuf[0]=0x%p (rbx)\n"
          "  jbuf[1]=0x%p (rbp)\n"
          "  jbuf[6]=0x%p (rsp)\n"
          "  jbuf[7]=0x%p (return addr)\n",
          setjmp_ret, (void *)jbuf[0], (void *)jbuf[1], (void *)jbuf[6],
          (void *)jbuf[7]);
  uartputs(debug_buf, strlen(debug_buf));

  if (setjmp_ret != 0) {
    /* Error occurred - clean up and return */
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: qbe_compile_page LONGJMP TRIGGERED errorbuf=%s\n",
            errorbuf ? errorbuf : "(nil)");
    uartputs(debug_buf, strlen(debug_buf));
    if (in_fp)
      exchange_fclose(in_fp);
    if (output_file)
      exchange_fclose(output_file);
    output_file = NULL;
    error_active = 0;
    qbe_reset_pool(); /* Reset memory pool on error */
    return -1;
  }

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: qbe_compile_page opening input handle\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Open input exchange page for reading */
  in_fp = exchange_fmemopen_handle(input, "r");
  if (!in_fp) {
    if (errorbuf && errorbuf_size > 0)
      snprintf(errorbuf, errorbuf_size, "Failed to open input exchange page");
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: qbe_compile_page FAILED to open input\n");
    uartputs(debug_buf, strlen(debug_buf));
    error_active = 0;
    return -1;
  }

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: qbe_compile_page opening output handle\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Open output exchange page for writing */
  output_file = exchange_fmemopen_handle(output, "w");
  if (!output_file) {
    if (errorbuf && errorbuf_size > 0)
      snprintf(errorbuf, errorbuf_size, "Failed to open output exchange page");
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: qbe_compile_page FAILED to open output\n");
    uartputs(debug_buf, strlen(debug_buf));
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

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: qbe_compile_page calling parse()\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Parse and compile - callbacks will be invoked for each function/data */
  parse(in_fp, "<exchange-page>", emit_data, emit_func);

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: qbe_compile_page parse() completed\n");
  uartputs(debug_buf, strlen(debug_buf));

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
