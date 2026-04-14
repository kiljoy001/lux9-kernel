#include <lux.h>
#include <server9p.h>

/*
 * Symbolic Reasoning Server (symbolicd)
 *
 * Userspace port of devsym.c (#Z)
 * Provides BigInt math evaluation and symbolic graph management via 9P.
 */

#define MINI_GMP_KERNEL 1

/* mini-gmp stubs for freestanding userspace */
#define _ASSERT_H 1
#define _CTYPE_H 1
#define _LIMITS_H 1
#define _STDIO_H 1
#define _STDLIB_H 1
#define _STRING_H 1
#define _FLOAT_H 1

typedef void FILE;
#define stderr ((void *)0)
#define fprintf(f, fmt, ...) (0)
#define fwrite(buf, sz, cnt, f) (0)
#define fputc(c, f) (0)
#define abort() sys_exit("abort")

#undef assert
#define assert(x)                                                              \
  if (!(x))                                                                    \
  sys_exit("assert failed")

/* Prototypes for functions in liblux.a */
void *malloc(ulong size);
void free(void *p);
usize strlen(const char *s);
void *memset(void *s, int c, usize n);
void *memmove(void *dest, const void *src, usize n);

/* Missing implementations in liblux.a */

ulong strtoul(const char *s, char **endp, int base) {
  ulong val = 0;
  (void)base;
  while (*s >= '0' && *s <= '9') {
    val = val * 10 + (ulong)(*s - '0');
    s++;
  }
  if (endp)
    *endp = (char *)s;
  return val;
}

void *realloc(void *old, ulong size) {
  (void)old;
  (void)size;
  sys_exit("realloc called unexpectedly");
  return nil;
}

