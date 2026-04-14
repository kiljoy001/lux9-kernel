#include "mpq_kernel.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/u.h"

#ifndef nil
#define nil ((void *)0)
#endif

/*
 * Rational Number Implementation for Lux9 Kernel.
 * Relies on mini-gmp mpz_* functions for correctness.
 */

void mpq_init(mpq_t q) {
  mpz_init(q->num);
  mpz_init(q->den);
  mpz_set_ui(q->den, 1);
}

void mpq_clear(mpq_t q) {
  mpz_clear(q->num);
  mpz_clear(q->den);
}

// Reduce to lowest terms: a/b -> (a/gcd)/(b/gcd)
// Also ensures denominator is positive.
void mpq_canonicalize(mpq_t q) {
  mpz_t gcd;
  mpz_t tmp;

  // 1. Handle sign: Denominator must be positive
  if (mpz_sgn(q->den) < 0) {
    mpz_neg(q->num, q->num);
    mpz_neg(q->den, q->den);
  }

  // 2. Handle zero: 0/x -> 0/1
  if (mpz_sgn(q->num) == 0) {
    mpz_set_ui(q->den, 1);
    return;
  }

  mpz_init(gcd);
  mpz_init(tmp);

  // 3. Divide by GCD
  mpz_gcd(gcd, q->num, q->den);
  if (mpz_cmp_ui(gcd, 1) > 0) {
    mpz_divexact(q->num, q->num, gcd); // Exact division is safe here
    mpz_divexact(q->den, q->den, gcd);
  }

  mpz_clear(gcd);
  mpz_clear(tmp);
}

void mpq_set(mpq_t dest, const mpq_t src) {
  mpz_set(dest->num, src->num);
  mpz_set(dest->den, src->den);
}

void mpq_set_ui(mpq_t q, unsigned long num, unsigned long den) {
  mpz_set_ui(q->num, num);
  mpz_set_ui(q->den, den);
  mpq_canonicalize(q);
}

void mpq_set_si(mpq_t q, long num, unsigned long den) {
  mpz_set_si(q->num, num);
  mpz_set_ui(q->den, den);
  mpq_canonicalize(q);
}

void mpq_neg(mpq_t res, const mpq_t op) {
  mpz_neg(res->num, op->num);
  mpz_set(res->den, op->den);
}

// res = op1 + op2 = (n1/d1) + (n2/d2) = (n1*d2 + n2*d1) / (d1*d2)
void mpq_add(mpq_t res, const mpq_t op1, const mpq_t op2) {
  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);

  // t1 = n1 * d2
  mpz_mul(t1, op1->num, op2->den);
  // t2 = n2 * d1
  mpz_mul(t2, op2->num, op1->den);

  // num = t1 + t2
  mpz_add(res->num, t1, t2);

  // den = d1 * d2
  mpz_mul(res->den, op1->den, op2->den);

  mpq_canonicalize(res);

  mpz_clear(t1);
  mpz_clear(t2);
}

// res = op1 - op2
void mpq_sub(mpq_t res, const mpq_t op1, const mpq_t op2) {
  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);

  mpz_mul(t1, op1->num, op2->den);
  mpz_mul(t2, op2->num, op1->den);
  mpz_sub(res->num, t1, t2);
  mpz_mul(res->den, op1->den, op2->den);

  mpq_canonicalize(res);

  mpz_clear(t1);
  mpz_clear(t2);
}

// res = op1 * op2 = (n1*n2) / (d1*d2)
void mpq_mul(mpq_t res, const mpq_t op1, const mpq_t op2) {
  mpz_mul(res->num, op1->num, op2->num);
  mpz_mul(res->den, op1->den, op2->den);
  mpq_canonicalize(res);
}

// res = op1 / op2 = (n1*d2) / (d1*n2)
void mpq_div(mpq_t res, const mpq_t op1, const mpq_t op2) {
  mpz_t temp_num;
  mpz_init(temp_num);

  // Need temp in case res == op1/op2 alias
  mpz_mul(temp_num, op1->num, op2->den);
  mpz_mul(res->den, op1->den, op2->num);
  mpz_set(res->num, temp_num);

  mpq_canonicalize(res);
  mpz_clear(temp_num);
}

/* Helpers */
char *mpq_get_str(char *str, int base, const mpq_t q) {
  // Format: "num/den"
  // Since str allocation strategy is tricky in kernel/mini-gmp,
  // we assume str is large enough or NULL (gmp style alloc).
  // For Lux9 devsym, we usually alloc manually.

  char *s_num = mpz_get_str(nil, base, q->num);
  char *s_den = mpz_get_str(nil, base, q->den);

  int len = strlen(s_num) + 1 + strlen(s_den) + 1;
  char *res = smalloc(len);
  if (!res)
    return nil;

  strcpy(res, s_num);
  strcat(res, "/");
  strcat(res, s_den);

  free(s_num);
  free(s_den);
  return res;
}
