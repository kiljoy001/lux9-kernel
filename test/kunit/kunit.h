#ifndef LUX9_KUNIT_H
#define LUX9_KUNIT_H

#include <stdarg.h>
#include <stdio.h>
#include "u.h"

struct kunit {
  const char *suite_name;
  const char *case_name;
  int assertions;
  int failed_assertions;
};

typedef void (*kunit_case_fn)(struct kunit *test);

struct kunit_case {
  const char *name;
  kunit_case_fn run_case;
};

struct kunit_suite {
  const char *name;
  const struct kunit_case *cases;
};

static inline int kunit_streq(const char *a, const char *b) {
  if (a == nil || b == nil)
    return a == b;
  while (*a != '\0' && *a == *b) {
    a++;
    b++;
  }
  return *a == *b;
}

static inline void kunit_record(struct kunit *test, int pass,
                                const char *expr, const char *file, int line,
                                const char *fmt, ...) {
  test->assertions++;
  if (pass)
    return;

  test->failed_assertions++;
  fprintf(stderr, "[KUnit][FAIL] %s.%s: %s (%s:%d)",
          test->suite_name, test->case_name, expr, file, line);

  if (fmt != nil) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, " ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
  }
  fprintf(stderr, "\n");
}

#define KUNIT_EXPECT_TRUE(test, cond)                                          \
  do {                                                                         \
    kunit_record((test), (cond) != 0, #cond, __FILE__, __LINE__, nil);        \
  } while (0)

#define KUNIT_EXPECT_EQ(test, left, right)                                     \
  do {                                                                         \
    long long _left = (long long)(left);                                       \
    long long _right = (long long)(right);                                     \
    kunit_record((test), _left == _right, #left " == " #right, __FILE__,      \
                 __LINE__, "left=%lld right=%lld", _left, _right);            \
  } while (0)

#define KUNIT_EXPECT_STREQ(test, left, right)                                  \
  do {                                                                         \
    const char *_left = (left);                                                \
    const char *_right = (right);                                              \
    kunit_record((test), kunit_streq(_left, _right), #left " == " #right,     \
                 __FILE__, __LINE__, "left=\"%s\" right=\"%s\"",             \
                 _left == nil ? "(nil)" : _left,                               \
                 _right == nil ? "(nil)" : _right);                            \
  } while (0)

#define KUNIT_CASE(fn)                                                         \
  {                                                                            \
    #fn, (fn)                                                                   \
  }

#define KUNIT_CASE_END                                                         \
  {                                                                            \
    nil, nil                                                                    \
  }

#endif
