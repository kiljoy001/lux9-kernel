/*
 * Setup 9P exchange page for a user process - POOL-BASED ALLOCATION.
 *
 * EXCHANGE POOL MODEL:
 * ====================
 * Each process gets its own physical page allocated from the global
 * exchange pool. This provides:
 * 1. Complete parent/child isolation - no shared pages after fork
 * 2. Capability-based access control via BlindLedger
 * 3. Proper resource tracking and cleanup on process exit
 *
 * The page is mapped at the fixed virtual address EXCHANGE_PAGE_ADDR
 * for userspace compatibility, but backed by a unique physical page
 * from the pool.
 */
#include "9p_router.h"
#include "dat.h"
#include "exchange_pool.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

int proc_setup_p9page(Proc *p) {
  print("DEBUG:proc_setup_p9page ENTRY p=%p\n", p);
  print("DEBUG:proc_setup_p9page reading p->kp...\n");
  if (p->kp)
    return 0; /* Kernel processes don't need this */

  print("DEBUG:proc_setup_p9page reading p->pid (offset in struct)...\n");
  ulong test_pid = p->pid;
  print("DEBUG:proc_setup_p9page p->pid=%lud\n", test_pid);

  if (p->seg[P9SEG] != nil)
    return 0; /* Already set up */

  /*
   * POOL-BASED ALLOCATION:
   * Allocate a physical page from the global exchange pool.
   * This ensures each process has its own isolated page.
   */
  UserCapability cap;
  BlindLedgerEntry entry;
  uintptr pa = 0;
  void *kva = nil; /* Kernel virtual address for p9page */

  if (global_pool != nil) {
    PoolError perr = global_pool_alloc_page(p, &cap);
    if (perr == POOL_OK) {
      /* Verify capability and get kernel virtual address */
      if (ledger_verify(&cap, &entry) == BLIND_LEDGER_OK) {
        /* NOTE: BlindLedger stores KADDR, not physical address!
         * The pool was allocated via xspanalloc() which returns KADDR.
         */
        kva = (void *)entry.physical_address; /* This is actually KADDR */
        pa = PADDR(kva); /* Convert to real physical address for MMU */
        print("proc_setup_p9page: pid=%lud pool page kva=%p pa=%#p\n", p->pid,
              kva, (void *)pa);
      } else {
        print("proc_setup_p9page: pid=%lud cap verify failed, fallback\n",
              p->pid);
        pa = 0;
      }
    } else {
      print("proc_setup_p9page: pid=%lud pool alloc failed (%d), fallback\n",
            p->pid, perr);
    }
  } else {
    print("proc_setup_p9page: global_pool not initialized, using fallback\n");
  }

  /*
   * FALLBACK: If pool allocation fails, allocate a raw page.
   * This shouldn't happen in normal operation but prevents boot failure.
   */
  if (pa == 0) {
    void *page = xspanalloc(BY2PG, BY2PG, 0);
    if (page == nil) {
      print("proc_setup_p9page: fallback xspanalloc failed\n");
      return -1;
    }
    kva = page; /* xspanalloc returns KADDR */
    pa = PADDR(page);
    print("proc_setup_p9page: pid=%lud fallback page kva=%p pa=%#p\n", p->pid,
          kva, (void *)pa);
  }

  /* Zero the page to prevent information leakage (use KADDR) */
  memset(kva, 0, BY2PG);

  /*
   * Create segment for the exchange page at fixed virtual address.
   * Physical page is already allocated (from pool or fallback).
   */
  Segment *s = newseg(SG_PHYSICAL, EXCHANGE_PAGE_ADDR, 1);
  if (s == nil) {
    print("proc_setup_p9page: newseg failed\n");
    /* TODO: Return page to pool on failure */
    return -1;
  }

  /*
   * Allocate Physseg and configure with our pool-allocated physical page.
   */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("proc_setup_p9page: malloc failed for pseg\n");
    putseg(s);
    return -1;
  }

  /* Configure physical segment with real physical address */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "9pexchange";
  s->pseg->pa = pa; /* Physical address from pool */
  s->pseg->size = BY2PG;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Clear any conflicting segments in the user's address space */
  for (int i = 0; i < NSEG; i++) {
    Segment *oseg = p->seg[i];
    if (oseg == nil)
      continue;
    if (EXCHANGE_PAGE_ADDR >= oseg->base && EXCHANGE_PAGE_ADDR < oseg->top) {
      print("proc_setup_p9page: clearing conflicting seg[%d] at base=%#p\n", i,
            oseg->base);
      p->seg[i] = nil;
      putseg(oseg);
      print("DEBUG:proc_setup_p9page post-clear-seg[%d] p->pid=%lud\n", i,
            p->pid);
    }
  }
  print("DEBUG:proc_setup_p9page post-loop p->pid=%lud\n", p->pid);

  /* Assign segment to process at P9SEG slot */
  p->seg[P9SEG] = s;
  print("DEBUG:proc_setup_p9page post-assignment p->pid=%lud\n", p->pid);

  /* Store kernel virtual address for p9_handle_doorbell */
  p->p9page = kva;

  print("proc_setup_p9page: pid=%lud seg=%p base=%#p pa=%#p kva=%p\n", p->pid,
        s, (void *)s->base, (void *)pa, p->p9page);

  return 0;
}
