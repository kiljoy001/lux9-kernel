#ifndef POOL_INTERNAL_H
#define POOL_INTERNAL_H

#include "pool_types.h"

/* External declarations */
extern void early_iprint(char *fmt, ...);
extern void uartputs(char *, int);

/* Tracing thresholds */
#define POOLALLOC_TRACE_THRESHOLD (4 * 1024)
#define POOLTRACE_THRESHOLD (16 * 1024)

/* pool_block.c - Block-level operations */
extern void *B2D(Pool *p, Alloc *a);
extern Alloc *D2B(Pool *p, void *v);
extern Alloc *blockmerge(Pool *p, Bhdr *a, Bhdr *b);
extern Alloc *blocksetdsize(Pool *p, Alloc *a, ulong dsize);
extern Bhdr *blocksetsize(Bhdr *b, ulong size);
extern ulong bsize2asize(Pool *p, ulong bsize);
extern ulong dsize2bsize(Pool *p, ulong dsize);
extern ulong getdsize(Alloc *a);
extern Alloc *trim(Pool *p, Alloc *a, ulong dsize);
extern void memmark(void *v, int sig, ulong size);

/* pool_arena.c - Arena management */
extern Arena *arenamerge(Pool *p, Arena *bot, Arena *top);
extern void poolnewarena(Pool *p, ulong asize);
extern int poolgrowarena(Pool *p, ulong bsize);
extern void poolcheckarena(Pool *p, Arena *a);
extern void pooldumparena(Pool *p, Arena *a);

/* pool_freelist.c - Free list tree management */
extern Free *pooladd(Pool *p, Alloc *a);
extern Alloc *pooldel(Pool *p, Free *f);
extern Free *treelookupgt(Free *t, ulong size);
extern Free *treesplay(Free *t, ulong size);
extern void checklist(Free *t);
extern void checktree(Free *t, int a, int b);

/* pool_debug.c - Debug and validation */
extern void blockcheck(Pool *p, Bhdr *b);
extern void poolcheckl(Pool *p);
extern void pooldumpl(Pool *p);
extern void logstack(Pool *p);

/* pool.c - Core allocation/free (internal functions) */
extern void *poolallocl(Pool *p, ulong dsize);
extern void poolfreel(Pool *p, void *v);
extern void *poolreallocl(Pool *p, void *v, ulong dsize);
extern int poolcompactl(Pool *p);

#endif /* POOL_INTERNAL_H */
