#ifndef _FNS_H_
#define _FNS_H_

#include "u.h"

/* Wrappers provided by test_clr_pipeline.c */
void *xalloc(usize n);
void xfree(void *p);
void *mallocz(usize n, int clr);
int snprint(char *buf, int len, char *fmt, ...);
char *kstrdup(char *s);
uintptr get_hhdm_offset(void);

// Mocks for kernel functions
#define KADDR(pa) ((void *)((pa) + get_hhdm_offset()))

/* Avoid conflicts with stdlib */
#define realloc(p, s) realloc(p, s)

#endif
