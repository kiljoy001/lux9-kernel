/*
 * Setup 9P exchange page for a user process - LAZY ALLOCATION MODEL.
 *
 * DEMAND-PAGED EXCHANGE PAGE:
 * ===========================
 * Instead of pre-allocating a physical page, we create only the segment
 * descriptor. The physical page is allocated on first access (page fault).
 *
 * Benefits:
 * 1. Fork isolation: Child never inherits parent's PTE - gets fresh page on
 * fault
 * 2. No MMU aliasing: Each process faults and allocates independently
 * 3. No PTE surgery: Standard fault-based allocation like heap/stack
 * 4. Reuses pool infrastructure: Can leverage devexchange.c allocators
 *
 * The segment is marked SG_PHYSICAL with no physical backing initially.
 * fault.c's fixfault() handles the first access and allocates a fresh page.
 */
#include "9p_router.h"
#include "dat.h"
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
   * Create a DEMAND-PAGED segment for the exchange page.
   * Physical page will be allocated on first fault.
   * This avoids fork/COW aliasing issues entirely.
   */
  Segment *s = newseg(SG_PHYSICAL, EXCHANGE_PAGE_ADDR, 1);
  if (s == nil) {
    print("proc_setup_p9page: newseg failed\n");
    return -1;
  }

  /*
   * CRITICAL: Allocate Physseg tracker but with NO physical address yet.
   * This tells fixfault() that physical allocation is needed.
   */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("proc_setup_p9page: malloc failed for pseg\n");
    putseg(s);
    return -1;
  }

  /* Configure physical segment as demand-paged (pa=0 signals not allocated) */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "9pexchange";
  s->pseg->pa = 0; /* NO physical address - allocate on fault */
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

  print("proc_setup_p9page: pid=%lud seg=%p base=%#p (DEMAND-PAGED - no "
        "physical page yet)\n",
        p->pid, s, (void *)s->base);

  /* NO physical allocation, NO pageown_acquire - happens on fault */
  return 0;
}
