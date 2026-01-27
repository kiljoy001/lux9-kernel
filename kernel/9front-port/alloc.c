#include "dat.h"
#include "error.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <pool.h>

static void poolprint(Pool *, char *, ...);
static void ppanic(Pool *, char *, ...);
static void plock(Pool *);
static void punlock(Pool *);

extern void uartputs(char *, int);

#define MALLOC_TRACE_THRESHOLD (4 * 1024)

/*@
  @ requires fmt == \null || \valid(fmt);
  @ assigns \nothing;
  @*/
static void malloctrace(const char *fmt, ...) {
  va_list v;
  char buf[128];
  int n;

  va_start(v, fmt);
  n = vseprint(buf, buf + sizeof buf, fmt, v) - buf;
  va_end(v);
  uartputs(buf, n);
}

typedef struct Private Private;
struct Private {
  Lock lk;
  char msg[256]; /* a rock for messages to be printed at unlock */
};

static Private pmainpriv;
/*
 * Pebble Arena Allocator: Backs Main/Image/Secret pools.
 * Acquires a Black Token for the entire arena.
 */
#include "pageown.h" /* For borrow checker registration */
#include <pebble.h>  /* For Pebble definitions */

/*
 * xalloc_driver: Pebble-aware allocator for drivers.
 * Charges the allocation against the current process's Pebble budget.
 * Returns zeroed memory.
 */
void *xalloc_driver(ulong size) {
  UserCapability cap;
  void *addr;
  PebbleWhite *white;
  PebbleState *ps;

  /*
   * Early-boot bypass: Before the first process (up) exists, Pebble
   * infrastructure (meta pool, ledger) may not be ready.
   * Use raw xalloc for initial pool arena allocations.
   */
  if (up == nil) {
    return xallocz_raw(size, 1);
  }

  ps = pebble_state();
  if (ps == nil)
    return nil;

  /* 1. Reserve budget (WHITE token) */
  white = pebble_issue_white(ps, nil, size);
  if (white == nil)
    return nil;

  /* 2. Allocate RAW memory (breaks recursion loop) */
  addr = xallocz_raw(size, 1);
  if (addr == nil) {
    pebble_return_white(ps, white);
    return nil;
  }

  /* 3. Bind and Verify WHITE */
  white->data_ptr = addr;
  void *black_handle;
  if (pebble_white_verify(white, &black_handle) != 0) {
    /* Verification failed - cleanup resources */
    xfree(addr);
    pebble_return_white(ps, white);
    return nil;
  }

  /* 4. Convert to BLACK token */
  if (pebble_black_alloc(white, addr, size, &cap) < 0) {
    /* pebble_black_alloc frees buf on error if it fails later steps?
     * Checking pebble_black_alloc implementation:
     * It calls borrow_release and xfree on error.
     * So we just return nil.
     */
    return nil;
  }

  /* 5. Register with borrow checker (after userspace init only)
   * This tracks all pool allocations for memory safety verification.
   * Only done after BOOT_USERINIT (state 18) when borrow checker is active.
   */
  extern int current_boot_state;
  if (current_boot_state >= 18) {
    extern uintptr saved_limine_hhdm_offset;
    /* Register each page in the allocation */
    uintptr pa = PADDR(addr);
    uintptr end_pa = pa + size;
      /*@ loop invariant 0 <= page_pa <= end_pa;
    @ loop assigns page_pa;
    @ loop variant end_pa - page_pa;
    @*/
  for (uintptr page_pa = pa & ~(BY2PG - 1); page_pa < end_pa;
         page_pa += BY2PG) {
      uintptr hhdm_va = page_pa + saved_limine_hhdm_offset;
      /* Don't panic on failure - page may already be tracked by another
       * allocation */
      pageown_acquire(up, page_pa, hhdm_va);
    }
  }

  return addr;
}

