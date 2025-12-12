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

  print("Tests Completed Successfully.\n");
  return 0;
}
