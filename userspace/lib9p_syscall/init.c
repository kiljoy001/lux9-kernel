/* Init - Process 1
 * Starts the resurrection server and registers Layer 2 services
 * Uses liblux for all syscalls
 */

#include <stdarg.h>

/* Minimal syscall declarations - avoid full lux.h to prevent header conflicts
 */
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;

typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;

typedef struct Qid {
  u64int path;
  u32int vers;
  u8int type;
} Qid;

typedef struct Dir {
  u16int type;
  u32int dev;
  Qid qid;
  u32int mode;
  u32int atime;
  u32int mtime;
  u64int length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
} Dir;

extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern int sys_create(char *path, int mode, uint perm);
extern int sys_spawn(char *path, char *argv[]);
extern void sys_exit(char *msg);
extern int sys_wait(void);
extern int sys_sleep(long ms);
extern int sys_pid(void);
extern int sys_pipe(int *fds);
extern int sys_mount(int fd, int afd, char *old, int flags, char *aname);
extern int sys_stat(char *path, Dir *d); // Use Dir*
extern int vsnprint(char *buf, int len, const char *fmt, va_list args);

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define RFFDG (1 << 2)
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define DMDIR 0x80000000
#define MREPL 0
#define MAFTER 1
#define MBEFORE 2
#define MCREATE 4

/* Minimal print implementation for liblux convM2S debug output */
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

static char sophia_fd_arg[32];
static char *sophia_args[] = {"sophia", sophia_fd_arg, 0};
static char hal_fd_arg[32];
static char *hal_args[] = {"hal", hal_fd_arg, 0};
static u8int hal_child_stack[4096];
static ulong hal_child_stack_top;

static int open_or_create(const char *path) {
  int fd = sys_open((char *)path, ORDWR);
  if (fd < 0)
    fd = sys_create((char *)path, ORDWR, 0666);
  return fd;
}

static int open_device(const char *path) {
  return sys_open((char *)path, ORDWR);
}

static int boot_file_exists(const char *path) {
  int fd = sys_open((char *)path, OREAD);
  if (fd < 0)
    return 0;
  sys_close(fd);
  return 1;
}

static void maybe_spawn_wasm_smoke_test(void) {
  static char *smoke_args[] = {"exec_wasm", 0};
  int pid;

  if (!boot_file_exists("#/./boot/exec_wasm"))
    return;
  if (!boot_file_exists("#/./boot/wasm_smoke.wasm"))
    return;

  init_print("init: spawning wasm smoke test...\n");
  pid = sys_spawn("#/./boot/exec_wasm", smoke_args);
  if (pid < 0) {
    init_print("init: wasm smoke spawn failed\n");
    return;
  }

  init_print("init: wasm smoke test running (pid ");
  print_int(pid);
  init_print(")\n");
}

static int read_backing_spec(char *out, int outsz) {
  int fd = sys_open("/cfg/sophia.device", OREAD);
  if (fd < 0)
    return 0;
  long n = sys_read(fd, out, outsz - 1);
  sys_close(fd);
  if (n <= 0)
    return 0;
  int len = (int)n;
  while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r' ||
                     out[len - 1] == ' ' || out[len - 1] == '\t')) {
    len--;
  }
  int start = 0;
  while (start < len && (out[start] == ' ' || out[start] == '\t'))
    start++;
  int dst = 0;
  for (int i = start; i < len; i++)
    out[dst++] = out[i];
  out[dst] = '\0';
  return dst;
}

static void trim_path(char *s) {
  int len = 0;
  while (s[len])
    len++;
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t'))
    len--;
  s[len] = '\0';
  int start = 0;
  while (s[start] == ' ' || s[start] == '\t')
    start++;
  if (start == 0)
    return;
  int dst = 0;
  while (s[start])
    s[dst++] = s[start++];
  s[dst] = '\0';
}

static void init_print_int(const char *label, int v) {
  char buf[64];
  int n = 0;
  while (label[n]) {
    buf[n] = label[n];
    n++;
  }
  if (v == 0) {
    buf[n++] = '0';
  } else {
    char tmp[16];
    int t = 0;
    int x = v;
    if (x < 0) {
      buf[n++] = '-';
      x = -x;
    }
    while (x > 0 && t < (int)sizeof(tmp)) {
      tmp[t++] = '0' + (x % 10);
      x /= 10;
    }
    while (t > 0)
      buf[n++] = tmp[--t];
  }
  buf[n++] = '\n';
  sys_write(2, buf, n);
}

int main(void) {
  init_print("=== Lux9 Init Starting ===\n");
  init_print("init: I AM NEW! Build ID: 4 (Resurrection Enabled)\n");

  /* Create /tmp directory */
  init_print("init: creating /tmp directory...\n");
  int tmp_fd = sys_open("/tmp", OREAD);
  if (tmp_fd < 0)
    tmp_fd = sys_create("/tmp", OREAD, DMDIR | 0777);
  if (tmp_fd < 0) {
    init_print("init: /tmp creation failed (may already exist)\n");
  } else {
    sys_close(tmp_fd);
  }

  /* /srv is router-managed and becomes available once resurrection starts. */
  init_print("init: deferring /srv setup until resurrection is running...\n");

  /* Start Resurrection Server */
  for (;;) {
    init_print("init: spawning resurrection server...\n");

    static char *res_args[] = {"resurrection", 0};

    int pid = sys_spawn("#/./boot/resurrection", res_args);
    if (pid < 0) {
      init_print("init: resurrection spawn failed! Retrying...\n");
      for (volatile int i = 0; i < 1000000; i++)
        ;
      continue;
    }

    /* Parent */
    init_print("init: resurrection server running (pid ");
    print_int(pid);
    init_print(")\n");
    maybe_spawn_wasm_smoke_test();

    /*
     * Resurrection owns service supervision. Keep init alive without
     * exercising the current sys_wait()/sleep path, which is still unstable.
     */
    for (;;)
      sys_sleep(60000);
  }
}
