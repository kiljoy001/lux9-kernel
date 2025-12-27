/* qbe_globals.c - QBE global variables for kernel use
 * Defines the Target T and debug array needed by QBE
 */

#include "all.h"

/* Global Target - set to amd64 System V ABI */
extern Target T_amd64_sysv;
Target T;

/* Initialize QBE globals - must be called before first use */
void
qbe_init_target(void)
{
	static int initialized = 0;
	if(!initialized){
		T = T_amd64_sysv;
		initialized = 1;
	}
}