static inline int gmp_isspace(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
static inline int gmp_isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int gmp_isxdigit(int c) {
  return gmp_isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline int gmp_isalpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline int gmp_isalnum(int c) {
  return gmp_isdigit(c) || gmp_isalpha(c);
}

#define isspace gmp_isspace
#define isdigit gmp_isdigit
#define isxdigit gmp_isxdigit
#define isalpha gmp_isalpha
#define isalnum gmp_isalnum

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif
#ifndef LONG_MAX
#define LONG_MAX 0x7FFFFFFFFFFFFFFFL
#endif
#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX - 1L)
#endif
#ifndef ULONG_MAX
#define ULONG_MAX 0xFFFFFFFFFFFFFFFFUL
#endif
#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif
#ifndef SHRT_MAX
#define SHRT_MAX 32767
#endif
#ifndef SHRT_MIN
#define SHRT_MIN (-SHRT_MAX - 1)
#endif
#ifndef USHRT_MAX
#define USHRT_MAX 65535
#endif
#ifndef UINT_MAX
#define UINT_MAX 0xFFFFFFFFU
#endif

#define MINI_GMP_DONT_USE_FLOAT_H 1

/* Now include mini-gmp implementation */
#include "mini-gmp.c"

/* Custom realloc for mini-gmp */
static void *gmp_ux_realloc(void *old, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    if (old)
      free(old);
    return nil;
  }
  void *new_p = malloc((ulong)new_size);
  if (!new_p)
    return nil;
  if (old) {
    size_t copy_sz = (old_size < new_size) ? old_size : new_size;
    memcpy(new_p, old, (usize)copy_sz);
    free(old);
  }
  return new_p;
}

/* Missing constants from lux.h/libc.h */
#define QTDIR 0x80
#define QTFILE 0x00
#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define MREPL 0
#define MCREATE 4

enum {
  SYM_INT = 1,
  SYM_ATOM,
  SYM_CONS,
  SYM_RAT,
};

typedef struct SymNode {
  int id;
  int type;
  int ref;
  char *owner;
  mpz_t val;
  mpz_t num, den;
  char *str;
  int car;
  int cdr;
  struct SymNode *next;
} SymNode;

typedef struct SymSession {
  char *resp;
  int resp_len;
} SymSession;

#define MAX_NODES 1024
static SymNode *nodes[MAX_NODES];
static int next_node_id = 1;

/* 9P QIDs */
enum {
  Qroot = 0,
  Qevalr,
  Qevalw,
  Qctl,
  Qhelp,
  Qversion,
  Qbandwidth,
  Qconsdir,
  Qnode_dir = 0x10,
  Qnode_type,
  Qnode_ref,
  Qnode_car,
  Qnode_cdr,
  Qnode_num,
  Qnode_den,
  Qnode_data,
};

#define MKQID(id, type) (((u64int)(type) << 16) | (u64int)(id))
#define QID_TYPE(path) ((path >> 16) & 0xFFFF)
#define QID_ID(path) (path & 0xFFFF)

/* Library helpers */

char *strdup_ux(const char *s) {
  usize n = strlen(s);
  char *d = malloc((ulong)n + 1);
  if (d) {
    for (usize i = 0; i <= n; i++)
      d[i] = s[i];
  }
  return d;
}

/* Node management */
static SymNode *new_node(int type, char *user) {
  if (next_node_id >= MAX_NODES)
    return nil;
  SymNode *n = malloc(sizeof(SymNode));
  if (!n)
    return nil;
  memset(n, 0, sizeof(SymNode));
  n->id = next_node_id++;
  n->type = type;
  n->ref = 1;
  n->owner = strdup_ux(user ? user : "eve");

  mpz_init(n->val);
  mpz_init(n->num);
  mpz_init(n->den);
  mpz_set_ui(n->den, 1);

  nodes[n->id] = n;
  return n;
}

/* Math Parser & Evaluator */
static int is_space_p(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
static void skipws_p(char **pp) {
  while (**pp && is_space_p((uchar) * *pp))
    (*pp)++;
}

static char *parse_token_p(char **pp) {
  skipws_p(pp);
  char *p = *pp;
  if (!*p || *p == '(' || *p == ')')
    return nil;
  char *tok = p;
  while (*p && !is_space_p((uchar)*p) && *p != '(' && *p != ')')
    p++;
  int len = (int)(p - tok);
  char *out = malloc((ulong)len + 1);
  memcpy(out, tok, (usize)len);
  out[len] = 0;
  *pp = p;
  return out;
}

static int parse_expr_p(char **pp, mpz_t out);

static int parse_list_p(char **pp, mpz_t out) {
  if (**pp != '(')
    return -1;
  (*pp)++;
  char *op = parse_token_p(pp);
  if (!op)
    return -1;

  mpz_t a, b;
  mpz_init(a);
  mpz_init(b);
  int rc = 0;

  if (parse_expr_p(pp, a) < 0 || parse_expr_p(pp, b) < 0)
    rc = -1;
  else if (strcmp(op, "add") == 0)
    mpz_add(out, a, b);
  else if (strcmp(op, "sub") == 0)
    mpz_sub(out, a, b);
  else if (strcmp(op, "mul") == 0)
    mpz_mul(out, a, b);
  else if (strcmp(op, "div") == 0)
    mpz_tdiv_q(out, a, b);
  else
    rc = -1;

  mpz_clear(a);
  mpz_clear(b);
  free(op);
  skipws_p(pp);
  if (**pp == ')')
    (*pp)++;
  return rc;
}

static int parse_expr_p(char **pp, mpz_t out) {
  skipws_p(pp);
  if (**pp == '(')
    return parse_list_p(pp, out);
  char *tok = parse_token_p(pp);
  if (!tok)
    return -1;
  int rc = mpz_set_str(out, tok, 10);
  free(tok);
  return rc == 0 ? 0 : -1;
}

/* 9P Handlers */
static void p9_attach(Req *r) {
  mkqid(&r->ofcall.qid, Qroot, 0, QTDIR);
  r->fid->qid = r->ofcall.qid;
  srv_respond(r, nil);
}

static void p9_walk(Req *r) {
  u64int path = r->fid->qid.path;
  if (r->ifcall.nwname == 0) {
    r->ofcall.nwqid = 0;
    srv_respond(r, nil);
    return;
  }

  char *name = r->ifcall.wname[0];
  if (path == Qroot) {
    if (strcmp(name, "evalw") == 0)
      mkqid(&r->ofcall.wqid[0], Qevalw, 0, QTFILE);
    else if (strcmp(name, "evalr") == 0)
      mkqid(&r->ofcall.wqid[0], Qevalr, 0, QTFILE);
    else if (strcmp(name, "ctl") == 0)
      mkqid(&r->ofcall.wqid[0], Qctl, 0, QTFILE);
    else if (strcmp(name, "help") == 0)
      mkqid(&r->ofcall.wqid[0], Qhelp, 0, QTFILE);
    else if (strcmp(name, "version") == 0)
      mkqid(&r->ofcall.wqid[0], Qversion, 0, QTFILE);
    else if (strcmp(name, "cons") == 0)
      mkqid(&r->ofcall.wqid[0], Qconsdir, 0, QTDIR);
    else {
      srv_respond(r, "file not found");
      return;
    }
    r->ofcall.nwqid = 1;
    srv_respond(r, nil);
  } else if (path == Qconsdir) {
    int id = (int)strtoul(name, nil, 10);
    if (id > 0 && id < next_node_id && nodes[id]) {
      mkqid(&r->ofcall.wqid[0], MKQID(id, Qnode_dir), 0, QTDIR);
      r->ofcall.nwqid = 1;
      srv_respond(r, nil);
    } else
      srv_respond(r, "node not found");
  } else if (QID_TYPE(path) == Qnode_dir) {
    int id = (int)QID_ID(path);
    if (strcmp(name, "type") == 0)
      mkqid(&r->ofcall.wqid[0], MKQID(id, Qnode_type), 0, QTFILE);
    else if (strcmp(name, "data") == 0)
      mkqid(&r->ofcall.wqid[0], MKQID(id, Qnode_data), 0, QTFILE);
    else {
      srv_respond(r, "file not found");
      return;
    }
    r->ofcall.nwqid = 1;
    srv_respond(r, nil);
  } else
    srv_respond(r, "not a directory");
}

static void p9_open(Req *r) {
  if (r->fid->qid.path == Qevalr || r->fid->qid.path == Qevalw) {
    SymSession *s = malloc(sizeof(SymSession));
    memset(s, 0, sizeof(SymSession));
    r->fid->aux = s;
  }
  srv_respond(r, nil);
}

static void p9_read(Req *r) {
  u64int path = r->fid->qid.path;
  SymSession *s = r->fid->aux;
  char *res = nil;

  if (path == Qevalr && s && s->resp)
    res = s->resp;
  else if (path == Qhelp)
    res = "symbolic math server\n";
  else if (path == Qversion)
    res = "symbolicd 1.0\n";
  else if (QID_TYPE(path) == Qnode_type) {
    SymNode *n = nodes[QID_ID(path)];
    res = (n->type == SYM_INT) ? "int\n" : "other\n";
  } else if (QID_TYPE(path) == Qnode_data) {
    SymNode *n = nodes[QID_ID(path)];
    if (n->type == SYM_INT) {
      char *z_str = mpz_get_str(nil, 10, n->val);
      usize len = strlen(z_str);
      if ((usize)r->ifcall.offset < len) {
        usize n_read = len - (usize)r->ifcall.offset;
        if (n_read > (usize)r->ifcall.count)
          n_read = (usize)r->ifcall.count;
        memcpy(r->ofcall.data, z_str + (usize)r->ifcall.offset, n_read);
        r->ofcall.count = (u32int)n_read;
      } else
        r->ofcall.count = 0;
      free(z_str);
      srv_respond(r, nil);
      return;
    }
  }

  if (res) {
    usize len = strlen(res);
    if ((usize)r->ifcall.offset < len) {
      usize n_read = len - (usize)r->ifcall.offset;
      if (n_read > (usize)r->ifcall.count)
        n_read = (usize)r->ifcall.count;
      memcpy(r->ofcall.data, res + (usize)r->ifcall.offset, n_read);
      r->ofcall.count = (u32int)n_read;
    } else
      r->ofcall.count = 0;
    srv_respond(r, nil);
  } else
    srv_respond(r, "read error");
}

static void p9_write(Req *r) {
  u64int path = r->fid->qid.path;
  SymSession *s = r->fid->aux;

  if (path == Qevalw && s) {
    char *expr = malloc((ulong)r->ifcall.count + 1);
    memcpy(expr, r->ifcall.data, (usize)r->ifcall.count);
    expr[r->ifcall.count] = 0;

    mpz_t z_res;
    mpz_init(z_res);
    char *p = expr;
    if (parse_expr_p(&p, z_res) == 0) {
      if (s->resp)
        free(s->resp);
      s->resp = mpz_get_str(nil, 10, z_res);
    }
    mpz_clear(z_res);
    free(expr);
    r->ofcall.count = r->ifcall.count;
    srv_respond(r, nil);
  } else if (path == Qctl) {
    char *cmd = malloc((ulong)r->ifcall.count + 1);
    memcpy(cmd, r->ifcall.data, (usize)r->ifcall.count);
    cmd[r->ifcall.count] = 0;

    char *p = cmd;
    char *tok = parse_token_p(&p);
    if (tok && strcmp(tok, "int") == 0) {
      char *val = parse_token_p(&p);
      SymNode *n = new_node(SYM_INT, nil);
      if (n)
        mpz_set_str(n->val, val ? val : "0", 10);
      free(val);
    }
    free(tok);
    free(cmd);
    r->ofcall.count = r->ifcall.count;
    srv_respond(r, nil);
  } else
    srv_respond(r, "write error");
}

int main(void) {
  static Srv s = {
      .attach = p9_attach,
      .walk = p9_walk,
      .open = p9_open,
      .read = p9_read,
      .write = p9_write,
  };

  /* Hook up our better realloc to mini-gmp */
  mp_set_memory_functions(gmp_default_alloc, gmp_ux_realloc, gmp_default_free);

  int pipe_fds[2];
  if (sys_pipe(pipe_fds) < 0)
    return 1;
  if (sys_srv_publish("/srv/symbolic", pipe_fds[1]) < 0)
    return 1;
  sys_close(pipe_fds[1]);

  srv_init(&s);
  srv_loop(&s, pipe_fds[0], pipe_fds[0]);
  return 0;
}
