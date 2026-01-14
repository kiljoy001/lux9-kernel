/* Init - Process 1
 * Starts the resurrection server and registers Layer 2 services
 * Uses liblux for all syscalls
 */

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
extern int sys_rfork(int flags);
extern int sys_exec(char *path, char *argv[]);
extern char *sys_exec_error(void);
extern void sys_exit(char *msg);
extern int sys_wait(void);
extern int sys_pid(void);
extern int sys_pipe(int *fds);
extern int sys_mount(int fd, int afd, char *old, int flags, char *aname);
extern int sys_stat(char *path, Dir *d); // Use Dir*

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define RFFDG (1 << 2)
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define DMDIR 0x80000000

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

int main(void) {
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
    /* CHILD: EXEC SOPHIA
     * CRITICAL: Do NOT call any syscalls before exec in RFMEM fork!
     * The child shares memory with parent, and syscalls overwrite the
     * shared exchange page, causing corruption and null pointer dereferences.
     */

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

    init_print("init: CHILD calling sys_exec...\n");
    char *args[] = {"sophia", fd_str, 0};
    int ret = sys_exec("#/./boot/sophia", args);
    /* If we get here, exec failed */
    init_print("init: exec FAILED ret=");
    print_int(ret);
    init_print("\n");
    sys_exit("exec failed");
  } else {
    /* PARENT: WAIT AND MOUNT */
    init_print("init: parent waiting for sophia...\n");

#define MREPL 0
#define MAFTER 1
#define MBEFORE 2
#define MCREATE 4

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define OEXEC 3

#define DMDIR 0x80000000

    /* Give Sophia time to initialize */
    for (volatile int i = 0; i < 10000000; i++)
      ;

    init_print("init: creating /mnt/sophia...\n");
    if (sys_create("/mnt/sophia", OREAD, DMDIR | 0777) < 0) {
      init_print("init: mkdir /mnt/sophia failed (may exist)\n");
    }

    init_print("init: mounting sophiafs on /mnt/sophia...\n");

    /* Mount using the pipe FD (pfd[0]) */
    /* We assume the child (sophia) is now listening on the other end (pfd[1])
     */
    /* and treating it as its root server connection. */

    if (sys_mount(pfd[0], -1, "/mnt/sophia", MREPL, "") < 0) {
      init_print("init: mount failed\n");
    } else {
      init_print("init: mount success!\n");
      // Optional: List directory/test access
    }

    init_print("init: sophia running (pid ");
    print_int(pid);
    init_print("). System stable.\n");

    for (;;) {
      /* Keep init alive */
    }
  }
}
