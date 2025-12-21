/* Test Simplification and Caching */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* === Types === */
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

/* === Cache === */
#define CACHE_SIZE 256
static sym_expr_t *cache[CACHE_SIZE];
static int cache_hits = 0, cache_misses = 0;

static uint64_t hash_expr(sym_expr_t *e) {
  if (!e)
    return 0;
  uint64_t h = 14695981039346656037ULL;
  h ^= (uint64_t)e->type;
  h *= 1099511628211ULL;
  switch (e->type) {
  case SYM_CONST:
    h ^= (uint64_t)e->data.value;
    break;
  case SYM_VAR:
    for (char *p = e->data.name; p && *p; p++) {
      h ^= *p;
      h *= 1099511628211ULL;
    }
    break;
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_POW:
    h ^= hash_expr(e->data.binary.left);
    h *= 1099511628211ULL;
    h ^= hash_expr(e->data.binary.right);
    break;
  default:
    if (e->type >= SYM_NEG)
      h ^= hash_expr(e->data.unary);
    break;
  }
  return h;
}

static bool expr_eq(sym_expr_t *a, sym_expr_t *b) {
  if (a == b)
    return true;
  if (!a || !b || a->type != b->type)
    return false;
  switch (a->type) {
  case SYM_CONST:
    return a->data.value == b->data.value;
  case SYM_VAR:
    return strcmp(a->data.name, b->data.name) == 0;
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_POW:
    return expr_eq(a->data.binary.left, b->data.binary.left) &&
           expr_eq(a->data.binary.right, b->data.binary.right);
  default:
    return expr_eq(a->data.unary, b->data.unary);
  }
}

static sym_expr_t *cache_get(sym_expr_t *e) {
  size_t idx = hash_expr(e) % CACHE_SIZE;
  if (cache[idx] && expr_eq(cache[idx], e)) {
    cache_hits++;
    return cache[idx];
  }
  cache_misses++;
  cache[idx] = e;
  return e;
}

/* === Constructors === */
static sym_expr_t *alloc(sym_type_t t) {
  sym_expr_t *e = calloc(1, sizeof(sym_expr_t));
  e->type = t;
  e->ref_count = 1;
  return e;
}
static sym_expr_t *C(int64_t v) {
  sym_expr_t *e = alloc(SYM_CONST);
  e->data.value = v;
  return cache_get(e);
}
static sym_expr_t *V(const char *n) {
  sym_expr_t *e = alloc(SYM_VAR);
  e->data.name = strdup(n);
  return cache_get(e);
}
static sym_expr_t *B(sym_type_t t, sym_expr_t *l, sym_expr_t *r) {
  sym_expr_t *e = alloc(t);
  e->data.binary.left = l;
  e->data.binary.right = r;
  return cache_get(e);
}
static sym_expr_t *U(sym_type_t t, sym_expr_t *u) {
  sym_expr_t *e = alloc(t);
  e->data.unary = u;
  return cache_get(e);
}

/* === Simplify === */
static bool is0(sym_expr_t *e) {
  return e && e->type == SYM_CONST && e->data.value == 0;
}
static bool is1(sym_expr_t *e) {
  return e && e->type == SYM_CONST && e->data.value == 1;
}

