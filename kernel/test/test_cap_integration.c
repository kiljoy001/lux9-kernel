/*
 * test_cap_integration.c - Integration Tests for Capability Token Flow
 *
 * Tests the complete flow:
 * 1. Service registration with resurrection
 * 2. Client token request (blind signing)
 * 3. Token presentation and verification
 */

#include "../include/capability.h"
#include "../include/libc.h"
#include "../include/u.h"

/* Test framework macros */
#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      print("FAIL: ");                                                         \
      print(msg);                                                              \
      print("\n");                                                             \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define TEST_PASS(name)                                                        \
  do {                                                                         \
    print("PASS: ");                                                           \
    print(name);                                                               \
    print("\n");                                                               \
  } while (0)

/* Mock implementations for standalone testing */
#ifndef TEST_STANDALONE
static vlong mock_nsec = 0;
vlong nsec(void) { return mock_nsec; }

static u8int mock_rand_seed = 0x55;
void randombytes(u8int *buf, usize len) {
  for (usize i = 0; i < len; i++)
    buf[i] = mock_rand_seed ^ (u8int)i;
}
#else
extern vlong mock_nsec;
extern u8int mock_rand_seed;
extern vlong nsec(void);
extern void randombytes(u8int *, usize);
#endif

/* Simulated resurrection signing key */
static CapMasterKey resurrection_master;
static CapEpochKeys resurrection_keys;
static int resurrection_initialized = 0;

static void init_resurrection(void) {
  if (resurrection_initialized)
    return;

  /* Initialize master key */
  for (int i = 0; i < 32; i++) {
    resurrection_master.secret[i] = (u8int)(0xDE + i);
  }
  resurrection_master.creation_epoch = 0;

  /* Derive current epoch keys */
  u64int epoch = cap_current_epoch(EPOCH_HOUR);
  cap_derive_epoch_keys(&resurrection_keys, &resurrection_master, epoch);

  resurrection_initialized = 1;
}

/*
 * Simulate resurrection signing a blinded request
 */
static int resurrection_blind_sign(CapBlindResponse *resp,
                                   const CapBlindRequest *req) {
  init_resurrection();

  /* Use current epoch's signing key */
  u8int full_key[64];
  memcpy(full_key, resurrection_keys.signing_seed, 32);
  memset(full_key + 32, 0, 32);

  return cap_blind_sign(resp, req, full_key);
}

/*
 * Test 1: Full Token Flow
 */
static int test_full_token_flow(void) {
  CapToken tok;
  CapBlindRequest req;
  CapBlindResponse resp;
  u8int client_id[32];
  u8int service_hash[32];

  /* Set up test time */
  mock_nsec = 10LL * 3600LL * 1000000000LL; /* Epoch 10 */
  resurrection_initialized = 0;             /* Force reinit */

  /* Set up client identity */
  for (int i = 0; i < 32; i++) {
    client_id[i] = (u8int)(0xC0 + i);
  }

  /* Step 1: Client initializes token */
  int ret = cap_token_init(&tok, "test_service", client_id, CAP_READ);
  TEST_ASSERT(ret == 0, "Token init should succeed");

  /* Step 2: Client prepares blind request */
  ret = cap_blind_prepare(&req, &tok);
  TEST_ASSERT(ret == 0, "Blind prepare should succeed");

  /* Step 3: Resurrection signs blindly */
  ret = resurrection_blind_sign(&resp, &req);
  TEST_ASSERT(ret == 0, "Blind sign should succeed");

  /* Step 4: Client unblinds to get final token */
  ret = cap_unblind(&tok, &resp, &req);
  TEST_ASSERT(ret == 0, "Unblind should succeed");

  /* Step 5: Service verifies token */
  cap_hash_service(service_hash, "test_service");
  ret = cap_token_verify(&tok, service_hash, resurrection_keys.signing_pubkey,
                         cap_current_epoch(EPOCH_HOUR));
  TEST_ASSERT(ret == 0, "Token verification should succeed");

  TEST_PASS("test_full_token_flow");
  return 0;
}

/*
 * Test 2: Token Rejected for Wrong Service
 */
static int test_wrong_service_rejected(void) {
  CapToken tok;
  CapBlindRequest req;
  CapBlindResponse resp;
  u8int client_id[32];
  u8int wrong_service_hash[32];

  mock_nsec = 10LL * 3600LL * 1000000000LL;
  resurrection_initialized = 0;

  for (int i = 0; i < 32; i++)
    client_id[i] = (u8int)i;

  /* Create token for "service_a" */
  cap_token_init(&tok, "service_a", client_id, CAP_READ);
  cap_blind_prepare(&req, &tok);
  resurrection_blind_sign(&resp, &req);
  cap_unblind(&tok, &resp, &req);

  /* Try to use it for "service_b" */
  cap_hash_service(wrong_service_hash, "service_b");
  int ret = cap_token_verify(&tok, wrong_service_hash,
                             resurrection_keys.signing_pubkey,
                             cap_current_epoch(EPOCH_HOUR));
  TEST_ASSERT(ret != 0, "Token should be rejected for wrong service");

  TEST_PASS("test_wrong_service_rejected");
  return 0;
}

/*
 * Test 3: Expired Token Rejected
 */
static int test_expired_token_rejected(void) {
  CapToken tok;
  CapBlindRequest req;
  CapBlindResponse resp;
  u8int client_id[32];
  u8int service_hash[32];

  /* Create token at epoch 10 */
  mock_nsec = 10LL * 3600LL * 1000000000LL;
  resurrection_initialized = 0;

  for (int i = 0; i < 32; i++)
    client_id[i] = (u8int)i;

  cap_token_init(&tok, "service", client_id, CAP_READ);
  cap_blind_prepare(&req, &tok);
  resurrection_blind_sign(&resp, &req);
  cap_unblind(&tok, &resp, &req);

  /* Move time forward to epoch 15 (5 hours later) */
  mock_nsec = 15LL * 3600LL * 1000000000LL;

  cap_hash_service(service_hash, "service");
  int ret =
      cap_token_verify(&tok, service_hash, resurrection_keys.signing_pubkey,
                       cap_current_epoch(EPOCH_HOUR));
  TEST_ASSERT(ret != 0, "Expired token should be rejected");

  TEST_PASS("test_expired_token_rejected");
  return 0;
}

/*
 * Main test runner
 */
int test_cap_integration_main(void) {
  int failures = 0;

  print("\n=== Capability Integration Tests ===\n\n");

  if (test_full_token_flow() != 0)
    failures++;
  if (test_wrong_service_rejected() != 0)
    failures++;
  if (test_expired_token_rejected() != 0)
    failures++;

  print("\n");
  if (failures == 0) {
    print("=== All integration tests passed! ===\n");
  } else {
    print("=== FAILURES in integration tests ===\n");
  }

  return failures;
}
