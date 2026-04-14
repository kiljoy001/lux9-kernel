#include "kunit.h"

extern void *memset(void *ap, int c, usize n);
extern void *memchr(const void *ap, int c, usize n);
extern void *memccpy(void *a1, const void *a2, int c, usize n);
extern char *strecpy(char *to, char *e, const char *from);

static void memset_sets_exact_span(struct kunit *test) {
  char buf[6] = "abcde";
  void *ret = memset(buf, 'x', 4);

  KUNIT_EXPECT_TRUE(test, ret == buf);
  KUNIT_EXPECT_EQ(test, buf[0], 'x');
  KUNIT_EXPECT_EQ(test, buf[1], 'x');
  KUNIT_EXPECT_EQ(test, buf[2], 'x');
  KUNIT_EXPECT_EQ(test, buf[3], 'x');
  KUNIT_EXPECT_EQ(test, buf[4], 'e');
}

static void memchr_finds_first_match_or_nil(struct kunit *test) {
  uchar data[] = {0x10, 0x42, 0x42, 0x99};
  uchar *found = memchr(data, 0x42, 4);
  void *missing = memchr(data, 0xff, 4);

  KUNIT_EXPECT_TRUE(test, found == data + 1);
  KUNIT_EXPECT_TRUE(test, missing == nil);
}

static void memccpy_stops_after_delimiter(struct kunit *test) {
  char src[] = "abc:def";
  char dst[16] = {0};
  char *ret = memccpy(dst, src, ':', 7);

  KUNIT_EXPECT_TRUE(test, ret == dst + 4);
  dst[4] = '\0';
  KUNIT_EXPECT_STREQ(test, dst, "abc:");
}

static void memccpy_returns_nil_when_not_found(struct kunit *test) {
  char src[] = "plan9";
  char dst[8] = {0};
  void *ret = memccpy(dst, src, 'x', 5);

  KUNIT_EXPECT_TRUE(test, ret == nil);
  KUNIT_EXPECT_EQ(test, dst[0], 'p');
  KUNIT_EXPECT_EQ(test, dst[1], 'l');
  KUNIT_EXPECT_EQ(test, dst[2], 'a');
  KUNIT_EXPECT_EQ(test, dst[3], 'n');
  KUNIT_EXPECT_EQ(test, dst[4], '9');
}

static void strecpy_writes_full_string_when_space_allows(struct kunit *test) {
  char buf[16];
  char *end = strecpy(buf, buf + sizeof(buf), "lux9");

  KUNIT_EXPECT_TRUE(test, end == buf + 4);
  KUNIT_EXPECT_EQ(test, *end, '\0');
  KUNIT_EXPECT_STREQ(test, buf, "lux9");
}

static void strecpy_truncates_and_terminates(struct kunit *test) {
  char buf[5];
  char *end = strecpy(buf, buf + sizeof(buf), "kernel");

  KUNIT_EXPECT_TRUE(test, end == buf + 4);
  KUNIT_EXPECT_EQ(test, buf[0], 'k');
  KUNIT_EXPECT_EQ(test, buf[1], 'e');
  KUNIT_EXPECT_EQ(test, buf[2], 'r');
  KUNIT_EXPECT_EQ(test, buf[3], 'n');
  KUNIT_EXPECT_EQ(test, buf[4], '\0');
}

static void strecpy_is_noop_when_range_empty(struct kunit *test) {
  char buf[4] = "abc";
  char *ret = strecpy(buf, buf, "x");

  KUNIT_EXPECT_TRUE(test, ret == buf);
  KUNIT_EXPECT_STREQ(test, buf, "abc");
}

static const struct kunit_case libc9_memory_cases[] = {
    KUNIT_CASE(memset_sets_exact_span),
    KUNIT_CASE(memchr_finds_first_match_or_nil),
    KUNIT_CASE(memccpy_stops_after_delimiter),
    KUNIT_CASE(memccpy_returns_nil_when_not_found),
    KUNIT_CASE(strecpy_writes_full_string_when_space_allows),
    KUNIT_CASE(strecpy_truncates_and_terminates),
    KUNIT_CASE(strecpy_is_noop_when_range_empty),
    KUNIT_CASE_END,
};

const struct kunit_suite libc9_memory_suite = {
    .name = "libc9_memory",
    .cases = libc9_memory_cases,
};