static sym_expr_t *simplify(sym_expr_t *e) {
  if (!e)
    return NULL;
  switch (e->type) {
  case SYM_CONST:
  case SYM_VAR:
    return e;
  case SYM_ADD: {
    sym_expr_t *l = simplify(e->data.binary.left);
    sym_expr_t *r = simplify(e->data.binary.right);
    if (l->type == SYM_CONST && r->type == SYM_CONST)
      return C(l->data.value + r->data.value);
    if (is0(l))
      return r;
    if (is0(r))
      return l;
    return B(SYM_ADD, l, r);
  }
  case SYM_SUB: {
    sym_expr_t *l = simplify(e->data.binary.left);
    sym_expr_t *r = simplify(e->data.binary.right);
    if (l->type == SYM_CONST && r->type == SYM_CONST)
      return C(l->data.value - r->data.value);
    if (is0(r))
      return l;
    if (expr_eq(l, r))
      return C(0);
    return B(SYM_SUB, l, r);
  }
  case SYM_MUL: {
    sym_expr_t *l = simplify(e->data.binary.left);
    sym_expr_t *r = simplify(e->data.binary.right);
    if (l->type == SYM_CONST && r->type == SYM_CONST)
      return C(l->data.value * r->data.value);
    if (is0(l) || is0(r))
      return C(0);
    if (is1(l))
      return r;
    if (is1(r))
      return l;
    return B(SYM_MUL, l, r);
  }
  case SYM_DIV: {
    sym_expr_t *l = simplify(e->data.binary.left);
    sym_expr_t *r = simplify(e->data.binary.right);
    if (l->type == SYM_CONST && r->type == SYM_CONST && r->data.value != 0)
      return C(l->data.value / r->data.value);
    if (is0(l))
      return C(0);
    if (is1(r))
      return l;
    if (expr_eq(l, r))
      return C(1);
    return B(SYM_DIV, l, r);
  }
  case SYM_POW: {
    sym_expr_t *base = simplify(e->data.binary.left);
    sym_expr_t *exp = simplify(e->data.binary.right);
    if (is0(exp))
      return C(1); /* x^0 = 1 */
    if (is1(exp))
      return base; /* x^1 = x */
    if (is0(base))
      return C(0); /* 0^n = 0 */
    if (is1(base))
      return C(1); /* 1^n = 1 */
    /* Constant folding */
    if (base->type == SYM_CONST && exp->type == SYM_CONST &&
        exp->data.value >= 0 && exp->data.value <= 10) {
      int64_t r = 1;
      for (int i = 0; i < exp->data.value; i++)
        r *= base->data.value;
      return C(r);
    }
    return B(SYM_POW, base, exp);
  }
  case SYM_NEG: {
    sym_expr_t *u = simplify(e->data.unary);
    if (u->type == SYM_CONST)
      return C(-u->data.value);
    if (u->type == SYM_NEG)
      return u->data.unary; /* --x = x */
    return U(SYM_NEG, u);
  }
  default:
    return U(e->type, simplify(e->data.unary));
  }
}

/* === Print === */
static void print(sym_expr_t *e) {
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
    print(e->data.binary.left);
    printf("+");
    print(e->data.binary.right);
    printf(")");
    break;
  case SYM_SUB:
    printf("(");
    print(e->data.binary.left);
    printf("-");
    print(e->data.binary.right);
    printf(")");
    break;
  case SYM_MUL:
    printf("(");
    print(e->data.binary.left);
    printf("*");
    print(e->data.binary.right);
    printf(")");
    break;
  case SYM_DIV:
    printf("(");
    print(e->data.binary.left);
    printf("/");
    print(e->data.binary.right);
    printf(")");
    break;
  case SYM_POW:
    printf("(");
    print(e->data.binary.left);
    printf("^");
    print(e->data.binary.right);
    printf(")");
    break;
  case SYM_NEG:
    printf("-");
    print(e->data.unary);
    break;
  default:
    printf("?");
    break;
  }
}

/* === Tests === */
static int tests = 0, passed = 0;
#define TEST(name, expr, expected_val)                                         \
  do {                                                                         \
    tests++;                                                                   \
    sym_expr_t *e = expr;                                                      \
    sym_expr_t *s = simplify(e);                                               \
    printf("%-25s ", name);                                                    \
    print(e);                                                                  \
    printf(" -> ");                                                            \
    print(s);                                                                  \
    if (s->type == SYM_CONST && s->data.value == expected_val) {               \
      printf(" [PASS]\n");                                                     \
      passed++;                                                                \
    } else {                                                                   \
      printf(" [FAIL] expected %lld\n", (long long)expected_val);              \
    }                                                                          \
  } while (0)

