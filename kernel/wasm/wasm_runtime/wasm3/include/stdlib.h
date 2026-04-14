#ifndef _WASM_STDLIB_H
#define _WASM_STDLIB_H

/* Lux9 Kernel Environment */
#include <u.h>

/* Map wasm3 requirements */
#define abs(x)       ((x) < 0 ? -(x) : (x))

/* Standard functions mapped to kernel */
void *malloc(ulong size);
void *calloc(ulong n, ulong size);
void free(void *ptr);
void *realloc(void *ptr, ulong size);
void abort(void);

/* NULL */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif