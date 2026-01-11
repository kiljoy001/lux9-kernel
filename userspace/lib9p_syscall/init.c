/* Init - Process 1
 * Starts the resurrection server and registers Layer 2 services
 * Uses liblux for all syscalls
 */

/* Minimal syscall declarations - avoid full lux.h to prevent header conflicts
 */
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;

extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern int sys_create(char *path, int mode, uint perm);
extern int sys_rfork(int flags);
extern int sys_exec(char *path, char *argv[]);
extern void sys_exit(char *msg);
extern int sys_wait(void);
extern int sys_pid(void);

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define RFFDG (1 << 2)
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)

/* Minimal print implementation for liblux convM2S debug output */
int print(char *fmt, ...) {
  int len = 0;
  while (fmt[len])
    len++;
  sys_write(2, (void *)fmt, len);
  return len;
}

static void init_print(const char *msg) {
  int len = 0;
  while (msg[len])
    len++;
  sys_write(1, (void *)msg, len);
}

static int init_strlen(const char *s) {
  int n = 0;
  while (*s++)
    n++;
  return n;
}

static void print_int(long n) {
  char buf[32];
  int i = 0;
  int neg = 0;
  if (n < 0) {
    neg = 1;
    n = -n;
  }
  if (n == 0) {
    buf[i++] = '0';
  } else {
    while (n > 0) {
      buf[i++] = (n % 10) + '0';
      n /= 10;
    }
  }
  if (neg)
    buf[i++] = '-';

  // Reverse
  for (int j = 0; j < i / 2; j++) {
    char tmp = buf[j];
    buf[j] = buf[i - 1 - j];
    buf[i - 1 - j] = tmp;
  }
  sys_write(1, buf, i);
}

static void register_service(const char *name, const char *exec_path) {
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
  fd = sys_create(path, OWRITE, 0666);
  if (fd < 0) {
    init_print("init: failed to create ");
    init_print(path);
    init_print("\n");
    return;
  }

  /* Write config */
  n = init_strlen(config);
  if (sys_write(fd, config, n) != n) {
    init_print("init: failed to write config to ");
    init_print(path);
    init_print("\n");
  }

  sys_close(fd);
}

void main(void) {
  init_print("=== Lux9 Init Starting ===\n");
  init_print("init: I AM NEW! Build ID: 1\n");
  int pid = sys_rfork(RFPROC | RFFDG);
  if (pid < 0) {
    init_print("init: rfork failed\n");
  } else if (pid == 0) {
    init_print("init: child running... execing wasm_test\n");
    char *args[] = {"wasm_test", 0};
    /* Use direct device path to bypass potential namespace issues */
    int ret = sys_exec("#/./boot/wasm_test", args);
    init_print("init: exec returned (FAILED)\n");
    sys_exit("exec failed");
  } else {
    init_print("init: parent created child\n");
    for (;;)
      ;
  }
}
