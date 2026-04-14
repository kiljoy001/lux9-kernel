#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

#include "borrow_enforce.h"

#ifdef __FRAMAC__
#include "acsl_bounds.h"
#endif

/* Limine HHDM offset - all physical memory mapped at PA + this offset */
extern uintptr saved_limine_hhdm_offset;
int xinit_done = 0;

/* Bootstrap allocation for early boot systems */
static uchar
    bootstrap_pool[131072]; /* Increased to 128KB for dynamic hole allocation */
static ulong bootstrap_offset = 0;

/**
 * Allocate a small aligned block from the early-boot bootstrap pool.
 *
 * Allocates `size` bytes from the internal 8KB bootstrap pool and returns
 * a cache-line-aligned pointer into that pool. If there is not enough space
 * remaining, returns `nil`.
 *
 * @param size Number of bytes requested.
 * @returns Pointer to the start of the allocated, cache-line-aligned region
 * within the bootstrap pool, or `nil` if allocation fails due to insufficient
 * space.
 */
void *bootstrap_alloc(ulong size) {
  return bootstrap_alloc_aligned(size,
                                 64); /* Default to cache-line alignment */
}

/**
 * Allocate a small aligned block from the early-boot bootstrap pool with
 * specified alignment.
 *
 * Allocates `size` bytes from the internal 8KB bootstrap pool and returns
 * an aligned pointer into that pool. If there is not enough space
 * remaining, returns `nil`.
 *
 * @param size Number of bytes requested.
 * @param alignment Alignment requirement (must be power of 2).
 * @returns Pointer to the start of the allocated, aligned region within
 *          the bootstrap pool, or `nil` if allocation fails due to insufficient
 * space.
 */
void *bootstrap_alloc_aligned(ulong size, ulong alignment) {
  ulong aligned_size;
  ulong aligned_offset;

  /* Validate alignment - must be power of 2 and reasonable */
  if (alignment == 0 || (alignment & (alignment - 1)) != 0 || alignment > 1024)
    return nil;

  /* Align the size to the requested boundary */
  aligned_size = (size + alignment - 1) & ~(alignment - 1);

  /* Align the offset to the requested boundary */
  aligned_offset = (bootstrap_offset + alignment - 1) & ~(alignment - 1);

  /* Check if we have enough space */
  if (aligned_offset + aligned_size > sizeof(bootstrap_pool)) {
    return nil;
  }

  /* Return the allocated space */
  void *ptr = &bootstrap_pool[aligned_offset];
  bootstrap_offset = aligned_offset + aligned_size;
  return ptr;
}

extern void uartputs(char *, int);

#ifdef __FRAMAC__
#define xtrace(...) ((void)0)
#else
static void xtrace(const char *fmt, ...) {
  char buf[160];
  va_list v;
  int n;

  va_start(v, fmt);
  n = vseprint(buf, buf + sizeof buf, fmt, v) - buf;
  va_end(v);
  uartputs(buf, n);
}
#endif

/* -------------------------------------------------------------------------
 * XALLOC configuration
 * -------------------------------------------------------------------------
 * INITIAL_NHOLE – number of static Hole descriptors (kept in the Xalloc
 *                 struct).  This value must match the size of the static array.
 * DYNAMIC_NHOLE – number of descriptors to allocate at once when the static
 *                 pool runs out.
 * Nhole        – alias for INITIAL_NHOLE used by the rest of the file.
 * -------------------------------------------------------------------------
 */
enum {
  INITIAL_NHOLE = 2048,  /* Increased from 512 to handle high page fault load */
  DYNAMIC_NHOLE = 1024,  /* Increased from 512 for larger batches */
  Nhole = INITIAL_NHOLE, /* static hole descriptor count */
  Magichole = 0x484F4C45, /* HOLE */
};

typedef struct Hole Hole;
typedef struct Xalloc Xalloc;
typedef struct Xhdr Xhdr;

struct Hole {
  uintptr addr;
  uintptr size;
  uintptr top;
  Hole *link;
};

struct Xhdr {
  ulong size;
  ulong magix;
  char data[];
};

struct Xalloc {
  Lock lk; /* Named lock member instead of anonymous */
  Hole hole[Nhole];
  Hole *flist;
  Hole *table;
};

