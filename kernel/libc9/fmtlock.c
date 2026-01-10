#include <u.h>
#include <libc.h>

static Lock fmtl;

/*@
  @ assigns fmtl;
  @ ensures fmtl.locked == 1;
  @*/
void
_fmtlock(void)
{
	lock(&fmtl);
}

/*@
  @ requires fmtl.locked == 1;
  @ assigns fmtl;
  @ ensures fmtl.locked == 0;
  @*/
void
_fmtunlock(void)
{
	unlock(&fmtl);
}
