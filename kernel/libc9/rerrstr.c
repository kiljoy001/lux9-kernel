#include <u.h>
#include <libc.h>

/*@
  @ requires nbuf > 0 ==> \valid(buf + (0 .. nbuf-1));
  @ assigns buf[0 .. nbuf-1];
  @ ensures nbuf > 0 ==> buf[nbuf-1] == '\0' || \exists integer i; 0 <= i < nbuf && buf[i] == '\0';
  @*/
void
rerrstr(char *buf, uint nbuf)
{
	char tmp[ERRMAX];

	tmp[0] = '\0';
	errstr(tmp, sizeof tmp);
	utfecpy(buf, buf + nbuf, tmp);
	errstr(tmp, sizeof tmp);
}
