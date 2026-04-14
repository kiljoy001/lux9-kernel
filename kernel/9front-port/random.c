#include "dat.h"
#include "fns.h"
#include "lock_borrow.h"
#include "mem.h"
#include "monocypher.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

#include "libsec.h"

/* machine specific hardware random number generator */
extern void (*hwrandbuf)(void *, ulong);

static struct {
  QLock qlock;
  uchar key[32];
  uchar nonce[12];
  u32int counter;
} *rs;
static int random_initialized;
static LockDagNode lockdag_random_lrand = LOCKDAG_NODE("random-lrand");

typedef struct Seedbuf Seedbuf;
struct Seedbuf {
  ulong randomcount;
  uchar buf[64];
  uchar nbuf;
  uchar next;
  ushort bits;

  SHA2_512state ds;
};

static u32int
load32le(const uchar *p)
{
  return (u32int)p[0] | ((u32int)p[1] << 8) | ((u32int)p[2] << 16) |
         ((u32int)p[3] << 24);
}

static void
randomrekey_locked(uchar key[32], uchar nonce[12], u32int counter)
{
  uchar rekey[44];
  u32int nextctr;

  nextctr = crypto_chacha20_ietf(rekey, nil, sizeof(rekey), key, nonce, counter);
  memmove(rs->key, rekey, sizeof(rs->key));
  memmove(rs->nonce, rekey + sizeof(rs->key), sizeof(rs->nonce));
  rs->counter = nextctr != 0 ? nextctr : 1;
  memset(rekey, 0, sizeof(rekey));
}

/*@
  @ requires  == \null || \valid();
  @ requires t == \null || \valid(t);
  @ assigns \nothing;
  @*/
static void randomsample(Ureg *, Timer *t) {
  Seedbuf *s = t->ta;

  if (s->randomcount == 0 || s->nbuf >= sizeof(s->buf))
    return;
  s->bits = (s->bits << 2) ^ s->randomcount;
  s->randomcount = 0;
  if (++s->next < 8 / 2)
    return;
  s->next = 0;
  s->buf[s->nbuf++] ^= s->bits;
  if (s->nbuf % 8 == 0)
    print("randomsample: nbuf=%d\n", s->nbuf);
}

/*@
  @ requires  == \null || \valid();
  @ assigns \nothing;
  @*/
static void randomseed(void *) {
  Seedbuf *s;
  uchar seed[64];

  s = secalloc(sizeof(Seedbuf));

  if (hwrandbuf != nil)
    (*hwrandbuf)(s->buf, sizeof(s->buf));

  /* Frequency close but not equal to HZ */
  up->tns = (vlong)(MS2HZ + 3) * 1000000LL;
  up->tmode = Tperiodic;
  up->tt = nil;
  up->ta = s;
  up->tf = randomsample;
  timeradd(&up->timer);
  print("randomseed: starting jitter loop, HZ=%d\n", HZ);
  while (*(volatile uchar *)&s->nbuf < 8) {
    if (++s->randomcount <= 10000)
      continue;
    sched();
    s->randomcount = 0;
  }
  timerdel(&up->timer);

  crypto_blake2b(seed, sizeof(seed), s->buf, sizeof(s->buf));
  memmove(rs->key, seed, sizeof(rs->key));
  memmove(rs->nonce, seed + sizeof(rs->key), sizeof(rs->nonce));
  rs->counter = load32le(seed + sizeof(rs->key) + sizeof(rs->nonce));
  if (rs->counter == 0)
    rs->counter = 1;
  qunlock(&rs->qlock);

  memset(seed, 0, sizeof(seed));
  secfree(s);

  pexit("", 1);
}

/*@
  @ assigns \nothing;
  @*/
void randominit(void) {
  if (random_initialized) {
    print("randominit: already initialized\n");
    return;
  }
  random_initialized = 1;
  rs = secalloc(sizeof(*rs));
  qlock(&rs->qlock); /* randomseed() unlocks once seeded */
  kproc("randomseed", randomseed, nil);
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
ulong randomread(void *p, ulong n) {
  uchar key[32];
  uchar nonce[12];
  u32int counter;
  uchar *buf;

  if (p == nil || n == 0)
    return 0;
  buf = p;

  if (rs == nil) {
    extern void chacha20_csprng_fill(u8int * buf, ulong len);
    chacha20_csprng_fill(buf, n);
    return n;
  }

  if (hwrandbuf != nil)
    (*hwrandbuf)(p, n);

  qlock(&rs->qlock);
  memmove(key, rs->key, sizeof(key));
  memmove(nonce, rs->nonce, sizeof(nonce));
  counter = rs->counter;
  randomrekey_locked(key, nonce, counter);
  qunlock(&rs->qlock);

  crypto_chacha20_ietf(buf, hwrandbuf != nil ? buf : nil, n, key, nonce, counter);
  memset(key, 0, sizeof(key));
  memset(nonce, 0, sizeof(nonce));

  return n;
}

/* used by fastrand() */
/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
void genrandom(uchar *p, int n) {
  /* Early boot fallback: use ChaCha20 CSPRNG if random subsystem not
   * initialized */
  if (rs == nil) {
    extern void chacha20_csprng_fill(u8int * buf, ulong len);
    chacha20_csprng_fill(p, n);
    return;
  }
  randomread(p, n);
}

/* used by rand(),nrand() */
/*@
  @ assigns \nothing;
  @*/
long lrand(void) {
  /* xoroshiro128+ algorithm */
  static int seeded = 0;
  static uvlong s[2];
  static BorrowLock lrand_lock = {
      .key = (uintptr)s,
      .dag_node = &lockdag_random_lrand,
  };
  ulong r;

  if (seeded == 0) {
    randomread(s, sizeof(s));
    seeded = (s[0] | s[1]) != 0;
  }

  borrow_lock(&lrand_lock);
  r = (s[0] + s[1]) >> 33;
  s[1] ^= s[0];
  s[0] = (s[0] << 55 | s[0] >> 9) ^ s[1] ^ (s[1] << 14);
  s[1] = (s[1] << 36 | s[1] >> 28);
  borrow_unlock(&lrand_lock);

  return r;
}
