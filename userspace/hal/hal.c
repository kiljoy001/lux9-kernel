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
#include <u.h>
#define SET(x) ((x) = 0)
#include <libc.h>

/* Headers or prototypes */
void channel_manager_init(void);
int family_tpm_init(void);
void rump_integration_init(void);

/* Stubs for missing liblux symbols */
char *argv0;

void sysfatal(char *fmt, ...) {
  // crash
  print("FATAL: %s\n", fmt);
  *(int *)0 = 0;
  while (1)
    ;
}

int print(char *fmt, ...) {
  // Minimal stub
  return 0;
}

int fprint(int fd, char *fmt, ...) { return 0; }

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

char *strncpy(char *dest, char *src, long n) {
  long i;
  for (i = 0; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
  for (; i < n; i++)
    dest[i] = '\0';
  return dest;
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
  ARGBEGIN {
  case 'd':
    hal_ctx.debug_level++;
    break;
  default:
    usage();
  }
  ARGEND;

  print("HAL: Starting Hardware Abstraction Layer...\n");

  /* Initialize core subsystems */
  // resource_pool_init(); /* Handled by families */
  family_init_registry();
  channel_manager_init();

  /* Initialize families */
  pcifamily_init();
  family_tpm_init();

  /* Connect to Rump Server */
  rump_integration_init();

  /* Main Event Loop */
  hal_ctx.running = 1;
  while (hal_ctx.running) {
    // Handle 9P requests
    // Handle Rump events
    sleep(1000); // Temporary yield
  }

  return 0;
}
