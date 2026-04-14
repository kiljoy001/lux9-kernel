#include "kunit.h"
#include "capability.h"
#include <string.h>

extern vlong kunit_mock_nsec_value;
extern u8int kunit_mock_random_seed;

extern void crypto_eddsa_key_pair(u8int secret_key[64], u8int public_key[32],
                                  u8int seed[32]);
extern void crypto_eddsa_sign(u8int signature[64], const u8int secret_key[64],
                              const u8int *message, usize message_size);

static vlong hour_to_nsec(u64int hour_epoch) {
  return (vlong)hour_epoch * 3600LL * 1000000000LL;
}

static void fill_client_id(u8int client_id[32], u8int base) {
  for (int i = 0; i < 32; i++) {
    client_id[i] = (u8int)(base + i);
  }
}

static void serialize_token_message(const CapToken *tok, u8int message[80]) {
  memcpy(message, tok->service_hash, 32);
  memcpy(message + 32, tok->client_commit, 32);
  memcpy(message + 64, &tok->epoch, 8);
  memcpy(message + 72, &tok->flags, 8);
}

static void sign_token_message(CapToken *tok, const u8int secret_key[64]) {
  u8int message[80];
  serialize_token_message(tok, message);
  crypto_eddsa_sign(tok->signature, secret_key, message, sizeof(message));
}

static int has_any_nonzero(const u8int *buf, usize len) {
  for (usize i = 0; i < len; i++) {
    if (buf[i] != 0) {
      return 1;
    }
  }
  return 0;
}

static void cap_current_epoch_converts_units(struct kunit *test) {
  kunit_mock_nsec_value = hour_to_nsec(26);

  KUNIT_EXPECT_EQ(test, (long long)cap_current_epoch(EPOCH_HOUR), 26);
  KUNIT_EXPECT_EQ(test, (long long)cap_current_epoch(EPOCH_DAY), 1);
  KUNIT_EXPECT_EQ(test, (long long)cap_current_epoch(EPOCH_SESSION), 0);
}

static void cap_epoch_valid_obeys_grace_window(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, cap_epoch_valid(10, 10, 1), 1);
  KUNIT_EXPECT_EQ(test, cap_epoch_valid(9, 10, 1), 1);
  KUNIT_EXPECT_EQ(test, cap_epoch_valid(8, 10, 1), 0);
  KUNIT_EXPECT_EQ(test, cap_epoch_valid(11, 10, 1), 0);
}

static void cap_hash_service_is_stable_and_distinct(struct kunit *test) {
  u8int hash_a[CAP_HASH_SIZE];
  u8int hash_b[CAP_HASH_SIZE];
  u8int hash_c[CAP_HASH_SIZE];

  cap_hash_service(hash_a, "ledger");
  cap_hash_service(hash_b, "ledger");
  cap_hash_service(hash_c, "pebble");

  KUNIT_EXPECT_EQ(test, memcmp(hash_a, hash_b, CAP_HASH_SIZE), 0);
  KUNIT_EXPECT_TRUE(test, memcmp(hash_a, hash_c, CAP_HASH_SIZE) != 0);
}

static void cap_derive_epoch_keys_is_deterministic_per_epoch(struct kunit *test) {
  CapMasterKey master = {0};
  CapEpochKeys keys_a = {0};
  CapEpochKeys keys_b = {0};
  CapEpochKeys keys_c = {0};

  for (int i = 0; i < 32; i++) {
    master.secret[i] = (u8int)(0x20 + i);
  }

  KUNIT_EXPECT_EQ(test, cap_derive_epoch_keys(&keys_a, &master, 7), 0);
  KUNIT_EXPECT_EQ(test, cap_derive_epoch_keys(&keys_b, &master, 7), 0);
  KUNIT_EXPECT_EQ(test, cap_derive_epoch_keys(&keys_c, &master, 8), 0);

  KUNIT_EXPECT_EQ(test, keys_a.epoch, 7);
  KUNIT_EXPECT_EQ(test, keys_b.epoch, 7);
  KUNIT_EXPECT_EQ(test, keys_c.epoch, 8);
  KUNIT_EXPECT_EQ(test, memcmp(keys_a.signing_seed, keys_b.signing_seed, 32), 0);
  KUNIT_EXPECT_EQ(test, memcmp(keys_a.hmac_key, keys_b.hmac_key, 32), 0);
}

static void cap_token_init_populates_epoch_hashes_and_flags(struct kunit *test) {
  CapToken tok = {0};
  u8int client_id[32];

  fill_client_id(client_id, 0x80);
  kunit_mock_nsec_value = hour_to_nsec(5);
  kunit_mock_random_seed = 0x3c;

  KUNIT_EXPECT_EQ(test,
                  cap_token_init(&tok, "srv/ledger", client_id,
                                 CAP_READ | CAP_WRITE),
                  0);
  KUNIT_EXPECT_EQ(test, tok.epoch, 5);
  KUNIT_EXPECT_EQ(test, (long long)tok.flags, CAP_READ | CAP_WRITE);
  KUNIT_EXPECT_TRUE(test, has_any_nonzero(tok.service_hash, CAP_HASH_SIZE));
  KUNIT_EXPECT_TRUE(test, has_any_nonzero(tok.client_commit, CAP_HASH_SIZE));
}

