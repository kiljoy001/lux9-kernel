#include "kunit.h"
#include "blind_ledger.h"
#include "pebble.h"

static void blind_ledger_contract_constants_match_design(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, BLIND_LEDGER_TOKEN_UNIT, 8);
  KUNIT_EXPECT_EQ(test, BLIND_LEDGER_CAP_SIZE, 32);
  KUNIT_EXPECT_EQ(test, BLIND_LEDGER_SECRET_SIZE, 32);
}

static void user_capability_layout_is_stable(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, (long long)sizeof(UserCapability), 64);
  KUNIT_EXPECT_EQ(test, (long long)sizeof(((UserCapability *)0)->uuid), 16);
  KUNIT_EXPECT_EQ(test, (long long)sizeof(((UserCapability *)0)->hash), 32);
}

static void capability_permission_bits_do_not_overlap(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, CAP_PERM_READ & CAP_PERM_WRITE, 0);
  KUNIT_EXPECT_EQ(test, CAP_PERM_READ & CAP_PERM_EXEC, 0);
  KUNIT_EXPECT_EQ(test, CAP_PERM_TRANSFER & CAP_PERM_GRANT, 0);
}

static void pebble_pointer_wave_projection_round_trips(struct kunit *test) {
  void *raw = (void *)0x1000ULL;
  void *w6 = PEBBLE_PROJECT(raw, PEBBLE_WAVE_6);
  void *w7 = PEBBLE_PROJECT(raw, PEBBLE_WAVE_7);

  KUNIT_EXPECT_TRUE(test, PEBBLE_PTR_ADDR(w6) == raw);
  KUNIT_EXPECT_TRUE(test, PEBBLE_PTR_ADDR(w7) == raw);
  KUNIT_EXPECT_EQ(test, PEBBLE_PTR_WAVE(w6), PEBBLE_WAVE_6);
  KUNIT_EXPECT_EQ(test, PEBBLE_PTR_WAVE(w7), PEBBLE_WAVE_7);
  KUNIT_EXPECT_TRUE(test, PEBBLE_TUNED(w6, PEBBLE_WAVE_6));
  KUNIT_EXPECT_TRUE(test, PEBBLE_TUNED(w7, PEBBLE_WAVE_7));
}

static void pebble_budget_costs_track_token_size(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, PEBBLE_BYTES_PER_TOKEN, 8);
  KUNIT_EXPECT_EQ(test, PEBBLE_FD_COST * PEBBLE_BYTES_PER_TOKEN, 1024);
  KUNIT_EXPECT_EQ(test, PEBBLE_MOUNT_COST * PEBBLE_BYTES_PER_TOKEN, 4096);
  KUNIT_EXPECT_EQ(test, PEBBLE_PIPE_COST * PEBBLE_BYTES_PER_TOKEN, 64 * 1024);
}

static const struct kunit_case critical_contracts_cases[] = {
    KUNIT_CASE(blind_ledger_contract_constants_match_design),
    KUNIT_CASE(user_capability_layout_is_stable),
    KUNIT_CASE(capability_permission_bits_do_not_overlap),
    KUNIT_CASE(pebble_pointer_wave_projection_round_trips),
    KUNIT_CASE(pebble_budget_costs_track_token_size),
    KUNIT_CASE_END,
};

const struct kunit_suite critical_contracts_suite = {
    .name = "critical_contracts",
    .cases = critical_contracts_cases,
};
