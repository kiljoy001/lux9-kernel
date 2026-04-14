/*
 * pool_block.c - Block-level operations for pool allocator
 *
 * This module handles individual block operations including:
 * - Block/data pointer conversions (B2D, D2B)
 * - Block size calculations and conversions
 * - Block merging and splitting
 * - Block metadata management
 */

#include "pool_internal.h"
#include "pool_types.h"
#include <libc.h>
#include <pool.h>
#include <u.h>

extern void *pool_debug_last_free_caller;
extern void *pool_debug_last_free_v;
extern void *alloc_debug_last_free_caller;
extern void *alloc_debug_last_free_v;

/* Data magic for overflow detection */
uchar datamagic[] = {0xFE, 0xF1, 0xF0, 0xFA};

/*
 * B2D: Convert block pointer to data pointer (with validation)
 */
void *B2D(Pool *p, Alloc *a) {
  if (a->magic != ALLOC_MAGIC)
    p->panic(p, "B2D called on unworthy block");
  return _B2D(a);
}

/*
 * D2B: Convert data pointer to block pointer (with validation)
 * Handles alignment magic to find the actual block start
 */
Alloc *D2B(Pool *p, void *v) {
  Alloc *a;
  ulong *u;

  if (p == nil) {
    uartputs("D2B: FATAL p is nil\n", 20);
    while (1)
      ;
  }

  if (v == nil) {
    uartputs("D2B: FATAL v is nil\n", 20);
    while (1)
      ;
  }

  /* Add bounds checking to prevent crashes on corrupted pointers */
  if ((uintptr)v > 0xffffffffffffffe0 && (uintptr)v < 0xffffffffffffffff) {
    char buf[64];
    snprint(buf, sizeof(buf), "D2B: Suspicious v=%p caller=%p\n", v,
            getcallerpc(&p));
    uartputs(buf, strlen(buf));
  }

  /* Add early validation to catch obviously corrupted pointers */
  if ((uintptr)v < 0x1000) {
    char buf[64];
    snprint(buf, sizeof(buf), "D2B: INVALID v=%p too low caller=%p - continuing with nil\n", v,
            getcallerpc(&p));
    uartputs(buf, strlen(buf));
    /* Return nil instead of crashing - let caller handle gracefully */
    return nil;
  }

  if ((uintptr)v & (sizeof(ulong) - 1))
    v = (char *)v - ((uintptr)v & (sizeof(ulong) - 1));
  u = v;

  if ((uintptr)u < 0x1000) {
    char buf[64];
    snprint(buf, sizeof(buf), "D2B: FATAL u=%p too low caller=%p\n", u,
            getcallerpc(&p));
    uartputs(buf, strlen(buf));
    while (1)
      ;
  }

  /* Dangerous dereference here if u is low/invalid */
  while (u[-1] == ALIGN_MAGIC)
    u--;

  if ((uintptr)u < 0x1000) {
    char buf[80];
    snprint(buf, sizeof(buf), "D2B: FATAL u became low after scan caller=%p\n",
            getcallerpc(&p));
    uartputs(buf, strlen(buf));
    while (1)
      ;
  }

  a = _D2B(u);
  /* Check magic safely? Accessing a->magic might fault if a is invalid address
   */
  if (a->magic != ALLOC_MAGIC) {
    char buf[192];
    snprint(buf, sizeof(buf),
            "D2B: magic bad %lx at %p v=%p u=%p caller=%p lastfree=%p "
            "lastv=%p kfree=%p kv=%p\n",
            a->magic, a, v, u, getcallerpc(&p), pool_debug_last_free_caller,
            pool_debug_last_free_v, alloc_debug_last_free_caller,
            alloc_debug_last_free_v);
    uartputs(buf, strlen(buf));
    while (1)
      ;
  }
  return a;
}

/*
 * dsize2bsize: Convert user data size to block size
 * Accounts for header, tail, minimum sizes, and quantum alignment
 */
/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
ulong dsize2bsize(Pool *p, ulong sz) {
  sz += sizeof(Bhdr) + sizeof(Btail);
  if (sz < p->minblock)
    sz = p->minblock;
  if (sz < MINBLOCKSIZE)
    sz = MINBLOCKSIZE;
  sz = (sz + p->quantum - 1) & ~(p->quantum - 1);
  return sz;
}

