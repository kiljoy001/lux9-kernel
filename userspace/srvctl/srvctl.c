/* srvctl - Service registration helper for resurrection server
 *
 * Registers plain executables with the resurrection server via /srv.
 * Services don't need self-registration code - this helper does it for them.
 *
 * Usage:
 *   srvctl register <name> <exec_path>  - Register a new service
 *
 * The resurrection server's /srv accepts:
 *   - Tcreate to create /srv/<name>
 *   - Twrite with "exec=<path>" to configure and auto-start
 */

/* Minimal syscall declarations - avoid full lux.h to prevent header conflicts
 */
typedef unsigned int uint;
typedef unsigned long ulong;

extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern void sys_exit(char *msg);
extern int sys_create(char *path, int mode, uint perm);

#define OREAD 0
#define OWRITE 1
#define ORDWR 2

/* Minimal print implementation for liblux */
int print(char *fmt, ...) {
  int len = 0;
  while (fmt[len])
    len++;
  sys_write(2, (void *)fmt, len);
  return len;
}

static void srvctl_print(const char *msg) {
  int len = 0;
  while (msg[len])
    len++;
  sys_write(1, (void *)msg, len);
}

static int srvctl_strlen(const char *s) {
  int n = 0;
  while (*s++)
    n++;
  return n;
}

static int srvctl_strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

static void srvctl_strcpy(char *dst, const char *src) {
  while ((*dst++ = *src++))
    ;
}

/*
 * Register a service with the resurrection server.
 * Creates /srv/<name> and writes exec=<exec_path> to configure it.
 */
static int register_service(const char *name, const char *exec_path) {
  char path[128];
  char config[256];
  int i, fd, n;

  /* Build path: /srv/<name> */
  i = 0;
  path[i++] = '/';
  path[i++] = 's';
  path[i++] = 'r';
  path[i++] = 'v';
  path[i++] = '/';
  for (const char *p = name; *p && i < 120; p++)
    path[i++] = *p;
  path[i] = 0;

  /* Build config: exec=<exec_path> */
  i = 0;
  config[i++] = 'e';
  config[i++] = 'x';
  config[i++] = 'e';
  config[i++] = 'c';
  config[i++] = '=';
  for (const char *p = exec_path; *p && i < 250; p++)
    config[i++] = *p;
  config[i] = 0;

  /* Create /srv/<name> */
  srvctl_print("srvctl: creating ");
  srvctl_print(path);
  srvctl_print("\n");

  fd = sys_create(path, OWRITE, 0666);
  if (fd < 0) {
    srvctl_print("srvctl: failed to create ");
    srvctl_print(path);
    srvctl_print("\n");
    return -1;
  }

  /* Write config */
  srvctl_print("srvctl: writing config: ");
  srvctl_print(config);
  srvctl_print("\n");

  n = srvctl_strlen(config);
  if (sys_write(fd, config, n) != n) {
    srvctl_print("srvctl: failed to write config\n");
    sys_close(fd);
    return -1;
  }

  sys_close(fd);
  srvctl_print("srvctl: service registered successfully\n");
  return 0;
}

static void print_usage(void) {
  srvctl_print("Usage: srvctl <command> [args...]\n");
  srvctl_print("\n");
  srvctl_print("Commands:\n");
  srvctl_print("  register <name> <exec_path>  Register a new service\n");
  srvctl_print("\n");
  srvctl_print("Example:\n");
  srvctl_print("  srvctl register my_service /boot/my_binary\n");
}

/* Entry point - argc/argv passed from start.S via stack */
void main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    sys_exit("no command");
  }

  if (srvctl_strcmp(argv[1], "register") == 0) {
    if (argc < 4) {
      srvctl_print("srvctl: register requires <name> and <exec_path>\n");
      sys_exit("missing args");
    }
    if (register_service(argv[2], argv[3]) < 0) {
      sys_exit("register failed");
    }
  } else if (srvctl_strcmp(argv[1], "help") == 0 ||
             srvctl_strcmp(argv[1], "-h") == 0 ||
             srvctl_strcmp(argv[1], "--help") == 0) {
    print_usage();
  } else {
    srvctl_print("srvctl: unknown command: ");
    srvctl_print(argv[1]);
    srvctl_print("\n");
    print_usage();
    sys_exit("unknown command");
  }

  sys_exit(0);
}