void *xallocz_driver(ulong size, int zero) {
  /* xalloc_driver always zeroes, so we ignore 'zero' param for now or assert
   * it's 1 */
  return xalloc_driver(size);
}

void *smalloc_driver(ulong size) { return xalloc_driver(size); }

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void xfree_driver(void *p) {
  PebbleState *ps;
  PebbleBlack *pb;

  if (p == nil)
    return;

  if (up == nil) {
    xfree(p);
    return;
  }

  ps = pebble_state();
  if (ps == nil) {
    xfree(p);
    return;
  }

  pb = pebble_lookup_black_by_addr(ps, p);
  if (pb != nil) {
    pebble_black_free(&pb->capability);
  } else {
    /* Fallback for untracked allocations (e.g. early boot leftovers) */
    xfree(p);
  }
}

/* pebble_arena_alloc is now xalloc_driver */
static void *pebble_arena_alloc(ulong size) { return xalloc_driver(size); }

static Pool pmainmem = {
    .name = "Main",
    .maxsize = 32 * 1024 * 1024,
    .minarena = 128 * 1024,
    .quantum = 32,
    .alloc = xalloc, /* REVERTED TO xalloc FOR DEBUGGING */
    .merge = xmerge,
    .flags = POOL_TOLERANCE,

    .lock = plock,
    .unlock = punlock,
    .print = poolprint,
    .panic = ppanic,

    .private = &pmainpriv,
};

static Private pimagpriv;
static Pool pimagmem = {
    .name = "Image",
    .maxsize = 64 * 1024 * 1024,
    .minarena = 2 * 1024 * 1024,
    .quantum = 32,
    .alloc = xalloc, /* REVERTED TO xalloc FOR DEBUGGING */
    .merge = xmerge,
    .flags = 0,

    .lock = plock,
    .unlock = punlock,
    .print = poolprint,
    .panic = ppanic,

    .private = &pimagpriv,
};

static Private psecrpriv;
static Pool psecrmem = {
    .name = "Secrets",
    .maxsize = 16 * 1024 * 1024,
    .minarena = 64 * 1024,
    .quantum = 32,
    .alloc = xalloc, /* REVERTED TO xalloc FOR DEBUGGING */
    .merge = xmerge,
    .flags = POOL_ANTAGONISM,

    .lock = plock,
    .unlock = punlock,
    .print = poolprint,
    .panic = ppanic,

    .private = &psecrpriv,
};

Pool *mainmem = &pmainmem;
Pool *imagmem = &pimagmem;
Pool *secrmem = &psecrmem;

/*
 * Meta Memory Pool: Backed by raw xalloc, used for Pebble/Ledger internals.
 * This breaks the recursion loop: Pebble uses Meta to track Main, Main uses
 * Pebble.
 */
static Private pmetapriv;
static Pool pmetamem = {
    .name = "Meta",
    .maxsize = 16 * 1024 * 1024,
    .minarena = 4096,
    .quantum = 32,
    .alloc = xalloc_raw, /* RAW BACKING - No Borrow Checker tracking */
    .merge = xmerge,
    .flags = 0,

    .lock = plock,
    .unlock = punlock,
    .print = poolprint,
    .panic = ppanic,

    .private = &pmetapriv,
};
Pool *metamem = &pmetamem;

/*@
  @ assigns \nothing;
  @*/
int checkmainmemlockkey(void) {
  Private *pv = &pmainpriv;
  return pv->lk.key;
}

/*
 * because we can't print while we're holding the locks,
 * we have the save the message and print it once we let go.
 */
/*@
  @ requires p == \null || \valid(p);
  @ requires fmt == \null || \valid(fmt);
  @ assigns \nothing;
  @*/
static void poolprint(Pool *p, char *fmt, ...) {
  va_list v;
  Private *pv;

  pv = p->private;
  va_start(v, fmt);
  vseprint(pv->msg + strlen(pv->msg), pv->msg + sizeof pv->msg, fmt, v);
  va_end(v);
}

