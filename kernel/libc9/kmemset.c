#include "u.h"
#include "portlib.h"

void*
memset(void *ap, int c, usize n)
{
	char *p;

	p = ap;
	while(n > 0) {
		*p++ = c;
		n--;
	}
	return ap;
}
