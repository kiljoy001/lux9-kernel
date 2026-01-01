#include "u.h"
#include "../port/error.h"
#include "../port/lib.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"

/*
 * /dev/pebble - Pebble Memory Accounting Interface
 *
 * Provides 9P interface for Pebble memory operations, replacing
 * syscalls to align with "Everything is a file" philosophy.
 */

enum {
  Qdir = 0,
  Qissue,
  Qverify,
  Qalloc,
  Qfree,
  Qstats,
  Qbudget,
};

typedef struct PebbleChanState {
  char *resp;
  ulong resp_len;
} PebbleChanState;

static Dirtab pebbledir[] = {
    ".",      {Qdir, 0, QTDIR}, 0, DMDIR | 0555, "issue", {Qissue}, 0, 0666,
    "verify", {Qverify},        0, 0666,         "alloc", {Qalloc}, 0, 0666,
    "free",   {Qfree},          0, 0666,         "stats", {Qstats}, 0, 0444,
    "budget", {Qbudget},        0, 0666,
};

static void pebinit(void) { print("pebble: 9P interface initialized\n"); }

static Chan *pebattach(char *spec) { return devattach('B', spec); }

static Walkqid *pebwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, pebbledir, nelem(pebbledir), devgen);
}

static int pebstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, pebbledir, nelem(pebbledir), devgen);
}

