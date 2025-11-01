#include "u.h"
#include <lib.h>
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum {
	MemReserved	= 4,
};

void
memreserve(uintptr pa, uintptr size)
{
	assert(conf.mem[0].npage == 0);

	size += pa & (BY2PG-1);
	size &= ~(BY2PG-1);
	pa &= ~(BY2PG-1);
	memmapadd(pa, size, MemReserved);
}
