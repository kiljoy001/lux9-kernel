#include "9p_router.h"
#include "dat.h"
#include "exchange.h"
#include "exchange_pool.h"
#include "fns.h"
#include "mem.h"
#include "u.h"
#include "uuid.h"

/*@
  @ assigns \nothing;
  @*/
void test_hybrid_batching() {
  ExchangeRequest req;
  ExchangeHandle cap1, cap2, cap3;
  PoolError err;
  Proc *p = up;

  print("Testing hybrid batching...\n");

  /* 1. Request small message to /srv/dns */
  req.size = 128;
  req.type = EXCHANGE_TYPE_AUTO;
  strncpy(req.path, "/srv/dns", KNAMELEN - 1);
  req.path[KNAMELEN - 1] = '\0';
  err = pool_prepare_hybrid(p, &req, &cap1);
  if (err != POOL_OK) {
    print("FAIL: first small request failed (err=%d)\n", err);
    return;
  }

  /* 2. Request second small message to SAME destination */
  err = pool_prepare_hybrid(p, &req, &cap2);
  if (err != POOL_OK) {
    print("FAIL: second small request failed (err=%d)\n", err);
    return;
  }

  if (uuid_compare(&cap1.uuid, &cap2.uuid) != 0) {
    print("FAIL: small messages to same destination got different pages\n");
  } else {
    print("PASS: same destination batched onto same page\n");
  }

  /* 3. Request large message */
  req.size = 2048; /* Larger than RING_DATA_SIZE (248) */
  req.type = EXCHANGE_TYPE_AUTO;
  err = pool_prepare_hybrid(p, &req, &cap3);
  if (err != POOL_OK) {
    print("FAIL: large request failed (err=%d)\n", err);
    return;
  }

  if (uuid_compare(&cap3.uuid, &cap1.uuid) == 0) {
    print("FAIL: large message shared page with small messages\n");
  } else {
    print("PASS: large message got dedicated page\n");
  }

  /* 4. Request small message to DIFFERENT destination */
  ExchangeHandle cap4;
  req.size = 64;
  strncpy(req.path, "/srv/fs", KNAMELEN - 1);
  req.path[KNAMELEN - 1] = '\0';
  err = pool_prepare_hybrid(p, &req, &cap4);
  if (err != POOL_OK) {
    print("FAIL: different destination request failed (err=%d)\n", err);
    return;
  }

  if (uuid_compare(&cap4.uuid, &cap1.uuid) == 0) {
    print("FAIL: different destinations shared same page\n");
  } else {
    print("PASS: different destinations got different pages\n");
  }

  print("Hybrid IPC tests complete.\n");
}
