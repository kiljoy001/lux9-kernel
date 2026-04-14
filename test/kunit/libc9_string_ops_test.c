#include "kunit.h"

extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, ulong n);
extern char *strcat(char *dst, const char *src);
extern char *strncat(char *dst, const char *src, ulong n);
extern char *strchr(const char *s, int c);
extern int strncmp(const char *s1, const char *s2, ulong n);
extern int cistrncmp(char *s1, char *s2, int n);
extern char *strstr(const char *s1, const char *s2);
extern void *memset(void *ap, int c, usize n);

static void strcpy_copies_and_returns_destination(struct kunit *test) {
  char dst[16] = {0};
  char *ret = strcpy(dst, "router");

  KUNIT_EXPECT_TRUE(test, ret == dst);
  KUNIT_EXPECT_STREQ(test, dst, "router");
}

static void strncpy_zero_pads_when_source_shorter(struct kunit *test) {
  char dst[8];

  memset(dst, 'x', sizeof(dst));
  strncpy(dst, "ab", 5);

  KUNIT_EXPECT_EQ(test, dst[0], 'a');
  KUNIT_EXPECT_EQ(test, dst[1], 'b');
  KUNIT_EXPECT_EQ(test, dst[2], '\0');
  KUNIT_EXPECT_EQ(test, dst[3], '\0');
  KUNIT_EXPECT_EQ(test, dst[4], '\0');
  KUNIT_EXPECT_EQ(test, dst[5], 'x');
}

static void strncpy_truncates_without_terminator(struct kunit *test) {
  char dst[4] = {'x', 'x', 'x', 'x'};

  strncpy(dst, "kernel", 3);

  KUNIT_EXPECT_EQ(test, dst[0], 'k');
  KUNIT_EXPECT_EQ(test, dst[1], 'e');
  KUNIT_EXPECT_EQ(test, dst[2], 'r');
  KUNIT_EXPECT_EQ(test, dst[3], 'x');
}

static void strcat_and_strncat_append(struct kunit *test) {
  char dst[16] = "lux";
  char *ret1 = strcat(dst, "9");
  char *ret2 = strncat(dst, "-os", 8);

  KUNIT_EXPECT_TRUE(test, ret1 == dst);
  KUNIT_EXPECT_TRUE(test, ret2 == dst);
  KUNIT_EXPECT_STREQ(test, dst, "lux9-os");
}

static void strchr_finds_byte_and_terminator(struct kunit *test) {
  const char *s = "service";
  char *v = strchr(s, 'v');
  char *z = strchr(s, '\0');

  KUNIT_EXPECT_TRUE(test, v != nil);
  KUNIT_EXPECT_EQ(test, (long long)(v - s), 3);
  KUNIT_EXPECT_EQ(test, *v, 'v');
  KUNIT_EXPECT_TRUE(test, z == s + 7);
}

static void strchr_returns_nil_when_missing(struct kunit *test) {
  KUNIT_EXPECT_TRUE(test, strchr("srv", 'z') == nil);
}

static void strncmp_and_cistrncmp_compare_sign_only(struct kunit *test) {
  char mixed_a[] = "HeLLo";
  char mixed_b[] = "hello";
  char less[] = "abc";
  char greater[] = "AbD";

  KUNIT_EXPECT_EQ(test, strncmp("kernel", "kernx", 4), 0);
  KUNIT_EXPECT_EQ(test, strncmp("abc", "abd", 3), -1);
  KUNIT_EXPECT_EQ(test, strncmp("abd", "abc", 3), 1);
  KUNIT_EXPECT_EQ(test, cistrncmp(mixed_a, mixed_b, 5), 0);
  KUNIT_EXPECT_EQ(test, cistrncmp(less, greater, 3), -1);
}

static void strstr_handles_empty_and_first_match(struct kunit *test) {
  const char *haystack = "abcabc";
  char *empty = strstr(haystack, "");
  char *found = strstr(haystack, "cab");
  char *missing = strstr(haystack, "xyz");

  KUNIT_EXPECT_TRUE(test, empty == haystack);
  KUNIT_EXPECT_TRUE(test, found == haystack + 2);
  KUNIT_EXPECT_TRUE(test, missing == nil);
}

static const struct kunit_case libc9_string_ops_cases[] = {
    KUNIT_CASE(strcpy_copies_and_returns_destination),
    KUNIT_CASE(strncpy_zero_pads_when_source_shorter),
    KUNIT_CASE(strncpy_truncates_without_terminator),
    KUNIT_CASE(strcat_and_strncat_append),
    KUNIT_CASE(strchr_finds_byte_and_terminator),
    KUNIT_CASE(strchr_returns_nil_when_missing),
    KUNIT_CASE(strncmp_and_cistrncmp_compare_sign_only),
    KUNIT_CASE(strstr_handles_empty_and_first_match),
    KUNIT_CASE_END,
};

const struct kunit_suite libc9_string_ops_suite = {
    .name = "libc9_string_ops",
    .cases = libc9_string_ops_cases,
};
