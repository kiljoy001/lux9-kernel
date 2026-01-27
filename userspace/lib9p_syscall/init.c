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
#define MREPL 0
#define MAFTER 1
#define MBEFORE 2
#define MCREATE 4

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

static int read_backing_spec(char *out, int outsz) {
  int fd = sys_open("/cfg/sophia.device", OREAD);
  if (fd < 0)
    return 0;
  long n = sys_read(fd, out, outsz - 1);
  sys_close(fd);
  if (n <= 0)
    return 0;
  int len = (int)n;
  while (len > 0 &&
         (out[len - 1] == '\n' || out[len - 1] == '\r' || out[len - 1] == ' ' ||
          out[len - 1] == '\t')) {
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
  init_print("init: I AM NEW! Build ID: 3\n");

  /* Create /tmp directory */
  init_print("init: creating /tmp directory...\n");
  int tmp_fd = sys_create("/tmp", OREAD, DMDIR | 0777);
  if (tmp_fd < 0) {
    init_print("init: /tmp creation failed (may already exist)\n");
  } else {
    sys_close(tmp_fd);
  }

  /* Create /srv directory */
  init_print("init: creating /srv directory...\n");
  int srv_fd = sys_create("/srv", OREAD, DMDIR | 0777);
  if (srv_fd < 0) {
    init_print("init: /srv creation failed (may already exist)\n");
  } else {
    sys_close(srv_fd);
  }

  /* Start HAL server and mount /srv/ahci0 */
  int hal_pfd[2];
  if (sys_pipe(hal_pfd) < 0) {
    init_print("init: hal sys_pipe failed\n");
  } else {
    init_print_int("init: hal fd0=", hal_pfd[0]);
    init_print_int("init: hal fd1=", hal_pfd[1]);
    int n = 0;
    int fd = hal_pfd[1];
    if (fd == 0) {
      hal_fd_arg[n++] = '0';
    } else {
      char temp[16];
      int t = 0;
      while (fd > 0) {
        temp[t++] = '0' + (fd % 10);
        fd /= 10;
      }
      while (t > 0)
        hal_fd_arg[n++] = temp[--t];
    }
    hal_fd_arg[n] = '\0';
    init_print("init: hal fd arg=");
    init_print(hal_fd_arg);
    init_print("\n");
    init_print_int("init: hal fd arg0=", (int)(unsigned char)hal_fd_arg[0]);
    init_print_int("init: hal fd arg1=", (int)(unsigned char)hal_fd_arg[1]);
    hal_child_stack_top =
        ((ulong)(hal_child_stack + sizeof(hal_child_stack))) & ~0xFUL;

    int pid = sys_rfork(RFPROC | RFFDG | RFMEM);
    if (pid < 0) {
      init_print("init: hal rfork failed\n");
    } else if (pid == 0) {
      asm volatile("mov %0, %%rsp\n\t"
                   "xor %%rbp, %%rbp\n\t"
                   :
                   : "r"(hal_child_stack_top)
                   : "memory");
      sys_exec("#/./boot/hal", hal_args);
      for (;;)
        ;
    } else {
      init_print("init: mounting hal on /srv/ahci0...\n");
      for (volatile int i = 0; i < 10000000; i++)
        ;
      if (sys_mount(hal_pfd[0], -1, "/srv/ahci0", MREPL | MCREATE, "") < 0) {
        init_print("init: hal mount failed\n");
      } else {
        init_print("init: hal mount success\n");
      }
    }
  }

  /* Create Sophia persistence files (prefer root, fallback to /tmp) */
  int shared_backing = 0;
  int disk_fd = -1;
  int journal_fd = -1;
  char backing_spec[128];
  char disk_path[128];

  int spec_len = read_backing_spec(backing_spec, sizeof(backing_spec));
  if (spec_len > 0) {
    int di = 0;
    for (int i = 0; i < spec_len && di < (int)sizeof(disk_path) - 1; i++)
      disk_path[di++] = backing_spec[i];
    disk_path[di] = '\0';
    trim_path(disk_path);

    init_print("init: opening ");
    init_print(disk_path);
    init_print("...\n");
    disk_fd = open_device(disk_path);
    if (disk_fd >= 0) {
      journal_fd = open_device(disk_path);
      if (journal_fd >= 0) {
        shared_backing = 1;
        init_print("init: using single backing device for Sophia\n");
      }
    }
    if (disk_fd < 0 || journal_fd < 0) {
      if (disk_fd >= 0)
        sys_close(disk_fd);
      if (journal_fd >= 0)
        sys_close(journal_fd);
      disk_fd = -1;
      journal_fd = -1;
      shared_backing = 0;
    }
  }

  if (disk_fd < 0) {
    init_print("init: opening /dev/sd0...\n");
    disk_fd = open_device("/dev/sd0");
    if (disk_fd >= 0) {
      journal_fd = open_device("/dev/sd0");
      if (journal_fd >= 0) {
        shared_backing = 1;
        init_print("init: using /dev/sd0 for Sophia backing\n");
      } else {
        sys_close(disk_fd);
        disk_fd = -1;
      }
    }
  }

  init_print("init: opening /sophia.disk...\n");
  if (disk_fd < 0)
    disk_fd = open_or_create("/sophia.disk");
  if (disk_fd < 0) {
    init_print("init: WARNING: /sophia.disk unavailable, trying /tmp\n");
    disk_fd = open_or_create("/tmp/sophia.disk");
  }

  init_print("init: opening /sophia.journal...\n");
  if (journal_fd < 0)
    journal_fd = open_or_create("/sophia.journal");
  if (journal_fd < 0) {
    init_print("init: WARNING: /sophia.journal unavailable, trying /tmp\n");
    journal_fd = open_or_create("/tmp/sophia.journal");
  }

  if (disk_fd < 0 || journal_fd < 0) {
    if (disk_fd >= 0)
      sys_close(disk_fd);
    if (journal_fd >= 0)
      sys_close(journal_fd);

    init_print("init: WARNING: using #r/ram for Sophia backing store\n");
    disk_fd = sys_open("#r/ram", ORDWR);
    journal_fd = sys_open("#r/ram", ORDWR);
    if (disk_fd >= 0 && journal_fd >= 0)
      shared_backing = 1;
  }

  if (disk_fd < 0 || journal_fd < 0) {
    init_print("init: ERROR: failed to open storage fds\n");
  } else {
    init_print("init: disk_fd = ");
    print_int(disk_fd);
    init_print("\n");
    init_print("init: journal_fd = ");
    print_int(journal_fd);
    init_print("\n");
  }
  if (shared_backing) {
    init_print("init: shared backing = 1\n");
  } else {
    init_print("init: shared backing = 0\n");
  }

  /* Create a communication pipe for 9P */
  int pfd[2];
  if (sys_pipe(pfd) < 0) {
    init_print("init: sys_pipe failed\n");
    sys_exit("pipe failed");
  }

  /* Build fd arg before rfork to avoid stack sharing issues */
  int n = 0;
  int fd = pfd[1]; /* Child uses write end */
  if (fd == 0) {
    sophia_fd_arg[n++] = '0';
  } else {
    char temp[16];
    int t = 0;
    while (fd > 0) {
      temp[t++] = '0' + (fd % 10);
      fd /= 10;
    }
    while (t > 0)
      sophia_fd_arg[n++] = temp[--t];
  }
  sophia_fd_arg[n++] = ',';
  sophia_fd_arg[n++] = shared_backing ? '1' : '0';
  sophia_fd_arg[n] = '\0';

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
     * exchange page which corrupts the parent. sys_exec is safe because it
     * replaces the entire address space.
     */
    int ret = sys_exec("#/./boot/sophia", sophia_args);
    /* If we get here, exec failed */
    (void)ret;
    for (;;)
      ;
  } else {
    /* PARENT: WAIT AND MOUNT */
    init_print("init: parent waiting for sophia...\n");

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
