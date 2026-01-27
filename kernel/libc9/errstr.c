#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*@
  @ requires buf == \null || (nbuf > 0 ==> \valid(buf + (0 .. nbuf-1)));
  @ assigns buf == \null || nbuf == 0 ? \nothing : buf[0 .. \min(nbuf, ERRMAX)-1];
  @ ensures \result == 0;
  @ behavior null_or_empty:
  @   assumes buf == \null || nbuf == 0;
  @   assigns \nothing;
  @   ensures \result == 0;
  @ behavior no_process:
  @   assumes buf != \null && nbuf > 0 && up == \null;
  @   assigns buf[0];
  @   ensures buf[0] == '\0';
  @   ensures \result == 0;
  @ behavior valid:
  @   assumes buf != \null && nbuf > 0 && up != \null;
  @   assigns buf[0 .. \min(nbuf, ERRMAX)-1];
  @   ensures \result == 0;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
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
