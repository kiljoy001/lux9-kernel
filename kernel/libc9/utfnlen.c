#include <u.h>
#include <libc.h>

/*@
  @ requires m >= 0;
  @ requires \valid_read(s + (0 .. m-1)) || (\exists integer k; 0 <= k < m && s[k] == '\0');
  @ assigns \nothing;
  @ ensures \result >= 0;
  @ ensures \result <= m;
  @*/
int
utfnlen(char *s, long m)
{
	int c;
	long n;
	Rune rune;
	char *es;

	es = s + m;
	/*@
	  @ loop invariant 0 <= n;
	  @ loop invariant s >= \at(s, Pre);
	  @ loop invariant s <= es;
	  @ loop invariant es == \at(s, Pre) + m;
	  @ loop invariant \valid_read(s);
	  @ loop assigns n, s, c, rune;
	  @ loop variant es - s;
	  @*/
	for(n = 0; s < es; n++) {
		c = *(uchar*)s;
		if(c < Runeself){
			if(c == '\0')
				break;
			s++;
			continue;
		}
		if(!fullrune(s, es-s))
			break;
		s += chartorune(&rune, s);
	}
	return n;
}
