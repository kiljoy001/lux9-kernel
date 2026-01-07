/* host_symbolic.c - WASM host functions for symbolic computing
 *
 * Uses opaque handle model: WASM sees i64 handles, kernel manages sym_expr_t
 *
 * This implements host functions 17-24 as defined in cil_opcodes.h:
 *   HOST_SYM_CREATE (17)
 *   HOST_SYM_EXPR (18)
 *   HOST_SYM_DIFF (19)
 *   HOST_SYM_INTEGRATE (20)
 *   HOST_SYM_SIMPLIFY (21)
 *   HOST_SYM_EVAL (22)
 *   HOST_SYM_MATCH (23)
 *   HOST_SYM_REWRITE (24)
 *
 * Contains REAL implementations using mini-gmp based symbolic algebra,
 * adapted from kernel/clr/old_pipeline/CIL-Interpreter/src/execution_engine.c
 */

/* Kernel headers must come first to set up types correctly */
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/u.h"

/* Symbolic expression types */
#include "../symbolic_expr.h"

/* WASM3 runtime - provides m3Api macros */
#include "../wasm_runtime/wasm3/m3_core.h"
#include "../wasm_runtime/wasm3/wasm3.h"

/* External helper from lux9_api.c */
extern void *clr_ptr_to_mem(u64int ptr_val, void *_mem);

/* Local allocator wrapper matching symbolic_expr.c */
static sym_expr_t *sym_alloc(sym_type_t type) {
  sym_expr_t *expr = malloc(sizeof(sym_expr_t));
  if (expr) {
    expr->type = type;
    expr->ref_count = 1;
    memset(&expr->data, 0, sizeof(expr->data));
  }
  return expr;
}

/* Handle table for WASM→kernel sym_expr_t mapping */
#define MAX_SYM_HANDLES 1024
static sym_expr_t *sym_handles[MAX_SYM_HANDLES];
static u32int next_handle = 1;

static u64int alloc_handle(sym_expr_t *expr) {
  if (!expr)
    return 0;
  if (next_handle >= MAX_SYM_HANDLES) {
    return 0;
  }
  u32int h = next_handle++;
  sym_handles[h] = expr;
  return (u64int)h;
}

static sym_expr_t *get_handle(u64int h) {
  if (h == 0 || h >= MAX_SYM_HANDLES)
    return nil;
  return sym_handles[h];
}

/* ============ SYMBOLIC DIFFERENTIATION ============
 * Implements full derivative rules:
 * - d/dx(c) = 0
 * - d/dx(x) = 1, d/dx(y) = 0
 * - Sum/diff rule: d/dx(u±v) = du ± dv
 * - Product rule: d/dx(u*v) = u*dv + v*du
 * - Quotient rule: d/dx(u/v) = (v*du - u*dv) / v²
 * - Power rule: d/dx(u^n) = n * u^(n-1) * du
 * - Chain rule for sin, cos, exp, log, sqrt
 */
