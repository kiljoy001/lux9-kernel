#include "u.h"
#include "portlib.h"

char*
strcat(char *s1, char *s2)
{
	char *os1;

	os1 = s1;
	while(*s1++)
		;
	s1--;
	while((*s1++ = *s2++))
		;
	return os1;
}
