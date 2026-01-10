#include <u.h>
#include <libc.h>

/*@
  @ requires \valid(to + (0 .. (e - to) - 1));
  @ requires to <= e;
  @ requires \valid_read(from + (0..));
  @ requires \exists integer n; n >= 0 && from[n] == '\0';
  @ assigns to[0 .. (e - to) - 1];
  @ ensures \result >= to && \result < e;
  @ ensures *\result == '\0';
  @ behavior empty_buffer:
  @   assumes to >= e;
  @   ensures \result == to;
  @ behavior sufficient_space:
  @   assumes to < e;
  @   assumes \strlen(from) < e - to;
  @   ensures \result == to + \strlen(from);
  @   ensures \forall integer i; 0 <= i < \strlen(from) ==> to[i] == from[i];
  @   ensures to[\strlen(from)] == '\0';
  @ behavior insufficient_space:
  @   assumes to < e;
  @   assumes \strlen(from) >= e - to;
  @   ensures \result == e - 1;
  @   ensures \forall integer i; 0 <= i < e - to - 1 ==> to[i] == from[i];
  @   ensures to[e - to - 1] == '\0';
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char*
strecpy(char *to, char *e, char *from)
{
	if(to >= e)
		return to;
	to = memccpy(to, from, '\0', e - to);
	if(to == nil){
		to = e - 1;
		*to = '\0';
	}else{
		to--;
	}
	return to;
}