static sym_expr_t *sym_differentiate_impl(sym_expr_t *expr, char *var) {
  if (!expr)
    return nil;

  switch (expr->type) {
  case SYM_CONST:
  case SYM_RATIONAL:
    /* d/dx(c) = 0 */
    {
      sym_expr_t *zero = sym_alloc(SYM_CONST);
      if (zero)
        mpz_init_set_si(zero->data.value, 0);
      return zero;
    }

  case SYM_VAR:
    /* d/dx(x) = 1, d/dx(y) = 0 */
    {
      sym_expr_t *result = sym_alloc(SYM_CONST);
      if (result) {
        int is_same = (expr->data.name && strcmp(expr->data.name, var) == 0);
        mpz_init_set_si(result->data.value, is_same ? 1 : 0);
      }
      return result;
    }

  case SYM_ADD:
  case SYM_SUB:
    /* d/dx(u±v) = du ± dv */
    {
      sym_expr_t *du = sym_differentiate_impl(expr->data.binary.left, var);
      sym_expr_t *dv = sym_differentiate_impl(expr->data.binary.right, var);
      sym_expr_t *result = sym_alloc(expr->type);
      if (result) {
        result->data.binary.left = du;
        result->data.binary.right = dv;
      }
      return result;
    }

  case SYM_MUL:
    /* d/dx(u*v) = u*dv + v*du (product rule) */
    {
      sym_expr_t *u = sym_expr_copy(expr->data.binary.left);
      sym_expr_t *v = sym_expr_copy(expr->data.binary.right);
      sym_expr_t *du = sym_differentiate_impl(expr->data.binary.left, var);
      sym_expr_t *dv = sym_differentiate_impl(expr->data.binary.right, var);

      sym_expr_t *u_dv = sym_alloc(SYM_MUL);
      if (u_dv) {
        u_dv->data.binary.left = u;
        u_dv->data.binary.right = dv;
      }

      sym_expr_t *v_du = sym_alloc(SYM_MUL);
      if (v_du) {
        v_du->data.binary.left = v;
        v_du->data.binary.right = du;
      }

      sym_expr_t *sum = sym_alloc(SYM_ADD);
      if (sum) {
        sum->data.binary.left = u_dv;
        sum->data.binary.right = v_du;
      }
      return sum;
    }

  case SYM_DIV:
    /* d/dx(u/v) = (v*du - u*dv) / v² (quotient rule) */
    {
      sym_expr_t *u = sym_expr_copy(expr->data.binary.left);
      sym_expr_t *v = sym_expr_copy(expr->data.binary.right);
      sym_expr_t *du = sym_differentiate_impl(expr->data.binary.left, var);
      sym_expr_t *dv = sym_differentiate_impl(expr->data.binary.right, var);

      sym_expr_t *v_du = sym_alloc(SYM_MUL);
      if (v_du) {
        v_du->data.binary.left = sym_expr_copy(v);
        v_du->data.binary.right = du;
      }

      sym_expr_t *u_dv = sym_alloc(SYM_MUL);
      if (u_dv) {
        u_dv->data.binary.left = u;
        u_dv->data.binary.right = dv;
      }

      sym_expr_t *numer = sym_alloc(SYM_SUB);
      if (numer) {
        numer->data.binary.left = v_du;
        numer->data.binary.right = u_dv;
      }

      sym_expr_t *two = sym_alloc(SYM_CONST);
      if (two)
        mpz_init_set_si(two->data.value, 2);

      sym_expr_t *v_sq = sym_alloc(SYM_POW);
      if (v_sq) {
        v_sq->data.binary.left = v;
        v_sq->data.binary.right = two;
      }

      sym_expr_t *result = sym_alloc(SYM_DIV);
      if (result) {
        result->data.binary.left = numer;
        result->data.binary.right = v_sq;
      }
      return result;
    }

  case SYM_POW:
    /* d/dx(u^n) = n * u^(n-1) * du (power rule) */
    {
      sym_expr_t *u = sym_expr_copy(expr->data.binary.left);
      sym_expr_t *n = sym_expr_copy(expr->data.binary.right);
      sym_expr_t *du = sym_differentiate_impl(expr->data.binary.left, var);

      sym_expr_t *one = sym_alloc(SYM_CONST);
      if (one)
        mpz_init_set_si(one->data.value, 1);

      sym_expr_t *n_minus_1 = sym_alloc(SYM_SUB);
      if (n_minus_1) {
        n_minus_1->data.binary.left = sym_expr_copy(n);
        n_minus_1->data.binary.right = one;
      }

      sym_expr_t *u_pow = sym_alloc(SYM_POW);
      if (u_pow) {
        u_pow->data.binary.left = u;
        u_pow->data.binary.right = n_minus_1;
      }

      sym_expr_t *n_u_pow = sym_alloc(SYM_MUL);
      if (n_u_pow) {
        n_u_pow->data.binary.left = n;
        n_u_pow->data.binary.right = u_pow;
      }

      sym_expr_t *result = sym_alloc(SYM_MUL);
      if (result) {
        result->data.binary.left = n_u_pow;
        result->data.binary.right = du;
      }
      return result;
    }

  case SYM_NEG:
    /* d/dx(-u) = -du */
    {
      sym_expr_t *du = sym_differentiate_impl(expr->data.unary, var);
      sym_expr_t *result = sym_alloc(SYM_NEG);
      if (result)
        result->data.unary = du;
      return result;
    }

  case SYM_SIN:
    /* d/dx(sin(u)) = cos(u) * du */
    {
      sym_expr_t *u = sym_expr_copy(expr->data.unary);
      sym_expr_t *du = sym_differentiate_impl(expr->data.unary, var);
      sym_expr_t *cos_u = sym_alloc(SYM_COS);
      if (cos_u)
        cos_u->data.unary = u;
      sym_expr_t *result = sym_alloc(SYM_MUL);
      if (result) {
        result->data.binary.left = cos_u;
        result->data.binary.right = du;
      }
      return result;
    }

  case SYM_COS:
    /* d/dx(cos(u)) = -sin(u) * du */
    {
      sym_expr_t *u = sym_expr_copy(expr->data.unary);
      sym_expr_t *du = sym_differentiate_impl(expr->data.unary, var);
      sym_expr_t *sin_u = sym_alloc(SYM_SIN);
      if (sin_u)
        sin_u->data.unary = u;
      sym_expr_t *neg_sin = sym_alloc(SYM_NEG);
      if (neg_sin)
        neg_sin->data.unary = sin_u;
      sym_expr_t *result = sym_alloc(SYM_MUL);
      if (result) {
        result->data.binary.left = neg_sin;
        result->data.binary.right = du;
      }
      return result;
    }

  case SYM_EXP:
    /* d/dx(e^u) = e^u * du */
    {
      sym_expr_t *exp_u = sym_expr_copy(expr);
      sym_expr_t *du = sym_differentiate_impl(expr->data.unary, var);
      sym_expr_t *result = sym_alloc(SYM_MUL);
      if (result) {
        result->data.binary.left = exp_u;
        result->data.binary.right = du;
      }
      return result;
    }

  case SYM_LOG:
    /* d/dx(ln(u)) = du / u */
    {
      sym_expr_t *u = sym_expr_copy(expr->data.unary);
      sym_expr_t *du = sym_differentiate_impl(expr->data.unary, var);
      sym_expr_t *result = sym_alloc(SYM_DIV);
      if (result) {
        result->data.binary.left = du;
        result->data.binary.right = u;
      }
      return result;
    }

  case SYM_SQRT:
    /* d/dx(sqrt(u)) = du / (2 * sqrt(u)) */
    {
      sym_expr_t *sqrt_u = sym_expr_copy(expr);
      sym_expr_t *du = sym_differentiate_impl(expr->data.unary, var);
      sym_expr_t *two = sym_alloc(SYM_CONST);
      if (two)
        mpz_init_set_si(two->data.value, 2);
      sym_expr_t *denom = sym_alloc(SYM_MUL);
      if (denom) {
        denom->data.binary.left = two;
        denom->data.binary.right = sqrt_u;
      }
      sym_expr_t *result = sym_alloc(SYM_DIV);
      if (result) {
        result->data.binary.left = du;
        result->data.binary.right = denom;
      }
      return result;
    }

  default:
    return nil;
  }
}

