/*
 * pool.c - Main pool allocator API
 *
 * This allocator takes blocks from a coarser allocator (p->alloc) and
 * uses them as arenas.
 *
 * An arena is split into a sequence of blocks of variable size.  The
 * blocks begin with a Bhdr that denotes the length (including the Bhdr)
 * of the block.  An arena begins with an Arena header block (Arena,
 * ARENA_MAGIC) and ends with a Bhdr block with magic ARENATAIL_MAGIC and
 * size 0.  Intermediate blocks are either allocated or free.  At the end
 * of each intermediate block is a Btail, which contains information
 * about where the block starts.  This is useful for walking backwards.
 *
 * Free blocks (Free*) have a magic value of FREE_MAGIC in their Bhdr
 * headers.  They are kept in a binary tree (p->freeroot) traversible by
 * walking ->left and ->right.  Each node of the binary tree is a pointer
 * to a circular doubly-linked list (next, prev) of blocks of identical
 * size.  Blocks are added to this ``tree of lists'' by pooladd(), and
 * removed by pooldel().
 *
 * When freed, adjacent blocks are coalesced to create larger blocks when
 * possible.
 *
 * Allocated blocks (Alloc*) have one of two magic values: ALLOC_MAGIC or
 * UNALLOC_MAGIC.  When blocks are released from the pool, they have
 * magic value UNALLOC_MAGIC.  Once the block has been trimmed by trim()
 * and the amount of user-requested data has been recorded in the
 * datasize field of the tail, the magic value is changed to ALLOC_MAGIC.
 * All blocks returned to callers should be of type ALLOC_MAGIC, as
 * should all blocks passed to us by callers.  The amount of data the user
 * asked us for can be found by subtracting the short in tail->datasize
 * from header->size.  Further, the up to at most four bytes between the
 * end of the user-requested data block and the actual Btail structure are
 * marked with a magic value, which is checked to detect user overflow.
 *
 * The arenas returned by p->alloc are kept in a doubly-linked list
 * (p->arenalist) running through the arena headers, sorted by descending
 * base address (prev, next).  When a new arena is allocated, we attempt
 * to merge it with its two neighbors via p->merge.
 */

#include "pool_internal.h"
#include "pool_types.h"
#include <libc.h>
#include <pool.h>
#include <u.h>

/*
 * Utility: freefromfront - Free space from front of block if large enough
 */
static Alloc *freefromfront(Pool *p, Alloc *b, ulong skip) {
  Alloc *bb;

  skip = skip & ~(p->quantum - 1);
  if (skip >= 0x1000 ||
      (skip >= b->size >> 2 && skip >= MINBLOCKSIZE && skip >= p->minblock)) {
    bb = (Alloc *)((uchar *)b + skip);
    bb->magic = UNALLOC_MAGIC;
    blocksetsize(bb, b->size - skip);
    b->magic = UNALLOC_MAGIC;
    blocksetsize(b, skip);
    pooladd(p, b);
    return bb;
  }
  return b;
}

/*
 * Utility: alignptr - Align pointer to specified alignment and offset
 */
static void *alignptr(void *v, ulong align, long offset) {
  char *c;
  ulong off;

  c = v;
  if (align) {
    off = ((ulong)(uintptr)c) % align;
    if (off != offset) {
      offset -= off;
      if (offset < 0)
        offset += align;
      c += offset;
    }
  }
  return c;
}

/*
 * Compaction support
 */
enum {
  FLOATING_MAGIC_INTERNAL =
      0xCBCBCBCB, /* temporarily neither allocated nor in the free tree */
};

/*
 * arenacompact: Compact an arena by shifting all free blocks to the end
 * Assumes pool lock is held
 */
