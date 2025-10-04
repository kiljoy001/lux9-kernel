#include "u.h"
#include "portlib.h"

long
strlen(char *s)
{
	char *p;
	p = s;
	while(*p)
		p++;
	return p - s;
}
