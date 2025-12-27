/*
 * symbolic_expr.c - Symbolic Expression Implementation
 *
 * Core implementation of symbolic expression operations
 */

#include "symbolic_expr.h"

/* Expression creation functions */

sym_expr_t *sym_expr_create_var(const char *name) {
  sym_expr_t *expr = malloc(sizeof(sym_expr_t));
  if (!expr)
    return NULL;

  expr->type = SYM_VAR;
  expr->ref_count = 1;
  expr->data.name = strdup(name);

  return expr;
}

sym_expr_t *sym_expr_create_const(long value) {
  sym_expr_t *expr = malloc(sizeof(sym_expr_t));
  if (!expr)
    return NULL;

  expr->type = SYM_CONST;
  expr->ref_count = 1;
  // Store simple integer value for now
  // TODO: Implement proper big integer support

  return expr;
}

sym_expr_t *sym_expr_create_rational(long num, long den) {
  sym_expr_t *expr = malloc(sizeof(sym_expr_t));
  if (!expr)
    return NULL;

  expr->type = SYM_RATIONAL;
  expr->ref_count = 1;
  // TODO: Implement rational number support

  return expr;
}

sym_expr_t *sym_expr_create_unary(sym_type_t type, sym_expr_t *operand) {
  sym_expr_t *expr = malloc(sizeof(sym_expr_t));
  if (!expr)
    return NULL;

  expr->type = type;
  expr->ref_count = 1;
  expr->data.unary = sym_expr_copy(operand);

  return expr;
}

sym_expr_t *sym_expr_create_binary(sym_type_t type, sym_expr_t *left,
                                   sym_expr_t *right) {
  sym_expr_t *expr = malloc(sizeof(sym_expr_t));
  if (!expr)
    return NULL;

  expr->type = type;
  expr->ref_count = 1;
  expr->data.binary.left = sym_expr_copy(left);
  expr->data.binary.right = sym_expr_copy(right);

  return expr;
}

/* Memory management */

sym_expr_t *sym_expr_copy(sym_expr_t *expr) {
  if (!expr)
    return NULL;

  expr->ref_count++;
  return expr;
}

void sym_expr_free(sym_expr_t *expr) {
  if (!expr || --expr->ref_count > 0)
    return;

  switch (expr->type) {
  case SYM_VAR:
    free(expr->data.name);
    break;
  case SYM_CONST:
    // TODO: Clean up big integer
    break;
  case SYM_RATIONAL:
    // TODO: Clean up rational number
    break;
  case SYM_NEG:
    sym_expr_free(expr->data.unary);
    break;
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_POW:
    sym_expr_free(expr->data.binary.left);
    sym_expr_free(expr->data.binary.right);
    break;
  default:
    break;
  }

  free(expr);
}

/* Utility functions */

int sym_expr_equals(sym_expr_t *a, sym_expr_t *b) {
  if (!a || !b)
    return a == b;
  if (a->type != b->type)
    return 0;

  switch (a->type) {
  case SYM_VAR:
    return strcmp(a->data.name, b->data.name) == 0;
  case SYM_CONST:
    // TODO: Compare big integers
    return 1; // Placeholder
  case SYM_RATIONAL:
    // TODO: Compare rational numbers
    return 1; // Placeholder
  case SYM_NEG:
    return sym_expr_equals(a->data.unary, b->data.unary);
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_POW:
    return sym_expr_equals(a->data.binary.left, b->data.binary.left) &&
           sym_expr_equals(a->data.binary.right, b->data.binary.right);
  default:
    return 0;
  }
}

void sym_expr_print(sym_expr_t *expr) {
  if (!expr) {
    print("NULL");
    return;
  }

  switch (expr->type) {
  case SYM_VAR:
    print("%s", expr->data.name);
    break;
  case SYM_CONST:
    print("<const>"); // TODO: Print actual value
    break;
  case SYM_RATIONAL:
    print("<rational>"); // TODO: Print actual value
    break;
  case SYM_NEG:
    print("-");
    sym_expr_print(expr->data.unary);
    break;
  case SYM_ADD:
    print("(");
    sym_expr_print(expr->data.binary.left);
    print(" + ");
    sym_expr_print(expr->data.binary.right);
    print(")");
    break;
  case SYM_SUB:
    print("(");
    sym_expr_print(expr->data.binary.left);
    print(" - ");
    sym_expr_print(expr->data.binary.right);
    print(")");
    break;
  case SYM_MUL:
    print("(");
    sym_expr_print(expr->data.binary.left);
    print(" * ");
    sym_expr_print(expr->data.binary.right);
    print(")");
    break;
  case SYM_DIV:
    print("(");
    sym_expr_print(expr->data.binary.left);
    print(" / ");
    sym_expr_print(expr->data.binary.right);
    print(")");
    break;
  case SYM_POW:
    print("(");
    sym_expr_print(expr->data.binary.left);
    print(" ^ ");
    sym_expr_print(expr->data.binary.right);
    print(")");
    break;
  default:
    print("<?>");
    break;
  }
}
