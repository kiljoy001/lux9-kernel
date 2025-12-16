/* mem.h stub - userspace version */
#ifndef _MEM_H_
#define _MEM_H_

#include <stdlib.h>
#include <string.h>

#define xalloc malloc
#define xfree free
#define xrealloc realloc

void* xallocz(size_t size);

#endif