static void cap_blind_prepare_outputs_nonzero_blinded_payload(struct kunit *test) {
  CapToken tok = {0};
  CapBlindRequest req = {0};
  u8int client_id[32];

  fill_client_id(client_id, 0x30);
  kunit_mock_nsec_value = hour_to_nsec(9);
  kunit_mock_random_seed = 0x55;
  KUNIT_EXPECT_EQ(test, cap_token_init(&tok, "srv/pebble", client_id, CAP_READ),
                  0);

  KUNIT_EXPECT_EQ(test, cap_blind_prepare(&req, &tok), 0);
  KUNIT_EXPECT_TRUE(test,
                    has_any_nonzero(req.blinding_factor,
                                    sizeof(req.blinding_factor)));
  KUNIT_EXPECT_TRUE(test, has_any_nonzero(req.blinded_data,
                                          sizeof(req.blinded_data)));
}

static void cap_keys_valid_for_epoch_obeys_grace(struct kunit *test) {
  CapEpochKeys keys = {0};
  keys.epoch = 10;

  KUNIT_EXPECT_EQ(test, cap_keys_valid_for_epoch(&keys, 10), 1);
  KUNIT_EXPECT_EQ(test, cap_keys_valid_for_epoch(&keys, 11), 1);
  KUNIT_EXPECT_EQ(test, cap_keys_valid_for_epoch(&keys, 12), 0);
  KUNIT_EXPECT_EQ(test, cap_keys_valid_for_epoch(&keys, 9), 0);
}

static void cap_token_verify_accepts_valid_signature(struct kunit *test) {
  CapToken tok = {0};
  u8int client_id[32];
  u8int secret_key[64];
  u8int public_key[32];
  u8int seed[32];

  fill_client_id(client_id, 0x10);
  kunit_mock_nsec_value = hour_to_nsec(12);
  kunit_mock_random_seed = 0x7a;

  for (int i = 0; i < 32; i++) {
    seed[i] = (u8int)(0xa0 + i);
  }
  memset(secret_key, 0, sizeof(secret_key));
  memcpy(secret_key, seed, 32);
  crypto_eddsa_key_pair(secret_key, public_key, seed);

  KUNIT_EXPECT_EQ(test, cap_token_init(&tok, "srv/critical", client_id, CAP_READ),
                  0);
  sign_token_message(&tok, secret_key);

  KUNIT_EXPECT_EQ(test, cap_token_verify(&tok, tok.service_hash, public_key, 12),
                  0);
}

static void cap_token_verify_rejects_wrong_service_and_expiry(struct kunit *test) {
  CapToken tok = {0};
  u8int client_id[32];
  u8int secret_key[64];
  u8int public_key[32];
  u8int wrong_service_hash[CAP_HASH_SIZE];
  u8int seed[32];

  fill_client_id(client_id, 0x44);
  kunit_mock_nsec_value = hour_to_nsec(4);
  kunit_mock_random_seed = 0x22;

  for (int i = 0; i < 32; i++) {
    seed[i] = (u8int)(0xc0 + i);
  }
  memset(secret_key, 0, sizeof(secret_key));
  memcpy(secret_key, seed, 32);
  crypto_eddsa_key_pair(secret_key, public_key, seed);

  KUNIT_EXPECT_EQ(test, cap_token_init(&tok, "srv/ledger", client_id, CAP_READ),
                  0);
  sign_token_message(&tok, secret_key);
  cap_hash_service(wrong_service_hash, "srv/other");

  KUNIT_EXPECT_EQ(
      test, cap_token_verify(&tok, wrong_service_hash, public_key, tok.epoch), -2);
  KUNIT_EXPECT_EQ(test,
                  cap_token_verify(&tok, tok.service_hash, public_key,
                                   tok.epoch + EPOCH_GRACE + 1),
                  -1);
}

static const struct kunit_case blind_cap_cases[] = {
    KUNIT_CASE(cap_current_epoch_converts_units),
    KUNIT_CASE(cap_epoch_valid_obeys_grace_window),
    KUNIT_CASE(cap_hash_service_is_stable_and_distinct),
    KUNIT_CASE(cap_derive_epoch_keys_is_deterministic_per_epoch),
    KUNIT_CASE(cap_token_init_populates_epoch_hashes_and_flags),
    KUNIT_CASE(cap_blind_prepare_outputs_nonzero_blinded_payload),
    KUNIT_CASE(cap_keys_valid_for_epoch_obeys_grace),
    KUNIT_CASE(cap_token_verify_accepts_valid_signature),
    KUNIT_CASE(cap_token_verify_rejects_wrong_service_and_expiry),
    KUNIT_CASE_END,
};

const struct kunit_suite blind_cap_suite = {
    .name = "blind_cap",
    .cases = blind_cap_cases,
};
