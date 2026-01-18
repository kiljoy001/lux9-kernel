/* proc_p9setup_stub.c - Lazy exchange page allocation setup
 *
 * Creates a stub P9SEG segment that triggers on-demand allocation
 * via page fault handler when process first accesses its p9uaddr.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"

int proc_setup_p9seg_stub(Proc *p) {
  if (p->kp)
    return 0; /* Kernel processes don't need this */

  if (p->seg[P9SEG] != nil)
    return 0; /* Already set up */

  if (p->p9uaddr == 0)
    p->p9uaddr = p9_pick_uaddr(p, nil);
  if (p->p9uaddr == 0)
    p->p9uaddr = EXCHANGE_PAGE_ADDR;

  /* Create empty segment for lazy allocation */
  Segment *s = newseg(SG_PHYSICAL, p->p9uaddr, 1);
  if (s == nil) {
    print("proc_setup_p9seg_stub: newseg failed\n");
    return -1;
  }

  /* Allocate Physseg with pa=0 to trigger lazy allocation on first fault */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("proc_setup_p9seg_stub: malloc failed for pseg\n");
    putseg(s);
    return -1;
  }

  /* Configure with pa=0 to enable lazy allocation */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED | SG_NOEXEC;
  s->pseg->name = "9pexchange_lazy";
  s->pseg->pa = 0; /* Zero PA triggers lazy allocation in fault handler */
  s->pseg->size = BY2PG;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Clear any conflicting segments */
  for (int i = 0; i < NSEG; i++) {
    Segment *oseg = p->seg[i];
    if (oseg == nil)
      continue;
    if (p->p9uaddr >= oseg->base && p->p9uaddr < oseg->top) {
      print("proc_setup_p9seg_stub: clearing conflicting seg[%d]\n", i);
      p->seg[i] = nil;
      putseg(oseg);
    }
  }

  /* Assign stub segment to P9SEG slot */
  p->seg[P9SEG] = s;

  print("proc_setup_p9seg_stub: pid=%lud stub seg created (lazy alloc enabled)\n",
        p->pid);

  return 0;
}
