#include "libc.h"
#include "lux.h"
#include "u.h"

static int read_pid2_proc(u8int out[16]) {
  char path[64];
  int fd;
  long n;

  snprint(path, sizeof(path), "/proc/%d/pid2", getpid());
  fd = sys_open(path, OREAD);
  if (fd < 0)
    return -1;
  n = sys_read(fd, out, 16);
  sys_close(fd);
  return (n == 16) ? 0 : -1;
}

static u16int pid2_data_b(const u8int pid2[16]) {
  return (u16int)(((pid2[6] & 0x0F) << 8) | pid2[7]);
}

int main(void) {
  u8int pid2_sys[16];
  u8int pid2_proc[16];

  if (sys_getpid2(pid2_sys, sizeof(pid2_sys)) < 0)
    return 1;
  if (read_pid2_proc(pid2_proc) < 0)
    return 2;
  if (memcmp(pid2_sys, pid2_proc, 16) != 0)
    return 3;

  int pid = sys_rfork(RFPROC | RFNAMEG);
  if (pid < 0)
    return 4;

  if (pid == 0) {
    u8int child_before[16];
    u8int child_after[16];

    if (sys_getpid2(child_before, sizeof(child_before)) < 0)
      return 5;
    if (sys_bind("/boot", "/boot", MAFTER) < 0)
      return 0;
    if (sys_getpid2(child_after, sizeof(child_after)) < 0)
      return 6;
    if (pid2_data_b(child_before) == pid2_data_b(child_after))
      return 7;
    return 0;
  }

  return 0;
}
