/*
 * pool_arena.c - Arena management for pool allocator
 *
 * This module handles arena operations including:
 * - Arena creation and initialization
 * - Arena merging
 * - Arena validation and dumping
 * - Arena growth management
 */

#include <libc.h>
#include <pool.h>
#include <u.h>
#include "pool_types.h"
#include "pool_internal.h"

/*@
  @ requires \valid_read(fmt + (0..));
  @ requires \exists integer k; k >= 0 && fmt[k] == '\0';
  @ assigns \nothing;
  @*/
static void pooltrace(const char *fmt, ...) {
	char buf[128];
	va_list v;
	int n;

	va_start(v, fmt);
	n = vseprint(buf, buf + sizeof buf, fmt, v) - buf;
	va_end(v);
	uartputs(buf, n);
}

/*
 * arenasetsize: Set arena size, updating tail
 */
static void arenasetsize(Arena *a, ulong asize) {
	Bhdr *atail;

	a->asize = asize;
	atail = A2TB(a);
	atail->magic = ARENATAIL_MAGIC;
	atail->size = 0;
}

/*
 * poolnewarena: Allocate new arena
 */
void poolnewarena(Pool *p, ulong asize) {
	Arena *a;
	Arena *ap, *lastap;
	Alloc *b;

	if (asize > p->maxsize || p->cursize > p->maxsize - asize) {
		if (poolcompactl(p) == 0) {
			LOG(p, "pool too big: %llud+%lud > %llud\n", (uvlong)p->cursize, asize,
			    (uvlong)p->maxsize);
			werrstr("memory pool too large");
		}
		return;
	}

	pooltrace("poolnewarena: calling p->alloc(%lud)\n", asize);
	a = p->alloc(asize);
	if (a == nil) {
		pooltrace("poolnewarena: p->alloc returned nil\n");
		/* assume errstr set by p->alloc */
		return;
	}
	pooltrace("poolnewarena: got arena at %p\n", a);

	p->cursize += asize;

	/* arena hdr */
	a->magic = ARENA_MAGIC;
	blocksetsize(a, sizeof(Arena));
	arenasetsize(a, asize);
	pooltrace("poolnewarena: calling blockcheck(arena)\n");
	blockcheck(p, a);

	/* create one large block in arena */
	b = (Alloc *)A2B(a);
	pooltrace("poolnewarena: block at %p\n", b);
	b->magic = UNALLOC_MAGIC;
	blocksetsize(b, (uchar *)A2TB(a) - (uchar *)b);
	pooltrace("poolnewarena: calling blockcheck(block)\n");
	blockcheck(p, b);
	pooltrace("poolnewarena: calling pooladd\n");
	pooladd(p, b);
	pooltrace("poolnewarena: calling blockcheck after pooladd\n");
	blockcheck(p, b);

	/* sort arena into descending sorted arena list */
	pooltrace("poolnewarena: sorting arena list (head=%p, a=%p)\n", p->arenalist,
	          a);
	for (lastap = nil, ap = p->arenalist; ap > a; lastap = ap, ap = ap->down)
		;
	pooltrace("poolnewarena: arena list sorted, linking\n");

	if (a->down = ap) /* assign = */
		a->down->aup = a;

	if (a->aup = lastap) /* assign = */
		a->aup->down = a;
	else
		p->arenalist = a;
	pooltrace("poolnewarena: done\n");

	/* merge with surrounding arenas if possible */
	/* must do a with up before down with a (think about it) */
	// if (a->aup)
	//   arenamerge(p, a, a->aup);
	// if (a->down)
	//   arenamerge(p, a->down, a);
}

/*
 * poolgrowarena: Try to grow the pool to accommodate a block of size bsize
 * Returns 1 if successful, 0 otherwise
 */
int poolgrowarena(Pool *p, ulong bsize) {
	ulong asize, minsize;

	asize = bsize2asize(p, bsize);
	minsize = bsize + p->quantum;
	if (minsize < bsize)
		minsize = bsize;

	while (asize >= minsize) {
		if (bsize >= POOLALLOC_TRACE_THRESHOLD)
			pooltrace("poolgrowarena: attempting asize=%lud\n", asize);
		poolnewarena(p, asize);
		if (treelookupgt(p->freeroot, bsize) != nil)
			return 1;
		if (asize == minsize)
			break;
		asize /= 2;
		if (asize < minsize)
			asize = minsize;
	}
	return 0;
}

/*
 * blockgrow: Grow a block to encompass space past its end
 * If the block is free, remove it from the tree, resize, and re-add
 * If allocated, update size and trim
 */
static void blockgrow(Pool *p, Bhdr *b, ulong nsize) {
	if (b->magic == FREE_MAGIC) {
		Alloc *a;
		Bhdr *bnxt;
		a = pooldel(p, (Free *)b);
		blockcheck(p, a);
		blocksetsize(a, nsize);
		blockcheck(p, a);
		bnxt = B2NB(a);
		if (bnxt->magic == FREE_MAGIC)
			a = blockmerge(p, a, bnxt);
		blockcheck(p, a);
		pooladd(p, a);
	} else {
		Alloc *a;
		ulong dsize;

		a = (Alloc *)b;
		p->curalloc -= a->size;
		dsize = getdsize(a);
		blocksetsize(a, nsize);
		trim(p, a, dsize);
		p->curalloc += a->size;
	}
}

/*
 * arenamerge: Attempt to coalesce two arenas that might be adjacent
 * Returns merged arena on success, nil on failure
 */
Arena *arenamerge(Pool *p, Arena *bot, Arena *top) {
	Bhdr *bbot, *btop;
	Btail *t;
	ulong newsize;

	blockcheck(p, bot);
	blockcheck(p, top);
	assert(bot->aup == top && top > bot);

	newsize = top->asize + ((uchar *)top - (uchar *)bot);
	if (newsize < top->asize || p->merge == nil || p->merge(bot, top) == 0)
		return nil;

	/* remove top from list */
	if (bot->aup = top->aup) /* assign = */
		bot->aup->down = bot;
	else
		p->arenalist = bot;

	/* save ptrs to last block in bot, first block in top */
	t = B2PT(A2TB(bot));
	bbot = T2HDR(t);
	btop = A2B(top);
	blockcheck(p, bbot);
	blockcheck(p, btop);

	/* grow bottom arena to encompass top */
	arenasetsize(bot, newsize);

	/* grow bottom block to encompass space between arenas */
	blockgrow(p, bbot, (uchar *)btop - (uchar *)bbot);
	blockcheck(p, bbot);
	return bot;
}

/*
 * poolcheckarena: Validate an entire arena by walking all blocks
 */
void poolcheckarena(Pool *p, Arena *a) {
	Bhdr *b;
	Bhdr *atail;

	atail = A2TB(a);
	for (b = a; b->magic != ARENATAIL_MAGIC && b < atail; b = B2NB(b))
		blockcheck(p, b);
	blockcheck(p, b);
	if (b != atail)
		p->panic(p, "found wrong tail");
}

/*
 * pooldumparena: Dump arena contents for debugging
 */
void pooldumparena(Pool *p, Arena *a) {
	Bhdr *b;

	for (b = a; b->magic != ARENATAIL_MAGIC; b = B2NB(b))
		p->print(p, "(%p %.8lux %lud)", b, b->magic, b->size);
	p->print(p, "\n");
}