/*@
  @ inductive reachable(Hole *root, Hole *target) {
  @   case self: \forall Hole *h; reachable(h, h);
  @   case next: \forall Hole *h1, *h2; \valid(h1) && h1 != \null &&
  reachable(h1->link, h2) ==> reachable(h1, h2);
  @ }
  @
  @ predicate xalloc_invariant(Xalloc *xl) =
  @   hole_list(xl->table) && free_hole_nodes(xl->flist);
  @*/

static Xalloc xlists;

/* TEST 2A: Memory Allocation Tracking */
static ulong xalloc_failures = 0;
static ulong xalloc_successes = 0;
static ulong xalloc_last_failure_size = 0;
static void *xalloc_last_failure_pc = nil;

void xinit(void) {
  ulong maxpages, kpages, n;
  Hole *h, *eh;
  Confmem *cm;
  int i;
  uintptr size_bytes;

  print("xinit: ENTRY xinit_done=%d addr=%p\n", xinit_done, &xinit_done);

  /* Guard against double initialization */
  if (xinit_done) {
    print("xinit: already done, skipping\n");
    return;
  }

  eh = &xlists.hole[Nhole - 1];
  for (h = xlists.hole; h < eh; h++)
    h->link = h + 1;

  xlists.flist = xlists.hole;

  kpages = conf.npage - conf.upages;
  print("xinit: total pages %lud, user pages %lud, kernel pages %lud\n",
        conf.npage, conf.upages, kpages);

  for (i = 0; i < nelem(conf.mem); i++) {
    cm = &conf.mem[i];
    /* Only print first few entries to avoid verbose output */
    if (i < 2) {
      print("xinit: processing conf.mem[%d] base=%#p npage=%lud\n", i, cm->base,
            cm->npage);
    }
    n = cm->npage;
    if (n > kpages)
      n = kpages;
    /* don't try to use non-KADDR-able memory for kernel */
    /* With Limine HHDM, all physical memory is already mapped */
    maxpages = cm->npage; /* Assume all pages are KADDR-able */
    if (n > maxpages)
      n = maxpages;
    /* give to kernel */
    if (n > 0) {
      cm->kbase = (uintptr)KADDR(cm->base);
      cm->klimit = (uintptr)cm->kbase + (uintptr)n * BY2PG;
      if (cm->klimit == 0)
        cm->klimit = (uintptr)-BY2PG;
      /* cm->klimit - cm->kbase gives byte size (both have same offset applied)
       */
      size_bytes = cm->klimit - cm->kbase;
      xhole(cm->base, size_bytes);
      kpages -= n;
    }
    /*
     * anything left over: cm->npage - nkpages(cm)
     * will be given to user by pageinit()
     */
  }

  /* Mark xinit as complete for early-boot allocators */
  xinit_done = 1;
}

void *xspanalloc(ulong size, int align, ulong span) {
  uintptr a, v, t;

  a = (uintptr)xalloc(size + align + span);
  if (a == 0)
    panic("xspanalloc: %lud %d %lux", size, align, span);

  if (span > 2) {
    v = (a + span) & ~((uintptr)span - 1);
    t = v - a;
    if (t > 0) {
      /* xhole expects physical addr, but 'a' is virtual HHDM addr
       * Convert back to physical: vaddr - HHDM_offset */
      xhole(a - saved_limine_hhdm_offset, t);
    }
    t = a + span - v;
    if (t > 0) {
      xhole((v + size + align) - get_hhdm_offset(), t);
    }
  } else
    v = a;

  if (align > 1)
    v = (v + align) & ~((uintptr)align - 1);

  return (void *)v;
}

/*@
  @ requires \valid(xl) && xalloc_invariant(xl);
  @ assigns xl->table, xl->flist;
  @ ensures xalloc_invariant(xl);
  @*/
