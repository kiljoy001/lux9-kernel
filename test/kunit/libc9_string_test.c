#include "kunit.h"

extern ulong strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern int memcmp(const void *a1, const void *a2, usize n);
extern void *memmove(void *dst, const void *src, usize n);
extern void *memcpy(void *dst, const void *src, usize n);

static void strlen_handles_empty_and_ascii(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, (long long)strlen(""), 0);
  KUNIT_EXPECT_EQ(test, (long long)strlen("lux9"), 4);
  KUNIT_EXPECT_EQ(test, (long long)strlen("kunit-style"), 11);
}

static void strcmp_returns_sign_only(struct kunit *test) {
  KUNIT_EXPECT_EQ(test, strcmp("same", "same"), 0);
  KUNIT_EXPECT_EQ(test, strcmp("abc", "abd"), -1);
  KUNIT_EXPECT_EQ(test, strcmp("abd", "abc"), 1);
  KUNIT_EXPECT_EQ(test, strcmp("ab", "aba"), -1);
  KUNIT_EXPECT_EQ(test, strcmp("aba", "ab"), 1);
}

static void memcmp_uses_unsigned_ordering(struct kunit *test) {
  uchar a[] = {0x80, 0x10, 0x20};
  uchar b[] = {0x7f, 0x10, 0x20};
  uchar c[] = {0x80, 0x10, 0x20};

  KUNIT_EXPECT_EQ(test, memcmp(a, b, 3), 1);
  KUNIT_EXPECT_EQ(test, memcmp(b, a, 3), -1);
  KUNIT_EXPECT_EQ(test, memcmp(a, c, 3), 0);
}

static void memmove_handles_overlap(struct kunit *test) {
  char left_shift[8] = "abcde";
  char right_shift[8] = "abcde";

  memmove(left_shift + 1, left_shift, 4);
  left_shift[5] = '\0';
  KUNIT_EXPECT_STREQ(test, left_shift, "aabcd");

  memmove(right_shift, right_shift + 1, 4);
  right_shift[4] = '\0';
  KUNIT_EXPECT_STREQ(test, right_shift, "bcde");
}

static void memcpy_aliases_to_memmove(struct kunit *test) {
  char src[] = "kernel";
  char dst[16] = {0};
  void *ret = memcpy(dst, src, 7);

  KUNIT_EXPECT_TRUE(test, ret == dst);
  KUNIT_EXPECT_STREQ(test, dst, "kernel");
}

static const struct kunit_case libc9_string_cases[] = {
    KUNIT_CASE(strlen_handles_empty_and_ascii),
    KUNIT_CASE(strcmp_returns_sign_only),
    KUNIT_CASE(memcmp_uses_unsigned_ordering),
    KUNIT_CASE(memmove_handles_overlap),
    KUNIT_CASE(memcpy_aliases_to_memmove),
    KUNIT_CASE_END,
};

const struct kunit_suite libc9_string_suite = {
    .name = "libc9_string",
    .cases = libc9_string_cases,
};
