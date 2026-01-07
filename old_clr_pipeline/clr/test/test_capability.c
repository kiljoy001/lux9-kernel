/* test_capability.c - Unit tests for CLR Capability System
 *
 * Tests validate properties proven in Coq:
 *   - PermsBitmask.v: perms_subset_refl, perms_subset_trans
 *   - CapabilityModel.v: derived_perm_monotonic
 *   - DerivationChain.v: derived_chain_perms_monotonic
 *   - LedgerInvariants.v: find_cap_unique
 *
 * Compile with:
 *   gcc -DUSERSPACE_TEST -I.. -I../../include test_capability.c \
 *       ../clr_capability.c ../../libc/uuid.c -o test_capability &&
 * ./test_capability
 */

#define USERSPACE_TEST

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stub for uuid generation in userspace */
#include "../../include/uuid.h"

/* UUID stubs for testing */
static unsigned int uuid_counter = 1;

void uuid_new_v8(uuid_t *u) {
  memset(u->data, 0, 16);
  u->data[0] = (uuid_counter >> 24) & 0xFF;
  u->data[1] = (uuid_counter >> 16) & 0xFF;
  u->data[2] = (uuid_counter >> 8) & 0xFF;
  u->data[3] = uuid_counter & 0xFF;
  u->data[6] = 0x80; /* Version 8 */
  u->data[8] = 0x80; /* Variant 1 */
  uuid_counter++;
}

void uuid_clear(uuid_t *u) { memset(u->data, 0, 16); }

int uuid_compare(const uuid_t *a, const uuid_t *b) {
  return memcmp(a->data, b->data, 16);
}

void uuid_copy(uuid_t *dst, const uuid_t *src) {
  memcpy(dst->data, src->data, 16);
}

int uuid_is_null(const uuid_t *u) {
  for (int i = 0; i < 16; i++) {
    if (u->data[i] != 0)
      return 0;
  }
  return 1;
}

void uuid_unparse(const uuid_t *uu, char *out) {
  sprintf(
      out,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uu->data[0], uu->data[1], uu->data[2], uu->data[3], uu->data[4],
      uu->data[5], uu->data[6], uu->data[7], uu->data[8], uu->data[9],
      uu->data[10], uu->data[11], uu->data[12], uu->data[13], uu->data[14],
      uu->data[15]);
}