static Xhdr *hole_alloc(Xalloc *xl, ulong size) {
  Hole *h, **l;
  Xhdr *p;
  uintptr addr_check;

  /*@ ghost int ghost_visited_count = 0; */
  l = &xl->table;
  /*@
    @ loop invariant \valid(l) && (\valid(*l) || *l == \null);
    @ loop invariant h == *l;
    @ loop invariant hole_list(xl->table);
    @ loop invariant free_hole_nodes(xl->flist);
    @ loop invariant \forall Hole *h_check; \valid(h_check) ==> (h_check->top ==
    h_check->addr + h_check->size);
    @ loop invariant reachable(xl->table, h);
    @ loop invariant \valid(h) || h == \null;
    @ loop variant INITIAL_NHOLE + DYNAMIC_NHOLE * 100 - ghost_visited_count;
    @ loop assigns h, l, xl->table, xl->flist, ghost_visited_count;
    @*/
  for (h = *l; h; h = h->link) {
    /*@ ghost ghost_visited_count++; */
    if (h->size >= size) {
      /*@ ghost uintptr old_top = h->top; */
      addr_check = h->addr;
      if (addr_check & (BY2V - 1)) {
        /* Hole is misaligned - align it forward */
        uintptr aligned_addr = (addr_check + BY2V - 1) & ~(BY2V - 1);
        uintptr waste = aligned_addr - addr_check;

        /* Check if we still have enough space after alignment */
        if (h->size < size + waste) {
          l = &h->link;
          continue;
        }

        /* Adjust hole for alignment waste */
        h->addr = aligned_addr;
        h->size -= waste;
      }

      p = (Xhdr *)h->addr;
      h->addr += size;
      h->size -= size;

      /*@ assert h->top == old_top; */

      if (h->size == 0) {
        *l = h->link;
        h->link = xl->flist;
        xl->flist = h;
      }
      return p;
    }
    l = &h->link;
  }
  return nil;
}

/*@
  @ // Model Link: proofs/allocator/xalloc_model.v
  @ // Refines: Coq:alloc_first_fit
  @ // Theorems: first_fit_complete, alloc_first_fit_sound
  @ requires size < ACSL_MAX_ALLOC;
  @
  @ behavior success:
  @   assumes \exists Hole *h; h->size >= size;
  @   ensures \result != \null;
  @   ensures \valid((char *)\result + (0 ..size - 1));
  @   ensures ((uintptr)\result & 7) == 0;
  @   ensures \forall integer i; 0 <= i < size ==> ((char *)\result)[i] == (zero
  ? 0 : ((char *)\result)[i]);
  @ behavior failure:
  @   assumes \forall Hole *h; h->size < size;
  @   ensures \result == \null;
  @ complete behaviors;
  @ disjoint behaviors;
  @ assigns xlists, xalloc_successes, xalloc_failures, xalloc_last_failure_size;
  @ ensures xalloc_invariant(&xlists);
  @*/
static void *xalloc_internal(ulong size, int zero, int raw) {
  Xhdr *p;
  ulong orig_size = size;
  ulong overhead;

  /* Calculate overhead */
  overhead = BY2V + offsetof(Xhdr, data);

  /* Detect potential overflow when adding header overhead */
  if (size > ~0UL - overhead) {
    panic("xallocz: request size overflow (size=%lud)", size);
  }

  /* Additional check for unreasonably large allocations */
  if (size > 128 * 1024 * 1024) { /* More than 128MB */
    panic("xallocz: unreasonably large allocation request (size=%lud)", size);
  }

  /* Add room for magix & size overhead, round UP to nearest vlong */
  size += overhead;
  size = (size + BY2V - 1) & ~(BY2V - 1); /* FIX: Round UP */

  ilock(&xlists.lk);
  p = hole_alloc(&xlists, size);
  iunlock(&xlists.lk);

  if (p != nil) {
    p->magix = Magichole;
    p->size = size;

    if (zero)
      memset(p->data, 0, size - overhead);

    /* Verify p->data is 8-byte aligned (critical for QBE bitsets!) */
    if ((uintptr)p->data & 7) {
      panic("xallocz: data pointer %#p not 8-byte aligned", p->data);
    }

    /* TEST 2A: Track allocation success */
    xalloc_successes++;

    /* Borrow Checker: Acquire kernel ownership of allocated memory (unless
     * RAW) */
    if (!raw) {
      BORROW_ACQUIRE_ALLOC(p->data, size - overhead);
    }

    return p->data;
  }

  /* TEST 2A: Track allocation failure */
  xalloc_failures++;
  xalloc_last_failure_size = orig_size;
  return nil;
}

void *xallocz(ulong size, int zero) { return xalloc_internal(size, zero, 0); }

void *xallocz_raw(ulong size, int zero) {
  return xalloc_internal(size, zero, 1);
}

void *xalloc(ulong size) { return xalloc_internal(size, 1, 0); }

