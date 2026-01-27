#include <u.h>
#include "acsl_bounds.h"
#include <libc.h>

/*@
  @ requires valid_string(sep);
  @ requires \exists integer k; k >= 0 && sep[k] == '\0';
  @ assigns \nothing;
  @ ensures \result == 0 || \result == 1;
  @ ensures \result == 1 <==> strchr(sep, c) != nil;
  @*/
static int
isdelim(int c, char *sep)
{
	return strchr(sep, c) != nil;
}

/*@
  @ requires n > 0 ==> \valid(fields + (0 .. n-1));
  @ requires str != \null ==> valid_string(str);
  @ requires str != \null ==> \exists integer k; k >= 0 && str[k] == '\0';
  @ requires sep != \null ==> valid_string(sep);
  @ requires sep != \null ==> \exists integer k; k >= 0 && sep[k] == '\0';
  @ assigns str == \null ? \nothing : str[0 .. \strlen(\at(str, Pre))],
  @         (str == \null || fields == \null || n <= 0) ? \nothing : fields[0 .. n-1];
  @ ensures 0 <= \result <= n;
  @ behavior invalid_input:
  @   assumes str == \null || fields == \null || sep == \null || n <= 0;
  @   ensures \result == 0;
  @ behavior valid_input:
  @   assumes str != \null && fields != \null && sep != \null && n > 0;
  @   ensures 0 <= \result <= n;
  @   ensures \forall integer i; 0 <= i < \result ==>
  @             \valid_read(fields[i] + (0..)) && fields[i] >= str;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int
getfields(char *str, char **fields, int n, int skip, char *sep)
{
	char *s, *t;
	int nf;

	if(str == nil || fields == nil || sep == nil || n <= 0)
		return 0;

	s = str;
	nf = 0;
	/*@
	  @ loop invariant 0 <= nf <= n;
	  @ loop invariant s >= str;
	  @ loop invariant \valid(s);
	  @ loop assigns nf, s, t, str[0 .. \strlen(\at(str, Pre))], fields[0 .. n-1];
	  @ loop variant n - nf;
	  @*/
	while(nf < n){
		if(skip){
			/*@
			  @ loop invariant s >= \at(s, LoopEntry);
			  @ loop invariant \valid(s);
			  @ loop assigns s;
			  @*/
			while(*s != '\0' && isdelim(*s, sep))
				s++;
			if(*s == '\0')
				break;
		}
		fields[nf++] = s;
		t = s;
		/*@
		  @ loop invariant t >= s;
		  @ loop invariant \valid(t);
		  @ loop assigns t;
		  @*/
		while(*t != '\0' && !isdelim(*t, sep))
			t++;
		if(*t == '\0')
			break;
		*t++ = '\0';
		s = t;
	}

	return nf;
}
