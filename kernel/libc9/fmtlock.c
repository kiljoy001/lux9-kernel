#include <u.h>
#include <libc.h>

static Lock fmtl;

/* ACSL removed */
void
_fmtlock(void)
{
	lock(&fmtl);
}

/* ACSL removed */
void
_fmtunlock(void)
{
	unlock(&fmtl);
}
