/* fruity_userspace.h - Userspace-compatible Fruity IR types
 *
 * Avoids pulling in kernel headers (u.h, lib.h, etc.)
 * Uses standard C types instead of Plan 9 types for userspace tools.
 */

#ifndef FRUITY_USERSPACE_H
#define FRUITY_USERSPACE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declare types that would come from kernel headers */
typedef uint8_t u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef int32_t s32int;
typedef uint64_t u64int;
typedef int64_t s64int;
typedef size_t usize;
typedef size_t ulong;
typedef uintptr_t uintptr;
typedef intptr_t ssize;

/* Stub out kernel memory functions with stdlib equivalents */
#define xalloc malloc
#define xfree free
#define xrealloc realloc
#define memmove memmove
#define memset memset
#define strcmp strcmp
#define strlen strlen
#define strdup strdup

/* Stub print function */
#define print(msg) printf("%s", msg)

/* Include Fruity IR without u.h */
#ifndef _U_H_
#define _U_H_  /* Prevent u.h inclusion */
#endif

#ifndef _LIB_H_
#define _LIB_H_  /* Prevent lib.h inclusion */
#endif

#ifndef _MEM_H_
#define _MEM_H_  /* Prevent mem.h inclusion */
#endif

#ifndef _DAT_H_
#define _DAT_H_  /* Prevent dat.h inclusion */
#endif

#ifndef _FNS_H_
#define _FNS_H_  /* Prevent fns.h inclusion */
#endif

/* Now include Fruity IR */
#include "../../../kernel/clr/fruity/fruity_opcodes.h"
#include "../../../kernel/clr/fruity/fruity_ir.h"

#endif /* FRUITY_USERSPACE_H */
