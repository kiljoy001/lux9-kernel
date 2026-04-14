#include "kunit.h"

extern const struct kunit_suite libc9_string_suite;
extern const struct kunit_suite libc9_string_ops_suite;
extern const struct kunit_suite libc9_memory_suite;
extern const struct kunit_suite libc9_numeric_suite;
extern const struct kunit_suite blind_cap_suite;
extern const struct kunit_suite critical_contracts_suite;

static const struct kunit_suite *kunit_suites[] = {
    &libc9_string_suite,
    &libc9_string_ops_suite,
    &libc9_memory_suite,
    &libc9_numeric_suite,
    &blind_cap_suite,
    &critical_contracts_suite,
    nil,
};

int main(void) {
  int total_cases = 0;
  int failed_cases = 0;
  int total_assertions = 0;
  int failed_assertions = 0;

  for (int s = 0; kunit_suites[s] != nil; s++) {
    const struct kunit_suite *suite = kunit_suites[s];
    printf("[KUnit] suite: %s\n", suite->name);

    for (int c = 0; suite->cases[c].name != nil; c++) {
      const struct kunit_case *test_case = &suite->cases[c];
      struct kunit test = {
          .suite_name = suite->name,
          .case_name = test_case->name,
          .assertions = 0,
          .failed_assertions = 0,
      };

      test_case->run_case(&test);
      total_cases++;
      total_assertions += test.assertions;
      failed_assertions += test.failed_assertions;

      if (test.failed_assertions > 0) {
        failed_cases++;
        printf("[KUnit] FAIL: %s.%s (%d/%d failed assertions)\n",
               suite->name, test_case->name,
               test.failed_assertions, test.assertions);
      } else {
        printf("[KUnit] PASS: %s.%s (%d assertions)\n",
               suite->name, test_case->name, test.assertions);
      }
    }
  }

  printf("[KUnit] summary: cases=%d failed_cases=%d assertions=%d failed_assertions=%d\n",
         total_cases, failed_cases, total_assertions, failed_assertions);
  return failed_cases == 0 ? 0 : 1;
}
