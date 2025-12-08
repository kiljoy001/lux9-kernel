/* kernel_compat.c - Compatibility layer for QBE in kernel
 * Maps GNU libc functions to Plan 9 equivalents
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* QBE global variables - Defined weakly so QBE code can link */
/* These will be properly initialized by qbe_kernel_wrapper.c */
char debug['Z'+1];  /* Debug flags array */

/* Map vsnprintf to Plan 9 vsnprint */
int
vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args)
{
	return vsnprint(buf, size, (char*)fmt, args);
}

/* Stub for stack canary check - kernel doesn't use stack protector */
void
__stack_chk_fail(void)
{
	panic("stack protector triggered in QBE code");
}
