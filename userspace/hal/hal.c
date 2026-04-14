/*
 * Hardware Abstraction Layer (HAL) Server
 *
 * The HAL server acts as the central coordinator for device families in
 * userspace. It manages:
 * 1. Device Family Registry (PCI, USB, etc.)
 * 2. Resource Pools (I/O memory, IRQs)
 * 3. Channel Management for client access
 * 4. Rump Kernel Integration for driver reuse
 */

#include "family.h"
#include "family_pci.h"
#include <stdarg.h>
#include <u.h>
#define SET(x) ((x) = 0)
#include <libc.h>

/* Headers or prototypes */
void channel_manager_init(void);
int family_tpm_init(void);
void rump_integration_init(void);
int hal_service_main(void);

/* Stubs for missing liblux symbols */
char *argv0;

extern long sys_write(int fd, void *buf, long n);
extern ulong strlen(const char *s);
extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);

void sysfatal(char *fmt, ...) {
  // crash
  print("FATAL: %s\n", fmt);
  *(int *)0 = 0;
  while (1)
    ;
}

int print(char *fmt, ...) {
  char buf[1024];
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprint(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n > 0) {
    sys_write(1, buf, n);
  }
  return n;
}

int fprint(int fd, char *fmt, ...) {
  char buf[1024];
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprint(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n > 0) {
    sys_write(fd, buf, n);
  }
  return n;
}


int sleep(long ms) { return 0; }

void exits(char *s) {
  // syscall_exit(0);
  while (1)
    ;
}

// chartorune might be needed by ARGBEGIN
int chartorune(Rune *rune, char *str) {
  *rune = *str;
  return 1;
}

static void hal_probe_path(char *path) {
  int fd;

  fd = sys_open(path, 0);
  print("HAL: probe open %s -> %d\n", path, fd);
  if (fd >= 0)
    sys_close(fd);
}

static void hal_dump_caps(void) {
  char buf[256];
  int fd;
  long n;

  fd = sys_open("#Y/ctl", 0);
  print("HAL: probe open #Y/ctl -> %d\n", fd);
  if (fd < 0)
    return;
  n = sys_read(fd, buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = 0;
    print("HAL: current caps:\n%s", buf);
  } else {
    print("HAL: failed to read #Y/ctl (%ld)\n", n);
  }
  sys_close(fd);
}

/* Global HAL Context */
struct HalContext {
  int running;
  int debug_level;
};

struct HalContext hal_ctx;

void usage(void) {
  fprint(2, "usage: hal [-d]\n");
  exits("usage");
}

int main(int argc, char *argv[]) {
  // Add basic troubleshooting output
  const char *startup_msg = "HAL: Starting Hardware Abstraction Layer...\n";
  sys_write(1, (void*)startup_msg, strlen(startup_msg));

  ARGBEGIN {
  case 'd':
    hal_ctx.debug_level++;
    const char *debug_msg = "HAL: Debug mode enabled\n";
    sys_write(1, (void*)debug_msg, strlen(debug_msg));
    break;
  default:
    usage();
  }
  ARGEND;

  sys_write(1, (void*)"HAL: Initializing core subsystems...\n", 38);
  /* Initialize core subsystems */
  family_init_registry();
  channel_manager_init();
  hal_dump_caps();
  hal_probe_path(PCI_SHARP_ROOT);
  hal_probe_path(PCI_BUS_PATH);

  sys_write(1, (void*)"HAL: Initializing device families...\n", 37);
  /* Initialize families */
  pcifamily_init();
  family_tpm_init();

  sys_write(1, (void*)"HAL: Connecting to Rump Server...\n", 34);
  /* Connect to Rump Server */
  rump_integration_init();

  sys_write(1, (void*)"HAL: Entering service loop...\n", 29);
  return hal_service_main();
}
