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
  print("proc_setup_p9page: pid=%lud seg=%p base=%#p (4KB single-page model)\n",
        p->pid, s, (void *)s->base);

  /* Map single exchange page to userspace */
  extern void userpmap(uintptr va, uintptr pa, int perms);
  userpmap(EXCHANGE_PAGE_ADDR, PADDR(p->p9page), PTEVALID | PTEUSER | PTEWRITE);

  /* Register exchange page with borrow checker - process initially owns it */
  extern uintptr saved_limine_hhdm_offset;
  uintptr pa = PADDR(p->p9page);
  uintptr hhdm_va = pa + saved_limine_hhdm_offset;

  if (pageown_acquire(p, pa, hhdm_va) != POWN_OK) {
    print("proc_setup_p9page: failed to acquire ownership of exchange page\n");
  }

  return 0;
}
