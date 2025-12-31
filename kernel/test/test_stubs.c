/*
 * test_stubs.c - Stubs for Standalone Capability Tests
 *
 * Provides implementations of kernel functions for host testing.
 * Crypto is provided by real monocypher.c linked in the test binary.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../include/u.h"

/* Print function */
void print(const char *msg) { fputs(msg, stdout); }

/* Mock control variables - tests can set these */
vlong mock_nsec_value = 0;
vlong mock_nsec = 0; /* Alias for integration tests */
u8int mock_random_seed = 0x42;
u8int mock_rand_seed = 0x55; /* Alias for integration tests */

/* Time function - uses mock if set, else real time */
vlong nsec(void) {
  if (mock_nsec_value != 0)
    return mock_nsec_value;
  if (mock_nsec != 0)
    return mock_nsec;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (vlong)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* Random bytes - deterministic based on mock seeds for repeatable tests */
void randombytes(u8int *buf, usize len) {
  for (usize i = 0; i < len; i++) {
    buf[i] = mock_random_seed ^ mock_rand_seed ^ (u8int)i;
  }
}

/* Kernel string length wrapper */
usize kstrlen(const char *s) { return strlen(s); }
