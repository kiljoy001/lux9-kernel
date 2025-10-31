#include <u.h>
#include <libc.h>
#include "fmtdef.h"
#include "error.h"

int
errfmt(Fmt *f)
{
	char buf[ERRMAX];

	rerrstr(buf, sizeof buf);
	return _fmtcpy(f, buf, utflen(buf), strlen(buf));
}
