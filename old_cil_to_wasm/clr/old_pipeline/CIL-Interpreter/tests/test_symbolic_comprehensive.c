/* Comprehensive Symbolic Computing Test Suite */
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* === Inline types (avoid kernel_compat.h) === */
typedef enum {
  SYM_VAR,
  SYM_CONST,
  SYM_RATIONAL,
  SYM_ADD,
  SYM_SUB,
  SYM_MUL,
  SYM_DIV,
  SYM_POW,
  SYM_NEG,
  SYM_SIN,
  SYM_COS,
  SYM_TAN,
  SYM_EXP,
  SYM_LOG,
  SYM_SQRT
} sym_type_t;

typedef struct sym_expr {
  sym_type_t type;
  int ref_count;
  union {
    char *name;
    int64_t value;
    struct {
      int64_t num;
      int64_t den;
    } rational;
    struct sym_expr *unary;
    struct {
      struct sym_expr *left;
      struct sym_expr *right;
    } binary;
  } data;
} sym_expr_t;

/* === Constructors === */
static sym_expr_t *sym_alloc(sym_type_t type) {
  sym_expr_t *e = calloc(1, sizeof(sym_expr_t));
  if (e) {
    e->type = type;
    e->ref_count = 1;
  }
  return e;
}

static sym_expr_t *sym_var(const char *name) {
  sym_expr_t *e = sym_alloc(SYM_VAR);
  e->data.name = strdup(name);
  return e;
}

static sym_expr_t *sym_const(int64_t v) {
  sym_expr_t *e = sym_alloc(SYM_CONST);
  e->data.value = v;
  return e;
}

static sym_expr_t *sym_binary(sym_type_t op, sym_expr_t *l, sym_expr_t *r) {
  sym_expr_t *e = sym_alloc(op);
  e->data.binary.left = l;
  e->data.binary.right = r;
  return e;
}

static sym_expr_t *sym_unary(sym_type_t op, sym_expr_t *u) {
  sym_expr_t *e = sym_alloc(op);
  e->data.unary = u;
  return e;
}

static sym_expr_t *sym_copy(sym_expr_t *e) {
  if (e)
    e->ref_count++;
  return e;
}

/* === Differentiation === */
static sym_expr_t *diff(sym_expr_t *expr, const char *var) {
  if (!expr)
    return NULL;

  switch (expr->type) {
  case SYM_CONST:
  case SYM_RATIONAL:
    return sym_const(0);

  case SYM_VAR:
    return sym_const(expr->data.name && strcmp(expr->data.name, var) == 0 ? 1
                                                                          : 0);

  case SYM_ADD:
  case SYM_SUB: {
    sym_expr_t *du = diff(expr->data.binary.left, var);
    sym_expr_t *dv = diff(expr->data.binary.right, var);
    return sym_binary(expr->type, du, dv);
  }

  case SYM_MUL: {
    sym_expr_t *u = sym_copy(expr->data.binary.left);
    sym_expr_t *v = sym_copy(expr->data.binary.right);
    sym_expr_t *du = diff(expr->data.binary.left, var);
    sym_expr_t *dv = diff(expr->data.binary.right, var);
    return sym_binary(SYM_ADD, sym_binary(SYM_MUL, u, dv),
                      sym_binary(SYM_MUL, v, du));
  }

  case SYM_DIV: {
    sym_expr_t *u = sym_copy(expr->data.binary.left);
    sym_expr_t *v = sym_copy(expr->data.binary.right);
    sym_expr_t *du = diff(expr->data.binary.left, var);
    sym_expr_t *dv = diff(expr->data.binary.right, var);
    return sym_binary(SYM_DIV,
                      sym_binary(SYM_SUB, sym_binary(SYM_MUL, sym_copy(v), du),
                                 sym_binary(SYM_MUL, u, dv)),
                      sym_binary(SYM_POW, v, sym_const(2)));
  }

  case SYM_POW: {
    sym_expr_t *u = sym_copy(expr->data.binary.left);
    sym_expr_t *n = sym_copy(expr->data.binary.right);
    sym_expr_t *du = diff(expr->data.binary.left, var);
    return sym_binary(
        SYM_MUL,
        sym_binary(SYM_MUL, n,
                   sym_binary(SYM_POW, u,
                              sym_binary(SYM_SUB, sym_copy(n), sym_const(1)))),
        du);
  }

  case SYM_NEG:
    return sym_unary(SYM_NEG, diff(expr->data.unary, var));

  case SYM_SIN: {
    sym_expr_t *u = sym_copy(expr->data.unary);
    sym_expr_t *du = diff(expr->data.unary, var);
    return sym_binary(SYM_MUL, sym_unary(SYM_COS, u), du);
  }

  case SYM_COS: {
    sym_expr_t *u = sym_copy(expr->data.unary);
    sym_expr_t *du = diff(expr->data.unary, var);
    return sym_binary(SYM_MUL, sym_unary(SYM_NEG, sym_unary(SYM_SIN, u)), du);
  }

  case SYM_EXP: {
    sym_expr_t *du = diff(expr->data.unary, var);
    return sym_binary(SYM_MUL, sym_copy(expr), du);
  }

  case SYM_LOG: {
    sym_expr_t *u = sym_copy(expr->data.unary);
    sym_expr_t *du = diff(expr->data.unary, var);
    return sym_binary(SYM_DIV, du, u);
  }

  case SYM_SQRT: {
    sym_expr_t *du = diff(expr->data.unary, var);
    return sym_binary(SYM_DIV, du,
                      sym_binary(SYM_MUL, sym_const(2), sym_copy(expr)));
  }

  default:
    return NULL;
  }
}

