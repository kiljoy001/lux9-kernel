#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

int
errstr(char *buf, uint nbuf)
{
	char *err;

	if(buf == nil || nbuf == 0)
		return 0;

	if(nbuf > ERRMAX)
		nbuf = ERRMAX;

	if(up == nil){
		buf[0] = '\0';
		return 0;
	}

	err = up->errstr;
	if(err == nil){
		buf[0] = '\0';
		return 0;
	}

	utfecpy(err, err + nbuf, buf);
	utfecpy(buf, buf + nbuf, up->syserrstr);

	up->errstr = up->syserrstr;
	up->syserrstr = err;

	return 0;
}
