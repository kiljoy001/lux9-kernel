/*
 * strdup - duplicate a string
 */
#include "u.h"
#include "../include/libc.h"

char*
strdup(char *s)
{
	char *new;
	usize len;

	if(s == nil)
		return nil;

	len = strlen(s) + 1;
	new = mallocz(len, 0);
	if(new == nil)
		return nil;

	memmove(new, s, len);
	return new;
}
