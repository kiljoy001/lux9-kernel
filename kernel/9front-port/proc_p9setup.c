#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

/*
 * Setup 9P exchange page for a user process.
 *
 * SINGLE 4KB PAGE MODEL:
 * ======================
 * Each process gets ONE 4KB page for syscall communication.
 * Ownership flips between process and kernel via borrow checker:
 *
 *   Process owns → writes request → syscall → Kernel owns
 *   Kernel reads request, writes reply → returns → Process owns
 *
 * This is more memory-efficient (4KB vs 8KB per process) and
 * provides stronger isolation (only one owner can access at a time).
 *
 * The borrow_transfer() calls happen in p9_handle_doorbell().
 */
int proc_setup_p9page(Proc *p) {
  if (p->kp)
    return 0; /* Kernel processes don't need this */

  if (p->p9page != nil)
    return 0; /* Already allocated */

  /* Allocate SINGLE 4KB page for exchange (ownership-flip model) */
  p->p9page = mallocalign(BY2PG, BY2PG, 0, 0);
  if (p->p9page == nil) {
    print("proc_setup_p9page: mallocalign failed\n");
    return -1;
  }
  memset(p->p9page, 0, BY2PG);

  /* Create a physical segment for the exchange page (1 page now) */
  Segment *s = newseg(SG_PHYSICAL, EXCHANGE_PAGE_ADDR, 1);
  if (s == nil) {
    print("proc_setup_p9page: newseg failed\n");
    return -1;
  }

  /* Allocate Physseg tracker for this segment */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("proc_setup_p9page: malloc failed for pseg\n");
    putseg(s);
    return -1;
  }

  /* Configure physical segment backing */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "9pexchange";
  s->pseg->pa = PADDR(p->p9page);
  s->pseg->size = BY2PG; /* Single page */
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Assign segment to process at P9SEG slot */
  p->seg[P9SEG] = s;
  print("proc_setup_p9page: pid=%lud seg=%p base=%#p p9page=%p PADDR=%#p "
        "pseg->pa=%#p\n",
        p->pid, s, (void *)s->base, p->p9page, PADDR(p->p9page), s->pseg->pa);

  /*
   * CRITICAL: Invalidate any inherited PTE for this address.
   * When forking, the child inherits the parent's page table which may
   * have EXCHANGE_PAGE_ADDR already mapped to the parent's physical page.
   * We need to zero out that PTE so a page fault occurs and mapphys()
   * correctly maps the child's new physical page.
   *
   * Note: We use mmuwalk to find and zero the PTE in the current process's
   * page table. This only works correctly when called from the child's
   * context (after it starts running), but since fork calls this before
   * the child runs, we set the segment up and the mapping will be fixed
   * when the child first accesses the page through fixfault/mapphys.
   */
  extern uintptr *mmuwalk(uintptr *, uintptr, int, int);
  uintptr *pte = mmuwalk(m->pml4, EXCHANGE_PAGE_ADDR, 0, 0);
  if (pte != nil && *pte != 0) {
    print("proc_setup_p9page: invalidating inherited PTE at %#p, old=%#llux\n",
          EXCHANGE_PAGE_ADDR, (uvlong)*pte);
    *pte = 0;
    /* Flush TLB for this address */
    __asm__ volatile("invlpg (%0)" ::"r"(EXCHANGE_PAGE_ADDR) : "memory");
  }

  /* Register exchange page with borrow checker - process initially owns it */
  extern uintptr saved_limine_hhdm_offset;
  uintptr pa = PADDR(p->p9page);
  uintptr hhdm_va = pa + saved_limine_hhdm_offset;

  if (pageown_acquire(p, pa, hhdm_va) != POWN_OK) {
    print("proc_setup_p9page: failed to acquire ownership of exchange page\n");
  }

  return 0;
}