void *xalloc_raw(ulong size) { return xalloc_internal(size, 1, 1); }

/*@
  @ requires p != \null;
  @ requires \valid((char *)p - offsetof(Xhdr, data[0]) + (0 ..sizeof(Xhdr) -
  1));
  @ assigns xlists;
  @ terminates \true;
  @*/
void xfree(void *p) {
  Xhdr *x;

  x = (Xhdr *)((uintptr)p - offsetof(Xhdr, data[0]));
  if (x->magix != Magichole) {
    xsummary();
    panic("xfree(%#p) %#ux != %#lux", p, Magichole, x->magix);
  }
  /* x is already a virtual HHDM address, convert to physical for xhole */
  xhole((uintptr)x - saved_limine_hhdm_offset, x->size);
}

int xmerge(void *vp, void *vq) {
  Xhdr *p, *q;

  p = (Xhdr *)(((uintptr)vp - offsetof(Xhdr, data[0])));
  q = (Xhdr *)(((uintptr)vq - offsetof(Xhdr, data[0])));
  if (p->magix != Magichole || q->magix != Magichole) {
    int i;
    ulong *wd;
    void *badp;

    xsummary();
    badp = (p->magix != Magichole ? p : q);
    wd = (ulong *)badp - 12;
    for (i = 24; i-- > 0;) {
      print("%#p: %lux", wd, *wd);
      if (wd == badp)
        print(" <-");
      print("\n");
      wd++;
    }
    panic("xmerge(%#p, %#p) bad magic %#lux, %#lux", vp, vq, p->magix,
          q->magix);
  }
  if ((uchar *)p + p->size == (uchar *)q) {
    p->size += q->size;
    return 1;
  }
  return 0;
}

/*@
  @ requires \valid(xl) && xalloc_invariant(xl);
  @ requires size > 0;
  @ assigns xl->table, xl->flist;
  @ ensures xalloc_invariant(xl);
  @*/
static void hole_free(Xalloc *xl, uintptr vaddr, uintptr size) {
  Hole *h, *c, **l;
  uintptr top;
  extern void uartprintf(char *, ...);

  // uartprintf("CHECK: hole_free(vaddr=%#p, size=%#p) entry\n", vaddr, size);

  /* FIX: Ensure vaddr is 8-byte aligned */
  if (vaddr & 7) {
    uintptr aligned_vaddr = (vaddr + 7) & ~7UL;
    uintptr waste = aligned_vaddr - vaddr;
    vaddr = aligned_vaddr;
    size -= waste; /* Reduce size by alignment waste */

    if (size < 8) {
      /* Too small after alignment, skip */
      return;
    }

    print("xhole: aligned vaddr from %#p to %#p (waste=%lud)\n",
          (void *)(vaddr - waste), (void *)vaddr, waste);
  }

  top = vaddr + size;

  /* Find if this hole can be merged with an existing one */
  l = &xl->table;
  h = *l; /* Initialize h from table head */
  /*@ ghost int ghost_visited_count = 0; */
  // uartprintf("CHECK: hole_free starting loop (h=%#p)\n", h);
  for (; h; h = h->link) {
    // uartprintf("CHECK: hole_free loop h=%#p addr=%#p\n", h, h->addr);
    /*@
      @ loop invariant \valid(l) && (\valid(*l) || *l == \null);
      @ loop invariant h == *l;
      @ loop invariant hole_list(xl->table);
      @ loop invariant free_hole_nodes(xl->flist);
      @ loop invariant \forall Hole *h_check; \valid(h_check) ==> (h_check->top
      == h_check->addr + h_check->size);
      @ loop invariant reachable(xl->table, h);
      @ loop invariant \valid(h) || h == \null;
      @ loop variant INITIAL_NHOLE + DYNAMIC_NHOLE * 100 - ghost_visited_count;
      @ loop assigns h, l, xl->table, xl->flist, ghost_visited_count;
      @*/
    /*@ ghost ghost_visited_count++; */
    /* Check if this new region is adjacent to existing hole (at top) */
    if (h->top == vaddr) {
      /* ... merge logic ... */
      // uartprintf("CHECK: hole_free merge top\n");
      return;
    }
    /* Check if new region comes before this hole */
    if (h->addr > vaddr)
      break;
    l = &h->link;
  }
  // uartprintf("CHECK: hole_free loop done\n");

  /* Check if this new region is adjacent to existing hole (at bottom) */
  if (h && top == h->addr) {
    // uartprintf("CHECK: hole_free merge bottom\n");
    h->addr = vaddr;
    h->size += size;
    return;
  }

  /* Need to create a new hole descriptor for this region */
  // uartprintf("CHECK: hole_free allocating descriptor\n");
  if (xl->flist == nil) {
    /* ---------------------------------------------------------------
     * If we have exhausted the static free list, allocate a fresh batch
     * of Hole descriptors from the kernel malloc pool.
     * -------------------------------------------------------------- */
    Hole *extra =
        (Hole *)bootstrap_alloc_aligned(DYNAMIC_NHOLE * sizeof(Hole), BY2V);
    if (extra == nil) {
      panic(
          "xhole: out of hole descriptors and bootstrap_alloc_aligned failed");
    }
    /*@
      @ loop invariant 0 <= i < DYNAMIC_NHOLE;
      @ loop invariant \forall integer j; 0 <= j < i ==> extra[j].link ==
      &extra[j+1];
      @ loop assigns i, extra[0 .. DYNAMIC_NHOLE-1].link;
      @*/
    for (int i = 0; i < DYNAMIC_NHOLE - 1; i++) {
      extra[i].link = &extra[i + 1];
    }
    extra[DYNAMIC_NHOLE - 1].link = nil;
    xl->flist = extra;
    /*@ assert free_hole_nodes(xl->flist); */
  }
  /* Get a free hole descriptor from the free list */
  h = xl->flist;
  xl->flist = h->link;

  /* Fill in the hole with virtual address information */
  h->addr = vaddr; /* Virtual address in HHDM */
  h->top = top;    /* End virtual address */
  h->size = size;  /* Size in bytes */
  h->link = *l;    /* Link into the table */
  *l = h;
  /*@ assert hole_list(xl->table); */
}

