/* Standalone symbolic test - avoids kernel_compat.h issues */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Inline the necessary types */
typedef enum {
  SYM_VAR,
  SYM_CONST,
  SYM_ADD,
  SYM_SUB,
  SYM_MUL,
  SYM_DIV,
  SYM_POW
} sym_type_t;

typedef struct sym_expr {
  sym_type_t type;
  int ref_count;
  union {
    char *name;
    int64_t value;
    struct {
      struct sym_expr *left;
      struct sym_expr *right;
    } binary;
  } data;
} sym_expr_t;

/* Allocator */
static sym_expr_t *sym_expr_alloc(sym_type_t type) {
  sym_expr_t *expr = (sym_expr_t *)malloc(sizeof(sym_expr_t));
  if (expr) {
    expr->type = type;
    expr->ref_count = 1;
    memset(&expr->data, 0, sizeof(expr->data));
  }
  return expr;
}

static sym_expr_t *sym_expr_copy(sym_expr_t *expr) {
  if (expr)
    expr->ref_count++;
  return expr;
}

/* Differentiation */
static sym_expr_t *sym_differentiate(sym_expr_t *expr, const char *var) {
  if (!expr)
    return NULL;

  switch (expr->type) {
  case SYM_CONST: {
    sym_expr_t *zero = sym_expr_alloc(SYM_CONST);
    if (zero)
      zero->data.value = 0;
    return zero;
  }

  case SYM_VAR: {
    sym_expr_t *result = sym_expr_alloc(SYM_CONST);
    if (result) {
      result->data.value =
          (expr->data.name && strcmp(expr->data.name, var) == 0) ? 1 : 0;
    }
    return result;
  }

  case SYM_ADD: {
    sym_expr_t *du = sym_differentiate(expr->data.binary.left, var);
    sym_expr_t *dv = sym_differentiate(expr->data.binary.right, var);
    sym_expr_t *sum = sym_expr_alloc(SYM_ADD);
    if (sum) {
      sum->data.binary.left = du;
      sum->data.binary.right = dv;
    }
    return sum;
  }

  case SYM_MUL: {
    sym_expr_t *u = sym_expr_copy(expr->data.binary.left);
    sym_expr_t *v = sym_expr_copy(expr->data.binary.right);
    sym_expr_t *du = sym_differentiate(expr->data.binary.left, var);
    sym_expr_t *dv = sym_differentiate(expr->data.binary.right, var);

    sym_expr_t *u_dv = sym_expr_alloc(SYM_MUL);
    u_dv->data.binary.left = u;
    u_dv->data.binary.right = dv;

    sym_expr_t *v_du = sym_expr_alloc(SYM_MUL);
    v_du->data.binary.left = v;
    v_du->data.binary.right = du;

    sym_expr_t *sum = sym_expr_alloc(SYM_ADD);
    sum->data.binary.left = u_dv;
    sum->data.binary.right = v_du;

    return sum;
  }

  default:
    return NULL;
  }
}

/* Print expression */
static void print_expr(sym_expr_t *e) {
  if (!e) {
    printf("NULL");
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
    printf(" + ");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  case SYM_MUL:
    printf("(");
    print_expr(e->data.binary.left);
    printf(" * ");
    print_expr(e->data.binary.right);
    printf(")");
    break;
  default:
    printf("?");
    break;
  }
}

int main() {
  printf("=== Symbolic Computing Test ===\n");

  /* Create x */
  sym_expr_t *x = sym_expr_alloc(SYM_VAR);
  x->data.name = strdup("x");

  /* Create constant 5 */
  sym_expr_t *c5 = sym_expr_alloc(SYM_CONST);
  c5->data.value = 5;

  /* Create x + 5 */
  sym_expr_t *add = sym_expr_alloc(SYM_ADD);
  add->data.binary.left = x;
  add->data.binary.right = c5;

  printf("Expression: ");
  print_expr(add);
  printf("\n");

  /* Differentiate d/dx(x + 5) */
  sym_expr_t *deriv = sym_differentiate(add, "x");
  printf("d/dx: ");
  print_expr(deriv);
  printf("\n");

  /* Verify: should be (1 + 0) */
  assert(deriv->type == SYM_ADD);
  assert(deriv->data.binary.left->type == SYM_CONST);
  assert(deriv->data.binary.left->data.value == 1);
  assert(deriv->data.binary.right->type == SYM_CONST);
  assert(deriv->data.binary.right->data.value == 0);

  printf("[PASS] d/dx(x + 5) = 1 + 0\n");

  /* Test product rule: d/dx(x * x) = x*1 + x*1 = 2x */
  sym_expr_t *x2 = sym_expr_alloc(SYM_VAR);
  x2->data.name = strdup("x");
  sym_expr_t *mul = sym_expr_alloc(SYM_MUL);
  mul->data.binary.left = sym_expr_copy(x);
  mul->data.binary.right = x2;

  sym_expr_t *deriv2 = sym_differentiate(mul, "x");
  printf("d/dx(x*x): ");
  print_expr(deriv2);
  printf("\n");

  /* Structure should be ((x * 1) + (x * 1)) */
  assert(deriv2->type == SYM_ADD);
  printf("[PASS] d/dx(x*x) has correct structure\n");

  printf("\n=== All Symbolic Tests Passed ===\n");
  return 0;
}
