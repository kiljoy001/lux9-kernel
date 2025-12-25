/*
 * symbolic_expr.h - Symbolic Expression Data Structures
 *
 * Core data structures for symbolic computing using mini-gmp
 */

#ifndef SYMBOLIC_EXPR_H
#define SYMBOLIC_EXPR_H

#include "../symbolic/mini-gmp.h"
#include <stddef.h>

/* Symbolic expression types */
typedef enum {
    SYM_VAR,      // Variable (e.g., "x")
    SYM_CONST,    // Constant value (using mpz_t)
    SYM_RATIONAL, // Rational number (num/den)
    SYM_ADD,      // Addition
    SYM_SUB,      // Subtraction
    SYM_MUL,      // Multiplication
    SYM_DIV,      // Division
    SYM_POW,      // Exponentiation
    SYM_NEG,      // Negation
    SYM_SIN,      // Sine function
    SYM_COS,      // Cosine function
    SYM_TAN,      // Tangent function
    SYM_EXP,      // Exponential function
    SYM_LOG,      // Natural logarithm
    SYM_SQRT      // Square root
} sym_type_t;

/* Forward declaration */
struct sym_expr;
typedef struct sym_expr sym_expr_t;

/* Rational number structure */
typedef struct {
    mpz_t num;  // Numerator
    mpz_t den;  // Denominator
} sym_rational_t;

/* Symbolic expression structure */
struct sym_expr {
    sym_type_t type;
    int ref_count;  // Reference counting for memory management
    union {
        char *name;   // Variable name (SYM_VAR)
        mpz_t value;  // Constant value (SYM_CONST)
        sym_rational_t rational;  // Rational number (SYM_RATIONAL)
        sym_expr_t *unary;  // Unary operation operand
        struct {
            sym_expr_t *left;   // Left operand (binary operations)
            sym_expr_t *right;  // Right operand (binary operations)
        } binary;
    } data;
};

/* Function prototypes */
sym_expr_t *sym_expr_create_var(const char *name);
sym_expr_t *sym_expr_create_const(long value);
sym_expr_t *sym_expr_create_rational(long num, long den);
sym_expr_t *sym_expr_create_unary(sym_type_t type, sym_expr_t *operand);
sym_expr_t *sym_expr_create_binary(sym_type_t type, sym_expr_t *left, sym_expr_t *right);

sym_expr_t *sym_expr_copy(sym_expr_t *expr);
void sym_expr_free(sym_expr_t *expr);

int sym_expr_equals(sym_expr_t *a, sym_expr_t *b);
void sym_expr_print(sym_expr_t *expr);

#endif // SYMBOLIC_EXPR_H
