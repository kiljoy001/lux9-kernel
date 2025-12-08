/* stubs.c - Userspace stubs for kernel functions */

#include <stdlib.h>
#include <string.h>

void* xallocz(size_t size)
{
	void *p = malloc(size);
	if (p)
		memset(p, 0, size);
	return p;
}
