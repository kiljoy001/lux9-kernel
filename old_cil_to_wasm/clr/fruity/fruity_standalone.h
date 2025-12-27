/* fruity_standalone.h - Type compatibility for standalone compilation
 *
 * When compiling Fruity IR code outside the kernel, we need to define
 * the kernel's u32int/s32int/ulong types.
 */

#ifndef FRUITY_STANDALONE_H
#define FRUITY_STANDALONE_H

#include <stdint.h>
#include <stddef.h>

/* Kernel type aliases */
typedef uint8_t  u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;

typedef int8_t   s8int;
typedef int16_t  s16int;
typedef int32_t  s32int;
typedef int64_t  s64int;

typedef unsigned long ulong;
typedef long          vlong;

typedef uintptr_t uintptr;

/* Kernel nil */
#ifndef nil
#define nil NULL
#endif

#endif /* FRUITY_STANDALONE_H */