/*@
  @ requires p == \null || \valid(p);
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
static int arenacompact(Pool *p, Arena *a) {
  Bhdr *b, *wb, *eb, *nxt;
  int compacted;

  if (p->move == nil)
    p->panic(p, "don't call me when pool->move is nil\n");

  poolcheckarena(p, a);
  eb = A2TB(a);
  compacted = 0;
  for (b = wb = A2B(a); b && b < eb; b = nxt) {
    nxt = B2NB(b);
    switch (b->magic) {
    case FREE_MAGIC:
      pooldel(p, (Free *)b);
      b->magic = FLOATING_MAGIC;
      break;
    case ALLOC_MAGIC:
      if (wb != b) {
        memmove(wb, b, b->size);
        p->move(_B2D(b), _B2D(wb));
        compacted = 1;
      }
      wb = B2NB(wb);
      break;
    }
  }

  /*
   * the only free data is now at the end of the arena, pointed
   * at by wb.  all we need to do is set its size and get out.
   */
  if (wb < eb) {
    wb->magic = UNALLOC_MAGIC;
    blocksetsize(wb, (uchar *)eb - (uchar *)wb);
    pooladd(p, (Alloc *)wb);
  }

  return compacted;
}

/*
 * poolcompactl: Compact a pool by compacting each individual arena
 */
/*@
  @ requires pool == \null || \valid(pool);
  @ assigns \nothing;
  @*/
int poolcompactl(Pool *pool) {
  Arena *a;
  int compacted;

  if (pool->move == nil || pool->lastcompact == pool->nfree)
    return 0;

  pool->lastcompact = pool->nfree;
  compacted = 0;
  for (a = pool->arenalist; a; a = a->down)
    compacted |= arenacompact(pool, a);
  return compacted;
}

/*
 * poolallocl: Attempt to allocate block to hold dsize user bytes
 * Assumes lock held
 */
void *poolallocl(Pool *p, ulong dsize) {
  ulong bsize;
  Free *fb;
  Alloc *ab;

  if (dsize >= 0x80000000UL) { /* for sanity, overflow */
    werrstr("invalid allocation size");
    return nil;
  }

  bsize = dsize2bsize(p, dsize);

  fb = treelookupgt(p->freeroot, bsize);
  if (fb == nil) {
    poolnewarena(p, bsize2asize(p, bsize));
    if ((fb = treelookupgt(p->freeroot, bsize)) == nil) {
      if (!poolgrowarena(p, bsize)) {
        /* assume poolnewarena failed and set %r */
        return nil;
      }
      fb = treelookupgt(p->freeroot, bsize);
      if (fb == nil)
        return nil;
    }
  }

  ab = trim(p, pooldel(p, fb), dsize);
  p->curalloc += ab->size;
  antagonism { memset(B2D(p, ab), 0xDF, dsize); }
  return B2D(p, ab);
}

/*
 * poolreallocl: Attempt to grow v to ndsize bytes
 * Assumes lock held
 */
void *poolreallocl(Pool *p, void *v, ulong ndsize) {
  Alloc *a;
  Bhdr *left, *right, *newb;
  Btail *t;
  ulong nbsize;
  ulong odsize;
  ulong obsize;
  void *nv;

  if (v == nil) /* for ANSI */
    return poolallocl(p, ndsize);
  if (ndsize == 0) {
    poolfreel(p, v);
    return nil;
  }
  a = D2B(p, v);
  blockcheck(p, a);
  odsize = getdsize(a);
  obsize = a->size;

  /* can reuse the same block? */
  nbsize = dsize2bsize(p, ndsize);
  if (nbsize <= a->size) {
  Returnblock:
    if (v != _B2D(a))
      memmove(_B2D(a), v, odsize);
    a = trim(p, a, ndsize);
    p->curalloc -= obsize;
    p->curalloc += a->size;
    v = B2D(p, a);
    return v;
  }

  /* can merge with surrounding blocks? */
  right = B2NB(a);
  if (right->magic == FREE_MAGIC && a->size + right->size >= nbsize) {
    a = blockmerge(p, a, right);
    goto Returnblock;
  }

  t = B2PT(a);
  left = T2HDR(t);
  if (left->magic == FREE_MAGIC && left->size + a->size >= nbsize) {
    a = blockmerge(p, left, a);
    goto Returnblock;
  }

  if (left->magic == FREE_MAGIC && right->magic == FREE_MAGIC &&
      left->size + a->size + right->size >= nbsize) {
    a = blockmerge(p, blockmerge(p, left, a), right);
    goto Returnblock;
  }

  if ((nv = poolallocl(p, ndsize)) == nil)
    return nil;

  /* maybe the new block is next to us; if so, merge */
  left = T2HDR(B2PT(a));
  right = B2NB(a);
  newb = D2B(p, nv);
  if (left == newb || right == newb) {
    if (left == newb || left->magic == FREE_MAGIC)
      a = blockmerge(p, left, a);
    if (right == newb || right->magic == FREE_MAGIC)
      a = blockmerge(p, a, right);
    // assert(a->size >= nbsize);  /* Disabled: overly strict */
    goto Returnblock;
  }

  /* enough cleverness */
  memmove(nv, v, odsize);
  antagonism { memset((char *)nv + odsize, 0xDE, ndsize - odsize); }
  poolfreel(p, v);
  return nv;
}