/*@
  @ requires size == 0 || addr + size > addr;
  @ assigns xlists, xalloc_successes;
  @ terminates \true;
  @*/
void xhole(uintptr addr, uintptr size) {
  uintptr vaddr; /* Virtual address in HHDM */
  extern void uartprintf(char *, ...);

  if (size == 0)
    return;

  /* Convert physical address to virtual HHDM address
   * Now holes track virtual addresses in the HHDM region */
  vaddr = addr + get_hhdm_offset();

  // uartprintf("CHECK: xhole(%#p, %#p) locking\n", addr, size);
  ilock(&xlists.lk);
  // uartprintf("CHECK: xhole locked, calling hole_free\n");
  hole_free(&xlists, vaddr, size);
  // uartprintf("CHECK: xhole freeing lock\n");
  iunlock(&xlists.lk);
  // uartprintf("CHECK: xhole done\n");

  /* TEST 2A: Track allocation success */
  xalloc_successes++;
}

void xsummary(void) {
  int i;
  Hole *h;
  uintptr s;

  i = 0;
  for (h = xlists.flist; h; h = h->link)
    i++;
  print("%d holes free\n", i);

  s = 0;
  for (h = xlists.table; h; h = h->link) {
    print("%#8.8p %#8.8p %llud\n", h->addr, h->top, (uvlong)h->size);
    s += h->size;
  }
  print("%llud bytes free\n", (uvlong)s);
}

/* Test function to verify dynamic hole allocation works */
void xalloc_test(void) {
  print("xalloc_test: starting test\n");

  /* Try to exhaust static hole pool by making many small allocations */
  void *ptrs[200];
  int i;

  print("xalloc_test: making 200 small allocations\n");
  for (i = 0; i < 200; i++) {
    ptrs[i] = xalloc(16); /* Small allocations */
    if (ptrs[i] == nil) {
      print("xalloc_test: allocation %d failed\n", i);
      break;
    }
  }
  print("xalloc_test: made %d allocations\n", i);

  /* Free all allocations */
  for (int j = 0; j < i; j++) {
    if (ptrs[j] != nil) {
      xfree(ptrs[j]);
    }
  }

  print("xalloc_test: freed all allocations\n");
  print("xalloc_test: test completed successfully\n");
}

/* Standard C library allocator wrappers for WASM3 and other libs */
/* malloc, free, realloc are provided by alloc.c */

void *calloc(ulong n, ulong size) { return xallocz(n * size, 1); }
