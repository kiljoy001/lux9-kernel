/*
 * kernel_mini_gmp.h - Kernel-compatible mini-gmp wrapper
 *
 * Provides mini-gmp functionality adapted for kernel use
 */

#ifndef KERNEL_MINI_GMP_H
#define KERNEL_MINI_GMP_H

#include "../../port/lib.h"
#include "../../9front-pc64/mem.h"

/* Define size_t for mini-gmp */
typedef unsigned long size_t;

/* Forward declaration of mpz_t */
typedef struct {
    int _mp_alloc;    /* Number of limbs allocated */
    int _mp_size;     /* Number of limbs used (signed) */
    unsigned long *_mp_d; /* Pointer to the limbs */
} __mpz_struct;

typedef __mpz_struct mpz_t[1];

/* Basic mpz functions needed for symbolic computing */
void mpz_init(mpz_t x);
void mpz_init_set(mpz_t dest, mpz_t src);
void mpz_init_set_si(mpz_t dest, long int src);
void mpz_clear(mpz_t x);
void mpz_set(mpz_t dest, mpz_t src);
void mpz_set_si(mpz_t dest, long int src);
void mpz_set_ui(mpz_t dest, unsigned long int src);
int mpz_cmp(mpz_t a, mpz_t b);
int mpz_cmp_si(mpz_t a, long int b);
int mpz_cmp_ui(mpz_t a, unsigned long int b);
void mpz_add(mpz_t dest, mpz_t a, mpz_t b);
void mpz_sub(mpz_t dest, mpz_t a, mpz_t b);
void mpz_mul(mpz_t dest, mpz_t a, mpz_t b);
void mpz_neg(mpz_t dest, mpz_t src);
char *mpz_get_str(char *str, int base, mpz_t x);

#endif // KERNEL_MINI_GMP_H