/*
 * poolallocalignl: Allocate as described below; assumes pool locked
 */
static void *poolallocalignl(Pool *p, ulong dsize, ulong align, long offset,
                             ulong span) {
  ulong asize;
  void *v;
  char *c;
  ulong *u;
  int skip;
  Alloc *b;

  /*
   * allocate block
   * 	dsize bytes
   *	addr == offset (modulo align)
   *	does not cross span-byte block boundary
   *
   * to satisfy alignment, just allocate an extra
   * align bytes and then shift appropriately.
   *
   * to satisfy span, try once and see if we're
   * lucky.  the second time, allocate 2x asize
   * so that we definitely get one not crossing
   * the boundary.
   */
  if (align) {
    if (offset < 0)
      offset = align - ((-offset) % align);
    offset %= align;
  }
  asize = dsize + align;
  v = poolallocl(p, asize);
  if (v == nil)
    return nil;
  if (span && (uintptr)v / span != ((uintptr)v + asize) / span) {
    /* try again */
    poolfreel(p, v);
    v = poolallocl(p, 2 * asize);
    if (v == nil)
      return nil;
  }

  /*
   * figure out what pointer we want to return
   */
  c = alignptr(v, align, offset);
  if (span && (uintptr)c / span != (uintptr)(c + dsize - 1) / span) {
    c += span - (uintptr)c % span;
    c = alignptr(c, align, offset);
    if ((uintptr)c / span != (uintptr)(c + dsize - 1) / span) {
      poolfreel(p, v);
      werrstr("cannot satisfy dsize %lud span %lud with align %lud+%ld", dsize,
              span, align, offset);
      return nil;
    }
  }
  skip = c - (char *)v;

  /*
   * free up the skip bytes before that pointer
   * or mark it as unavailable.
   */
  b = _D2B(v);
  p->curalloc -= b->size;
  b = freefromfront(p, b, skip);
  v = _B2D(b);
  skip = c - (char *)v;
  if (c > (char *)v) {
    u = v;
    while (c >= (char *)u + sizeof(ulong))
      *u++ = ALIGN_MAGIC;
  }
  trim(p, b, skip + dsize);
  p->curalloc += b->size;
  // assert(D2B(p, c) == b);  /* Disabled: overly strict */
  antagonism { memset(c, 0xDD, dsize); }
  return c;
}

/*
 * poolfreel: Free block obtained from poolalloc; assumes lock held
 */
/*@
  @ requires p == \null || \valid(p);
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void poolfreel(Pool *p, void *v) {
  Alloc *ab;
  Bhdr *back, *fwd;

  if (v == nil) /* for ANSI */
    return;

  ab = D2B(p, v);
  blockcheck(p, ab);

  if (p->flags & POOL_NOREUSE) {
    int n;

    ab->magic = DEAD_MAGIC;
    n = getdsize(ab) - 8;
    if (n > 0)
      memset((uchar *)v + 8, 0xDA, n);
    return;
  }

  p->nfree++;
  p->curalloc -= ab->size;
  back = T2HDR(B2PT(ab));
  if (back->magic == FREE_MAGIC)
    ab = blockmerge(p, back, ab);

  fwd = B2NB(ab);
  if (fwd->magic == FREE_MAGIC)
    ab = blockmerge(p, ab, fwd);

  pooladd(p, ab);
}

