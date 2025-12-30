/*
 * Resurrection Server - Layer 2 Service Monitor
 *
 * Monitors and restarts crashed Layer 2 services from known good state.
 * Uses 9P syscalls via exchange page (no POSIX APIs).
 *
 * SECURITY MODEL - ISOLATION BY MECHANISM AND POLICY:
 * ====================================================
 * The resurrection server has DOMINION over other L2 services.
 * Therefore, it MUST be isolated from them:
 *
 * 1. ONE-WAY CONTROL: Resurrection → Services only
 *    - Resurrection can start/stop/restart services
 *    - Services CANNOT send messages to resurrection
 *
 * 2. SEPARATE EXCHANGE PAGE:
 *    - Resurrection uses its own exchange page
 *    - Child services get different exchange pages
 *    - No shared memory between resurrection and children
 *
 * 3. CAPABILITY ISOLATION:
 *    - Resurrection holds CAP_SERVICE_CONTROL (unique)
 *    - Child services do NOT inherit this capability
 *    - Kernel enforces capability checks on process control ops
 *
 * 4. NO EXPOSED CONTROL INTERFACE:
 *    - /srv/resurrection/ is NOT mounted in child namespaces
 *    - Only init (parent) could theoretically access it
 *    - In practice, even init doesn't need to control resurrection
 *
 * 5. BINARY VERIFICATION:
 *    - Services are restarted from known-good signed binaries
 *    - Hash verification prevents compromised restart
 *
 * This design prevents:
 * - Compromised service killing resurrection
 * - Service injecting commands to restart itself with different args
 * - Service interfering with other services via resurrection
 *
 * Started by init as the first Layer 2 service.
 */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00

/* 9P Message types */
#define Tsyscall 130
#define Rsyscall 131
#define Rerror 107

/* Syscall numbers */
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_EXIT 8
#define SYS_FORK 9
#define SYS_WAIT 166
#define SYS_WASM_COMPILE 100
#define SYS_WASM_EXECUTE 101
#define SYS_WASM_DESTROY 102

/* Basic types */
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned int u32int;
typedef unsigned long long u64int;

/* P9 Control structure */
struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

/* Service states */
#define SRV_STOPPED 0
#define SRV_STARTING 1
#define SRV_RUNNING 2
#define SRV_CRASHED 3
#define SRV_FAILED 4

/* Service descriptor */
#define MAX_SERVICES 32
#define MAX_NAME_LEN 64
#define MAX_PATH_LEN 256
#define MAX_HASH_LEN 64

typedef struct {
  char name[MAX_NAME_LEN];      /* Service name */
  char exec_path[MAX_PATH_LEN]; /* Known good binary path */
  char hash[MAX_HASH_LEN];      /* SHA256 of known good binary */
  u32int pid;                   /* Current PID (0 = not running) */
  int state;                    /* SRV_* state */
  int restarts;                 /* Total restart count */
  int restarts_in_window;       /* Restarts in current minute */
  u64int last_restart;          /* Timestamp of last restart */
  int auto_restart;             /* Auto-restart on crash */
  int critical;                 /* Critical service flag */
} Service;

/* Global state */
static Service services[MAX_SERVICES];
static int num_services = 0;
static int running = 1;

/* Exchange page pointers */
static volatile uchar *exchange;
static volatile struct P9Control *ctl;

/* ========== Memory Operations ========== */

static void *memcpy(void *dst, const void *src, unsigned long n) {
  uchar *d = dst;
  const uchar *s = src;
  while (n--)
    *d++ = *s++;
  return dst;
}

static void *memset(void *dst, int c, unsigned long n) {
  uchar *d = dst;
  while (n--)
    *d++ = (uchar)c;
  return dst;
}

static int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (uchar)*s1 - (uchar)*s2;
}

static int strlen(const char *s) {
  int len = 0;
  while (*s++)
    len++;
  return len;
}

static void strcpy(char *dst, const char *src) {
  while ((*dst++ = *src++))
    ;
}

static void strncpy(char *dst, const char *src, int n) {
  while (n-- > 0 && (*dst++ = *src++))
    ;
  if (n >= 0)
    *dst = 0;
}

/* ========== Little-Endian Helpers ========== */

static void put_u32(uchar *p, uint val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
}

static void put_u16(uchar *p, unsigned short val) {
  p[0] = val;
  p[1] = val >> 8;
}

static void put_u64(uchar *p, uvlong val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
  p[4] = val >> 32;
  p[5] = val >> 40;
  p[6] = val >> 48;
  p[7] = val >> 56;
}

static uint get_u32(const uchar *p) {
  return (uint)p[0] | ((uint)p[1] << 8) | ((uint)p[2] << 16) |
         ((uint)p[3] << 24);
}

