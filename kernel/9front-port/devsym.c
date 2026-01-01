#include "u.h"
#include "../port/error.h"
#include "../port/lib.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "symbolic/mini-gmp.h"

extern void minigmp_init(void);

enum {
  Qdir = 0,
  Qevalr,
  Qevalw,
  Qhelp,
  Qversion,
};

typedef struct SymChanState SymChanState;
struct SymChanState {
  char *resp;
  ulong resp_len;
};

static int sym_ready;

static Dirtab symdir[] = {
    ".", {Qdir, 0, QTDIR}, 0, DMDIR | 0555,   "evalr",   {Qevalr}, 0, 0444,
    "evalw", {Qevalw},     0, 0222,           "help",    {Qhelp}, 0, 0444,
    "version", {Qversion}, 0, 0444,
};

static char sym_help[] =
    "usage: write to evalw, read from evalr\n"
    "expr: (op <a> [<b> [<c>]]) with nesting\n"
    "ops: add sub mul div rem mod pow powm gcd lcm\n"
    "     abs neg cmp and or xor not shl shr popcount\n"
    "numbers are base-10 integers\n";

static void syminit(void) {
  if (sym_ready)
    return;
  minigmp_init();
  sym_ready = 1;
}

static Chan *symattach(char *spec) { return devattach('Z', spec); }

static Walkqid *symwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, symdir, nelem(symdir), devgen);
}

static int symstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, symdir, nelem(symdir), devgen);
}

static Chan *symopen(Chan *c, int omode) {
  SymChanState *st;

  c = devopen(c, omode, symdir, nelem(symdir), devgen);
  if (c->aux == nil && c->qid.path != Qdir) {
    st = smalloc(sizeof(*st));
    if (st == nil)
      error(Enomem);
    st->resp = nil;
    st->resp_len = 0;
    c->aux = st;
  }
  return c;
}

static void symclose(Chan *c) {
  SymChanState *st;

  st = (SymChanState *)c->aux;
  if (st != nil) {
    free(st->resp);
    free(st);
    c->aux = nil;
  }
}