/*
 * Public API functions
 */

void *poolalloc(Pool *p, ulong n) {
  void *v;

  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  v = poolallocl(p, n);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  if (p->logstack && (p->flags & POOL_LOGGING))
    p->logstack(p);
  LOG(p, "poolalloc %p %lud = %p\n", p, n, v);
  p->unlock(p);
  return v;
}

void *poolallocalign(Pool *p, ulong n, ulong align, long offset, ulong span) {
  void *v;

  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  v = poolallocalignl(p, n, align, offset, span);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  if (p->logstack && (p->flags & POOL_LOGGING))
    p->logstack(p);
  LOG(p, "poolallocalign %p %lud %lud %ld %lud = %p\n", p, n, align, offset,
      span, v);
  p->unlock(p);
  return v;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
int poolcompact(Pool *p) {
  int rv;

  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  rv = poolcompactl(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  LOG(p, "poolcompact %p\n", p);
  p->unlock(p);
  return rv;
}

void *poolrealloc(Pool *p, void *v, ulong n) {
  void *nv;

  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  nv = poolreallocl(p, v, n);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  if (p->logstack && (p->flags & POOL_LOGGING))
    p->logstack(p);
  LOG(p, "poolrealloc %p %p %ld = %p\n", p, v, n, nv);
  p->unlock(p);
  return nv;
}

/*@
  @ requires p == \null || \valid(p);
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void poolfree(Pool *p, void *v) {
  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  poolfreel(p, v);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  if (p->logstack && (p->flags & POOL_LOGGING))
    p->logstack(p);
  LOG(p, "poolfree %p %p\n", p, v);
  p->unlock(p);
}

/*
 * Return the real size of a block, and let the user use it.
 */
/*@
  @ requires p == \null || \valid(p);
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
ulong poolmsize(Pool *p, void *v) {
  Alloc *b;
  ulong dsize;

  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  if (v == nil) /* consistency with other braindead ANSI-ness */
    dsize = 0;
  else {
    b = D2B(p, v);
    dsize = (b->size & ~(p->quantum - 1)) - sizeof(Bhdr) - sizeof(Btail);
    // assert(dsize >= getdsize(b));  /* Disabled: overly strict */
    blocksetdsize(p, b, dsize);
  }
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  if (p->logstack && (p->flags & POOL_LOGGING))
    p->logstack(p);
  LOG(p, "poolmsize %p %p = %ld\n", p, v, dsize);
  p->unlock(p);
  return dsize;
}

/*@
  @ requires p == \null || \valid(p);
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
int poolisoverlap(Pool *p, void *v, ulong n) {
  Arena *a;

  p->lock(p);
  for (a = p->arenalist; a != nil; a = a->down)
    if ((uchar *)v + n > (uchar *)a && (uchar *)v < (uchar *)a + a->asize)
      break;
  p->unlock(p);
  return a != nil;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void poolreset(Pool *p) {
  Arena *a;

  if (p == nil)
    return;

  p->lock(p);
  paranoia { poolcheckl(p); }
  verbosity { pooldumpl(p); }
  p->cursize = 0;
  p->curfree = 0;
  p->curalloc = 0;
  p->lastcompact = p->nfree = 0;
  p->freeroot = nil;
  a = p->arenalist;
  p->arenalist = nil;
  LOG(p, "poolreset %p\n", p);
  p->unlock(p);

  while (a != nil) {
    Arena *next = a->down;
    ulong asize = a->asize;
    antagonism { memmark(a, 0xFF, asize); }
    if (p->free)
      p->free(a, asize);
    a = next;
  }
}

/*
 * Debugging APIs
 */

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void poolcheck(Pool *p) {
  p->lock(p);
  poolcheckl(p);
  p->unlock(p);
}

/*@
  @ requires p == \null || \valid(p);
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void poolblockcheck(Pool *p, void *v) {
  if (v == nil)
    return;

  p->lock(p);
  blockcheck(p, D2B(p, v));
  p->unlock(p);
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void pooldump(Pool *p) {
  p->lock(p);
  pooldumpl(p);
  p->unlock(p);
}
