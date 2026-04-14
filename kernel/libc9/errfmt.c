#include <u.h>
#include <libc.h>
#include "fmtdef.h"
#include "error.h"

/*@
  @ requires \valid(f);
  @ assigns *f, buf[0 .. ERRMAX-1];
  @ ensures \result >= 0 || \result == -1;
  @*/
int
errfmt(Fmt *f)
{
	char buf[ERRMAX];

	rerrstr(buf, sizeof buf);
	return _fmtcpy(f, buf, utflen(buf), strlen(buf));
}