/*
 * bsize2asize: Convert block size to arena size
 */
/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
ulong bsize2asize(Pool *p, ulong sz) {
  sz += sizeof(Arena) + sizeof(Btail);
  if (sz < p->minarena)
    sz = p->minarena;
  sz = (sz + p->quantum) & ~(p->quantum - 1);
  return sz;
}

/*
 * blockmerge: Merge two adjacent blocks
 * Both blocks are removed from pool if necessary
 */
Alloc *blockmerge(Pool *pool, Bhdr *a, Bhdr *b) {
  Btail *t;

  assert(B2NB(a) == b);

  if (a->magic == FREE_MAGIC)
    pooldel(pool, (Free *)a);
  if (b->magic == FREE_MAGIC)
    pooldel(pool, (Free *)b);

  t = B2T(a);
  t->size = (ulong)Poison;
  t->magic0 = NOT_MAGIC;
  t->magic1 = NOT_MAGIC;
  PSHORT(t->datasize, NOT_MAGIC);

  a->size += b->size;
  t = B2T(a);
  t->size = a->size;
  PSHORT(t->datasize, 0xFFFF);

  b->size = NOT_MAGIC;
  b->magic = NOT_MAGIC;

  a->magic = UNALLOC_MAGIC;
  return (Alloc *)a;
}

/*
 * blocksetsize: Set the total size of a block, fixing tail pointers
 */
Bhdr *blocksetsize(Bhdr *b, ulong bsize) {
  Btail *t;

  assert(b->magic != FREE_MAGIC /* blocksetsize */);

  b->size = bsize;
  t = B2T(b);
  t->size = b->size;
  t->magic0 = TAIL_MAGIC0;
  t->magic1 = TAIL_MAGIC1;
  return b;
}

/*
 * getdsize: Return the requested data size for an allocated block
 */
/*@
  @ requires b == \null || \valid(b);
  @ assigns \nothing;
  @*/
ulong getdsize(Alloc *b) {
  Btail *t;
  t = B2T(b);
  return b->size - SHORT(t->datasize);
}

/*
 * blocksetdsize: Set the user data size of a block
 * Also sets overflow detection magic bytes
 */
Alloc *blocksetdsize(Pool *p, Alloc *b, ulong dsize) {
  Btail *t;
  uchar *q, *eq;

  assert(b->size >= dsize2bsize(p, dsize));
  assert(b->size - dsize < 0x10000);

  t = B2T(b);
  PSHORT(t->datasize, b->size - dsize);

  q = (uchar *)_B2D(b) + dsize;
  eq = (uchar *)t;
  if (eq > q + 4)
    eq = q + 4;
  for (; q < eq; q++)
    *q = datamagic[((ulong)(uintptr)q) % nelem(datamagic)];

  return b;
}

/*
 * trim: Trim a block down to what is needed to hold dsize bytes of user data
 * If excess is large enough, split it off as a new free block
 */
Alloc *trim(Pool *p, Alloc *b, ulong dsize) {
  ulong extra, bsize;
  Alloc *frag;

  bsize = dsize2bsize(p, dsize);
  extra = b->size - bsize;
  if (b->size - dsize >= 0x10000 ||
      (extra >= bsize >> 2 && extra >= MINBLOCKSIZE && extra >= p->minblock)) {
    blocksetsize(b, bsize);
    frag = (Alloc *)B2NB(b);

    antagonism { memmark(frag, 0xF1, extra); }

    frag->magic = UNALLOC_MAGIC;
    blocksetsize(frag, extra);
    pooladd(p, frag);
  }

  b->magic = ALLOC_MAGIC;
  blocksetdsize(p, b, dsize);
  return b;
}

/*
 * memmark: Mark memory with a signature pattern for debugging
 * Pattern includes the signature byte and offset from start
 */
/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void memmark(void *v, int sig, ulong size) {
  uchar *p, *ep;
  ulong *lp, *elp;
  ulong words;

  lp = v;
  words = size / sizeof(*lp);
  elp = lp + words;
  while (lp < elp) {
    uintptr offset = (uintptr)((uchar *)lp - (uchar *)v);
    *lp++ = (sig << 24) ^ offset;
  }
  p = (uchar *)lp;
  ep = (uchar *)v + size;
  while (p < ep)
    *p++ = sig;
}
