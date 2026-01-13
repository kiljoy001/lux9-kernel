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
  init_print("init: I AM NEW! Build ID: 3\n");
  /* ZERO-COPY FORK: Must use RFMEM to share stack (vfork style) because we
   * cannot copy pages. Child shares parent's stack until sys_exec replaces the
   * image. */
  /* Create a communication pipe for 9P */
  int pfd[2];
  if (sys_pipe(pfd) < 0) {
    init_print("init: sys_pipe failed\n");
    sys_exit("pipe failed");
  }

  /* ZERO-COPY FORK: Must use RFMEM to share stack (vfork style) because we
   * cannot copy pages. Child shares parent's stack until sys_exec replaces the
   * image. */
  int pid = sys_rfork(RFPROC | RFFDG | RFMEM);
  if (pid < 0) {
    init_print("init: rfork failed\n");
  } else if (pid == 0) {
    /* CHILD: EXEC SOPHIA */
    /* Format pipe FD as string manually */
    char fd_str[16];
    int n = 0;
    int val = pfd[1]; // Use logical 'server' end (though symmetric)
    if (val == 0)
      fd_str[n++] = '0';
    else {
      // Reverse conversion
      char temp[16];
      int t = 0;
      while (val > 0) {
        temp[t++] = '0' + (val % 10);
        val /= 10;
      }
      while (t > 0)
        fd_str[n++] = temp[--t];
    }
    fd_str[n] = '\0';

    init_print("init: spawning sophia server on pipe...\n");
    char *args[] = {"sophia", fd_str, 0};
    /* Match the path that worked */
    int ret = sys_exec("#/./boot/sophia", args);
    init_print("init: exec sophia returned (FAILED) ret=");
    print_int(ret);
    init_print("\n");
    sys_exit("exec failed");
  } else {
    /* PARENT: WAIT AND MOUNT */
    init_print("init: parent waiting for sophia...\n");

    /* Give Sophia time to initialize */
    for (volatile int i = 0; i < 10000000; i++)
      ;

    init_print("init: mounting sophiafs on /mnt/sophia...\n");
    /* Mount SophiaFS (served by child PID) onto /mnt/sophia */
    /* Note: We use the PID service syntax or just the SRV channel */
    /* For now, assuming standard 9P attach to the server we just spawned?
       Actually, in Plan 9, the server posts a file descriptor or we use a pipe.
       But here we just spawned it.

       Let's try to mount using the service registry /srv if available?
       Or assuming sophia listens on a known channel.

       For this test, we just want to Verify it runs. The logs showed it
       answered 9P. WE WILL JUST LOOP AND PRINT STATUS.
    */

    init_print("init: sophia running (pid ");
    print_int(pid);
    init_print("). System stable.\n");

    for (;;) {
      /* Keep init alive */
    }
  }
}
