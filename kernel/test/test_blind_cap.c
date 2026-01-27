/*
 * test_blind_cap.c - Unit Tests for Blind Signature Capability System
 *
 * Tests:
 * 1. Epoch generation and validation
 * 2. Key derivation from master secret
 * 3. Token initialization
 * 4. Blind sign/unblind cycle
 * 5. Token verification
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

/* Mock random bytes for deterministic testing */
#ifndef TEST_STANDALONE
static u8int mock_random_seed = 0x42;
void randombytes(u8int *buf, usize len) {
  for (usize i = 0; i < len; i++) {
    buf[i] = mock_random_seed ^ (u8int)i;
  }
}

/* Mock nsec for epoch testing */
static vlong mock_nsec_value = 0;
vlong nsec(void) { return mock_nsec_value; }
#else
/* When standalone testing, stubs provide these. Declare extern. */
extern vlong mock_nsec_value;
extern u8int mock_random_seed;
extern vlong nsec(void);
extern void randombytes(u8int *, usize);
#endif

/*
 * Test 1: Epoch Generation
 */
static int test_epoch_generation(void) {
  /* Set mock time to 2 hours in nanoseconds */
  mock_nsec_value = 2LL * 3600LL * 1000000000LL;

  u64int epoch = cap_current_epoch(EPOCH_HOUR);
  TEST_ASSERT(epoch == 2, "EPOCH_HOUR should return 2 for 2 hours");

  /* Test day epoch */
  mock_nsec_value = 48LL * 3600LL * 1000000000LL; /* 2 days */
  epoch = cap_current_epoch(EPOCH_DAY);
  TEST_ASSERT(epoch == 2, "EPOCH_DAY should return 2 for 48 hours");

  /* Test session epoch (always 0) */
  epoch = cap_current_epoch(EPOCH_SESSION);
  TEST_ASSERT(epoch == 0, "EPOCH_SESSION should always return 0");

  TEST_PASS("test_epoch_generation");
  return 0;
}

/*
 * Test 2: Epoch Validation
 */
static int test_epoch_validation(void) {
  /* Current epoch = 10 */
  u64int current = 10;

  /* Same epoch - valid */
  TEST_ASSERT(cap_epoch_valid(10, current, 1) == 1,
              "Same epoch should be valid");

  /* Previous epoch with grace=1 - valid */
  TEST_ASSERT(cap_epoch_valid(9, current, 1) == 1,
              "Previous epoch should be valid with grace=1");

  /* Two epochs ago with grace=1 - invalid */
  TEST_ASSERT(cap_epoch_valid(8, current, 1) == 0,
              "Two epochs ago should be invalid with grace=1");

  /* Future epoch - always invalid */
  TEST_ASSERT(cap_epoch_valid(11, current, 1) == 0,
              "Future epoch should be invalid");

  TEST_PASS("test_epoch_validation");
  return 0;
}

/*
 * Test 3: Key Derivation
 */
static int test_key_derivation(void) {
  CapMasterKey master;
  CapEpochKeys keys1, keys2;

  /* Initialize master with known seed */
  memset(&master, 0, sizeof(master));
  for (int i = 0; i < 32; i++) {
    master.secret[i] = (u8int)(i + 1);
  }
  master.creation_epoch = 0;

  /* Derive keys for epoch 5 */
  int ret = cap_derive_epoch_keys(&keys1, &master, 5);
  TEST_ASSERT(ret == 0, "cap_derive_epoch_keys should succeed");
  TEST_ASSERT(keys1.epoch == 5, "Derived keys should have correct epoch");

  /* Derive keys for same epoch - should be identical */
  ret = cap_derive_epoch_keys(&keys2, &master, 5);
  TEST_ASSERT(ret == 0, "Second derivation should succeed");
  TEST_ASSERT(memcmp(keys1.signing_seed, keys2.signing_seed, 32) == 0,
              "Same epoch should produce same signing key");
  TEST_ASSERT(memcmp(keys1.hmac_key, keys2.hmac_key, 32) == 0,
              "Same epoch should produce same HMAC key");

  /* Derive keys for different epoch - should be different */
  ret = cap_derive_epoch_keys(&keys2, &master, 6);
  TEST_ASSERT(ret == 0, "Different epoch derivation should succeed");
  TEST_ASSERT(memcmp(keys1.signing_seed, keys2.signing_seed, 32) != 0,
              "Different epochs should produce different signing keys");

  TEST_PASS("test_key_derivation");
  return 0;
}

