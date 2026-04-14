#ifndef MINI_MPQ_H
#define MINI_MPQ_H

#include "mini-gmp.h"

typedef struct {
  mpz_t num;
  mpz_t den;
} mpq_t[1];

/*@
  @ predicate valid_mpq(mpq_t q) = \valid(q) && \valid_mpz(q->num) &&
  \valid_mpz(q->den);
  @*/

void mpq_init(mpq_t q);
void mpq_clear(mpq_t q);

/*@
  @ requires valid_mpq(q);
  @ assigns q->num, q->den;
  @ ensures mpz_cmp_ui(q->den, 0) > 0; // Denominator always positive
  @ ensures mpz_gcd(result, q->num, q->den) == 1; // Coprime (not fully
  expressible in this reduced ACSL without logic function)
  @*/
void mpq_canonicalize(mpq_t q);

void mpq_set(mpq_t dest, const mpq_t src);
void mpq_set_ui(mpq_t q, unsigned long num, unsigned long den);
void mpq_set_si(mpq_t q, long num, unsigned long den);
void mpq_neg(mpq_t res, const mpq_t op);

void mpq_add(mpq_t res, const mpq_t op1, const mpq_t op2);
void mpq_sub(mpq_t res, const mpq_t op1, const mpq_t op2);
void mpq_mul(mpq_t res, const mpq_t op1, const mpq_t op2);
void mpq_div(mpq_t res, const mpq_t op1, const mpq_t op2);

/* Helpers */
char *mpq_get_str(char *str, int base, const mpq_t q);

#endif
