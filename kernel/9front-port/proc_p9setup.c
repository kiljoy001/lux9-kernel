#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
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

  /* Allocate 2 pages (8KB) for asymmetric exchange */
  p->p9page = xallocz(2 * BY2PG, 1);
  if (p->p9page == nil) {
    print("proc_setup_p9page: failed to allocate p9page for pid %lud\n",
          p->pid);
    return -1;
  }

  /* Initialize P9Control block at offset 0x1F00 */
  P9Control *ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  memset(ctl, 0, sizeof(P9Control));
  ctl->status = P9_STATUS_IDLE;
  ctl->doorbell = 0;

  /* Map Page 0 (Requests) as Read-Write */
  userpmap(EXCHANGE_PAGE_ADDR, PADDR(p->p9page), PTEVALID | PTEUSER | PTEWRITE);

  /* Map Page 1 (Responses/Control) as Read-Only */
  userpmap(EXCHANGE_PAGE_ADDR + BY2PG, PADDR(p->p9page) + BY2PG,
           PTEVALID | PTEUSER);

  return 0;
}
