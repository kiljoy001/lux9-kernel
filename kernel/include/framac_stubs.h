/*
 * framac_stubs.h - Stubs for GCC builtins and missing functions for Frama-C
 */

#ifndef _FRAMAC_STUBS_H_
#define _FRAMAC_STUBS_H_

#ifdef __FRAMAC__

#include "acsl_bounds.h"

/* Minimal Rune definition for Frama-C stubs */
#ifndef _RUNE_DEFINED
#define _RUNE_DEFINED
typedef unsigned int Rune;
#endif

/* Frama-C requires a definition for __builtin_va_list if used in typedef */
#ifndef __builtin_va_list
#define __builtin_va_list void *
#endif

/* GCC Builtins Stubs - Frama-C now provides many of these via
 * __fc_gcc_builtins.h */
#define __builtin_expect(x, y) (x)
#define __builtin_unreachable()
#define __builtin_constant_p(x) 0
#define __builtin_prefetch(x, ...) ((void)0)
static inline unsigned long getcallerpc(void *p) {
  (void)p;
  return 0;
}

/* Attributes */
double __builtin_fabs(double x);
float __builtin_fabsf(float x);
double __builtin_inf(void);
float __builtin_inff(void);
double __builtin_nan(const char *str);
float __builtin_nanf(const char *str);

/* Attributes */
#define __attribute__(x)

/* Types - provided by native Plan 9 headers during preprocessing */
#ifdef __FRAMAC__
typedef void *va_list; // Direct definition for Frama-C
#else
#ifndef __builtin_va_list
typedef __builtin_va_list va_list;
#endif
#endif

/* Frama-C Specific - Must be guarded to prevent GCC pre-pass failure */
#ifdef __FRAMAC__
#ifndef __GCC_PREPROCESS__
#include "__fc_builtin.h"
#endif
#endif

/* Frama-C stubs are limited to compiler builtins and basic types. */

#endif /* __FRAMAC__ */
#endif /* _FRAMAC_STUBS_H_ */