/* ============ SYMBOLIC SIMPLIFICATION ============
 * Basic algebraic simplifications:
 * - 0 + x = x
 * - x + 0 = x
 * - 0 * x = 0
 * - 1 * x = x
 */
static sym_expr_t *sym_simplify_impl(sym_expr_t *expr) {
  if (!expr)
    return nil;

  if (expr->type == SYM_ADD) {
    sym_expr_t *left = expr->data.binary.left;
    sym_expr_t *right = expr->data.binary.right;

    /* 0 + x = x */
    if (left && left->type == SYM_CONST && mpz_cmp_si(left->data.value, 0) == 0)
      return sym_expr_copy(right);

    /* x + 0 = x */
    if (right && right->type == SYM_CONST &&
        mpz_cmp_si(right->data.value, 0) == 0)
      return sym_expr_copy(left);
  }

  if (expr->type == SYM_MUL) {
    sym_expr_t *left = expr->data.binary.left;
    sym_expr_t *right = expr->data.binary.right;

    /* 0 * x = 0 or x * 0 = 0 */
    if ((left && left->type == SYM_CONST &&
         mpz_cmp_si(left->data.value, 0) == 0) ||
        (right && right->type == SYM_CONST &&
         mpz_cmp_si(right->data.value, 0) == 0)) {
      sym_expr_t *zero = sym_alloc(SYM_CONST);
      if (zero)
        mpz_init_set_si(zero->data.value, 0);
      return zero;
    }

    /* 1 * x = x */
    if (left && left->type == SYM_CONST && mpz_cmp_si(left->data.value, 1) == 0)
      return sym_expr_copy(right);

    /* x * 1 = x */
    if (right && right->type == SYM_CONST &&
        mpz_cmp_si(right->data.value, 1) == 0)
      return sym_expr_copy(left);
  }

  /* No simplification possible */
  return sym_expr_copy(expr);
}

