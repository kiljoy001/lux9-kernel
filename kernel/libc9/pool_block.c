/*
 * pool_block.c - Block-level operations for pool allocator
 *
 * This module handles individual block operations including:
 * - Block/data pointer conversions (B2D, D2B)
 * - Block size calculations and conversions
 * - Block merging and splitting
 * - Block metadata management
 */

#include <libc.h>
#include <pool.h>
#include <u.h>
#include "pool_types.h"
#include "pool_internal.h"

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

	if ((uintptr)v & (sizeof(ulong) - 1))
		v = (char *)v - ((uintptr)v & (sizeof(ulong) - 1));
	u = v;
	while (u[-1] == ALIGN_MAGIC)
		u--;
	a = _D2B(u);
	if (a->magic != ALLOC_MAGIC)
		p->panic(p, "D2B called on non-block %p (double-free?)", v);
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
