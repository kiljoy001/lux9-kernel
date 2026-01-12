/*
 * pool_debug.c - Debug and validation functions for pool allocator
 *
 * This module provides:
 * - Block validation (blockcheck)
 * - Pool validation (poolcheckl)
 * - Debug dumping (pooldumpl, dumpblock)
 * - Stack logging support
 */

#include <libc.h>
#include <pool.h>
#include <u.h>
#include "pool_types.h"
#include "pool_internal.h"

/*
 * dumpblock: Print block's vital stats
 */
static void dumpblock(Pool *p, Bhdr *b) {
	ulong *dp;
	ulong dsize;
	uchar *cp;

	dp = (ulong *)b;
	p->print(p,
	         "pool %s block %p\nhdr %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux\n",
	         p->name, b, dp[0], dp[1], dp[2], dp[3], dp[4], dp[5], dp[6]);

	dp = (ulong *)B2T(b);
	p->print(p,
	         "tail %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux | %.8lux %.8lux\n",
	         dp[-6], dp[-5], dp[-4], dp[-3], dp[-2], dp[-1], dp[0], dp[1]);

	if (b->magic == ALLOC_MAGIC) {
		dsize = getdsize((Alloc *)b);
		if (dsize >= b->size) /* user data size corrupt */
			return;

		cp = (uchar *)_B2D(b) + dsize;
		p->print(p, "user data ");
		p->print(p, "%.2ux %.2ux %.2ux %.2ux  %.2ux %.2ux %.2ux %.2ux", cp[-8],
		         cp[-7], cp[-6], cp[-5], cp[-4], cp[-3], cp[-2], cp[-1]);
		p->print(p, " | %.2ux %.2ux %.2ux %.2ux  %.2ux %.2ux %.2ux %.2ux\n", cp[0],
		         cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
	}
}

/*
 * printblock: Print a block with a message
 */
static void printblock(Pool *p, Bhdr *b, char *msg) {
	p->print(p, "%s\n", msg);
	dumpblock(p, b);
}

/*
 * panicblock: Print a block and panic
 */
static void panicblock(Pool *p, Bhdr *b, char *msg) {
	p->print(p, "%s\n", msg);
	dumpblock(p, b);
	p->panic(p, "pool panic");
}

/*
 * blockcheck: Ensure a block is consistent with our expectations
 * Should only be called when holding pool lock
 */
void blockcheck(Pool *p, Bhdr *b) {
	Alloc *a;
	Btail *t;
	int i, n;
	uchar *q, *bq, *eq;
	ulong dsize;

	switch (b->magic) {
	default:
		panicblock(p, b, "bad magic");
	case FREE_MAGIC:
	case UNALLOC_MAGIC:
		t = B2T(b);
		if (t->magic0 != TAIL_MAGIC0 || t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic");
		if (T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");
		break;
	case DEAD_MAGIC:
		t = B2T(b);
		if (t->magic0 != TAIL_MAGIC0 || t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic");
		if (T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");
		n = getdsize((Alloc *)b);
		q = _B2D(b);
		q += 8;
		for (i = 8; i < n; i++)
			if (*q++ != 0xDA)
				panicblock(p, b, "dangling pointer write");
		break;
	case ARENA_MAGIC:
		b = A2TB((Arena *)b);
		if (b->magic != ARENATAIL_MAGIC)
			panicblock(p, b, "bad arena size");
		/* fall through */
	case ARENATAIL_MAGIC:
		if (b->size != 0)
			panicblock(p, b, "bad arena tail size");
		break;
	case ALLOC_MAGIC:
		a = (Alloc *)b;
		t = B2T(b);
		dsize = getdsize(a);
		bq = (uchar *)_B2D(a) + dsize;
		eq = (uchar *)t;

		if (t->magic0 != TAIL_MAGIC0) {
			/* if someone wrote exactly one byte over and it was a NUL, we sometimes
			 * only complain. */
			if ((p->flags & POOL_TOLERANCE) && bq == eq && t->magic0 == 0)
				printblock(p, b, "mem user overflow (magic0)");
			else
				panicblock(p, b, "corrupt tail magic0");
		}

		if (t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic1");
		if (T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");

		if (dsize2bsize(p, dsize) > a->size)
			panicblock(p, b, "too much block data");

		if (eq > bq + 4)
			eq = bq + 4;
		for (q = bq; q < eq; q++) {
			if (*q != datamagic[((uintptr)q) % nelem(datamagic)]) {
				if (q == bq && *q == 0 && (p->flags & POOL_TOLERANCE)) {
					printblock(p, b, "mem user overflow");
					continue;
				}
				panicblock(p, b, "mem user overflow");
			}
		}
		break;
	}
}

/*
 * poolcheckl: Validate entire pool (assumes lock held)
 */
void poolcheckl(Pool *p) {
	Arena *a;

	for (a = p->arenalist; a; a = a->down)
		poolcheckarena(p, a);
	if (p->freeroot)
		checktree(p->freeroot, 0, 1 << 30);
}

/*
 * pooldumpl: Dump pool state (assumes lock held)
 */
void pooldumpl(Pool *p) {
	Arena *a;

	p->print(p, "pool %p %s\n", p, p->name);
	for (a = p->arenalist; a; a = a->down)
		pooldumparena(p, a);
}

/*
 * logstack: Log stack trace (placeholder)
 */
void logstack(Pool *p) {
	if (p->logstack)
		p->logstack(p);
}