static int sym_is_space(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void sym_skipws(char **pp) {
  char *p = *pp;
  while (*p && sym_is_space((uchar)*p))
    p++;
  *pp = p;
}

static int sym_parse_token(char **pp, char **out) {
  char *p;
  char *tok;
  int len;

  sym_skipws(pp);
  p = *pp;
  if (*p == 0 || *p == '(' || *p == ')')
    return -1;
  tok = p;
  while (*p && !sym_is_space((uchar)*p) && *p != '(' && *p != ')')
    p++;
  len = p - tok;
  *out = smalloc(len + 1);
  if (*out == nil)
    return -1;
  memmove(*out, tok, len);
  (*out)[len] = 0;
  *pp = p;
  return 0;
}

static int sym_parse_expr(char **pp, mpz_t out);

static int sym_expect_rparen(char **pp) {
  sym_skipws(pp);
  if (**pp != ')')
    return -1;
  (*pp)++;
  return 0;
}

static int sym_mpz_to_ulong(const mpz_t v, unsigned long *out) {
  if (mpz_sgn(v) < 0 || !mpz_fits_ulong_p(v))
    return -1;
  *out = mpz_get_ui(v);
  return 0;
}

static int sym_parse_list(char **pp, mpz_t out) {
  char *op;
  mpz_t a, b, c;
  unsigned long ul;
  int cmp;

  if (**pp != '(')
    return -1;
  (*pp)++;
  if (sym_parse_token(pp, &op) < 0)
    return -1;

  if (strcmp(op, "abs") == 0 || strcmp(op, "neg") == 0 ||
      strcmp(op, "not") == 0 || strcmp(op, "popcount") == 0) {
    mpz_init(a);
    if (sym_parse_expr(pp, a) < 0)
      goto bad1;
    if (sym_expect_rparen(pp) < 0)
      goto bad1;
    if (strcmp(op, "abs") == 0)
      mpz_abs(out, a);
    else if (strcmp(op, "neg") == 0)
      mpz_neg(out, a);
    else if (strcmp(op, "not") == 0)
      mpz_com(out, a);
    else
      mpz_set_ui(out, mpz_popcount(a));
    mpz_clear(a);
    free(op);
    return 0;
  }

  if (strcmp(op, "pow") == 0 || strcmp(op, "shl") == 0 ||
      strcmp(op, "shr") == 0) {
    mpz_init(a);
    mpz_init(b);
    if (sym_parse_expr(pp, a) < 0)
      goto bad2;
    if (sym_parse_expr(pp, b) < 0)
      goto bad2;
    if (sym_expect_rparen(pp) < 0)
      goto bad2;
    if (sym_mpz_to_ulong(b, &ul) < 0)
      goto bad2;
    if (strcmp(op, "pow") == 0)
      mpz_pow_ui(out, a, ul);
    else if (strcmp(op, "shl") == 0)
      mpz_mul_2exp(out, a, ul);
    else
      mpz_tdiv_q_2exp(out, a, ul);
    mpz_clear(a);
    mpz_clear(b);
    free(op);
    return 0;
  }

  if (strcmp(op, "powm") == 0) {
    mpz_init(a);
    mpz_init(b);
    mpz_init(c);
    if (sym_parse_expr(pp, a) < 0)
      goto bad3;
    if (sym_parse_expr(pp, b) < 0)
      goto bad3;
    if (sym_parse_expr(pp, c) < 0)
      goto bad3;
    if (sym_expect_rparen(pp) < 0)
      goto bad3;
    if (mpz_sgn(c) == 0)
      goto bad3;
    mpz_powm(out, a, b, c);
    mpz_clear(a);
    mpz_clear(b);
    mpz_clear(c);
    free(op);
    return 0;
  }

  mpz_init(a);
  mpz_init(b);
  if (sym_parse_expr(pp, a) < 0)
    goto bad2;
  if (sym_parse_expr(pp, b) < 0)
    goto bad2;
  if (sym_expect_rparen(pp) < 0)
    goto bad2;

  if (strcmp(op, "add") == 0)
    mpz_add(out, a, b);
  else if (strcmp(op, "sub") == 0)
    mpz_sub(out, a, b);
  else if (strcmp(op, "mul") == 0)
    mpz_mul(out, a, b);
  else if (strcmp(op, "div") == 0) {
    if (mpz_sgn(b) == 0)
      goto bad2;
    mpz_tdiv_q(out, a, b);
  } else if (strcmp(op, "rem") == 0) {
    if (mpz_sgn(b) == 0)
      goto bad2;
    mpz_tdiv_r(out, a, b);
  } else if (strcmp(op, "mod") == 0) {
    if (mpz_sgn(b) == 0)
      goto bad2;
    mpz_mod(out, a, b);
  } else if (strcmp(op, "gcd") == 0) {
    mpz_gcd(out, a, b);
  } else if (strcmp(op, "lcm") == 0) {
    mpz_lcm(out, a, b);
  } else if (strcmp(op, "cmp") == 0) {
    cmp = mpz_cmp(a, b);
    mpz_set_si(out, cmp);
  } else if (strcmp(op, "and") == 0) {
    mpz_and(out, a, b);
  } else if (strcmp(op, "or") == 0) {
    mpz_ior(out, a, b);
  } else if (strcmp(op, "xor") == 0) {
    mpz_xor(out, a, b);
  } else
    goto bad2;

  mpz_clear(a);
  mpz_clear(b);
  free(op);
  return 0;

bad1:
  mpz_clear(a);
  free(op);
  return -1;
bad2:
  mpz_clear(a);
  mpz_clear(b);
  free(op);
  return -1;
bad3:
  mpz_clear(a);
  mpz_clear(b);
  mpz_clear(c);
  free(op);
  return -1;
}

static int sym_parse_number(char **pp, mpz_t out) {
  char *tok;
  int rc;

  if (sym_parse_token(pp, &tok) < 0)
    return -1;
  rc = mpz_set_str(out, tok, 10);
  free(tok);
  return (rc == 0) ? 0 : -1;
}

static int sym_parse_expr(char **pp, mpz_t out) {
  sym_skipws(pp);
  if (**pp == '(')
    return sym_parse_list(pp, out);
  return sym_parse_number(pp, out);
}

static long symread(Chan *c, void *va, long n, vlong offset) {
  SymChanState *st;

  switch (c->qid.path) {
  case Qdir:
    return devdirread(c, va, n, symdir, nelem(symdir), devgen);
  case Qevalr:
    st = (SymChanState *)c->aux;
    if (st == nil || st->resp == nil)
      return 0;
    return readstr(offset, va, n, st->resp);
  case Qhelp:
    return readstr(offset, va, n, sym_help);
  case Qversion:
    return readstr(offset, va, n, "mini-gmp\n");
  default:
    error(Egreg);
    return 0;
  }
}

static long symwrite(Chan *c, void *va, long n, vlong offset) {
  char *buf;
  mpz_t r;
  char *out;
  ulong outlen;
  ulong len;
  SymChanState *st;
  char *p;

  USED(offset);
  if (c->qid.path != Qevalw)
    error(Eperm);

  buf = smalloc(n + 1);
  if (buf == nil)
    error(Enomem);
  memmove(buf, va, n);
  buf[n] = 0;

  mpz_init(r);

  p = buf;
  out = nil;
  if (sym_parse_expr(&p, r) < 0)
    goto badarg;
  sym_skipws(&p);
  if (*p != 0)
    goto badarg;

  outlen = (ulong)mpz_sizeinbase(r, 10) + 3;
  out = smalloc(outlen);
  if (out == nil)
    goto nomem;
  mpz_get_str(out, 10, r);
  len = strlen(out);
  out[len++] = '\n';
  out[len] = 0;

  st = (SymChanState *)c->aux;
  if (st == nil) {
    st = smalloc(sizeof(*st));
    if (st == nil)
      goto nomem;
    st->resp = nil;
    st->resp_len = 0;
    c->aux = st;
  }
  free(st->resp);
  st->resp = out;
  st->resp_len = len;
  out = nil;

  mpz_clear(r);
  free(out);
  free(buf);
  return n;

badarg:
  mpz_clear(r);
  free(out);
  free(buf);
  error(Ebadarg);
  return 0;

nomem:
  mpz_clear(a);
  mpz_clear(b);
  mpz_clear(r);
  free(out);
  free(buf);
  error(Enomem);
  return 0;
}

Dev symdevtab = {
    'Z',    "symbolic",

    devreset, syminit,  devshutdown, symattach, symwalk,
    symstat,  symopen,  devcreate,   symclose,  symread,
    devbread, symwrite, devbwrite,   devremove, devwstat,
};
