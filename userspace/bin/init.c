/* init - first userspace process */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Syscall stubs */
extern int spawn(const char *path, char *const argv[]);
extern int mount(const char *path, int server_pid, const char *proto);
extern int bind(const char *new, const char *old, int flag);
extern int open(const char *path, int flags);
extern void exit(int status);
extern int wait(int *status);
extern void sleep_ms(int ms);

/* Constants for bind ( Plan 9 standard ) */
#define MREPL 0x0000   /* mount replaces old */
#define MBEFORE 0x0001 /* mount goes before others in union */
#define MAFTER 0x0002  /* mount goes after others in union */
#define MCREATE 0x0004 /* permit creation in new directory */

static void panic(const char *msg) {
  printf("PANIC: %s\n", msg);
  for (;;)
    ;
}

static void mount_system_fs(void) {
  printf("init: bootstrapping filesystems...\n");

  /* Bind kernel devices */
  /* /dev usually requires #c (console/dev) */
  printf("init: binding #c to /dev...\n");
  if (bind("#c", "/dev", MREPL) < 0) {
    printf("init: bind #c /dev failed (ignoring)\n");
  }

  /* /proc usually requires #p */
  printf("init: binding #p to /proc...\n");
  if (bind("#p", "/proc", MREPL) < 0) {
    printf("init: bind #p /proc failed\n");
  }

  /* bind #s to /srv if available */
  bind("#s", "/srv", MREPL);
}

int main(int argc, char *argv[]) {
  char *shell_args[] = {"/bin/sh", NULL};

  (void)argc;
  (void)argv;

  printf("\n");
  printf("=== Lux9 Init ===\n");

  /* Bootstrap devfs/procfs */
  mount_system_fs();

  /* Run Secure Ramdisk Test if needed (preserving from previous version just in
   * case) */
  /* But simplified to just spawn shell as primary goal */

  printf("init: spawning /bin/sh...\n");
  if (spawn("/bin/sh", shell_args) < 0) {
    printf("init: failed to spawn shell\n");
    panic("no shell");
  }

  /* Loop forever reaping children */
  while (1) {
    int status;
    wait(&status);
  }

  return 0;
}