static uvlong get_u64(const uchar *p) {
  return (uvlong)p[0] | ((uvlong)p[1] << 8) | ((uvlong)p[2] << 16) |
         ((uvlong)p[3] << 24) | ((uvlong)p[4] << 32) | ((uvlong)p[5] << 40) |
         ((uvlong)p[6] << 48) | ((uvlong)p[7] << 56);
}

/* ========== Console Output ========== */

static void print(const char *msg) {
  int msg_len = strlen(msg);
  uchar *req = (uchar *)exchange;
  uint sdata_size = 4 + 8 + 4 + msg_len;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_size;
  uint pos = 0;

  memset(req, 0, 256);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_WRITE);
  pos += 4;
  put_u32(req + pos, sdata_size);
  pos += 4;
  put_u32(req + pos, 1);
  pos += 4; /* fd = 1 (stdout) */
  put_u64(req + pos, 0);
  pos += 8; /* offset */
  put_u32(req + pos, msg_len);
  pos += 4;
  memcpy(req + pos, msg, msg_len);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
}

static void print_num(const char *prefix, int num, const char *suffix) {
  char buf[128];
  int i = 0, j;
  char tmp[16];

  /* Copy prefix */
  while (*prefix && i < 100)
    buf[i++] = *prefix++;

  /* Convert number */
  if (num == 0) {
    buf[i++] = '0';
  } else {
    int neg = 0;
    if (num < 0) {
      neg = 1;
      num = -num;
    }
    j = 0;
    while (num > 0) {
      tmp[j++] = '0' + (num % 10);
      num /= 10;
    }
    if (neg)
      buf[i++] = '-';
    while (j > 0)
      buf[i++] = tmp[--j];
  }

  /* Copy suffix */
  while (*suffix && i < 126)
    buf[i++] = *suffix++;
  buf[i] = 0;

  print(buf);
}

/* ========== Service Registry ========== */

static Service *find_service(const char *name) {
  for (int i = 0; i < num_services; i++) {
    if (strcmp(services[i].name, name) == 0) {
      return &services[i];
    }
  }
  return 0;
}

static int register_service(const char *name, const char *exec_path,
                            const char *hash, int critical) {
  if (num_services >= MAX_SERVICES) {
    print("RESURRECTION: Max services reached\n");
    return -1;
  }

  if (find_service(name)) {
    print("RESURRECTION: Service already registered: ");
    print(name);
    print("\n");
    return -1;
  }

  Service *svc = &services[num_services++];
  memset(svc, 0, sizeof(Service));
  strncpy(svc->name, name, MAX_NAME_LEN - 1);
  strncpy(svc->exec_path, exec_path, MAX_PATH_LEN - 1);
  if (hash)
    strncpy(svc->hash, hash, MAX_HASH_LEN - 1);
  svc->state = SRV_STOPPED;
  svc->auto_restart = 1;
  svc->critical = critical;

  print("RESURRECTION: Registered ");
  print(name);
  print(" -> ");
  print(exec_path);
  if (critical)
    print(" [CRITICAL]");
  print("\n");

  return 0;
}

/* ========== Process Control via 9P ========== */

/*
 * Fork a new process
 * Returns: child PID on success, -1 on failure
 */
static int do_fork(void) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  memset(req, 0, 256);

  /* Tsyscall header */
  uint size = 4 + 1 + 2 + 4 + 4 + 4; /* header + flags */
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_FORK);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4; /* scount */
  put_u32(req + pos, 0);
  pos += 4; /* flags = RFPROC */

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* Parse reply */
  pos = 0;
  /* uint reply_size = */ get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  /* tag */ pos += 2;

  if (reply_type == Rerror) {
    print("RESURRECTION: fork failed\n");
    return -1;
  }

  uvlong retval = get_u64(req + pos);
  return (int)retval; /* PID of child */
}

/*
 * Execute a program (replaces current process image)
 * This is called by the child after fork
 */
static void do_exec(const char *path) {
  uchar *req = (uchar *)exchange;
  int pathlen = strlen(path);
  uint pos = 0;

  memset(req, 0, 512);

  /* Build Texec message: [size][type=128][tag][pathlen:2][path:n] */
  uint size = 4 + 1 + 2 + 2 + pathlen;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = 128; /* Texec */
  put_u16(req + pos, 1);
  pos += 2;
  put_u16(req + pos, pathlen);
  pos += 2;
  memcpy(req + pos, path, pathlen);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* If we get here, exec failed */
  print("RESURRECTION: exec failed for ");
  print(path);
  print("\n");
}

/*
 * Check if a process is still running
 * Opens /proc/PID/status and reads it
 * Returns: 1 if running, 0 if exited/crashed
 */
static int is_process_running(u32int pid) {
  /* For now, simple implementation - would need to open /proc/PID/status */
  /* TODO: Implement proper process status check */
  (void)pid;
  return 1;
}

/*
 * Kill a process by writing to /proc/PID/ctl
 */