/*@
  @ requires p == \null || \valid(p);
  @ requires fmt == \null || \valid(fmt);
  @ assigns \nothing;
  @*/
static void ppanic(Pool *p, char *fmt, ...) {
  va_list v;
  Private *pv;
  char msg[sizeof pv->msg];

  pv = p->private;
  va_start(v, fmt);
  vseprint(pv->msg + strlen(pv->msg), pv->msg + sizeof pv->msg, fmt, v);
  va_end(v);
  memmove(msg, pv->msg, sizeof msg);
  iunlock(&pv->lk);
  panic("%s", msg);
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static void plock(Pool *p) {
  Private *pv;

  pv = p->private;
  ilock(&pv->lk);
  pv->lk.pc = getcallerpc(&p);
  pv->msg[0] = 0;
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static void punlock(Pool *p) {
  Private *pv;
  char msg[sizeof pv->msg];

  pv = p->private;
  if (pv->msg[0] == 0) {
    iunlock(&pv->lk);
    return;
  }

  memmove(msg, pv->msg, sizeof msg);
  iunlock(&pv->lk);
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void poolsummary(Pool *p) {
  print("%s max %llud cur %llud free %llud alloc %llud\n", p->name,
        (uvlong)p->maxsize, (uvlong)p->cursize, (uvlong)p->curfree,
        (uvlong)p->curalloc);
}

/*@
  @ assigns \nothing;
  @*/
void mallocsummary(void) {
  poolsummary(mainmem);
  poolsummary(imagmem);
  poolsummary(secrmem);
  poolsummary(metamem);
}

/* everything from here down should be the same in libc, libdebugmalloc, and the
 * kernel */
/* - except the code for malloc(), which alternately doesn't clear or does. */
/* - except the code for smalloc(), which lives only in the kernel. */

/*
 * Npadlong is the number of ulong's to leave at the beginning of
 * each allocated buffer for our own bookkeeping.  We return to the callers
 * a pointer that points immediately after our bookkeeping area.  Incoming
 * pointers must be decremented by that much, and outgoing pointers incremented.
 * The malloc tag is stored at MallocOffset from the beginning of the block,
 * and the realloc tag at ReallocOffset.  The offsets are from the true
 * beginning of the block, not the beginning the caller sees.
 *
 * The extra if(Npadlong != 0) in various places is a hint for the compiler to
 * compile out function calls that would otherwise be no-ops.
 */

/*	non tracing
 *
enum {
        Npadlong = 0,
        MallocOffset = 0,
        ReallocOffset = 0,
};
 *
 */

/* tracing */
enum { Npadlong = 2, MallocOffset = 0, ReallocOffset = 1 };

void *smalloc(ulong size) {
  void *v;

  while ((v = poolalloc(mainmem, size + Npadlong * sizeof(ulong))) == nil) {
    if (!waserror()) {
      resrcwait("no memory for smalloc");
      poperror();
    }
  }
  if (Npadlong) {
    v = (ulong *)v + Npadlong;
    setmalloctag(v, getcallerpc(&size));
  }
  memset(v, 0, size);
  return v;
}

void *malloc(ulong size) {
  void *v;

  if (size >= MALLOC_TRACE_THRESHOLD)
    malloctrace("malloc: request size=%lud\n", size);
  v = poolalloc(mainmem, size + Npadlong * sizeof(ulong));
  if (size >= MALLOC_TRACE_THRESHOLD)
    malloctrace("malloc: poolalloc returned raw=%p\n", v);
  if (v == nil)
    return nil;
  if (Npadlong) {
    v = (ulong *)v + Npadlong;
    setmalloctag(v, getcallerpc(&size));
    setrealloctag(v, 0);
  }
  memset(v, 0, size);
  if (size >= MALLOC_TRACE_THRESHOLD)
    malloctrace("malloc: returning %p\n", v);
  return v;
}

void *mallocz(ulong size, int clr) {
  void *v;

  v = poolalloc(mainmem, size + Npadlong * sizeof(ulong));
  if (v == nil)
    return nil;
  if (Npadlong) {
    v = (ulong *)v + Npadlong;
    setmalloctag(v, getcallerpc(&size));
    setrealloctag(v, 0);
  }
  if (clr)
    memset(v, 0, size);
  return v;
}

void *mallocalign(ulong size, ulong align, long offset, ulong span) {
  void *v;

  v = poolallocalign(mainmem, size + Npadlong * sizeof(ulong), align,
                     offset - Npadlong * sizeof(ulong), span);
  if (v == nil)
    return nil;
  if (Npadlong) {
    v = (ulong *)v + Npadlong;
    setmalloctag(v, getcallerpc(&size));
    setrealloctag(v, 0);
  }
  memset(v, 0, size);
  return v;
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void free(void *v) {
  if (v != nil)
    poolfree(mainmem, (ulong *)v - Npadlong);
}

void *realloc(void *v, ulong size) {
  void *nv;

  if (v != nil)
    v = (ulong *)v - Npadlong;
  if (Npadlong && size != 0)
    size += Npadlong * sizeof(ulong);
  nv = poolrealloc(mainmem, v, size);
  if (nv != nil) {
    nv = (ulong *)nv + Npadlong;
    setrealloctag(nv, getcallerpc(&v));
    if (v == nil)
      setmalloctag(nv, getcallerpc(&v));
  }
  return nv;
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
ulong msize(void *v) {
  return poolmsize(mainmem, (ulong *)v - Npadlong) - Npadlong * sizeof(ulong);
}

/* secret memory, used to back cryptographic keys and cipher states */
void *secalloc(ulong size) {
  void *v;

  while ((v = poolalloc(secrmem, size + Npadlong * sizeof(ulong))) == nil) {
    if (!waserror()) {
      resrcwait("no memory for secalloc");
      poperror();
    }
  }
  if (Npadlong) {
    v = (ulong *)v + Npadlong;
    setmalloctag(v, getcallerpc(&size));
    setrealloctag(v, 0);
  }
  memset(v, 0, size);
  return v;
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void secfree(void *v) {
  if (v != nil)
    poolfree(secrmem, (ulong *)v - Npadlong);
}

/*
 * Pebble Meta Allocator
 * Used by Pebble internals to allocate tracking structures.
 * Backed by RAW 'metamem' pool to avoid recursion.
 */
void *pebble_meta_alloc(ulong size) {
  void *v;
  while ((v = poolalloc(metamem, size + Npadlong * sizeof(ulong))) == nil) {
    if (!waserror()) {
      resrcwait("no memory for pebble_meta_alloc");
      poperror();
    }
  }
  if (Npadlong) {
    v = (ulong *)v + Npadlong;
    setmalloctag(v, getcallerpc(&size));
    setrealloctag(v, 0);
  }
  memset(v, 0, size);
  return v;
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void pebble_meta_free(void *v) {
  if (v != nil)
    poolfree(metamem, (ulong *)v - Npadlong);
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void setmalloctag(void *v, uintptr pc) {
  USED(v, pc);
  if (Npadlong <= MallocOffset || v == nil)
    return;
  ((ulong *)v)[-Npadlong + MallocOffset] = (ulong)pc;
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
void setrealloctag(void *v, uintptr pc) {
  USED(v, pc);
  if (Npadlong <= ReallocOffset || v == nil)
    return;
  ((ulong *)v)[-Npadlong + ReallocOffset] = (ulong)pc;
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
uintptr getmalloctag(void *v) {
  USED(v);
  if (Npadlong <= MallocOffset)
    return ~0;
  return (int)((ulong *)v)[-Npadlong + MallocOffset];
}

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
uintptr getrealloctag(void *v) {
  USED(v);
  if (Npadlong <= ReallocOffset)
    return ~0;
  return (int)((ulong *)v)[-Npadlong + ReallocOffset];
}