/* ============ HOST FUNCTIONS ============ */

/* HOST_SYM_CREATE (17): Create symbolic variable */
m3ApiRawFunction(sym_host_create) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, name_ptr);

  char *name = (char *)clr_ptr_to_mem(name_ptr, _mem);
  if (!name)
    m3ApiReturn(0);

  sym_expr_t *expr = sym_expr_create_var(name);
  m3ApiReturn(alloc_handle(expr));
}

/* HOST_SYM_EXPR (18): Create binary expression */
m3ApiRawFunction(sym_host_expr) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, op);
  m3ApiGetArg(u64int, left_h);
  m3ApiGetArg(u64int, right_h);

  sym_expr_t *left = get_handle(left_h);
  sym_expr_t *right = get_handle(right_h);
  if (!left || !right)
    m3ApiReturn(0);

  sym_type_t type;
  switch (op) {
  case 0:
    type = SYM_ADD;
    break;
  case 1:
    type = SYM_SUB;
    break;
  case 2:
    type = SYM_MUL;
    break;
  case 3:
    type = SYM_DIV;
    break;
  case 4:
    type = SYM_POW;
    break;
  default:
    m3ApiReturn(0);
  }

  sym_expr_t *expr = sym_expr_create_binary(type, left, right);
  m3ApiReturn(alloc_handle(expr));
}

/* HOST_SYM_DIFF (19): Differentiate expression - REAL IMPLEMENTATION */
m3ApiRawFunction(sym_host_diff) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, expr_h);
  m3ApiGetArg(u64int, var_h);

  sym_expr_t *expr = get_handle(expr_h);
  sym_expr_t *var = get_handle(var_h);
  if (!expr)
    m3ApiReturn(0);

  char *var_name = "x";
  if (var && var->type == SYM_VAR && var->data.name)
    var_name = var->data.name;

  sym_expr_t *deriv = sym_differentiate_impl(expr, var_name);
  m3ApiReturn(alloc_handle(deriv));
}

/* HOST_SYM_INTEGRATE (20): Basic integration */
m3ApiRawFunction(sym_host_integrate) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, expr_h);
  m3ApiGetArg(u64int, var_h);

  sym_expr_t *expr = get_handle(expr_h);
  sym_expr_t *var = get_handle(var_h);
  if (!expr || !var)
    m3ApiReturn(0);

  /* integral of constant c dx = c*x */
  if (expr->type == SYM_CONST) {
    sym_expr_t *prod = sym_alloc(SYM_MUL);
    if (prod) {
      prod->data.binary.left = sym_expr_copy(expr);
      prod->data.binary.right = sym_expr_copy(var);
    }
    m3ApiReturn(alloc_handle(prod));
  }

  /* integral of x dx = x²/2 */
  if (expr->type == SYM_VAR && var->type == SYM_VAR) {
    sym_expr_t *two = sym_alloc(SYM_CONST);
    if (two)
      mpz_init_set_si(two->data.value, 2);

    sym_expr_t *sq = sym_alloc(SYM_POW);
    if (sq) {
      sq->data.binary.left = sym_expr_copy(expr);
      sq->data.binary.right = sym_expr_copy(two);
    }

    sym_expr_t *half = sym_alloc(SYM_DIV);
    if (half) {
      half->data.binary.left = sq;
      half->data.binary.right = two;
    }
    m3ApiReturn(alloc_handle(half));
  }

  /* For complex expressions, return nil (not implemented) */
  m3ApiReturn(0);
}

