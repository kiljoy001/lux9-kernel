/*
 * framac_stubs.h - Stubs for GCC builtins and missing functions for Frama-C
 */

#ifndef _FRAMAC_STUBS_H_
#define _FRAMAC_STUBS_H_

#ifdef __FRAMAC__

/* GCC Builtins Stubs */
#define __builtin_expect(x, y) (x)
#define __builtin_unreachable()
#define __builtin_constant_p(x) 0
#define __builtin_prefetch(x, ...) ((void)0)
#define __builtin_clz(x) 0
#define __builtin_ctz(x) 0
#define __builtin_clzl(x) 0
#define __builtin_ctzl(x) 0
#define __builtin_clzll(x) 0
#define __builtin_ctzll(x) 0
#define __builtin_popcount(x) 0
#define __builtin_popcountl(x) 0
#define __builtin_popcountll(x) 0
#define __builtin_bswap16(x) (x)
#define __builtin_bswap32(x) (x)
#define __builtin_bswap64(x) (x)
#define __builtin_isnan(x) 0
#define __builtin_signbit(x) 0
#define __builtin_huge_valf() (1.0f / 0.0f)
#define __builtin_huge_val() (1.0 / 0.0)
#define __builtin_ceilf(x) (x)
#define __builtin_ceil(x) (x)
#define __builtin_floorf(x) (x)
#define __builtin_floor(x) (x)
#define __builtin_truncf(x) (x)
#define __builtin_trunc(x) (x)
#define __builtin_rintf(x) (x)
#define __builtin_rint(x) (x)

/* Math Builtins */
double __builtin_fabs(double x);
float __builtin_fabsf(float x);
double __builtin_inf(void);
float __builtin_inff(void);
double __builtin_nan(const char *str);
float __builtin_nanf(const char *str);

/* Attributes */
#define __attribute__(x)

/* Types */
#ifndef __builtin_va_list
typedef __builtin_va_list va_list;
#endif

/* Frama-C Specific */
#include "__fc_builtin.h"

#endif /* __FRAMAC__ */
#endif /* _FRAMAC_STUBS_H_ */
