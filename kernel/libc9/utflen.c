#include <u.h>
#include <libc.h>

/*@
  @ requires \valid_read(s + (0..));
  @ requires \exists integer k; k >= 0 && s[k] == '\0';
  @ assigns \nothing;
  @ ensures \result >= 0;
  @ ensures \result <= \strlen(s);
  @*/
int
utflen(char *s)
{
	int c;
	long n;
	Rune rune;

	n = 0;
	/*@
	  @ loop invariant n >= 0;
	  @ loop invariant s >= \at(s, Pre);
	  @ loop invariant \valid_read(s);
	  @ loop assigns c, n, s, rune;
	  @*/
	for(;;) {
		c = *(uchar*)s;
		if(c < Runeself) {
			if(c == 0)
				return n;
			s++;
		} else
			s += chartorune(&rune, s);
		n++;
	}
}