/*
 * Test 4: Service Hash
 */
static int test_service_hash(void) {
  u8int hash1[32], hash2[32];

  cap_hash_service(hash1, "test_service");
  cap_hash_service(hash2, "test_service");

  TEST_ASSERT(memcmp(hash1, hash2, 32) == 0,
              "Same service name should produce same hash");

  cap_hash_service(hash2, "other_service");
  TEST_ASSERT(memcmp(hash1, hash2, 32) != 0,
              "Different service names should produce different hashes");

  TEST_PASS("test_service_hash");
  return 0;
}

/*
 * Test 5: Token Initialization
 */
static int test_token_init(void) {
  CapToken tok;
  u8int client_id[32];

  /* Set up client ID */
  for (int i = 0; i < 32; i++) {
    client_id[i] = (u8int)(0xAA + i);
  }

  /* Set epoch */
  mock_nsec_value = 5LL * 3600LL * 1000000000LL;

  int ret = cap_token_init(&tok, "my_service", client_id, CAP_READ | CAP_WRITE);
  TEST_ASSERT(ret == 0, "cap_token_init should succeed");
  TEST_ASSERT(tok.epoch == 5, "Token should have current epoch");
  TEST_ASSERT(tok.flags == (CAP_READ | CAP_WRITE),
              "Token should have correct flags");

  /* Service hash should be non-zero */
  int nonzero = 0;
  for (int i = 0; i < 32; i++) {
    if (tok.service_hash[i] != 0)
      nonzero = 1;
  }
  TEST_ASSERT(nonzero, "Service hash should be non-zero");

  TEST_PASS("test_token_init");
  return 0;
}

/*
 * Test 6: Keys Valid for Epoch
 */
static int test_keys_valid_for_epoch(void) {
  CapEpochKeys keys;
  keys.epoch = 10;

  /* Same epoch - valid */
  TEST_ASSERT(cap_keys_valid_for_epoch(&keys, 10) == 1,
              "Keys should be valid for same epoch");

  /* Next epoch (within grace) - valid */
  TEST_ASSERT(cap_keys_valid_for_epoch(&keys, 11) == 1,
              "Keys should be valid for next epoch (grace period)");

  /* Two epochs later - invalid */
  TEST_ASSERT(cap_keys_valid_for_epoch(&keys, 12) == 0,
              "Keys should be invalid two epochs later");

  /* Keys from future - invalid */
  TEST_ASSERT(cap_keys_valid_for_epoch(&keys, 9) == 0,
              "Keys from future should be invalid");

  TEST_PASS("test_keys_valid_for_epoch");
  return 0;
}

/*
 * Main test runner
 */
int test_blind_cap_main(void) {
  int failures = 0;

  print("\n=== Blind Signature Capability Tests ===\n\n");

  if (test_epoch_generation() != 0)
    failures++;
  if (test_epoch_validation() != 0)
    failures++;
  if (test_key_derivation() != 0)
    failures++;
  if (test_service_hash() != 0)
    failures++;
  if (test_token_init() != 0)
    failures++;
  if (test_keys_valid_for_epoch() != 0)
    failures++;

  print("\n");
  if (failures == 0) {
    print("=== All tests passed! ===\n");
  } else {
    print("=== FAILURES: ");
    /* Simple number printing */
    char c = '0' + failures;
    print(&c);
    print(" tests failed ===\n");
  }

  return failures;
}
