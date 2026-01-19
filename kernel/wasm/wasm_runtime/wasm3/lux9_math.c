#include "dat.h"
#include "u.h"

/* Enable SSE for this file to support double/float arguments/returns */
#pragma GCC target("sse,sse2")

/* Math stubs for WASM and other components */
/*@
  @ assigns \nothing;
  @*/
double copysign(double x, double y) {
  u64int *ux = (u64int *)&x;
  u64int *uy = (u64int *)&y;
  *ux = (*ux & ~(1ULL << 63)) | (*uy & (1ULL << 63));
  return x;
}

/*@
  @ assigns \nothing;
  @*/
float copysignf(float x, float y) {
  u32int *ux = (u32int *)&x;
  u32int *uy = (u32int *)&y;
  *ux = (*ux & ~(1U << 31)) | (*uy & (1U << 31));
  return x;
}

/*@
  @ assigns \nothing;
  @*/
double sqrt(double x) {
  double res;
  __asm__("sqrtsd %1, %0" : "=x"(res) : "x"(x));
  return res;
}

/*@
  @ assigns \nothing;
  @*/
float sqrtf(float x) {
  float res;
  __asm__("sqrtss %1, %0" : "=x"(res) : "x"(x));
  return res;
}

/*@
  @ assigns \nothing;
  @*/
double floor(double x) {
  if (x != x)
    return x; /* NaN */
  if (x == 0.0)
    return x; /* +0, -0 */
  if (x >= 4503599627370496.0 || x <= -4503599627370496.0)
    return x;
  long long i = (long long)x;
  if ((double)i == x)
    return x; /* Integer */
  if (x < 0)
    return (double)(i - 1);
  return (double)i;
}

/*@
  @ assigns \nothing;
  @*/
double ceil(double x) {
  if (x != x)
    return x;
  if (x == 0.0)
    return x;
  if (x >= 4503599627370496.0 || x <= -4503599627370496.0)
    return x;
  long long i = (long long)x;
  if ((double)i == x)
    return x;
  if (x > 0)
    return (double)(i + 1);
  return (double)i;
}

/*@
  @ assigns \nothing;
  @*/
double trunc(double x) {
  if (x != x)
    return x;
  if (x == 0.0)
    return x;
  if (x >= 4503599627370496.0 || x <= -4503599627370496.0)
    return x;
  return (double)(long long)x;
}

/*@
  @ assigns \nothing;
  @*/
double rint(double x) {
  if (x != x)
    return x;
  if (x >= 4503599627370496.0 || x <= -4503599627370496.0)
    return x;
  double ret = floor(x);
  double frac = x - ret;
  if (frac < 0.5)
    return ret;
  if (frac > 0.5)
    return ret + 1.0;
  long long i = (long long)ret;
  if (i % 2 == 0)
    return ret;
  return ret + 1.0;
}

/* Float versions */
float floorf(float x) { return (float)floor((double)x); }
float ceilf(float x) { return (float)ceil((double)x); }
float truncf(float x) { return (float)trunc((double)x); }
float rintf(float x) { return (float)rint((double)x); }

/*@
  @ assigns \nothing;
  @*/
double fabs(double x) {
  u64int *ux = (u64int *)&x;
  *ux &= 0x7FFFFFFFFFFFFFFFULL;
  return x;
}

/*@
  @ assigns \nothing;
  @*/
float fabsf(float x) {
  u32int *ux = (u32int *)&x;
  *ux &= 0x7FFFFFFF;
  return x;
}

double sin(double x) { return 0.0; }
double cos(double x) { return 0.0; }
double pow(double x, double y) { return 0.0; }

/*@
  @ requires nptr == \null || \valid(nptr);
  @ requires endptr == \null || \valid(endptr);
  @ assigns \nothing;
  @*/
double strtod(const char *nptr, char **endptr) {
  if (endptr)
    *endptr = (char *)nptr;
  return 0.0;
}
