#include "kunit.h"

extern int atoi(const char *s);
extern long atol(char *s);
extern long strtol(const char *nptr, char **endptr, int base);
extern ulong strtoul(const char *nptr, char **endptr, int base);
extern uvlong strtoull(const char *nptr, char **endptr, int base);

static void atoi_and_atol_parse_whitespace_and_sign(struct kunit *test) {
  char atol_input[] = " \t-77tail";

  KUNIT_EXPECT_EQ(test, atoi("42"), 42);
  KUNIT_EXPECT_EQ(test, atoi("  +19x"), 19);
  KUNIT_EXPECT_EQ(test, atoi("  -17xyz"), -17);
  KUNIT_EXPECT_EQ(test, atol(atol_input), -77);
}

static void strtol_autodetects_base_and_tracks_endptr(struct kunit *test) {
  char input[] = "0x1fZ";
  char *end = nil;
  long value = strtol(input, &end, 0);

  KUNIT_EXPECT_EQ(test, value, 31);
  KUNIT_EXPECT_TRUE(test, end == input + 4);
  KUNIT_EXPECT_EQ(test, *end, 'Z');
}

static void strtol_invalid_base_sets_endptr_to_start(struct kunit *test) {
  char input[] = "123";
  char *end = nil;
  long value = strtol(input, &end, 37);

  KUNIT_EXPECT_EQ(test, value, 0);
  KUNIT_EXPECT_TRUE(test, end == input);
}

static void strtol_clamps_on_overflow(struct kunit *test) {
  char *pos_end = nil;
  char *neg_end = nil;
  long pos = strtol("99999999999999999999999", &pos_end, 10);
  long neg = strtol("-99999999999999999999999", &neg_end, 10);

  KUNIT_EXPECT_EQ(test, pos, 2147483647L);
  KUNIT_EXPECT_EQ(test, neg, -2147483648L);
  KUNIT_EXPECT_EQ(test, *pos_end, '\0');
  KUNIT_EXPECT_EQ(test, *neg_end, '\0');
}

static void strtoul_autodetects_octal_and_hex(struct kunit *test) {
  char octal[] = "077x";
  char hex[] = "0x2a!";
  char *oct_end = nil;
  char *hex_end = nil;
  ulong oct_value = strtoul(octal, &oct_end, 0);
  ulong hex_value = strtoul(hex, &hex_end, 0);

  KUNIT_EXPECT_EQ(test, (unsigned long long)oct_value, 63ULL);
  KUNIT_EXPECT_TRUE(test, oct_end == octal + 3);
  KUNIT_EXPECT_EQ(test, *oct_end, 'x');
  KUNIT_EXPECT_EQ(test, (unsigned long long)hex_value, 42ULL);
  KUNIT_EXPECT_TRUE(test, hex_end == hex + 4);
  KUNIT_EXPECT_EQ(test, *hex_end, '!');
}

static void strtoul_invalid_base_sets_endptr_to_start(struct kunit *test) {
  char input[] = "99";
  char *end = nil;
  ulong value = strtoul(input, &end, 1);

  KUNIT_EXPECT_EQ(test, (unsigned long long)value, 0ULL);
  KUNIT_EXPECT_TRUE(test, end == input);
}

static void strtoul_overflow_saturates(struct kunit *test) {
  char *end1 = nil;
  char *end2 = nil;
  ulong overflow1 = strtoul("999999999999999999999999", &end1, 10);
  ulong overflow2 = strtoul("1844674407370955161500", &end2, 10);

  KUNIT_EXPECT_EQ(test, (unsigned long long)overflow1,
                  (unsigned long long)overflow2);
  KUNIT_EXPECT_TRUE(test, overflow1 > 0);
  KUNIT_EXPECT_EQ(test, *end1, '\0');
  KUNIT_EXPECT_EQ(test, *end2, '\0');
}

static void strtoull_parses_and_saturates_on_overflow(struct kunit *test) {
  char value_input[] = "0x10rest";
  char *value_end = nil;
  uvlong value = strtoull(value_input, &value_end, 0);

  char *ov_end1 = nil;
  char *ov_end2 = nil;
  uvlong overflow1 = strtoull("1844674407370955161600", &ov_end1, 10);
  uvlong overflow2 = strtoull("9999999999999999999999999999", &ov_end2, 10);

  KUNIT_EXPECT_EQ(test, (unsigned long long)value, 16ULL);
  KUNIT_EXPECT_TRUE(test, value_end == value_input + 4);
  KUNIT_EXPECT_EQ(test, *value_end, 'r');
  KUNIT_EXPECT_EQ(test, (unsigned long long)overflow1,
                  (unsigned long long)overflow2);
  KUNIT_EXPECT_TRUE(test, overflow1 > 0);
  KUNIT_EXPECT_EQ(test, *ov_end1, '\0');
  KUNIT_EXPECT_EQ(test, *ov_end2, '\0');
}

static const struct kunit_case libc9_numeric_cases[] = {
    KUNIT_CASE(atoi_and_atol_parse_whitespace_and_sign),
    KUNIT_CASE(strtol_autodetects_base_and_tracks_endptr),
    KUNIT_CASE(strtol_invalid_base_sets_endptr_to_start),
    KUNIT_CASE(strtol_clamps_on_overflow),
    KUNIT_CASE(strtoul_autodetects_octal_and_hex),
    KUNIT_CASE(strtoul_invalid_base_sets_endptr_to_start),
    KUNIT_CASE(strtoul_overflow_saturates),
    KUNIT_CASE(strtoull_parses_and_saturates_on_overflow),
    KUNIT_CASE_END,
};

const struct kunit_suite libc9_numeric_suite = {
    .name = "libc9_numeric",
    .cases = libc9_numeric_cases,
};
