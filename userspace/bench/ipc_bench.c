#include "../lib/liblux/inc/lux.h"

/* Simple IPC Benchmark */

#define ITERATIONS 100000
#define OREAD 0
#define OWRITE 1

void panic(char *msg) {
  sys_print("FAIL: %s\n", msg);
  sys_exit("bench failed");
}

int main(void) {
  int fd;
  char buf[32];
  uvlong start, end, total;
  int i;

  sys_print("BENCH: Starting Ring IPC Benchmark (%d iterations)\n", ITERATIONS);

  /* 1. Setup Environment Variable */
  fd = sys_create("/env/BENCH", OWRITE, 0666);
  if (fd < 0)
    panic("create failed");
  sys_write(fd, "X", 1);
  sys_close(fd);

  /* 2. Open for reading */
  fd = sys_open("/env/BENCH", OREAD);
  if (fd < 0)
    panic("open failed");

  /* 3. Run Benchmark */
  start = sys_nsec();
  for (i = 0; i < ITERATIONS; i++) {
    if (sys_pread(fd, buf, 1, 0) != 1) {
      panic("read failed");
    }
  }
  end = sys_nsec();

  total = end - start;
  sys_print("BENCH: Total time: %llu ns\n", total);
  sys_print("BENCH: Average latency: %llu ns/op\n", total / ITERATIONS);

  sys_close(fd);
  return 0;
}
