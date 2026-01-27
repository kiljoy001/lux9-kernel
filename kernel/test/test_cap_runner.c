/*
 * test_cap_runner.c - Test Runner for All Capability Tests
 *
 * Aggregates and runs:
 * - Unit tests (test_blind_cap.c)
 * - Integration tests (test_cap_integration.c)
 * - Boot tests (test_cap_boot.c)
 */

/* Test entry points */
extern int test_blind_cap_main(void);
extern int test_cap_integration_main(void);
extern int test_cap_boot_main(void);

extern void print(const char *);

/*
 * Run all capability tests
 */
int run_all_cap_tests(void) {
  int total_failures = 0;

  print("\n");
  print("╔════════════════════════════════════════════╗\n");
  print("║  CAPABILITY SYSTEM TEST SUITE              ║\n");
  print("╚════════════════════════════════════════════╝\n");

  /* Unit tests */
  int unit_failures = test_blind_cap_main();
  total_failures += unit_failures;

  /* Integration tests */
  int integration_failures = test_cap_integration_main();
  total_failures += integration_failures;

  /* Boot tests (use real system if available) */
  int boot_failures = test_cap_boot_main();
  total_failures += boot_failures;

  /* Summary */
  print("\n");
  print("╔════════════════════════════════════════════╗\n");
  if (total_failures == 0) {
    print("║  ALL TESTS PASSED                          ║\n");
  } else {
    print("║  SOME TESTS FAILED                         ║\n");
  }
  print("╚════════════════════════════════════════════╝\n");
  print("\n");

  return total_failures;
}

/*
 * Standalone test main (for running tests outside of kernel)
 */
int main(void) { return run_all_cap_tests(); }
