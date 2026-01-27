#include <u.h>
#include <libc.h>
#include "fmtdef.h"

/*
 * public routine for final flush of a formatting buffer
 * to a file descriptor; returns total char count.
 */
/*@
  @ requires \valid(f);
  @ assigns *f;
  @ ensures \result == -1 || \result == f->nfmt;
  @*/
int
fmtfdflush(Fmt *f)
{
	if(_fmtFdFlush(f) <= 0)
		return -1;
	return f->nfmt;
}

/*
 * initialize an output buffer for buffered printing
 */
/*@
  @ requires \valid(f);
  @ requires size > 0 ==> \valid(buf + (0 .. size-1));
  @ requires fd >= 0;
  @ assigns *f;
  @ ensures \result == 0;
  @ ensures f->start == buf;
  @ ensures f->to == buf;
  @ ensures f->stop == buf + size;
  @ ensures f->nfmt == 0;
  @*/
int
fmtfdinit(Fmt *f, int fd, char *buf, int size)
{
	f->runes = 0;
	f->start = buf;
	f->to = buf;
	f->stop = buf + size;
	f->flush = _fmtFdFlush;
	f->farg = (void*)fd;
	f->nfmt = 0;
	return 0;
}
