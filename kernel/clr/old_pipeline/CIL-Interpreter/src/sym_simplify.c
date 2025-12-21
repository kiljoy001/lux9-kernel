/* sym_simplify.c - Expression Simplification and Caching
 *
 * Implements:
 * - Constant folding (3+5 → 8)
 * - Identity rules (x+0 → x, x*1 → x)
 * - Power rules (x^0 → 1, x^1 → x)
 * - Double negation (--x → x)
 * - Expression caching via hash-consing
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration - defined in execution_engine.h */
typedef struct sym_expr sym_expr_t;

/* ============== Expression Cache (Hash-Consing) ============== */

#define EXPR_CACHE_SIZE 1024

typedef struct expr_cache_entry {
  sym_expr_t *expr;
  uint64_t hash;
  struct expr_cache_entry *next;
} expr_cache_entry_t;

static expr_cache_entry_t *expr_cache[EXPR_CACHE_SIZE];

/* FNV-1a hash for expressions */
static uint64_t expr_hash(sym_expr_t *e) {
  if (!e)
    return 0;
  uint64_t h = 14695981039346656037ULL;

  h ^= (uint64_t)e->type;
  h *= 1099511628211ULL;

  switch (e->type) {
  case SYM_CONST:
    h ^= (uint64_t)e->data.value;
    h *= 1099511628211ULL;
    break;
  case SYM_VAR:
    if (e->data.name) {
      for (const char *p = e->data.name; *p; p++) {
        h ^= (uint64_t)*p;
        h *= 1099511628211ULL;
      }
    }
    break;
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_POW:
    h ^= expr_hash(e->data.binary.left);
    h *= 1099511628211ULL;
    h ^= expr_hash(e->data.binary.right);
    h *= 1099511628211ULL;
    break;
  case SYM_NEG:
  case SYM_SIN:
  case SYM_COS:
  case SYM_TAN:
  case SYM_EXP:
  case SYM_LOG:
  case SYM_SQRT:
    h ^= expr_hash(e->data.unary);
    h *= 1099511628211ULL;
    break;
  default:
    break;
  }
  return h;
}

/* Check structural equality */
static bool expr_equals(sym_expr_t *a, sym_expr_t *b) {
  if (a == b)
    return true;
  if (!a || !b)
    return false;
  if (a->type != b->type)
    return false;

  switch (a->type) {
  case SYM_CONST:
    return a->data.value == b->data.value;
  case SYM_VAR:
    return a->data.name && b->data.name &&
           strcmp(a->data.name, b->data.name) == 0;
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_POW:
    return expr_equals(a->data.binary.left, b->data.binary.left) &&
           expr_equals(a->data.binary.right, b->data.binary.right);
  case SYM_NEG:
  case SYM_SIN:
  case SYM_COS:
  case SYM_TAN:
  case SYM_EXP:
  case SYM_LOG:
  case SYM_SQRT:
    return expr_equals(a->data.unary, b->data.unary);
  default:
    return false;
  }
}

/* Lookup or insert in cache */
sym_expr_t *sym_cache_lookup(sym_expr_t *e) {
  if (!e)
    return NULL;

  uint64_t h = expr_hash(e);
  size_t idx = h % EXPR_CACHE_SIZE;

  /* Check existing entries */
  for (expr_cache_entry_t *ent = expr_cache[idx]; ent; ent = ent->next) {
    if (ent->hash == h && expr_equals(ent->expr, e)) {
      /* Found - return cached version */
      ent->expr->ref_count++;
      return ent->expr;
    }
  }

  /* Not found - add to cache */
  expr_cache_entry_t *new_ent = malloc(sizeof(expr_cache_entry_t));
  if (new_ent) {
    new_ent->expr = e;
    new_ent->hash = h;
    new_ent->next = expr_cache[idx];
    expr_cache[idx] = new_ent;
  }
  return e;
}

void sym_cache_clear(void) {
  for (size_t i = 0; i < EXPR_CACHE_SIZE; i++) {
    expr_cache_entry_t *ent = expr_cache[i];
    while (ent) {
      expr_cache_entry_t *next = ent->next;
      free(ent);
      ent = next;
    }
    expr_cache[i] = NULL;
  }
}

/* ============== Helper Predicates ============== */

static bool is_const(sym_expr_t *e, int64_t v) {
  return e && e->type == SYM_CONST && e->data.value == v;
}

static bool is_zero(sym_expr_t *e) { return is_const(e, 0); }
static bool is_one(sym_expr_t *e) { return is_const(e, 1); }

/* ============== Simplification ============== */

/* Forward declaration for recursive simplify */
sym_expr_t *sym_simplify(sym_expr_t *e);

/* Allocator (should match execution_engine.c) */
static sym_expr_t *sym_alloc(int type) {
  sym_expr_t *e = calloc(1, sizeof(sym_expr_t));
  if (e) {
    e->type = type;
    e->ref_count = 1;
  }
  return e;
}

static sym_expr_t *sym_const_new(int64_t v) {
  sym_expr_t *e = sym_alloc(SYM_CONST);
  e->data.value = v;
  return sym_cache_lookup(e);
}

static sym_expr_t *sym_copy(sym_expr_t *e) {
  if (e)
    e->ref_count++;
  return e;
}

