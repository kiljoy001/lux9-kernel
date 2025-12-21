/* il_compat.h - Compatibility layer for IL Parser
 *
 * Provides standard integer types for il_parser.h.
 */

#ifndef IL_COMPAT_H
#define IL_COMPAT_H

/* GCC provides stdint.h and stddef.h even in freestanding environment */
#include <stdint.h>
#include <stddef.h>

#ifndef USERSPACE_TEST
/* Kernel Mode additional definitions */
#include <u.h>

/* size_t might be defined by stddef.h, but if not: */
#ifndef _SIZE_T
typedef ulong size_t;
#define _SIZE_T
#endif

/* NULL */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif

#endif /* IL_COMPAT_H */