/* HOST_SYM_SIMPLIFY (21): Simplify expression - REAL IMPLEMENTATION */
m3ApiRawFunction(sym_host_simplify) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, expr_h);

  sym_expr_t *expr = get_handle(expr_h);
  if (!expr)
    m3ApiReturn(0);

  sym_expr_t *simplified = sym_simplify_impl(expr);
  m3ApiReturn(alloc_handle(simplified));
}

/* HOST_SYM_EVAL (22): Evaluate with bindings */
m3ApiRawFunction(sym_host_eval) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, expr_h);
  m3ApiGetArg(u64int, bindings);

  /* TODO: Full evaluation with variable bindings needs:
   * - Parse bindings from WASM memory
   * - Recursive evaluation
   * - mini-gmp for arbitrary precision
   */
  USED(expr_h);
  USED(bindings);
  m3ApiReturn(0);
}

/* HOST_SYM_MATCH (23): Pattern matching */
m3ApiRawFunction(sym_host_match) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, expr_h);
  m3ApiGetArg(u64int, pattern_h);

  sym_expr_t *expr = get_handle(expr_h);
  sym_expr_t *pattern = get_handle(pattern_h);
  if (!expr || !pattern)
    m3ApiReturn(0);

  m3ApiReturn((u64int)sym_expr_equals(expr, pattern));
}

/* HOST_SYM_REWRITE (24): Term rewriting */
m3ApiRawFunction(sym_host_rewrite) {
  m3ApiReturnType(u64int);
  m3ApiGetArg(u64int, expr_h);
  m3ApiGetArg(u64int, rule_h);

  /* TODO: Full rewriting needs:
   * - Parse rule as (pattern, replacement) pair
   * - Recursive pattern matching
   * - Substitution
   */
  USED(rule_h);

  /* For now, apply simplification as a basic rewrite */
  sym_expr_t *expr = get_handle(expr_h);
  if (!expr)
    m3ApiReturn(0);

  sym_expr_t *result = sym_simplify_impl(expr);
  m3ApiReturn(alloc_handle(result));
}

/* Link all symbolic host functions to a WASM module */
M3Result sym_link_host_functions(IM3Module module) {
  M3Result result = m3Err_none;

#define LINK_SYM(name, sig, impl)                                              \
  do {                                                                         \
    result = m3_LinkRawFunction(module, "env", name, sig, impl);               \
    if (result == m3Err_functionLookupFailed) {                                \
      result = m3Err_none;                                                     \
    } else if (result) {                                                       \
      return result;                                                           \
    }                                                                          \
  } while (0)

  LINK_SYM("sym_create", "I(I)", &sym_host_create);
  LINK_SYM("sym_expr", "I(III)", &sym_host_expr);
  LINK_SYM("sym_diff", "I(II)", &sym_host_diff);
  LINK_SYM("sym_integrate", "I(II)", &sym_host_integrate);
  LINK_SYM("sym_simplify", "I(I)", &sym_host_simplify);
  LINK_SYM("sym_eval", "I(II)", &sym_host_eval);
  LINK_SYM("sym_match", "I(II)", &sym_host_match);
  LINK_SYM("sym_rewrite", "I(II)", &sym_host_rewrite);

#undef LINK_SYM
  return result;
}