sym_expr_t *sym_simplify(sym_expr_t *e) {
  if (!e)
    return NULL;

  switch (e->type) {
  case SYM_CONST:
  case SYM_VAR:
  case SYM_RATIONAL:
    return sym_copy(e);

  case SYM_ADD: {
    sym_expr_t *l = sym_simplify(e->data.binary.left);
    sym_expr_t *r = sym_simplify(e->data.binary.right);

    /* Constant folding */
    if (l->type == SYM_CONST && r->type == SYM_CONST) {
      return sym_const_new(l->data.value + r->data.value);
    }
    /* Identity: 0 + x = x */
    if (is_zero(l))
      return sym_copy(r);
    /* Identity: x + 0 = x */
    if (is_zero(r))
      return sym_copy(l);

    sym_expr_t *res = sym_alloc(SYM_ADD);
    res->data.binary.left = l;
    res->data.binary.right = r;
    return sym_cache_lookup(res);
  }

  case SYM_SUB: {
    sym_expr_t *l = sym_simplify(e->data.binary.left);
    sym_expr_t *r = sym_simplify(e->data.binary.right);

    /* Constant folding */
    if (l->type == SYM_CONST && r->type == SYM_CONST) {
      return sym_const_new(l->data.value - r->data.value);
    }
    /* Identity: x - 0 = x */
    if (is_zero(r))
      return sym_copy(l);
    /* Identity: x - x = 0 */
    if (expr_equals(l, r))
      return sym_const_new(0);

    sym_expr_t *res = sym_alloc(SYM_SUB);
    res->data.binary.left = l;
    res->data.binary.right = r;
    return sym_cache_lookup(res);
  }

  case SYM_MUL: {
    sym_expr_t *l = sym_simplify(e->data.binary.left);
    sym_expr_t *r = sym_simplify(e->data.binary.right);

    /* Constant folding */
    if (l->type == SYM_CONST && r->type == SYM_CONST) {
      return sym_const_new(l->data.value * r->data.value);
    }
    /* Annihilator: 0 * x = 0 */
    if (is_zero(l) || is_zero(r))
      return sym_const_new(0);
    /* Identity: 1 * x = x */
    if (is_one(l))
      return sym_copy(r);
    /* Identity: x * 1 = x */
    if (is_one(r))
      return sym_copy(l);

    sym_expr_t *res = sym_alloc(SYM_MUL);
    res->data.binary.left = l;
    res->data.binary.right = r;
    return sym_cache_lookup(res);
  }

  case SYM_DIV: {
    sym_expr_t *l = sym_simplify(e->data.binary.left);
    sym_expr_t *r = sym_simplify(e->data.binary.right);

    /* Constant folding (integer division) */
    if (l->type == SYM_CONST && r->type == SYM_CONST && r->data.value != 0) {
      return sym_const_new(l->data.value / r->data.value);
    }
    /* Identity: 0 / x = 0 */
    if (is_zero(l))
      return sym_const_new(0);
    /* Identity: x / 1 = x */
    if (is_one(r))
      return sym_copy(l);
    /* Identity: x / x = 1 */
    if (expr_equals(l, r))
      return sym_const_new(1);

    sym_expr_t *res = sym_alloc(SYM_DIV);
    res->data.binary.left = l;
    res->data.binary.right = r;
    return sym_cache_lookup(res);
  }

  case SYM_POW: {
    sym_expr_t *base = sym_simplify(e->data.binary.left);
    sym_expr_t *exp = sym_simplify(e->data.binary.right);

    /* Power rule: x^0 = 1 */
    if (is_zero(exp))
      return sym_const_new(1);
    /* Power rule: x^1 = x */
    if (is_one(exp))
      return sym_copy(base);
    /* Power rule: 0^n = 0 (n > 0) */
    if (is_zero(base) && exp->type == SYM_CONST && exp->data.value > 0) {
      return sym_const_new(0);
    }
    /* Power rule: 1^n = 1 */
    if (is_one(base))
      return sym_const_new(1);
    /* Constant folding for small powers */
    if (base->type == SYM_CONST && exp->type == SYM_CONST) {
      int64_t b = base->data.value;
      int64_t n = exp->data.value;
      if (n >= 0 && n <= 10) {
        int64_t result = 1;
        for (int64_t i = 0; i < n; i++)
          result *= b;
        return sym_const_new(result);
      }
    }

    sym_expr_t *res = sym_alloc(SYM_POW);
    res->data.binary.left = base;
    res->data.binary.right = exp;
    return sym_cache_lookup(res);
  }

  case SYM_NEG: {
    sym_expr_t *u = sym_simplify(e->data.unary);

    /* Double negation: --x = x */
    if (u->type == SYM_NEG) {
      return sym_copy(u->data.unary);
    }
    /* Constant folding: -c = -c */
    if (u->type == SYM_CONST) {
      return sym_const_new(-u->data.value);
    }

    sym_expr_t *res = sym_alloc(SYM_NEG);
    res->data.unary = u;
    return sym_cache_lookup(res);
  }

  /* Unary functions - just simplify argument */
  case SYM_SIN:
  case SYM_COS:
  case SYM_TAN:
  case SYM_EXP:
  case SYM_LOG:
  case SYM_SQRT: {
    sym_expr_t *u = sym_simplify(e->data.unary);
    sym_expr_t *res = sym_alloc(e->type);
    res->data.unary = u;
    return sym_cache_lookup(res);
  }

  default:
    return sym_copy(e);
  }
}
