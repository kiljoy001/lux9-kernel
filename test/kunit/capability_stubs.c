#include "u.h"

vlong kunit_mock_nsec_value = 0;
u8int kunit_mock_random_seed = 0x42;

vlong nsec(void) { return kunit_mock_nsec_value; }

void randombytes(u8int *buf, usize len) {
  for (usize i = 0; i < len; i++) {
    buf[i] = (u8int)(kunit_mock_random_seed ^ (u8int)i);
  }
}
