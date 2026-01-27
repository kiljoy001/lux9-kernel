/*
 * test_cap_boot.c - Boot Test for Capability-Protected /srv
 *
 * This test runs during system boot to verify:
 * 1. Resurrection server initializes keys
 * 2. Services can register with /srv
 * 3. Token-based access control works
 *
 * Note: This requires the full system to be running.
 * It's invoked as part of the boot sequence.
 */

#include "../include/capability.h"
#include "../include/libc.h"
#include "../include/u.h"

/* For boot tests, we use the real system */
extern vlong nsec(void);
extern void randombytes(u8int *, usize);

#define BOOT_TEST_PASS(name)                                                   \
  do {                                                                         \
    print("[BOOT] PASS: ");                                                    \
    print(name);                                                               \
    print("\n");                                                               \
  } while (0)

#define BOOT_TEST_FAIL(name)                                                   \
  do {                                                                         \
    print("[BOOT] FAIL: ");                                                    \
    print(name);                                                               \
    print("\n");                                                               \
  } while (0)

/*
 * Test 1: Epoch system is working
 */
static int boot_test_epoch(void) {
  u64int epoch1 = cap_current_epoch(EPOCH_HOUR);

  /* Should get a non-zero epoch (unless system just started) */
  /* We just verify it doesn't crash and returns something */

  u64int epoch2 = cap_current_epoch(EPOCH_HOUR);

  /* Second call should return same or later epoch */
  if (epoch2 < epoch1) {
    BOOT_TEST_FAIL("epoch_stable");
    return -1;
  }

  BOOT_TEST_PASS("epoch_stable");
  return 0;
}

/*
 * Test 2: Key derivation works with real randomness
 */
static int boot_test_key_derivation(void) {
  CapMasterKey master;
  CapEpochKeys keys;

  /* Generate random master key */
  randombytes(master.secret, 32);
  master.creation_epoch = cap_current_epoch(EPOCH_HOUR);

  /* Derive keys */
  int ret = cap_derive_epoch_keys(&keys, &master, master.creation_epoch);
  if (ret != 0) {
    BOOT_TEST_FAIL("key_derivation");
    return -1;
  }

  /* Verify keys are non-zero */
  int nonzero = 0;
  for (int i = 0; i < 32; i++) {
    if (keys.signing_seed[i] != 0)
      nonzero = 1;
  }
  if (!nonzero) {
    BOOT_TEST_FAIL("key_nonzero");
    return -1;
  }

  BOOT_TEST_PASS("key_derivation");
  return 0;
}

/*
 * Test 3: Token creation works
 */
static int boot_test_token_creation(void) {
  CapToken tok;
  u8int client_id[32];

  /* Generate random client ID */
  randombytes(client_id, 32);

  int ret = cap_token_init(&tok, "boot_test_service", client_id,
                           CAP_READ | CAP_WRITE);
  if (ret != 0) {
    BOOT_TEST_FAIL("token_creation");
    return -1;
  }

  /* Verify token has correct epoch */
  u64int current = cap_current_epoch(EPOCH_HOUR);
  if (tok.epoch != current && tok.epoch != current - 1) {
    BOOT_TEST_FAIL("token_epoch");
    return -1;
  }

  /* Verify flags */
  if (tok.flags != (CAP_READ | CAP_WRITE)) {
    BOOT_TEST_FAIL("token_flags");
    return -1;
  }

  BOOT_TEST_PASS("token_creation");
  return 0;
}

/*
 * Test 4: Service hash determinism
 */
static int boot_test_service_hash(void) {
  u8int hash1[32], hash2[32];

  cap_hash_service(hash1, "resurrection");
  cap_hash_service(hash2, "resurrection");

  if (memcmp(hash1, hash2, 32) != 0) {
    BOOT_TEST_FAIL("service_hash_determinism");
    return -1;
  }

  BOOT_TEST_PASS("service_hash_determinism");
  return 0;
}

/*
 * Boot Test Runner
 *
 * Called during system initialization to verify capability system.
 * Returns 0 if all tests pass, non-zero on failure.
 */
int test_cap_boot_main(void) {
  int failures = 0;

  print("\n=== Capability System Boot Tests ===\n\n");

  if (boot_test_epoch() != 0)
    failures++;
  if (boot_test_key_derivation() != 0)
    failures++;
  if (boot_test_token_creation() != 0)
    failures++;
  if (boot_test_service_hash() != 0)
    failures++;

  print("\n");
  if (failures == 0) {
    print("=== Boot tests passed - capability system OK ===\n");
  } else {
    print("=== CRITICAL: Boot tests FAILED ===\n");
  }

  return failures;
}