#define TEST_EXPR(name, expr, check)                                           \
  do {                                                                         \
    tests++;                                                                   \
    sym_expr_t *e = expr;                                                      \
    sym_expr_t *s = simplify(e);                                               \
    printf("%-25s ", name);                                                    \
    print(e);                                                                  \
    printf(" -> ");                                                            \
    print(s);                                                                  \
    if (check) {                                                               \
      printf(" [PASS]\n");                                                     \
      passed++;                                                                \
    } else {                                                                   \
      printf(" [FAIL]\n");                                                     \
    }                                                                          \
  } while (0)

int main() {
  printf("=== Simplification Test Suite ===\n\n");

  /* Constant folding */
  TEST("3 + 5", B(SYM_ADD, C(3), C(5)), 8);
  TEST("10 - 4", B(SYM_SUB, C(10), C(4)), 6);
  TEST("3 * 4", B(SYM_MUL, C(3), C(4)), 12);
  TEST("15 / 3", B(SYM_DIV, C(15), C(3)), 5);
  TEST("2^3", B(SYM_POW, C(2), C(3)), 8);
  TEST("2^10", B(SYM_POW, C(2), C(10)), 1024);

  /* Identity rules */
  TEST_EXPR("0 + x = x", B(SYM_ADD, C(0), V("x")), s->type == SYM_VAR);
  TEST_EXPR("x + 0 = x", B(SYM_ADD, V("x"), C(0)), s->type == SYM_VAR);
  TEST_EXPR("x - 0 = x", B(SYM_SUB, V("x"), C(0)), s->type == SYM_VAR);
  TEST_EXPR("1 * x = x", B(SYM_MUL, C(1), V("x")), s->type == SYM_VAR);
  TEST_EXPR("x * 1 = x", B(SYM_MUL, V("x"), C(1)), s->type == SYM_VAR);
  TEST_EXPR("x / 1 = x", B(SYM_DIV, V("x"), C(1)), s->type == SYM_VAR);

  /* Annihilator */
  TEST("0 * x", B(SYM_MUL, C(0), V("x")), 0);
  TEST("x * 0", B(SYM_MUL, V("x"), C(0)), 0);
  TEST("0 / x", B(SYM_DIV, C(0), V("x")), 0);

  /* Power rules */
  TEST_EXPR("x^0 = 1", B(SYM_POW, V("x"), C(0)),
            s->type == SYM_CONST && s->data.value == 1);
  TEST_EXPR("x^1 = x", B(SYM_POW, V("x"), C(1)), s->type == SYM_VAR);
  TEST("0^5 = 0", B(SYM_POW, C(0), C(5)), 0);
  TEST("1^99 = 1", B(SYM_POW, C(1), C(99)), 1);

  /* Double negation */
  TEST_EXPR("--x = x", U(SYM_NEG, U(SYM_NEG, V("x"))), s->type == SYM_VAR);
  TEST("-5", U(SYM_NEG, C(5)), -5);

  /* Self-cancellation */
  TEST("x - x", B(SYM_SUB, V("x"), V("x")), 0);
  TEST_EXPR("x / x = 1", B(SYM_DIV, V("x"), V("x")),
            s->type == SYM_CONST && s->data.value == 1);

  /* Nested simplification */
  TEST("(2+3)*(4-2)",
       B(SYM_MUL, B(SYM_ADD, C(2), C(3)), B(SYM_SUB, C(4), C(2))), 10);

  printf("\n=== Results: %d/%d passed ===\n", passed, tests);
  printf("Cache stats: %d hits, %d misses (%.1f%% hit rate)\n", cache_hits,
         cache_misses,
         100.0 * cache_hits / (cache_hits + cache_misses + 0.001));

  return passed == tests ? 0 : 1;
}
