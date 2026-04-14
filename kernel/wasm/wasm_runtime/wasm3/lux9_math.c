#include "../../../include/u.h"
#include "../../../symbolic/mini-gmp.h"
#include "../../../symbolic/mpq_kernel.h"

/* Enable SSE for this file to support double/float arguments/returns */
#pragma GCC target("sse,sse2")

/* Helper to convert mpq_t back to double - Not needed */

/* Generic Taylor Series Solver using doubles */
/* sin(x) = x - x^3/3! + x^5/5! - x^7/7! ... */
double sin(double x) {
  /* Range reduction: x = x mod 2pi (approx) */
  double x_reduced = x;
  while (x_reduced > 6.283185307179586)
    x_reduced -= 6.283185307179586;
  while (x_reduced < -6.283185307179586)
    x_reduced += 6.283185307179586;

  double res = x_reduced;
  double term = x_reduced;
  double xsq = x_reduced * x_reduced;

  /* 10 terms */
  for (int n = 1; n < 10; n++) {
    term *= -xsq;
    term /= (2 * n) * (2 * n + 1);
    res += term;
  }
  return res;
}

/* cos(x) = 1 - x^2/2! + x^4/4! - x^6/6! ... */
double cos(double x) {
  double x_reduced = x;
  while (x_reduced > 6.283185307179586)
    x_reduced -= 6.283185307179586;
  while (x_reduced < -6.283185307179586)
    x_reduced += 6.283185307179586;

  double res = 1.0;
  double term = 1.0;
  double xsq = x_reduced * x_reduced;

  for (int n = 1; n < 10; n++) {
    term *= -xsq;
    term /= (2 * n - 1) * (2 * n);
    res += term;
  }
  return res;
}

/* exp(x) = 1 + x + x^2/2! + x^3/3! ... */
double exp(double x) {
  if (x == 0)
    return 1.0;
  if (x < -20)
    return 0.0;

  double res = 1.0;
  double term = 1.0;

  for (int n = 1; n < 20; n++) {
    term *= x;
    term /= n;
    res += term;
  }
  return res;
}

/* log(x) = 2 * sum( 1/(2n+1) * ((x-1)/(x+1))^(2n+1) ) */
double log(double x) {
  if (x <= 0)
    return 0.0 / 0.0; /* NaN */

  double multiplier = (x - 1) / (x + 1);
  double subterm = multiplier * multiplier;
  double term = multiplier;
  double res = term;

  for (int n = 1; n < 15; n++) {
    term *= subterm;
    res += term / (2 * n + 1);
  }
  return 2.0 * res;
}

double pow(double x, double y) {
  if (y == 0)
    return 1.0;
  if (x == 1.0)
    return 1.0;
  if (x == 0)
    return 0.0;
  if (x < 0) {
    if ((double)(long long)y == y) {
      double res = exp(y * log(-x));
      return ((long long)y % 2 == 0) ? res : -res;
    }
    return 0.0 / 0.0; /* NaN */
  }
  return exp(y * log(x));
}

/* Floating point stubs for IEEE-specific behavior still needed for WASM core */
double copysign(double x, double y) {
  unsigned long long ux = *(unsigned long long *)&x;
  unsigned long long uy = *(unsigned long long *)&y;
  ux = (ux & ~(1ULL << 63)) | (uy & (1ULL << 63));
  return *(double *)&ux;
}

float copysignf(float x, float y) {
  unsigned int ux = *(unsigned int *)&x;
  unsigned int uy = *(unsigned int *)&y;
  ux = (ux & ~(1U << 31)) | (uy & (1U << 31));
  return *(float *)&ux;
}

double sqrt(double x) {
  double res;
  __asm__("sqrtsd %1, %0" : "=x"(res) : "x"(x));
  return res;
}

float sqrtf(float x) {
  float res;
  __asm__("sqrtss %1, %0" : "=x"(res) : "x"(x));
  return res;
}

double fabs(double x) {
  unsigned long long ux = *(unsigned long long *)&x;
  ux &= 0x7FFFFFFFFFFFFFFFULL;
  return *(double *)&ux;
}

float fabsf(float x) {
  unsigned int ux = *(unsigned int *)&x;
  ux &= 0x7FFFFFFF;
  return *(float *)&ux;
}

/* Core Rounding and Primitives */

double NaN(void) { return 0.0 / 0.0; }
double Inf(int sign) { return sign < 0 ? -1.0 / 0.0 : 1.0 / 0.0; }
int isNaN(double x) { return x != x; }
int isInf(double x, int sign) {
  if (sign == 0)
    return x == Inf(1) || x == Inf(-1);
  return x == Inf(sign);
}

double floor(double x) {
  long long i = (long long)x;
  if ((double)i == x)
    return x;
  return (x < 0) ? (double)(i - 1) : (double)i;
}

double ceil(double x) {
  long long i = (long long)x;
  if ((double)i == x)
    return x;
  return (x > 0) ? (double)(i + 1) : (double)i;
}

double trunc(double x) { return (double)(long long)x; }

double rint(double x) {
  double f = x - floor(x);
  if (f < 0.5)
    return floor(x);
  if (f > 0.5)
    return ceil(x);
  long long i = (long long)floor(x);
  return (i % 2 == 0) ? floor(x) : ceil(x);
}

float floorf(float x) { return (float)floor((double)x); }
float ceilf(float x) { return (float)ceil((double)x); }
float truncf(float x) { return (float)trunc((double)x); }
float rintf(float x) { return (float)rint((double)x); }

/* IEEE Modf and Frexp/Ldexp still useful for symbolic bridge */
double modf(double x, double *iptr) {
  *iptr = trunc(x);
  return x - *iptr;
}

double ldexp(double x, int exp_val) {
  double res = x;
  if (exp_val > 0)
    while (exp_val--)
      res *= 2.0;
  else
    while (exp_val++)
      res /= 2.0;
  return res;
}

double frexp(double x, int *exp_val) {
  if (x == 0) {
    *exp_val = 0;
    return 0;
  }
  *exp_val = 0;
  double abs_x = (x < 0) ? -x : x;
  if (abs_x >= 1.0) {
    while (abs_x >= 1.0) {
      abs_x /= 2.0;
      (*exp_val)++;
    }
  } else {
    while (abs_x < 0.5) {
      abs_x *= 2.0;
      (*exp_val)--;
    }
  }
  return (x < 0) ? -abs_x : abs_x;
}

/*@
  @ requires nptr == \null || \valid(nptr);
  @ requires endptr == \null || \valid(endptr);
  @ assigns \nothing;
  @*/
double lux9_strtod(const char *nptr, char **endptr) {
  if (endptr)
    *endptr = (char *)nptr;
  return 0.0;
}
