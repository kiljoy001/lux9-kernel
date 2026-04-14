#include <u.h>
#include <libc.h>

/*@
  @ requires \valid_read(f);
  @ requires f->start == \null || \valid((char *)f->start);
  @ requires f->to == \null || \valid((char *)f->to);
  @ assigns *((char *)f->to);
  @ ensures \result == f->start;
  @ behavior null_start:
  @   assumes f->start == \null;
  @   ensures \result == \null;
  @ behavior valid_start:
  @   assumes f->start != \null;
  @   ensures \result == f->start;
  @   ensures *((char *)f->to) == '\0';
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char*
fmtstrflush(Fmt *f)
{
	if(f->start == nil)
		return nil;
	*(char*)f->to = '\0';
	return f->start;
}
