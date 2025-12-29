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
 * This MUST be called only for user processes, after newproc() returns.
 * It allocates and maps the exchange page into the user address space.
 */
int proc_setup_p9page(Proc *p) {
  if (p->kp)
    return 0; /* Kernel processes don't need this */

  if (p->p9page != nil)
    return 0; /* Already allocated */

  /* Allocate 2 pages (8KB) for asymmetric exchange, aligned to Page Size */
  /* Must be aligned so that PADDR(p->p9page) is the exact start of the physical
   * page mapped to userspace */
  p->p9page = mallocalign(2 * BY2PG, BY2PG, 0, 0);
  if (p->p9page == nil) {
    print("proc_setup_p9page: mallocalign failed\n");
    return -1;
  }
  memset(p->p9page, 0, 2 * BY2PG);

  /* Create a physical segment for the exchange page */
  Segment *s = newseg(SG_PHYSICAL, EXCHANGE_PAGE_ADDR, 2);
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
  s->pseg->size = 2 * BY2PG;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Assign segment to process at P9SEG slot */
  p->seg[P9SEG] = s;
  print(
      "proc_setup_p9page: assigned seg %p to p->seg[P9SEG], base=%#p top=%#p\n",
      s, (void *)s->base, (void *)s->top);

  /* Pre-map exchange pages so initcode doesn't fault on first write */
  extern void userpmap(uintptr va, uintptr pa, int perms);
  userpmap(EXCHANGE_PAGE_ADDR, PADDR(p->p9page), PTEVALID | PTEUSER | PTEWRITE);
  userpmap(EXCHANGE_PAGE_ADDR + BY2PG, PADDR(p->p9page) + BY2PG,
           PTEVALID | PTEUSER | PTEWRITE);

  /* Register exchange pages with borrow checker for ownership tracking */
  extern uintptr saved_limine_hhdm_offset;
  uintptr pa1 = PADDR(p->p9page);
  uintptr pa2 = PADDR(p->p9page) + BY2PG;
  uintptr hhdm_va1 = pa1 + saved_limine_hhdm_offset;
  uintptr hhdm_va2 = pa2 + saved_limine_hhdm_offset;

  /* Acquire ownership - these pages belong to this process */
  if (pageown_acquire(p, pa1, hhdm_va1) != POWN_OK) {
    print(
        "proc_setup_p9page: failed to acquire ownership of exchange page 1\n");
  }
  if (pageown_acquire(p, pa2, hhdm_va2) != POWN_OK) {
    print(
        "proc_setup_p9page: failed to acquire ownership of exchange page 2\n");
  }

  return 0;
}