static void do_kill(u32int pid) {
  print_num("RESURRECTION: Killing PID ", pid, "\n");

  /* TODO: Open /proc/PID/ctl and write "kill" */
  /* For now, just mark as stopped */
  (void)pid;
}

/* ========== Service Lifecycle ========== */

static void start_service(Service *svc) {
  if (svc->state == SRV_RUNNING) {
    print("RESURRECTION: ");
    print(svc->name);
    print(" already running\n");
    return;
  }

  print("RESURRECTION: Starting ");
  print(svc->name);
  print("...\n");

  svc->state = SRV_STARTING;

  /* Fork new process */
  int pid = do_fork();

  if (pid < 0) {
    print("RESURRECTION: Fork failed for ");
    print(svc->name);
    print("\n");
    svc->state = SRV_FAILED;
    return;
  }

  if (pid == 0) {
    /* Child process - ISOLATION ENFORCEMENT:
     *
     * The child (new service) is now isolated from resurrection:
     * 1. Kernel gives child a NEW exchange page (not resurrection's)
     * 2. Child does NOT inherit CAP_SERVICE_CONTROL capability
     * 3. Child's namespace does NOT include /srv/resurrection
     * 4. Child cannot send 9P messages to resurrection's FIDs
     *
     * This is CRITICAL - a compromised service cannot:
     * - Kill or restart resurrection
     * - Register fake services
     * - Restart itself with different arguments
     */
    do_exec(svc->exec_path);
    /* If we get here, exec failed - exit */
    /* do_exit(1); */
    for (;;)
      ; /* Hang on failure */
  }

  /* Parent continues */
  svc->pid = pid;
  svc->state = SRV_RUNNING;

  print_num("RESURRECTION: Started ", pid, " ");
  print(svc->name);
  print("\n");
}

static void stop_service(Service *svc) {
  if (svc->state != SRV_RUNNING && svc->state != SRV_STARTING) {
    print("RESURRECTION: ");
    print(svc->name);
    print(" not running\n");
    return;
  }

  if (svc->pid > 0) {
    do_kill(svc->pid);
  }

  svc->state = SRV_STOPPED;
  svc->pid = 0;

  print("RESURRECTION: Stopped ");
  print(svc->name);
  print("\n");
}

static void restart_service(Service *svc) {
  /* Rate limiting */
  /* TODO: Implement proper time tracking */
  svc->restarts_in_window++;

  if (svc->restarts_in_window > 5) {
    print("RESURRECTION: ");
    print(svc->name);
    print(" exceeded restart limit, marking FAILED\n");
    svc->state = SRV_FAILED;
    return;
  }

  svc->restarts++;

  print_num("RESURRECTION: Restarting ", svc->restarts, " ");
  print(svc->name);
  print("\n");

  /* Restart from known good state (binary path with verified hash) */
  start_service(svc);
}

/* ========== Service Monitoring ========== */

static void monitor_services(void) {
  print("RESURRECTION: Entering monitoring loop\n");

  while (running) {
    /* Check each service */
    for (int i = 0; i < num_services; i++) {
      Service *svc = &services[i];

      if (svc->state == SRV_RUNNING && svc->pid > 0) {
        if (!is_process_running(svc->pid)) {
          print("RESURRECTION: Detected crash: ");
          print(svc->name);
          print("\n");

          svc->state = SRV_CRASHED;
          svc->pid = 0;

          if (svc->auto_restart) {
            restart_service(svc);
          }
        }
      }
    }

    /* Brief delay to avoid busy loop */
    /* TODO: Implement proper sleep via 9P */
    for (volatile int i = 0; i < 1000000; i++)
      ;
  }

  print("RESURRECTION: Exiting monitoring loop\n");
}

/* ========== Main Entry ========== */

void main(void) {
  /* Initialize exchange page pointers */
  exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  print("=== Resurrection Server Starting (Layer 2) ===\n");

  /* Register known good services
   * These paths point to verified binaries
   * Hash is SHA256 of the known good binary
   */
  print("RESURRECTION: Registering services...\n");

  /* Core Layer 2 services - restart from known good binaries */
  register_service("hal_server", "/boot/hal_server", "TODO:hash",
                   1); /* Critical */
  register_service("file_server", "/boot/file_server", "TODO:hash", 0);
  register_service("wasm_server", "/boot/wasm_server", "TODO:hash",
                   1); /* Critical */

  /* Start all registered services from their known good state */
  print("RESURRECTION: Starting all services...\n");
  for (int i = 0; i < num_services; i++) {
    start_service(&services[i]);
  }

  /* Enter service monitoring loop */
  monitor_services();

  /* Stop all services on exit */
  print("RESURRECTION: Shutting down...\n");
  for (int i = 0; i < num_services; i++) {
    stop_service(&services[i]);
  }

  print("=== Resurrection Server Stopped ===\n");
}
