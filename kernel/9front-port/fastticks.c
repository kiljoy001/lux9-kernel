#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

uvlong
tscticks(uvlong *hz)
{
	if(hz != nil)
		*hz = m->cpuhz;

	cycles(&m->tscticks);
	return m->tscticks;
}

uvlong
fastticks(uvlong *hz)
{
	return arch->fastclock(hz);
}

ulong
µs(void)
{
	return fastticks2us((*arch->fastclock)(nil));
}