#include "../clr_capability.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                             \
  do {                                                                         \
    tests_run++;                                                               \
    printf("  TEST: %s... ", #name);                                           \
    if (test_##name()) {                                                       \
      tests_passed++;                                                          \
      printf("PASS\n");                                                        \
    } else {                                                                   \
      printf("FAIL\n");                                                        \
    }                                                                          \
  } while (0)

/* ========== Permission Tests (PermsBitmask.v) ========== */

/*
 * @coq_theorem: perms_subset_refl
 *   forall m, perms_subset m m
 */
int test_perms_subset_reflexive(void) {
  return cap_perms_subset(CAP_PERM_ALL, CAP_PERM_ALL) &&
         cap_perms_subset(CAP_PERM_READ, CAP_PERM_READ) &&
         cap_perms_subset(CAP_PERM_NONE, CAP_PERM_NONE);
}

/*
 * @coq_theorem: perms_subset_trans
 *   perms_subset a b -> perms_subset b c -> perms_subset a c
 */
int test_perms_subset_transitive(void) {
  /* a=READ_WRITE, b=READ_WRITE_EXEC, c=ALL */
  unsigned int a = CAP_PERM_READ | CAP_PERM_WRITE;
  unsigned int b = CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_EXEC;
  unsigned int c = CAP_PERM_ALL;

  int ab = cap_perms_subset(a, b);
  int bc = cap_perms_subset(b, c);
  int ac = cap_perms_subset(a, c);

  return ab && bc && ac;
}

int test_perms_not_subset(void) {
  /* WRITE is not a subset of READ */
  return !cap_perms_subset(CAP_PERM_WRITE, CAP_PERM_READ);
}

/* ========== Module Capability Tests (CapabilityModel.v) ========== */

int test_create_module_cap(void) {
  capability_manager_t *mgr = cap_manager_create();
  if (!mgr)
    return 0;

  clr_monotonic_capability_t *cap = cap_create_module(mgr, "TestAssembly");
  if (!cap) {
    cap_manager_destroy(mgr);
    return 0;
  }

  int result = cap->scope == CAP_SCOPE_MODULE && cap->parent_id == 0 &&
               cap->permissions == CAP_PERM_ALL && cap->derivation_depth == 0 &&
               cap->is_validated == 1 && !uuid_is_null(&cap->uuid);

  cap_manager_destroy(mgr);
  return result;
}

int test_module_cap_has_all_perms(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *cap = cap_create_module(mgr, "Test");

  int result = cap_check_permission(cap, CAP_PERM_READ) &&
               cap_check_permission(cap, CAP_PERM_WRITE) &&
               cap_check_permission(cap, CAP_PERM_EXEC) &&
               cap_check_permission(cap, CAP_PERM_TRANSFER) &&
               cap_check_permission(cap, CAP_PERM_GRANT);

  cap_manager_destroy(mgr);
  return result;
}

/* ========== Derivation Tests (DerivationChain.v) ========== */

/*
 * @coq_theorem: derived_perm_monotonic
 *   derived_from ct c p -> perms_subset (cap_perms c) (cap_perms p)
 */
int test_derive_reduces_perms(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *module = cap_create_module(mgr, "Test");

  /* Derive with subset of permissions */
  unsigned int reduced = CAP_PERM_READ | CAP_PERM_EXEC | CAP_PERM_GRANT;
  clr_monotonic_capability_t *class_cap =
      cap_derive_class(mgr, module, "MyClass", reduced);

  int result = class_cap != NULL && class_cap->permissions == reduced &&
               cap_perms_subset(class_cap->permissions, module->permissions);

  cap_manager_destroy(mgr);
  return result;
}

/*
 * This test validates that attempting to derive with more permissions
 * than the parent FAILS (key security property).
 */
int test_derive_rejects_excess_perms(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *module = cap_create_module(mgr, "Test");

  /* First derive with reduced perms */
  unsigned int reduced = CAP_PERM_READ | CAP_PERM_GRANT;
  clr_monotonic_capability_t *class_cap =
      cap_derive_class(mgr, module, "A", reduced);

  /* Try to derive from class_cap with MORE perms (should fail) */
  unsigned int excess = CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_GRANT;
  clr_monotonic_capability_t *bad_cap =
      cap_derive_class(mgr, class_cap, "B", excess);

  int result = bad_cap == NULL && mgr->rejections > 0;

  cap_manager_destroy(mgr);
  return result;
}

int test_derivation_depth_increases(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *module = cap_create_module(mgr, "Test");

  unsigned int perms = CAP_PERM_READ | CAP_PERM_GRANT;
  clr_monotonic_capability_t *c1 = cap_derive_class(mgr, module, "C1", perms);
  clr_monotonic_capability_t *c2 = cap_derive_class(mgr, c1, "C2", perms);
  clr_monotonic_capability_t *c3 = cap_derive_class(mgr, c2, "C3", perms);

  int result = module->derivation_depth == 0 && c1->derivation_depth == 1 &&
               c2->derivation_depth == 2 && c3->derivation_depth == 3;

  cap_manager_destroy(mgr);
  return result;
}

/* ========== Chain Validation Tests (ChainDecidability.v) ========== */

/*
 * @coq_theorem: derived_chain_table_perms_monotonic
 */
int test_chain_validation_succeeds(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *module = cap_create_module(mgr, "Test");

  unsigned int p1 = CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_GRANT;
  unsigned int p2 = CAP_PERM_READ | CAP_PERM_GRANT;
  unsigned int p3 = CAP_PERM_READ;

  clr_monotonic_capability_t *c1 = cap_derive_class(mgr, module, "C1", p1);
  clr_monotonic_capability_t *c2 = cap_derive_class(mgr, c1, "C2", p2);
  /* c3 won't have GRANT, so can't derive further */

  int result = cap_validate_chain(mgr, c1) && cap_validate_chain(mgr, c2);

  cap_manager_destroy(mgr);
  return result;
}

/* ========== Lookup Tests (LedgerInvariants.v) ========== */

/*
 * @coq_theorem: find_cap_unique
 */
int test_uuid_unique(void) {
  capability_manager_t *mgr = cap_manager_create();

  clr_monotonic_capability_t *c1 = cap_create_module(mgr, "A");
  clr_monotonic_capability_t *c2 = cap_create_module(mgr, "B");

  int result = uuid_compare(&c1->uuid, &c2->uuid) != 0;

  cap_manager_destroy(mgr);
  return result;
}

int test_find_by_uuid(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *c1 = cap_create_module(mgr, "A");
  clr_monotonic_capability_t *c2 = cap_create_module(mgr, "B");

  clr_monotonic_capability_t *found = cap_find_by_uuid(mgr, &c2->uuid);

  int result = found == c2 && found != c1;

  cap_manager_destroy(mgr);
  return result;
}

int test_find_by_id(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *c1 = cap_create_module(mgr, "A");
  clr_monotonic_capability_t *c2 = cap_create_module(mgr, "B");

  clr_monotonic_capability_t *found = cap_find_by_id(mgr, c1->cap_id);

  int result = found == c1 && found != c2;

  cap_manager_destroy(mgr);
  return result;
}

/* ========== Grant Permission Tests ========== */

int test_grant_required_for_derive(void) {
  capability_manager_t *mgr = cap_manager_create();
  clr_monotonic_capability_t *module = cap_create_module(mgr, "Test");

  /* Derive without GRANT permission */
  unsigned int no_grant = CAP_PERM_READ | CAP_PERM_WRITE;
  clr_monotonic_capability_t *c1 =
      cap_derive_class(mgr, module, "C1", no_grant);

  /* Try to derive from c1 (should fail - no GRANT) */
  clr_monotonic_capability_t *c2 =
      cap_derive_class(mgr, c1, "C2", CAP_PERM_READ);

  int result = c1 != NULL && c2 == NULL && mgr->rejections > 0;

  cap_manager_destroy(mgr);
  return result;
}

/* ========== Main ========== */

int main(void) {
  printf("=== CLR Capability System Tests ===\n");
  printf("Testing properties proven in proofs/capability/*.v\n\n");

  printf("Permission Tests (PermsBitmask.v):\n");
  TEST(perms_subset_reflexive);
  TEST(perms_subset_transitive);
  TEST(perms_not_subset);

  printf("\nModule Capability Tests (CapabilityModel.v):\n");
  TEST(create_module_cap);
  TEST(module_cap_has_all_perms);

  printf("\nDerivation Tests (DerivationChain.v):\n");
  TEST(derive_reduces_perms);
  TEST(derive_rejects_excess_perms);
  TEST(derivation_depth_increases);

  printf("\nChain Validation Tests (ChainDecidability.v):\n");
  TEST(chain_validation_succeeds);

  printf("\nLookup Tests (LedgerInvariants.v):\n");
  TEST(uuid_unique);
  TEST(find_by_uuid);
  TEST(find_by_id);

  printf("\nGrant Permission Tests:\n");
  TEST(grant_required_for_derive);

  printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);

  return tests_passed == tests_run ? 0 : 1;
}
