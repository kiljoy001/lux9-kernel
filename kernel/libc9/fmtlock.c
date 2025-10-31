#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

static Lock fmtlock;

void
_fmtlock(void)
{
	lock(&fmtlock);
}

void
_fmtunlock(void)
{
	unlock(&fmtlock);
}
