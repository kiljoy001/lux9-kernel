/* proc_p9setup_stub.c - Compatibility wrapper for old lazy-P9SEG callers */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
int proc_setup_p9seg_stub(Proc *p) {
  return proc_setup_p9page(p);
}
