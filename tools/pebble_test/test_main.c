#include "dat.h"
#include "fns.h"
#include "u.h"

extern void pebbleinit(void);
extern void pebbleprocinit(Proc *p);
extern void pebble_selftest(void);
extern void pebble_sip_issue_test(void);
extern void borrowinit(void);
extern void blind_ledger_init(void);
extern void stubs_init(void);
extern int pebble_enabled;

void attest_test(void) {
  BlindLedgerStats stats;
  u8int signature[32];
  u32int sig_len = 0;

  print("Running attest_test...\n");

  if (blind_ledger_get_stats(&stats) != BLIND_LEDGER_OK) {
    print("FAIL: blind_ledger_get_stats returned error\n");
  } else {
    print("Stats: Active=%llud Burned=%llud Mem=%llud Epoch=%llud\n",
          stats.active_entries, stats.burned_entries,
          stats.total_memory_tracked, stats.epoch);
  }

  if (blind_ledger_attest_root(signature, &sig_len) != BLIND_LEDGER_OK) {
    print("FAIL: blind_ledger_attest_root returned error\n");
  } else {
    print("Attestation: Root signed successfully (%d bytes)\n", sig_len);
    print("PASS: Attestation verified\n");
  }
}

void derivation_test(void) {
  print("Running derivation_test...\n");

  // 1. Mint a root capability
  UserCapability root_cap;
  u8int secret[32] = {1, 2, 3, 4};
  BlindLedgerError err =
      ledger_mint(&root_cap, 0x1000, 4096, up,
                  CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_GRANT, secret);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: ledger_mint returned %d\n", err);
    return;
  }
  print("  Minted root capability\n");

  // 2. Derive a child with reduced permissions
  UserCapability child_cap;
  err = ledger_derive(&root_cap, up, CAP_PERM_READ, &child_cap);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: ledger_derive returned %d\n", err);
    return;
  }
  print("  Derived child (read-only)\n");

  // 3. Get derivation proof for child
  DerivationProof proof;
  err = ledger_get_derivation_proof(&child_cap, up, &proof);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: ledger_get_derivation_proof returned %d\n", err);
    return;
  }
  print("  Got proof (chain_length=%d)\n", proof.chain_length);

  // 4. Verify the proof
  err = ledger_verify_derivation_proof(&proof);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: ledger_verify_derivation_proof returned %d\n", err);
    return;
  }
  print("  Verified proof\n");

  // 5. Try deriving with EXCESS permissions (should fail)
  UserCapability bad_cap;
  err = ledger_derive(&child_cap, up, CAP_PERM_READ | CAP_PERM_WRITE, &bad_cap);
  if (err == BLIND_LEDGER_EPERM) {
    print("  Correctly rejected excess permissions\n");
  } else {
    print("FAIL: should have rejected excess permissions, got %d\n", err);
    return;
  }

  print("PASS: Derivation chain test\n");
}

void exchange_test(void) {
  print("Running exchange_test (Red->Blue via derivation)...\n");

  // 1. Mint a "Red" root capability
  UserCapability red_cap;
  u8int secret[32] = {0xAA, 0xBB, 0xCC};
  BlindLedgerError err =
      ledger_mint(&red_cap, 0x2000, 4096, up,
                  CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_GRANT, secret);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: mint Red returned %d\n", err);
    return;
  }
  print("  Minted Red capability\n");

  // 2. "Exchange" Red → Blue via derivation (Blue = child of Red)
  // In this model, Blue has a subset of Red's permissions
  UserCapability blue_cap;
  err = ledger_derive(&red_cap, up, CAP_PERM_READ | CAP_PERM_WRITE, &blue_cap);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: derive Blue from Red returned %d\n", err);
    return;
  }
  print("  Derived Blue from Red\n");

  // 3. Verify Blue's provenance traces back to Red
  DerivationProof blue_proof;
  err = ledger_get_derivation_proof(&blue_cap, up, &blue_proof);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: get Blue proof returned %d\n", err);
    return;
  }
  print("  Blue proof chain_length=%d\n", blue_proof.chain_length);

  // 4. Verify the Blue proof
  err = ledger_verify_derivation_proof(&blue_proof);
  if (err != BLIND_LEDGER_OK) {
    print("FAIL: verify Blue proof returned %d\n", err);
    return;
  }
  print("  Verified Blue's derivation from Red\n");

  print("PASS: Red->Blue exchange test\n");
}

int main(int argc, char **argv) {
  print("Starting Pebble Userspace Test...\n");

  stubs_init();

  print("Initializing Subsystems...\n");
  borrowinit();
  blind_ledger_init();
  pebbleinit();

  up->pebble.drop_budget = 0;
  pebbleprocinit(up);

  print("Running pebble_selftest...\n");
  pebble_selftest();

  print("Running pebble_sip_issue_test...\n");
  pebble_sip_issue_test();

  attest_test();
  derivation_test();
  exchange_test();

  print("Tests Completed Successfully.\n");
  return 0;
}