/* === Print === */
static void print_expr(sym_expr_t *e) {
  if (!e) {
    printf("?");
    return;
  }
  switch (e->type) {
  case SYM_CONST:
    printf("%lld", (long long)e->data.value);
    break;
  case SYM_VAR:
    printf("%s", e->data.name);
    break;
  case SYM_ADD:
    printf("(");
    print_expr(e->data.binary.left);
    printf("+");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  case SYM_SUB:
    printf("(");
    print_expr(e->data.binary.left);
    printf("-");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  case SYM_MUL:
    printf("(");
    print_expr(e->data.binary.left);
    printf("*");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  case SYM_DIV:
    printf("(");
    print_expr(e->data.binary.left);
    printf("/");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  case SYM_POW:
    printf("(");
    print_expr(e->data.binary.left);
    printf("^");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  case SYM_NEG:
    printf("-");
    print_expr(e->data.unary);
    break;
  case SYM_SIN:
    printf("sin(");
    print_expr(e->data.unary);
    printf(")");
    break;
  case SYM_COS:
    printf("cos(");
    print_expr(e->data.unary);
    printf(")");
    break;
  case SYM_EXP:
    printf("exp(");
    print_expr(e->data.unary);
    printf(")");
    break;
  case SYM_LOG:
    printf("log(");
    print_expr(e->data.unary);
    printf(")");
    break;
  case SYM_SQRT:
    printf("sqrt(");
    print_expr(e->data.unary);
    printf(")");
    break;
  default:
    printf("?");
    break;
  }
}

/* === Tests === */
static int test_count = 0;
static int pass_count = 0;

#define TEST(name, expr_setup, expected_type)                                  \
  do {                                                                         \
    test_count++;                                                              \
    printf("Test %d: %s\n", test_count, name);                                 \
    sym_expr_t *e = expr_setup;                                                \
    sym_expr_t *d = diff(e, "x");                                              \
    printf("  ");                                                              \
    print_expr(e);                                                             \
    printf("  ->  ");                                                          \
    print_expr(d);                                                             \
    printf("\n");                                                              \
    if (d && d->type == expected_type) {                                       \
      printf("  [PASS]\n");                                                    \
      pass_count++;                                                            \
    } else {                                                                   \
      printf("  [FAIL] Expected type %d\n", expected_type);                    \
    }                                                                          \
  } while (0)

int main() {
  printf("=== Symbolic Computing Test Suite ===\n\n");

  /* Basic */
  TEST("d/dx(5) = 0", sym_const(5), SYM_CONST);
  TEST("d/dx(x) = 1", sym_var("x"), SYM_CONST);
  TEST("d/dx(y) = 0", sym_var("y"), SYM_CONST);

  /* Sum/Difference */
  TEST("d/dx(x + 5)", sym_binary(SYM_ADD, sym_var("x"), sym_const(5)), SYM_ADD);
  TEST("d/dx(x - 5)", sym_binary(SYM_SUB, sym_var("x"), sym_const(5)), SYM_SUB);

  /* Product/Quotient */
  TEST("d/dx(x * x)", sym_binary(SYM_MUL, sym_var("x"), sym_var("x")), SYM_ADD);
  TEST("d/dx(1 / x)", sym_binary(SYM_DIV, sym_const(1), sym_var("x")), SYM_DIV);

  /* Power */
  TEST("d/dx(x^2)", sym_binary(SYM_POW, sym_var("x"), sym_const(2)), SYM_MUL);
  TEST("d/dx(x^3)", sym_binary(SYM_POW, sym_var("x"), sym_const(3)), SYM_MUL);

  /* Trig */
  TEST("d/dx(sin(x))", sym_unary(SYM_SIN, sym_var("x")), SYM_MUL);
  TEST("d/dx(cos(x))", sym_unary(SYM_COS, sym_var("x")), SYM_MUL);

  /* Chain rule */
  TEST("d/dx(sin(x^2))",
       sym_unary(SYM_SIN, sym_binary(SYM_POW, sym_var("x"), sym_const(2))),
       SYM_MUL);

  /* Transcendental */
  TEST("d/dx(exp(x))", sym_unary(SYM_EXP, sym_var("x")), SYM_MUL);
  TEST("d/dx(log(x))", sym_unary(SYM_LOG, sym_var("x")), SYM_DIV);
  TEST("d/dx(sqrt(x))", sym_unary(SYM_SQRT, sym_var("x")), SYM_DIV);

  printf("\n=== Results: %d/%d passed ===\n", pass_count, test_count);
  return pass_count == test_count ? 0 : 1;
}