static Chan *pebopen(Chan *c, int omode) {
  PebbleChanState *st;

  c = devopen(c, omode, pebbledir, nelem(pebbledir), devgen);
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

static void pebclose(Chan *c) {
  PebbleChanState *st;

  st = (PebbleChanState *)c->aux;
  if (st != nil) {
    free(st->resp);
    free(st);
    c->aux = nil;
  }
}

static int pebhexval(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

static int pebparse_hex_bytes(const char *p, uchar *out, int outlen) {
  int i, hi, lo;

  for (i = 0; i < outlen; i++) {
    hi = pebhexval(*p++);
    lo = pebhexval(*p++);
    if (hi < 0 || lo < 0)
      return -1;
    out[i] = (hi << 4) | lo;
  }
  return 0;
}

static int pebparse_cap_hash(const char *buf, UserCapability *cap) {
  const char *p = buf;

  while (*p == ' ' || *p == '\t')
    p++;
  if (strncmp(p, "cap", 3) == 0 && (p[3] == ' ' || p[3] == '\t')) {
    p += 3;
  } else if (strncmp(p, "hash", 4) == 0 &&
             (p[4] == ' ' || p[4] == '\t')) {
    p += 4;
  }
  while (*p == ' ' || *p == '\t')
    p++;
  if (*p == 0)
    return -1;
  if (pebparse_hex_bytes(p, cap->hash, BLIND_LEDGER_CAP_SIZE) < 0)
    return -1;
  cap->size = 0;
  cap->type = 0;
  cap->perms = 0;
  return 0;
}

static int pebparse_uintptr(const char *buf, uintptr *out) {
  char *end;
  uvlong v;
  const char *p = buf;

  while (*p == ' ' || *p == '\t')
    p++;
  if (strncmp(p, "white", 5) == 0 && (p[5] == ' ' || p[5] == '\t')) {
    p += 5;
    while (*p == ' ' || *p == '\t')
      p++;
  }
  v = strtoull(p, &end, 0);
  if (end == p)
    return -1;
  *out = (uintptr)v;
  return 0;
}

static void pebsetresp(Chan *c, const char *msg) {
  PebbleChanState *st;
  ulong len;

  st = (PebbleChanState *)c->aux;
  if (st == nil)
    return;
  free(st->resp);
  len = strlen(msg);
  st->resp = smalloc(len + 1);
  if (st->resp == nil)
    error(Enomem);
  memmove(st->resp, msg, len + 1);
  st->resp_len = len;
}

static long pebread(Chan *c, void *va, long n, vlong off) {
  char *buf;
  long rv = 0;
  PebbleChanState *st;

  switch ((ulong)c->qid.path) {
  case Qdir:
    return devdirread(c, va, n, pebbledir, nelem(pebbledir), devgen);

  case Qissue:
  case Qverify:
  case Qalloc:
  case Qfree:
  case Qbudget:
    st = (PebbleChanState *)c->aux;
    if (st == nil || st->resp == nil)
      return 0;
    return readstr(off, va, n, st->resp);

  case Qstats:
    buf = smalloc(1024);
    /* Assuming pebble_state() returns current proc's state or global stats */
    PebbleState *ps = pebble_state();
    if (ps) {
      rv = snprint(
          buf, 1024, "budget: %lud\ninuse: %lud\nred: %lud\nblue: %lud\n",
          ps->colorless_bank, ps->black_inuse, ps->red_count, ps->blue_count);
    } else {
      rv = snprint(buf, 1024, "pebble state unavailable\n");
    }
    rv = readstr(off, va, n, buf);
    free(buf);
    return rv;

  default:
    error(Eperm);
  }
  return 0;
}

static long pebwrite(Chan *c, void *va, long n, vlong off) {
  char *buf;
  char tmp[256];
  ulong size;
  PebbleWhite *pw;
  void *handle;
  PebbleState *ps;

  USED(off);

  buf = smalloc(n + 1);
  if (buf == nil)
    error(Enomem);
  memmove(buf, va, n);
  buf[n] = 0;

  ps = pebble_state();
  if (ps == nil) {
    free(buf);
    error("pebble state not initialized");
  }

  switch ((ulong)c->qid.path) {
  case Qissue:
    size = strtoul(buf, 0, 0);
    if (size == 0) {
      free(buf);
      error(Ebadarg);
    }
    pw = pebble_issue_white(ps, nil, size);
    if (pw == nil) {
      free(buf);
      error(PEBBLE_E_AGAIN);
    }
    snprint(tmp, sizeof(tmp), "%#p\n", pw);
    pebsetresp(c, tmp);
    break;

  case Qalloc:
    /* Simple alloc: "size <bytes>" */
    size = strtoul(buf, 0, 0);
    if (size > 0) {
      UserCapability cap;
      void *addr;
      if (pebble_alloc_with_white(size, &cap, &addr) == 0) {
        snprint(tmp, sizeof(tmp), "cap %H size %llud perms %ud addr %#p\n", cap.hash,
                cap.size, cap.perms, addr);
        pebsetresp(c, tmp);
      } else {
        free(buf);
        error(Enomem);
      }
    } else {
      free(buf);
      error(Ebadarg);
    }
    break;

  case Qverify: {
    uintptr wptr;
    if (pebparse_uintptr(buf, &wptr) < 0) {
      free(buf);
      error(Ebadarg);
    }
    handle = nil;
    if (pebble_white_verify((PebbleWhite *)wptr, &handle) != 0) {
      free(buf);
      error(PEBBLE_E_PERM);
    }
    snprint(tmp, sizeof(tmp), "%#p\n", handle);
    pebsetresp(c, tmp);
  } break;

  case Qfree: {
    UserCapability cap;
    if (pebparse_cap_hash(buf, &cap) < 0) {
      free(buf);
      error(Ebadarg);
    }
    pebble_black_free(&cap);
    pebsetresp(c, "ok\n");
  } break;

  case Qbudget:
    /* Budget request: "size nonce" */
    /* Parse size and nonce from textual input */
    {
      char *sp;
      ulong req_size;
      u64int nonce;

      req_size = strtoul(buf, &sp, 0);
      if (sp == buf || req_size == 0) {
        free(buf);
        error(Ebadarg);
      }
      while (*sp == ' ' || *sp == '\t')
        sp++;
      nonce = strtoull(sp, 0, 0);

      if (pebble_increase_budget(req_size, nonce) < 0) {
        free(buf);
        error("pebble: PoW verification failed");
      }
      snprint(tmp, sizeof(tmp), "ok budget %lud\n", pebble_get_budget());
      pebsetresp(c, tmp);
    }
    break;

  default:
    free(buf);
    error(Eperm);
  }

  free(buf);
  return n;
}

Dev pebbledevtab = {
    'B',      "pebble",

    devreset, pebinit,  devshutdown, pebattach, pebwalk,
    pebstat,  pebopen,  devcreate,   pebclose,  pebread,
    devbread, pebwrite, devbwrite,   devremove, devwstat,
};
