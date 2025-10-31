#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

extern uvlong rdtsc(void);

uvlong
tscticks(uvlong *hz)
{
	if(hz != nil)
		*hz = 1000000000ULL;
	return rdtsc();
}

uvlong
fastticks(uvlong *hz)
{
	if(arch->fastclock != nil)
		return arch->fastclock(hz);
	if(hz != nil)
		*hz = 1000000ULL;
	return 0;
}

ulong
µs(void)
{
	return fastticks2us(fastticks(nil));
}
