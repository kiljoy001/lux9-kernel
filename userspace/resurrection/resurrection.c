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

#include <lux.h>
#include "../lib/liblux/src/lux_bootstrap.h"
#include <stdarg.h>

int print(const char *fmt, ...);

/* Mount flags */
#define MREPL 0x0000
#define MBEFORE 0x0001
#define MAFTER 0x0002
#define MCREATE 0x0004
#define DMDIR 0x80000000
#define QTDIR 0x80
#define QTFILE 0x00
#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define DMDIR 0x80000000

/* Rfork flags */
#define RFFDG (1 << 2)
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define RFNOWAIT (1 << 6)

/* Syscall declarations from liblux */
extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern void sys_exit(char *msg);
extern int sys_create(char *path, int mode, uint perm);
extern int sys_exec(char *path, char *argv[]);

/* Service states */
#define SRV_STOPPED 0
#define SRV_STARTING 1
#define SRV_RUNNING 2
#define SRV_CRASHED 3
#define SRV_FAILED 4
#define SRV_HUNG 5

/* Service descriptor */
#define MAX_SERVICES 32
#define MAX_NAME_LEN 64
#define MAX_PATH_LEN 256
#define MAX_HASH_LEN 64
#define CAP_HASH_SIZE 32
#define CAP_SIG_SIZE 64
#define CAP_NONCE_SIZE 16
#define EPOCH_HOUR 0
#define EPOCH_GRACE 1
#define RESTART_WINDOW_NS (60ULL * 1000000000ULL) // 60 seconds in nanoseconds

typedef struct {
  char name[MAX_NAME_LEN];          /* Service name */
  char exec_path[MAX_PATH_LEN];     /* Known good binary path */
  char endpoint[MAX_PATH_LEN];      /* Published 9P endpoint path */
  uchar service_hash[MAX_HASH_LEN]; /* SHA256 of known good binary */
  uchar cap_service_hash[CAP_HASH_SIZE]; /* H(service name) for token binding */
  u32int pid;                       /* Current PID (0 = not running) */
  u32int last_pid;                  /* Previous PID (for distress correlation) */
  uuid_t pid2;                      /* Lux9 Secure PID2 (UUID) */
  int state;                        /* SRV_* state */
  int restarts;                     /* Total restart count */
  int restarts_in_window;           /* Restarts in current minute */
  u64int last_restart;              /* Timestamp of last restart */
  int auto_restart;                 /* Auto-restart on crash */
  int critical;                     /* Critical service flag */
  int system_service;               /* Hardcoded system service */
  int uses_pipe;                    /* Create pipe and pass fd arg */
  uuid_t owner_pid2;                /* Process identity that registered service */
  int owner_pid2_valid;             /* owner_pid2 contains a real requester */
  int caps_revoked;                 /* Service tokens temporarily blocked */
  u64int last_heartbeat;            /* Timestamp of last health check */
  u64int heartbeat_timeout_ns;      /* 0 = disabled, otherwise required period */
  int missed_heartbeats;            /* Count of consecutive failures */
  Qid qid;                          /* Unique file ID for 9P */
} Service;

/* ========== Wave 7 Distress Signal Handling ========== */

/*
 * Distress Event Structure - must match kernel/include/pebble.h DistressEvent
 * Size: 24 bytes
 */
typedef struct DistressEvent {
  u64int timestamp;        /* nsec() at signal time */
  u32int pid;              /* Process ID */
  unsigned short reason;   /* DISTRESS_* code */
  unsigned short severity; /* DISTRESS_SEV_* level */
  u64int context;          /* Reason-specific data */
} DistressEvent;

/* Thread Stacks for RFMEM Safety */
static uchar srv_stack[16384] __attribute__((aligned(16)));
static uchar distress_stack[4096] __attribute__((aligned(16)));
static uchar watchdog_stack[4096] __attribute__((aligned(16)));

/* Synchronization for RFMEM stack switching */
static volatile int child_ready = 0;

static void make_exchange_path(char *buf, int buflen, int chan_id,
                               const char *suffix) {
  snprint(buf, buflen, "#X/%d/%s", chan_id, suffix);
}

/* Shims for thread entry points */
static void do_sleep_ms(int ms);
static void srv_loop(int fd) __attribute__((unused));
static void srv_loop_ring_raw(void);

static void srv_loop_shim(void *arg) {
  (void)arg;
  child_ready = 1;
  print("RESURRECTION: CHECKPOINT: /srv child stack switch complete\n");
  srv_loop_ring_raw();
  sys_exit("srv ring loop exited");
}

static void distress_loop(void); /* Forward decl */

static void distress_loop_shim(void *arg) {
  child_ready = 1;
  print("RESURRECTION: CHECKPOINT: Distress child stack switch complete\n");
  (void)arg;
  distress_loop();
  sys_exit("distress loop exited");
}

static void watchdog_loop(void); /* Forward decl */

static void watchdog_loop_shim(void *arg) {
  child_ready = 1;
  print("RESURRECTION: CHECKPOINT: Watchdog child stack switch complete\n");
  (void)arg;
  watchdog_loop();
  sys_exit("watchdog loop exited");
}

static void monitor_services(void);

/* Distress Reason Codes - must match kernel */
#define DISTRESS_MEMORY_CORRUPT 1   /* Memory corruption detected */
#define DISTRESS_CAP_VIOLATION 2    /* Capability/permission violation */
#define DISTRESS_PANIC_IMMINENT 3   /* Process about to panic */
#define DISTRESS_RESOURCE_EXHAUST 4 /* Resource exhaustion (OOM, FD limit) */
#define DISTRESS_USER_ABORT 5       /* User-initiated abort */
#define DISTRESS_BORROW_FAULT 6     /* BorrowChecker ownership violation */
#define DISTRESS_VAULT_BREACH 7     /* Vault integrity compromised */
#define DISTRESS_MANUAL_LOCKDOWN 100

/* Distress Severity Levels */
#define DISTRESS_SEV_INFO 0
#define DISTRESS_SEV_WARN 1
#define DISTRESS_SEV_ERROR 2
#define DISTRESS_SEV_CRITICAL 3

/* Forward declarations for distress handler */
static void handle_distress(DistressEvent *ev);
static void distress_loop(void);
static void stop_service(Service *svc);
static Service *find_service_by_pid(u32int pid);
static void uuid_to_str(const uuid_t *u, char *out);
static void srv_uuid_clear(uuid_t *u);
static int srv_request_matches_pid2(const uuid_t *pid2, int pid2_valid);
static int get_proc_pid2(int pid, uuid_t *out);

/* Forward Declarations */
/* Print function - also used by liblux for debugging */
int print(const char *fmt, ...);
static void print_num(const char *prefix, int num, const char *suffix);
int srv_create_entry(const char *name, int pid);
static void start_service(Service *svc);
static void restart_service(Service *svc);
static void monitor_services(void);

/* Locking Primitives */
static int srv_lock = 0;

static void lock(int *l) {
  int x = 1;
  while (x) {
    /* Simple spinlock using xchg */
    __asm__ volatile("xchgl %0, %1" : "+r"(x), "+m"(*l));
  }
}

static void unlock(int *l) { *l = 0; }

/* Global state */
static Service services[MAX_SERVICES];
static int num_services = 0;
static int running = 1;
static int srv_fd = -1;
static char srv_ctl_path[64];
static char srv_ring_path[64];
static int system_lockdown = 0;
static int system_lockdown_reason = 0;
static u32int system_lockdown_pid = 0;
static u64int system_lockdown_since = 0;
static uuid_t srv_current_pid2;
static int srv_current_pid2_valid = 0;
static uuid_t srv_admin_pid2;
static int srv_admin_pid2_valid = 0;

#define MAX_REVOKED_TOKENS 128
typedef struct RevokedToken {
  int active;
  uchar fingerprint[CAP_HASH_SIZE];
} RevokedToken;

static RevokedToken revoked_tokens[MAX_REVOKED_TOKENS];

/* Exchange page pointers */

/* ========== /srv 9P Server State ========== */

/*
 * Use shared 9P library from kernel/include/fcall.h
 * For freestanding build, we include the kernel headers directly
 */

/* Capability flags */
#define CAP_READ (1 << 0)
#define CAP_WRITE (1 << 1)
#define CAP_EXEC (1 << 2)
#define CAP_ADMIN (1 << 3)
#define CAP_DELEGATE (1 << 4)

/* CapToken structure (matches kernel/include/capability.h) */
typedef struct CapToken {
  uchar service_hash[CAP_HASH_SIZE];
  uchar client_commit[CAP_HASH_SIZE];
  u64int epoch;
  u64int flags;
  uchar signature[CAP_SIG_SIZE];
} CapToken;

static int service_slot_active(const Service *s) { return s != 0 && s->name[0] != 0; }

static void service_slot_clear(Service *s) {
  if (s != 0)
    memset(s, 0, sizeof(*s));
}

typedef struct CapBlindRequest {
  uchar blinded_data[CAP_HASH_SIZE + CAP_HASH_SIZE + 16];
  uchar blinding_factor[32];
} CapBlindRequest;

typedef struct CapBlindResponse {
  uchar blind_signature[CAP_SIG_SIZE];
} CapBlindResponse;

enum {
  CAPREQ_ISSUE = 1,
  CAPREQ_REFRESH = 2,
};

#define CAP_SERVICE_NAME_LEN 64

typedef struct CapIssueRequest {
  u32int op;
  char service[CAP_SERVICE_NAME_LEN];
  uchar client_id[CAP_HASH_SIZE];
  u64int flags;
} CapIssueRequest;

typedef struct CapRefreshRequest {
  u32int op;
  CapToken token;
  uchar client_id[CAP_HASH_SIZE];
} CapRefreshRequest;

/* 9P Message types (from include/fcall.h) */
enum {
  P9_Tversion = 100,
  P9_Rversion,
  P9_Tauth = 102,
  P9_Rauth,
  P9_Tattach = 104,
  P9_Rattach,
  P9_Terror = 106,
  P9_Rerror,
  P9_Tflush = 108,
  P9_Rflush,
  P9_Twalk = 110,
  P9_Rwalk,
  P9_Topen = 112,
  P9_Ropen,
  P9_Tcreate = 114,
  P9_Rcreate,
  P9_Tread = 116,
  P9_Rread,
  P9_Twrite = 118,
  P9_Rwrite,
  P9_Tclunk = 120,
  P9_Rclunk,
  P9_Tremove = 122,
  P9_Rremove,
  P9_Tstat = 124,
  P9_Rstat,
  P9_Twstat = 126,
  P9_Rwstat
};

#define QTDIR 0x80
#define QTFILE 0x00

/* Resurrection signing key (Ed25519) */
static uchar resurrection_privkey[64];
static uchar resurrection_pubkey[32];
static int keys_initialized = 0;

/* FID tracking for 9P server */
#define MAX_FIDS 128
#define FID_FREE 0
#define FID_ROOT 1
#define FID_AUTH 2
#define FID_SERVICE_DIR 3
#define FID_CTL 4
#define FID_STATUS 5
#define FID_DATA 6
#define FID_ENDPOINT 7
#define FID_RESCTL 8
#define FID_ROOTCTL 9

typedef struct SrvFid {
  u32int fid;
  int type;          /* FID_FREE, FID_ROOT, FID_ENTRY, FID_AUTH */
  int srv_idx;       /* Index into services[] if FID_ENTRY */
  int authenticated; /* Has presented valid token */
  int created_ctl;   /* Freshly created service ctl fid */
  u64int cap_flags;  /* Permissions granted by token */
  u32int client_pid; /* Owning process */
  uuid_t client_pid2;
  int client_pid2_valid;
  Qid qid;           /* Current qid */
  uchar auth_reply[sizeof(CapToken)];
  u32int auth_reply_len;
} SrvFid;

static SrvFid srv_fids[MAX_FIDS];

/* Current epoch for capability verification */
#define CAP_EPOCH_HOUR_NS (3600ULL * 1000000000ULL)
static u64int current_epoch = 0;

/* Forward declarations for helper functions used by 9P handlers */
static void put_u32(uchar *p, uint val);
static void put_u16(uchar *p, unsigned short val);
static void put_u64(uchar *p, uvlong val);
static unsigned short get_u16(const uchar *p);
static uint get_u32(const uchar *p);
static uvlong get_u64(const uchar *p);

/* Type definitions for crypto */
typedef uchar u8int;

/* Forward declarations for crypto functions */
extern int cap_epoch_valid(u64int e, u64int current, u64int grace);
extern int cap_token_init(CapToken *tok, const char *service,
                          const u8int *client_id, u64int flags);
extern int cap_token_verify(const CapToken *tok, const u8int *service_hash,
                            const u8int *pubkey, u64int current_epoch);
extern int cap_blind_sign(CapBlindResponse *resp, const CapBlindRequest *req,
                          const u8int *privkey);
extern u64int cap_current_epoch(int type);
extern void cap_hash_service(u8int *out, const char *service_name);
extern void crypto_blake2b(u8int *hash, usize hash_size, const u8int *message,
                           usize message_size);
extern void crypto_eddsa_sign(u8int signature[64], const u8int secret_key[64],
                              const u8int *message, usize message_size);
extern void crypto_eddsa_key_pair(u8int *sk, u8int *pk, u8int *seed);
extern void crypto_blake2b_init(void *ctx, unsigned long long outlen);
extern void crypto_blake2b_update(void *ctx, const u8int *in,
                                  unsigned long long inlen);
extern void crypto_blake2b_final(void *ctx, u8int *out);

/* Monocypher context type - 256 bytes should be sufficient for blake2b context
 */
typedef struct {
  uchar data[256];
} crypto_blake2b_ctx;

/* Forward declarations for internal helper functions */
static int do_open(const char *path, int mode);
static int do_read(int fd, char *buf, int count);
static void do_close(int fd);
static int do_write(int fd, const void *buf, int count);

/* Syscall for time (needed by blind_cap.c) */
#define SYS_NSEC 53

#ifndef UNIT_TEST
long long nsec(void) {
  return (long long)sys_nsec();
}
#endif

static u32int rng_state = 0xDEADBEEF;
static int rng_secure_available = -1; /* -1 = unknown, 0 = no, 1 = yes */

/*
 * Cryptographically secure random bytes from /dev/random.
 * Falls back to insecure xorshift only if /dev/random is unavailable.
 */
void randombytes(void *buf, unsigned long long len) {
  u8int *b = buf;

  /* Try /dev/random first (backed by RDRAND or ChaCha20 CSPRNG) */
  if (rng_secure_available != 0) {
    int fd = do_open("/dev/random", 0 /* OREAD */);
    if (fd >= 0) {
      rng_secure_available = 1;
      long n = do_read(fd, (char *)buf, (int)len);
      do_close(fd);
      if (n == (long)len)
        return; /* Success - got all bytes from secure source */
    } else {
      if (rng_secure_available == -1) {
        rng_secure_available = 0;
        print("RESURRECTION: WARNING - /dev/random unavailable, using insecure "
              "fallback!\n");
      }
    }
  }

  /* Fallback: INSECURE xorshift32 seeded from nsec() */
  if (rng_state == 0xDEADBEEF) {
    rng_state = (u32int)nsec();
    if (rng_state == 0)
      rng_state = 0x12345678;
  }
  for (unsigned long long i = 0; i < len; i++) {
    u32int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    b[i] = (u8int)x;
  }
}

static void refresh_cap_epoch(void) { current_epoch = cap_current_epoch(EPOCH_HOUR); }

static void ensure_resurrection_keys(void) {
  uchar seed[32];

  if (!keys_initialized) {
    randombytes(seed, sizeof(seed));
    crypto_eddsa_key_pair(resurrection_privkey, resurrection_pubkey, seed);
    memset(seed, 0, sizeof(seed));
    keys_initialized = 1;
  }
  refresh_cap_epoch();
}

/*
 * Sleep for a given number of milliseconds using nsec() timing.
 * This is much better than busy-waiting as it allows polling with timing.
 */
static void do_sleep_ms(int ms) {
  if (ms > 0 && sys_sleep(ms) == 0)
    return;

  long long target = nsec() + ((long long)ms * 1000000LL);
  while (nsec() < target) {
    /* Yield by doing a minimal syscall check - could be improved with
       a proper yield syscall if available */
    volatile int i;
    for (i = 0; i < 1000; i++)
      ; /* Small busy wait to avoid hammering nsec() */
  }
}

/* ========== /srv FID Management ========== */

static SrvFid *srv_alloc_fid(u32int fid) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (srv_fids[i].type == FID_FREE) {
      memset(&srv_fids[i], 0, sizeof(srv_fids[i]));
      srv_fids[i].fid = fid;
      srv_fids[i].authenticated = 0;
      srv_fids[i].created_ctl = 0;
      if (srv_current_pid2_valid) {
        uuid_copy(&srv_fids[i].client_pid2, &srv_current_pid2);
        srv_fids[i].client_pid2_valid = 1;
      } else {
        srv_uuid_clear(&srv_fids[i].client_pid2);
        srv_fids[i].client_pid2_valid = 0;
      }
      return &srv_fids[i];
    }
  }
  return 0;
}

static SrvFid *srv_lookup_fid(u32int fid) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (srv_fids[i].type != FID_FREE && srv_fids[i].fid == fid &&
        ((srv_fids[i].client_pid2_valid == 0 && srv_current_pid2_valid == 0) ||
         srv_request_matches_pid2(&srv_fids[i].client_pid2,
                                  srv_fids[i].client_pid2_valid)))
      return &srv_fids[i];
  }
  return 0;
}

static void srv_free_fid(SrvFid *f) {
  if (f) {
    f->type = FID_FREE;
    f->fid = 0;
    f->authenticated = 0;
    f->created_ctl = 0;
    f->client_pid2_valid = 0;
    srv_uuid_clear(&f->client_pid2);
  }
}

/* ========== /srv Entry Management ========== */

static int srv_find_entry(const char *name) {
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i]))
      continue;
    if (strcmp(services[i].name, name) == 0)
      return i;
  }
  return -1;
}

static int srv_endpoint_service_name(const char *name, char *out) {
  int len;

  len = strlen(name);
  if (len <= 9 || strcmp(name + len - 9, ".endpoint") != 0)
    return -1;
  if (len - 9 >= MAX_NAME_LEN)
    return -1;
  memcpy(out, name, len - 9);
  out[len - 9] = 0;
  return 0;
}

static void srv_endpoint_name(const Service *s, char *out, int outlen) {
  snprint(out, outlen, "%s.endpoint", s->name);
}

static void srv_set_endpoint(Service *s, const char *endpoint, int len) {
  if (len >= MAX_PATH_LEN)
    len = MAX_PATH_LEN - 1;
  memcpy(s->endpoint, endpoint, len);
  s->endpoint[len] = 0;
  while (len > 0 &&
         (s->endpoint[len - 1] == '\n' || s->endpoint[len - 1] == '\r' ||
          s->endpoint[len - 1] == ' ' || s->endpoint[len - 1] == '\t')) {
    s->endpoint[--len] = 0;
  }
}

static void srv_trim_trailing_slots(void) {
  while (num_services > 0 && !service_slot_active(&services[num_services - 1]))
    num_services--;
}

static void srv_free_service_fids(int srv_idx) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (srv_fids[i].type == FID_FREE)
      continue;
    if (srv_fids[i].srv_idx == srv_idx)
      srv_free_fid(&srv_fids[i]);
  }
}

static const char *srv_state_name(int state) {
  switch (state) {
  case SRV_STOPPED:
    return "STOPPED";
  case SRV_STARTING:
    return "STARTING";
  case SRV_RUNNING:
    return "RUNNING";
  case SRV_CRASHED:
    return "CRASHED";
  case SRV_FAILED:
    return "FAILED";
  case SRV_HUNG:
    return "HUNG";
  default:
    return "UNKNOWN";
  }
}

static const char *srv_lockdown_reason_name(int reason) {
  switch (reason) {
  case 0:
    return "none";
  case DISTRESS_MEMORY_CORRUPT:
    return "memory_corrupt";
  case DISTRESS_CAP_VIOLATION:
    return "cap_violation";
  case DISTRESS_PANIC_IMMINENT:
    return "panic_imminent";
  case DISTRESS_RESOURCE_EXHAUST:
    return "resource_exhaust";
  case DISTRESS_USER_ABORT:
    return "user_abort";
  case DISTRESS_BORROW_FAULT:
    return "borrow_fault";
  case DISTRESS_VAULT_BREACH:
    return "vault_breach";
  case DISTRESS_MANUAL_LOCKDOWN:
    return "manual";
  default:
    return "unknown";
  }
}

static void srv_uuid_clear(uuid_t *u) {
  memset(u->data, 0, sizeof(u->data));
}

static int srv_uuid_is_null(const uuid_t *u) {
  for (int i = 0; i < 16; i++) {
    if (u->data[i] != 0)
      return 0;
  }
  return 1;
}

static int srv_uuid_equal(const uuid_t *a, const uuid_t *b) {
  return memcmp(a->data, b->data, 16) == 0;
}

static void srv_set_current_pid2(const uuid_t *pid2) {
  if (pid2 == nil || srv_uuid_is_null(pid2)) {
    srv_uuid_clear(&srv_current_pid2);
    srv_current_pid2_valid = 0;
    return;
  }
  uuid_copy(&srv_current_pid2, pid2);
  srv_current_pid2_valid = 1;
}

static void srv_clear_current_pid2(void) {
  srv_uuid_clear(&srv_current_pid2);
  srv_current_pid2_valid = 0;
}

static int srv_request_matches_pid2(const uuid_t *pid2, int pid2_valid) {
  if (!pid2_valid || !srv_current_pid2_valid)
    return 0;
  return srv_uuid_equal(&srv_current_pid2, pid2);
}

static int srv_request_is_bootstrap_admin(void) {
  return srv_request_matches_pid2(&srv_admin_pid2, srv_admin_pid2_valid);
}

static int srv_request_is_owner(const Service *s) {
  return srv_request_matches_pid2(&s->owner_pid2, s->owner_pid2_valid);
}

static int srv_request_is_service_instance(const Service *s) {
  if (s == 0)
    return 0;
  if (s->state != SRV_RUNNING && s->state != SRV_STARTING)
    return 0;
  return srv_request_matches_pid2(&s->pid2, !srv_uuid_is_null(&s->pid2));
}

static void srv_bind_service_owner(Service *s) {
  if (!srv_current_pid2_valid)
    return;
  uuid_copy(&s->owner_pid2, &srv_current_pid2);
  s->owner_pid2_valid = 1;
}

static void srv_capture_bootstrap_admin(void) {
  char buf[16];
  int fd;
  long n;
  int ppid;

  fd = sys_open("#c/ppid", OREAD);
  if (fd < 0)
    return;
  n = sys_read(fd, buf, sizeof(buf) - 1);
  sys_close(fd);
  if (n <= 0)
    return;
  buf[n] = 0;
  ppid = atoi(buf);
  if (ppid <= 0)
    return;
  if (get_proc_pid2(ppid, &srv_admin_pid2) == 0)
    srv_admin_pid2_valid = 1;
}

static void srv_refresh_cap_service_hash(Service *s) {
  if (s == 0 || s->name[0] == 0) {
    if (s != 0)
      memset(s->cap_service_hash, 0, sizeof(s->cap_service_hash));
    return;
  }
  cap_hash_service(s->cap_service_hash, s->name);
}

static void srv_token_fingerprint(const CapToken *tok, uchar out[CAP_HASH_SIZE]) {
  crypto_blake2b(out, CAP_HASH_SIZE, (const u8int *)tok, sizeof(*tok));
}

static int srv_token_is_revoked(const CapToken *tok) {
  uchar fp[CAP_HASH_SIZE];

  srv_token_fingerprint(tok, fp);
  for (int i = 0; i < MAX_REVOKED_TOKENS; i++) {
    if (!revoked_tokens[i].active)
      continue;
    if (memcmp(revoked_tokens[i].fingerprint, fp, sizeof(fp)) == 0)
      return 1;
  }
  return 0;
}

static int srv_revoke_token_fingerprint(const uchar fp[CAP_HASH_SIZE]) {
  int free_idx = -1;

  for (int i = 0; i < MAX_REVOKED_TOKENS; i++) {
    if (!revoked_tokens[i].active) {
      if (free_idx < 0)
        free_idx = i;
      continue;
    }
    if (memcmp(revoked_tokens[i].fingerprint, fp, CAP_HASH_SIZE) == 0)
      return 0;
  }
  if (free_idx < 0)
    return -1;
  revoked_tokens[free_idx].active = 1;
  memcpy(revoked_tokens[free_idx].fingerprint, fp, CAP_HASH_SIZE);
  return 0;
}

static void srv_clear_token_revocations(void) {
  memset(revoked_tokens, 0, sizeof(revoked_tokens));
}

static int srv_count_revoked_tokens(void) {
  int n = 0;
  for (int i = 0; i < MAX_REVOKED_TOKENS; i++) {
    if (revoked_tokens[i].active)
      n++;
  }
  return n;
}

static void srv_set_service_caps_revoked(Service *s, int revoked) {
  if (s != 0)
    s->caps_revoked = revoked ? 1 : 0;
}

static int srv_count_revoked_services(void) {
  int n = 0;
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i]))
      continue;
    if (services[i].caps_revoked)
      n++;
  }
  return n;
}

static void srv_set_all_caps_revoked(int revoked) {
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i]))
      continue;
    services[i].caps_revoked = revoked ? 1 : 0;
  }
}

static int srv_verify_service_token(Service *s, const CapToken *tok,
                                    u64int *flags_out) {
  if (s == 0 || tok == 0)
    return -1;
  if (s->caps_revoked)
    return -1;
  if (srv_token_is_revoked(tok))
    return -1;
  if (!cap_epoch_valid(tok->epoch, current_epoch, 2))
    return -1;
  if (cap_token_verify(tok, s->cap_service_hash, resurrection_pubkey,
                       current_epoch) != 0)
    return -1;
  if (flags_out)
    *flags_out = tok->flags;
  return 0;
}

static int srv_verify_root_admin_token(const CapToken *tok, u64int *flags_out) {
  uchar resurrection_hash[CAP_HASH_SIZE];

  if (tok == 0)
    return -1;
  if (srv_token_is_revoked(tok))
    return -1;
  cap_hash_service(resurrection_hash, "resurrection");
  if (!cap_epoch_valid(tok->epoch, current_epoch, 2))
    return -1;
  if (cap_token_verify(tok, resurrection_hash, resurrection_pubkey,
                       current_epoch) != 0)
    return -1;
  if ((tok->flags & CAP_ADMIN) == 0)
    return -1;
  if (flags_out)
    *flags_out = tok->flags;
  return 0;
}

static int srv_copy_trimmed(char *dst, int dstlen, const uchar *src, u32int count) {
  int n;

  if (dstlen <= 0)
    return 0;
  n = (count >= (u32int)dstlen) ? dstlen - 1 : (int)count;
  if (n > 0)
    memcpy(dst, src, n);
  dst[n] = 0;
  while (n > 0 &&
         (dst[n - 1] == '\n' || dst[n - 1] == '\r' || dst[n - 1] == ' ' ||
          dst[n - 1] == '\t')) {
    dst[--n] = 0;
  }
  while (*dst == ' ' || *dst == '\t') {
    memmove(dst, dst + 1, n);
    if (n > 0)
      n--;
  }
  return n;
}

static int srv_parse_bool(const char *value, int *out) {
  if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
      strcmp(value, "on") == 0 || strcmp(value, "yes") == 0) {
    *out = 1;
    return 0;
  }
  if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
      strcmp(value, "off") == 0 || strcmp(value, "no") == 0) {
    *out = 0;
    return 0;
  }
  return -1;
}

static int srv_parse_heartbeat_timeout(const char *value, u64int *out_ns) {
  int ms;

  if (strcmp(value, "off") == 0 || strcmp(value, "0") == 0) {
    *out_ns = 0;
    return 0;
  }
  ms = atoi(value);
  if (ms <= 0)
    return -1;
  *out_ns = (u64int)ms * 1000000ULL;
  return 0;
}

static int srv_hex_value(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static int srv_parse_hex_bytes(const char *hex, uchar *out, int outlen) {
  for (int i = 0; i < outlen; i++) {
    int hi = srv_hex_value(hex[i * 2]);
    int lo = srv_hex_value(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0)
      return -1;
    out[i] = (uchar)((hi << 4) | lo);
  }
  return 0;
}

static int srv_has_prefix(const char *s, const char *prefix) {
  while (*prefix) {
    if (*s++ != *prefix++)
      return 0;
  }
  return 1;
}

static int srv_parse_lockdown_reason(const char *value) {
  if (value == 0 || *value == 0 || strcmp(value, "manual") == 0)
    return DISTRESS_MANUAL_LOCKDOWN;
  if (strcmp(value, "memory_corrupt") == 0)
    return DISTRESS_MEMORY_CORRUPT;
  if (strcmp(value, "cap_violation") == 0)
    return DISTRESS_CAP_VIOLATION;
  if (strcmp(value, "panic_imminent") == 0)
    return DISTRESS_PANIC_IMMINENT;
  if (strcmp(value, "resource_exhaust") == 0)
    return DISTRESS_RESOURCE_EXHAUST;
  if (strcmp(value, "user_abort") == 0)
    return DISTRESS_USER_ABORT;
  if (strcmp(value, "borrow_fault") == 0)
    return DISTRESS_BORROW_FAULT;
  if (strcmp(value, "vault_breach") == 0)
    return DISTRESS_VAULT_BREACH;
  return -1;
}

static void srv_copy_issue_service_name(char *dst, int dstlen,
                                        const char src[CAP_SERVICE_NAME_LEN]) {
  int n;

  if (dstlen <= 0)
    return;
  n = CAP_SERVICE_NAME_LEN;
  if (n >= dstlen)
    n = dstlen - 1;
  memcpy(dst, src, n);
  dst[n] = 0;
  while (n > 0 && dst[n - 1] == 0)
    n--;
  dst[n] = 0;
}

static void srv_fill_client_id(const uchar requested[CAP_HASH_SIZE],
                               uchar out[CAP_HASH_SIZE]) {
  if (srv_current_pid2_valid) {
    crypto_blake2b(out, CAP_HASH_SIZE, srv_current_pid2.data,
                   sizeof(srv_current_pid2.data));
    return;
  }
  memcpy(out, requested, CAP_HASH_SIZE);
}

static void srv_sign_token(CapToken *tok) {
  uchar message[80];

  memcpy(message, tok->service_hash, CAP_HASH_SIZE);
  memcpy(message + 32, tok->client_commit, CAP_HASH_SIZE);
  memcpy(message + 64, &tok->epoch, 8);
  memcpy(message + 72, &tok->flags, 8);
  crypto_eddsa_sign(tok->signature, resurrection_privkey, message, sizeof(message));
}

static int srv_service_name_from_hash(const uchar hash[CAP_HASH_SIZE],
                                      char *name, int namelen, Service **service_out) {
  uchar resurrection_hash[CAP_HASH_SIZE];

  if (name == 0 || namelen <= 0)
    return -1;

  cap_hash_service(resurrection_hash, "resurrection");
  if (memcmp(hash, resurrection_hash, CAP_HASH_SIZE) == 0) {
    strncpy(name, "resurrection", namelen - 1);
    name[namelen - 1] = 0;
    if (service_out)
      *service_out = 0;
    return 0;
  }

  for (int i = 0; i < num_services; i++) {
    Service *s = &services[i];
    if (!service_slot_active(s))
      continue;
    if (memcmp(hash, s->cap_service_hash, CAP_HASH_SIZE) != 0)
      continue;
    strncpy(name, s->name, namelen - 1);
    name[namelen - 1] = 0;
    if (service_out)
      *service_out = s;
    return 0;
  }

  return -1;
}

static u64int srv_allowed_issue_flags(const char *service_name, const Service *s) {
  if (srv_request_is_bootstrap_admin())
    return CAP_READ | CAP_WRITE | CAP_EXEC | CAP_ADMIN | CAP_DELEGATE;

  if (service_name != 0 && strcmp(service_name, "resurrection") == 0)
    return 0;

  if (s != 0) {
    if (srv_request_is_owner(s))
      return CAP_READ | CAP_WRITE | CAP_EXEC | CAP_ADMIN | CAP_DELEGATE;
    if (srv_request_is_service_instance(s))
      return CAP_READ | CAP_WRITE | CAP_EXEC;
  }

  return CAP_READ | CAP_WRITE;
}

static int srv_issue_token_for_service(const char *service_name,
                                       const uchar requested_client_id[CAP_HASH_SIZE],
                                       u64int flags, CapToken *out) {
  Service *s;
  uchar client_id[CAP_HASH_SIZE];
  u64int allowed;

  if (service_name == 0 || *service_name == 0 || out == 0)
    return -1;

  if (strcmp(service_name, "resurrection") == 0) {
    s = 0;
  } else {
    int idx = srv_find_entry(service_name);
    if (idx < 0)
      return -1;
    s = &services[idx];
    if (s->caps_revoked && !srv_request_is_bootstrap_admin())
      return -1;
  }

  allowed = srv_allowed_issue_flags(service_name, s);
  if (allowed == 0)
    return -1;
  if ((flags & ~allowed) != 0)
    return -1;

  srv_fill_client_id(requested_client_id, client_id);
  if (cap_token_init(out, service_name, client_id, flags) != 0)
    return -1;
  srv_sign_token(out);
  return 0;
}

static int srv_refresh_token_from_request(const CapRefreshRequest *req, CapToken *out) {
  char service_name[CAP_SERVICE_NAME_LEN + 1];
  Service *s;

  if (req == 0 || out == 0)
    return -1;

  if (srv_service_name_from_hash(req->token.service_hash, service_name,
                                 sizeof(service_name), &s) < 0)
    return -1;

  if (strcmp(service_name, "resurrection") == 0) {
    if (!srv_request_is_bootstrap_admin() &&
        srv_verify_root_admin_token(&req->token, 0) != 0)
      return -1;
  } else {
    if (s == 0)
      return -1;
    if (s->caps_revoked && !srv_request_is_bootstrap_admin())
      return -1;
    if (!srv_request_is_bootstrap_admin() &&
        srv_verify_service_token(s, &req->token, 0) != 0)
      return -1;
  }

  return srv_issue_token_for_service(service_name, req->client_id,
                                     req->token.flags, out);
}

static int srv_valid_service_name(const char *name) {
  int len;
  char svc_name[MAX_NAME_LEN];

  len = strlen(name);
  if (len <= 0 || len >= MAX_NAME_LEN)
    return 0;
  if (strcmp(name, ".resurrection") == 0 || strcmp(name, "ctl") == 0 ||
      strcmp(name, "status") == 0 || strcmp(name, "data") == 0)
    return 0;
  if (srv_endpoint_service_name(name, svc_name) == 0)
    return 0;
  for (int i = 0; i < len; i++) {
    if (name[i] == '/' || name[i] == '\n' || name[i] == '\r' ||
        name[i] == '\t')
      return 0;
  }
  return 1;
}

static int srv_rename_service(Service *s, const char *newname) {
  int idx;

  if (strcmp(s->name, newname) == 0)
    return 0;
  if (s->system_service)
    return -1;
  if (s->state == SRV_RUNNING || s->state == SRV_STARTING)
    return -2;
  if (!srv_valid_service_name(newname))
    return -3;
  idx = (int)(s - services);
  for (int i = 0; i < num_services; i++) {
    if (i == idx || !service_slot_active(&services[i]))
      continue;
    if (strcmp(services[i].name, newname) == 0)
      return -4;
  }
  strncpy(s->name, newname, MAX_NAME_LEN - 1);
  s->name[MAX_NAME_LEN - 1] = 0;
  srv_refresh_cap_service_hash(s);
  return 0;
}

static void srv_touch_heartbeat(Service *s) {
  s->last_heartbeat = nsec();
  s->missed_heartbeats = 0;
}

static int srv_start_blocked(const Service *s) {
  return system_lockdown && !s->critical;
}

static int srv_format_root_ctl(char *buf, int len) {
  int pos = 0;
  char admin_pid2_str[33] = "00000000000000000000000000000000";

  ensure_resurrection_keys();
  if (srv_admin_pid2_valid)
    uuid_to_str(&srv_admin_pid2, admin_pid2_str);
  pos += snprint(buf + pos, len - pos,
                 "epoch=%llu\n"
                 "bootstrap_admin_pid2=%s\n"
                 "lockdown=%d\n"
                 "lockdown_reason=%s\n"
                 "lockdown_pid=%d\n"
                 "lockdown_since_nsec=%llu\n"
                 "revoked_tokens=%d\n"
                 "revoked_services=%d\n",
                 (unsigned long long)current_epoch, admin_pid2_str, system_lockdown,
                 srv_lockdown_reason_name(system_lockdown_reason),
                 system_lockdown_pid,
                 (unsigned long long)system_lockdown_since,
                 srv_count_revoked_tokens(), srv_count_revoked_services());
  for (int i = 0; i < num_services && pos < len; i++) {
    Service *s = &services[i];
    if (!service_slot_active(s) || !s->caps_revoked)
      continue;
    pos += snprint(buf + pos, len - pos, "revoked_service=%s\n", s->name);
  }
  return pos;
}

static int srv_format_ctl(Service *s, char *buf, int len) {
  char owner_pid2_str[33];

  if (s->owner_pid2_valid)
    uuid_to_str(&s->owner_pid2, owner_pid2_str);
  else
    strcpy(owner_pid2_str, "00000000000000000000000000000000");
  return snprint(buf, len,
                 "name=%s\n"
                 "state=%s\n"
                 "exec=%s\n"
                 "endpoint=%s\n"
                 "owner_pid2=%s\n"
                 "pid=%d\n"
                 "last_pid=%d\n"
                 "auto_restart=%d\n"
                 "critical=%d\n"
                 "system_service=%d\n"
                 "caps_revoked=%d\n"
                 "uses_pipe=%d\n"
                 "heartbeat_timeout_ms=%llu\n"
                 "last_heartbeat_nsec=%llu\n"
                 "missed_heartbeats=%d\n"
                 "lockdown=%d\n"
                 "lockdown_reason=%s\n"
                 "lockdown_pid=%d\n"
                 "lockdown_since_nsec=%llu\n",
                 s->name, srv_state_name(s->state), s->exec_path, s->endpoint,
                 owner_pid2_str, s->pid, s->last_pid, s->auto_restart, s->critical,
                 s->system_service, s->caps_revoked, s->uses_pipe,
                 (unsigned long long)(s->heartbeat_timeout_ns / 1000000ULL),
                 (unsigned long long)s->last_heartbeat, s->missed_heartbeats,
                 system_lockdown, srv_lockdown_reason_name(system_lockdown_reason),
                 system_lockdown_pid, (unsigned long long)system_lockdown_since);
}

static int srv_wstat_name(const uchar *stat, u32int nstat, char *name, int namelen) {
  const uchar *p;
  const uchar *ep;
  u16int total;
  u16int n;

  if (nstat < 2 + 2 + 4 + 13 + 4 + 4 + 4 + 8 + 2 + 2 + 2 + 2)
    return -1;
  p = stat;
  ep = stat + nstat;
  total = get_u16(p);
  if (nstat != (u32int)(2 + total))
    return -1;
  p += 2 + 2 + 4 + 13 + 4 + 4 + 4 + 8;
  if (p + 2 > ep)
    return -1;
  n = get_u16(p);
  p += 2;
  if (p + n > ep)
    return -1;
  if (namelen <= 0)
    return -1;
  if (n >= (u16int)namelen)
    n = namelen - 1;
  if (n > 0)
    memcpy(name, p, n);
  name[n] = 0;
  return n;
}

static void srv_enter_lockdown(int reason, u32int pid) {
  if (!system_lockdown) {
    system_lockdown = 1;
    system_lockdown_reason = reason;
    system_lockdown_pid = pid;
    system_lockdown_since = nsec();
    print("RESURRECTION: SYSTEM LOCKDOWN reason=%s pid=%d\n",
          srv_lockdown_reason_name(reason), pid);
  }
  for (int i = 0; i < num_services; i++) {
    Service *s = &services[i];
    if (!service_slot_active(s) || s->critical)
      continue;
    if (s->state == SRV_RUNNING || s->state == SRV_STARTING)
      stop_service(s);
  }
}

/* ========== 9P Message Building ========== */

/* ========== /srv Entry Management ========== */

static u32int srv_build_error(uchar *buf, unsigned short tag, const char *err) {
  uint pos = 0;
  int errlen = strlen(err);
  u32int size = 4 + 1 + 2 + 2 + errlen;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rerror;
  put_u16(buf + pos, tag);
  pos += 2;
  put_u16(buf + pos, errlen);
  pos += 2;
  for (int i = 0; i < errlen; i++)
    buf[pos++] = err[i];

  return size;
}

static u32int srv_build_rattach(uchar *buf, unsigned short tag, Qid *qid) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 13;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rattach;
  put_u16(buf + pos, tag);
  pos += 2;
  buf[pos++] = qid->type;
  put_u32(buf + pos, qid->vers);
  pos += 4;
  put_u64(buf + pos, qid->path);
  pos += 8;

  return size;
}

static u32int srv_build_rwalk(uchar *buf, unsigned short tag, int nwqid,
                              Qid *qids) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 2 + (nwqid * 13);

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rwalk;
  put_u16(buf + pos, tag);
  pos += 2;
  put_u16(buf + pos, nwqid);
  pos += 2;
  for (int i = 0; i < nwqid; i++) {
    buf[pos++] = qids[i].type;
    put_u32(buf + pos, qids[i].vers);
    pos += 4;
    put_u64(buf + pos, qids[i].path);
    pos += 8;
  }

  return size;
}

static u32int srv_build_ropen(uchar *buf, unsigned short tag, Qid *qid,
                              u32int iounit) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 13 + 4;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Ropen;
  put_u16(buf + pos, tag);
  pos += 2;
  buf[pos++] = qid->type;
  put_u32(buf + pos, qid->vers);
  pos += 4;
  put_u64(buf + pos, qid->path);
  pos += 8;
  put_u32(buf + pos, iounit);
  pos += 4;

  return size;
}

static u32int srv_build_rcreate(uchar *buf, unsigned short tag, Qid *qid,
                                u32int iounit) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 13 + 4;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rcreate;
  put_u16(buf + pos, tag);
  pos += 2;
  buf[pos++] = qid->type;
  put_u32(buf + pos, qid->vers);
  pos += 4;
  put_u64(buf + pos, qid->path);
  pos += 8;
  put_u32(buf + pos, iounit);
  pos += 4;

  return size;
}

static u32int srv_build_rversion(uchar *buf, unsigned short tag, u32int msize,
                                 const char *version) {
  uint pos = 0;
  int versionlen = strlen(version);
  u32int size = 4 + 1 + 2 + 4 + 2 + versionlen;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rversion;
  put_u16(buf + pos, tag);
  pos += 2;
  put_u32(buf + pos, msize);
  pos += 4;
  put_u16(buf + pos, versionlen);
  pos += 2;
  for (int i = 0; i < versionlen; i++)
    buf[pos++] = version[i];

  return size;
}

static u32int __attribute__((unused))
srv_build_rauth(uchar *buf, unsigned short tag, Qid *aqid, uchar *sigdata,
                u32int siglen) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 13 + 4 + siglen;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rauth;
  put_u16(buf + pos, tag);
  pos += 2;
  buf[pos++] = aqid->type;
  put_u32(buf + pos, aqid->vers);
  pos += 4;
  put_u64(buf + pos, aqid->path);
  pos += 8;
  /* Extension: include blind signature */
  put_u32(buf + pos, siglen);
  pos += 4;
  for (u32int i = 0; i < siglen; i++)
    buf[pos++] = sigdata[i];

  return size;
}

static u32int srv_build_rwrite(uchar *buf, unsigned short tag, u32int count) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 4;
  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rwrite;
  put_u16(buf + pos, tag);
  pos += 2;
  put_u32(buf + pos, count);
  pos += 4;
  return size;
}

static u32int srv_build_rclunk(uchar *buf, unsigned short tag) {
  uint pos = 0;
  u32int size = 4 + 1 + 2;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rclunk;
  put_u16(buf + pos, tag);
  pos += 2;

  return size;
}

static u32int srv_build_rflush(uchar *buf, unsigned short tag) {
  uint pos = 0;
  u32int size = 4 + 1 + 2;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rflush;
  put_u16(buf + pos, tag);
  pos += 2;

  return size;
}

static u32int srv_build_rremove(uchar *buf, unsigned short tag) {
  uint pos = 0;
  u32int size = 4 + 1 + 2;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rremove;
  put_u16(buf + pos, tag);
  pos += 2;

  return size;
}

static u32int srv_build_rwstat(uchar *buf, unsigned short tag) {
  uint pos = 0;
  u32int size = 4 + 1 + 2;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rwstat;
  put_u16(buf + pos, tag);
  pos += 2;

  return size;
}

static u32int srv_build_rread(uchar *buf, unsigned short tag, uchar *data,
                              u32int count) {
  uint pos = 0;
  u32int size = 4 + 1 + 2 + 4 + count;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rread;
  put_u16(buf + pos, tag);
  pos += 2;
  put_u32(buf + pos, count);
  pos += 4;
  for (u32int i = 0; i < count; i++)
    buf[pos++] = data[i];

  return size;
}

static u32int srv_build_rstat(uchar *buf, unsigned short tag, Qid *qid,
                              const char *name, u64int length, u32int mode) {
  uint pos = 0;
  int namelen = strlen(name);

  /* Build stat structure inline:
   * size[2] type[2] dev[4] qid[13] mode[4] atime[4] mtime[4] length[8]
   * name[s] uid[s] gid[s] muid[s]
   * Where s = 2-byte length + string data
   */
  int stat_size =
      2 + 4 + 13 + 4 + 4 + 4 + 8 + (2 + namelen) + (2 + 0) + (2 + 0) + (2 + 0);
  u32int size = 4 + 1 + 2 + 2 + stat_size;

  put_u32(buf + pos, size);
  pos += 4;
  buf[pos++] = P9_Rstat;
  put_u16(buf + pos, tag);
  pos += 2;

  /* Total stat size (including the 2-byte size field itself) */
  put_u16(buf + pos, stat_size);
  pos += 2;

  /* stat structure */
  put_u16(buf + pos, stat_size - 2); /* size (excludes itself) */
  pos += 2;
  put_u16(buf + pos, 0); /* type */
  pos += 2;
  put_u32(buf + pos, 0); /* dev */
  pos += 4;
  buf[pos++] = qid->type;
  put_u32(buf + pos, qid->vers);
  pos += 4;
  put_u64(buf + pos, qid->path);
  pos += 8;
  put_u32(buf + pos, mode);
  pos += 4;
  put_u32(buf + pos, 0); /* atime */
  pos += 4;
  put_u32(buf + pos, 0); /* mtime */
  pos += 4;
  put_u64(buf + pos, length);
  pos += 8;
  put_u16(buf + pos, namelen);
  pos += 2;
  for (int i = 0; i < namelen; i++)
    buf[pos++] = name[i];
  put_u16(buf + pos, 0); /* uid */
  pos += 2;
  put_u16(buf + pos, 0); /* gid */
  pos += 2;
  put_u16(buf + pos, 0); /* muid */
  pos += 2;

  return size;
}

/* ========== 9P Message Handlers ========== */

static u32int srv_handle_version(uchar *req, uchar *resp) {
  u32int msize;
  unsigned short tag = get_u16(req + 5);

  msize = get_u32(req + 7);
  if (msize == 0 || msize > 8192)
    msize = 8192;
  return srv_build_rversion(resp, tag, msize, "9P2000");
}

static u32int srv_handle_flush(uchar *req, uchar *resp) {
  unsigned short tag = get_u16(req + 5);
  (void)req;
  return srv_build_rflush(resp, tag);
}

/*
 * Handle Tattach: Client attaches to /srv root
 * Returns root Qid (directory)
 */
static u32int srv_handle_attach(uchar *req, uchar *resp) {
  /* Parse Tattach: [size][type][tag][fid][afid][uname][aname] */
  uint pos = 7; /* Skip size, type, tag */
  u32int fid = get_u32(req + pos);
  pos += 4;
  u32int afid = get_u32(req + pos);
  pos += 4;
  (void)afid; /* Auth fid ignored for now */

  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_alloc_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "no fids available");

  /* Set up root fid */
  f->type = FID_ROOT;
  f->qid.type = QTDIR;
  f->qid.vers = 0;
  f->qid.path = 1; /* Root path is 1 */

  return srv_build_rattach(resp, tag, &f->qid);
}

/*
 * Handle Twalk: Navigate to service entry
 */
static u32int srv_handle_walk(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  pos += 4;
  u32int newfid = get_u32(req + pos);
  pos += 4;
  unsigned short nwname = get_u16(req + pos);
  pos += 2;
  unsigned short tag = get_u16(req + 5);

  SrvFid *oldf = srv_lookup_fid(fid);
  if (!oldf)
    return srv_build_error(resp, tag, "unknown fid");

  SrvFid *newf = (fid == newfid) ? oldf : srv_alloc_fid(newfid);
  if (!newf)
    return srv_build_error(resp, tag, "no fids available");

  if (fid != newfid) {
    newf->type = oldf->type;
    newf->qid = oldf->qid;
    newf->srv_idx = oldf->srv_idx;
    newf->created_ctl = 0;
  }

  Qid wqids[16];
  int nwqid = 0;

  for (int i = 0; i < nwname && i < 16; i++) {
    unsigned short namelen = get_u16(req + pos);
    pos += 2;
    char name[MAX_NAME_LEN];
    for (int j = 0; j < namelen && j < MAX_NAME_LEN - 1; j++)
      name[j] = req[pos + j];
    name[namelen < MAX_NAME_LEN ? namelen : MAX_NAME_LEN - 1] = 0;
    pos += namelen;

    if (newf->type == FID_ROOT) {
      /* Walking from root to a service directory */
      char svc_name[MAX_NAME_LEN];
      int idx;

      if (strcmp(name, "ctl") == 0) {
        newf->type = FID_ROOTCTL;
        newf->srv_idx = -1;
        newf->qid.type = QTFILE;
        newf->qid.vers = 0;
        newf->qid.path = 3;
        wqids[nwqid++] = newf->qid;
        continue;
      }

      if (strcmp(name, ".resurrection") == 0) {
        newf->type = FID_RESCTL;
        newf->srv_idx = -1;
        newf->qid.type = QTFILE;
        newf->qid.vers = 0;
        newf->qid.path = 2;
        wqids[nwqid++] = newf->qid;
        continue;
      }

      if (srv_endpoint_service_name(name, svc_name) == 0)
        idx = srv_find_entry(svc_name);
      else
        idx = srv_find_entry(name);
      if (idx < 0)
        break; /* Not found - partial walk */
      newf->srv_idx = idx;
      if (srv_endpoint_service_name(name, svc_name) == 0) {
        newf->type = FID_ENDPOINT;
        newf->qid.type = QTFILE;
        newf->qid.vers = 0;
        newf->qid.path = services[idx].qid.path | 4;
      } else {
        newf->type = FID_SERVICE_DIR;
        newf->qid = services[idx].qid;
      }
      wqids[nwqid++] = newf->qid;
    } else if (newf->type == FID_SERVICE_DIR) {
      /* Walking inside service directory */
      newf->srv_idx = oldf->srv_idx; /* Inherit idx */

      if (strcmp(name, "ctl") == 0) {
        newf->type = FID_CTL;
        newf->qid.type = QTFILE;
        newf->qid.vers = 0;
        newf->qid.path = services[newf->srv_idx].qid.path | 1;
      } else if (strcmp(name, "status") == 0) {
        newf->type = FID_STATUS;
        newf->qid.type = QTFILE;
        newf->qid.vers = 0;
        newf->qid.path = services[newf->srv_idx].qid.path | 2;
      } else if (strcmp(name, "data") == 0) {
        newf->type = FID_DATA;
        newf->qid.type = QTFILE;
        newf->qid.vers = 0;
        newf->qid.path = services[newf->srv_idx].qid.path | 3;
      } else {
        break; /* Unknown file */
      }
      wqids[nwqid++] = newf->qid;
    } else {
      break; /* Can't walk further */
    }
  }

  if (nwqid == 0 && nwname > 0 && fid != newfid)
    srv_free_fid(newf);

  return srv_build_rwalk(resp, tag, nwqid, wqids);
}

/*
 * Handle Tcreate: Register a new service in /srv
 */
static u32int srv_handle_create(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  pos += 4;
  unsigned short namelen = get_u16(req + pos);
  pos += 2;
  char name[MAX_NAME_LEN];
  for (int i = 0; i < namelen && i < MAX_NAME_LEN - 1; i++)
    name[i] = req[pos + i];
  name[namelen < MAX_NAME_LEN ? namelen : MAX_NAME_LEN - 1] = 0;
  pos += namelen;

  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f || f->type != FID_ROOT)
    return srv_build_error(resp, tag, "create in non-directory");

  {
    char svc_name[MAX_NAME_LEN];
    if (srv_endpoint_service_name(name, svc_name) == 0) {
      int idx = srv_find_entry(svc_name);
      if (idx < 0)
        idx = srv_create_entry(svc_name, 0);
      if (idx < 0)
        return srv_build_error(resp, tag, "no space for service");
      if (services[idx].owner_pid2_valid &&
          !srv_request_is_owner(&services[idx]))
        return srv_build_error(resp, tag, "permission denied");
      if (!services[idx].owner_pid2_valid)
        srv_bind_service_owner(&services[idx]);
      f->type = FID_ENDPOINT;
      f->srv_idx = idx;
      f->created_ctl = 0;
      f->qid.type = QTFILE;
      f->qid.vers = 0;
      f->qid.path = services[idx].qid.path | 4;
      return srv_build_rcreate(resp, tag, &f->qid, 4096);
    }
  }

  if (srv_find_entry(name) >= 0)
    return srv_build_error(resp, tag, "service exists");
  int idx = srv_create_entry(
      name, 0 /* Service is created stopped and not yet spawned */);
  if (idx < 0)
    return srv_build_error(resp, tag, "no space for service");

  /* New service created via file system - starts as STOPPED
     waiting for configuration via WRITE */
  Service *s = &services[idx];
  s->state = SRV_STOPPED;
  s->pid = 0;
  srv_bind_service_owner(s);

  print("RESURRECTION: Dynamic Service Registered: ");

  print(name);
  print("\n");

  /*
   * Return the ctl file as the created/open fid so callers can immediately
   * write exec=/cmd= configuration after create("/srv/<name>").
   */
  f->type = FID_CTL;
  f->srv_idx = idx;
  f->created_ctl = 1;
  f->qid.type = QTFILE;
  f->qid.vers = 0;
  f->qid.path = services[idx].qid.path | 1;

  return srv_build_rcreate(resp, tag, &f->qid, 4096);
}

/*
 * Handle Topen: Verify capability token before granting access
 */
static u32int srv_handle_open(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  pos += 4;
  uchar mode = req[pos++];
  (void)mode;

  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "unknown fid");

  if (f->type == FID_ROOT) {
    /* Opening root directory - always allowed */
    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  if (f->type == FID_ROOTCTL) {
    ensure_resurrection_keys();
    f->authenticated = 0;
    f->cap_flags = 0;
    if (srv_request_is_bootstrap_admin()) {
      f->authenticated = 1;
      f->cap_flags = CAP_ADMIN;
    }
    {
      u32int size = get_u32(req);
      if (size >= pos + sizeof(CapToken)) {
        CapToken *tok = (CapToken *)(req + pos);
        if (srv_verify_root_admin_token(tok, &f->cap_flags) == 0)
          f->authenticated = 1;
      }
    }
    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  if (f->type == FID_RESCTL) {
    f->auth_reply_len = 0;
    ensure_resurrection_keys();
    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  if (f->type == FID_SERVICE_DIR) {
    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  if (f->type == FID_ENDPOINT) {
    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  if (f->type == FID_CTL) {
    ensure_resurrection_keys();
    /* Allow open, but mark as unauthenticated.
       Client must write CapToken to authenticate. */
    f->authenticated = 0;
    f->created_ctl = 0;

    /* If extension data present, try to verify immediately */
    u32int size = get_u32(req);
    if (size >= pos + sizeof(CapToken)) {
      CapToken *tok = (CapToken *)(req + pos);
      if (srv_verify_service_token(&services[f->srv_idx], tok, &f->cap_flags) == 0) {
        f->authenticated = 1;
      }
    }

    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  if (f->type == FID_STATUS || f->type == FID_DATA) {
    if (mode != 0 && mode != 0) /* OREAD=0 */
      return srv_build_error(resp, tag, "read-only");
    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  return srv_build_error(resp, tag, "cannot open");
}

/*
 * Handle Tauth: this server does not expose blind-sign auth.
 * Token issuance is done explicitly through /srv/.resurrection writes.
 */
static u32int srv_handle_auth(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int afid = get_u32(req + pos);
  pos += 4;
  /* Skip uname and aname */
  unsigned short unamelen = get_u16(req + pos);
  pos += 2 + unamelen;
  unsigned short anamelen = get_u16(req + pos);
  pos += 2 + anamelen;

  unsigned short tag = get_u16(req + 5);
  (void)afid;
  (void)pos;
  return srv_build_error(resp, tag, "auth disabled");
}

/*
 * Handle Twrite: Write to file (or authenticate)
 */
static u32int srv_handle_write(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  pos += 4;
  u64int offset = get_u64(req + pos);
  pos += 8;
  u32int count = get_u32(req + pos);
  pos += 4;
  /* Data follows */
  uchar *data = req + pos;

  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "unknown fid");

  if (f->type == FID_ROOTCTL) {
    char cmdbuf[MAX_PATH_LEN + 64];
    int cmdlen;

    ensure_resurrection_keys();
    (void)offset;

    if (count == sizeof(CapToken)) {
      CapToken *tok = (CapToken *)data;
      if (srv_verify_root_admin_token(tok, &f->cap_flags) == 0) {
        f->authenticated = 1;
        return srv_build_rwrite(resp, tag, count);
      }
      return srv_build_error(resp, tag, "permission denied");
    }

    cmdlen = srv_copy_trimmed(cmdbuf, sizeof(cmdbuf), data, count);
    if (cmdlen <= 0)
      return srv_build_rwrite(resp, tag, count);
    if (!f->authenticated && srv_request_is_bootstrap_admin()) {
      f->authenticated = 1;
      f->cap_flags = CAP_ADMIN;
    }
    if (!f->authenticated || !(f->cap_flags & CAP_ADMIN))
      return srv_build_error(resp, tag, "authentication required");

    if (strcmp(cmdbuf, "lockdown clear") == 0) {
      system_lockdown = 0;
      system_lockdown_reason = 0;
      system_lockdown_pid = 0;
      system_lockdown_since = 0;
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "lockdown enter")) {
      const char *arg = cmdbuf + strlen("lockdown enter");
      int reason;

      while (*arg == ' ' || *arg == '\t')
        arg++;
      reason = srv_parse_lockdown_reason(arg);
      if (reason < 0)
        return srv_build_error(resp, tag, "invalid reason");
      srv_enter_lockdown(reason, 0);
      return srv_build_rwrite(resp, tag, count);
    }

    if (strcmp(cmdbuf, "lockdown status") == 0)
      return srv_build_rwrite(resp, tag, count);

    if (strcmp(cmdbuf, "revoke all") == 0) {
      srv_set_all_caps_revoked(1);
      return srv_build_rwrite(resp, tag, count);
    }

    if (strcmp(cmdbuf, "allow all") == 0 || strcmp(cmdbuf, "unrevoke all") == 0) {
      srv_set_all_caps_revoked(0);
      return srv_build_rwrite(resp, tag, count);
    }

    if (strcmp(cmdbuf, "revoke clear") == 0) {
      srv_clear_token_revocations();
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "revoke service=")) {
      int idx = srv_find_entry(cmdbuf + 15);
      if (idx < 0)
        return srv_build_error(resp, tag, "unknown service");
      srv_set_service_caps_revoked(&services[idx], 1);
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "allow service=") ||
        srv_has_prefix(cmdbuf, "unrevoke service=")) {
      const char *name = srv_has_prefix(cmdbuf, "allow service=") ? cmdbuf + 14
                                                                   : cmdbuf + 17;
      int idx = srv_find_entry(name);
      if (idx < 0)
        return srv_build_error(resp, tag, "unknown service");
      srv_set_service_caps_revoked(&services[idx], 0);
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "revoke token=")) {
      const char *hex = cmdbuf + 13;
      uchar fingerprint[CAP_HASH_SIZE];

      if ((int)strlen(hex) != CAP_HASH_SIZE * 2)
        return srv_build_error(resp, tag, "invalid token");
      if (srv_parse_hex_bytes(hex, fingerprint, CAP_HASH_SIZE) < 0)
        return srv_build_error(resp, tag, "invalid token");
      if (srv_revoke_token_fingerprint(fingerprint) < 0)
        return srv_build_error(resp, tag, "revoke full");
      return srv_build_rwrite(resp, tag, count);
    }

    return srv_build_error(resp, tag, "unknown cmd");
  }

  if (f->type == FID_CTL) {
    Service *s = &services[f->srv_idx];
    char cmdbuf[MAX_PATH_LEN + 32];
    int cmdlen;
    int boolval;
    u64int heartbeat_ns;
    ensure_resurrection_keys();
    /* Allow configuration if not authenticated (registration phase)
       OR if authenticated (control phase) */

    /* Check for authentication token first */
    if (count == sizeof(CapToken)) {
      CapToken *tok = (CapToken *)data;
      if (srv_verify_service_token(s, tok, &f->cap_flags) == 0) {
        f->authenticated = 1;
        return srv_build_rwrite(resp, tag, count);
      }
      /* Fallthrough: might be config data */
    }

    cmdlen = srv_copy_trimmed(cmdbuf, sizeof(cmdbuf), data, count);
    if (cmdlen <= 0)
      return srv_build_rwrite(resp, tag, count);

    if (strcmp(cmdbuf, "heartbeat") == 0) {
      if (!f->authenticated && !srv_request_is_owner(s))
        return srv_build_error(resp, tag, "authentication required");
      if (f->authenticated &&
          !(f->cap_flags & (CAP_WRITE | CAP_EXEC | CAP_ADMIN)))
        return srv_build_error(resp, tag, "permission denied");
      srv_touch_heartbeat(s);
      return srv_build_rwrite(resp, tag, count);
    }

    /* If matches "exec=" pattern, treat as configuration */
    if (srv_has_prefix(cmdbuf, "exec=")) {
      /*
       * Initial registration is allowed on a freshly created service entry.
       * After that, authenticated callers need CAP_ADMIN or CAP_EXEC.
       */
      if (!f->created_ctl && !f->authenticated && !srv_request_is_owner(s)) {
        return srv_build_error(resp, tag, "authentication required");
      }
      if (f->authenticated && !(f->cap_flags & (CAP_ADMIN | CAP_EXEC))) {
        print("RESURRECTION: Permission denied for exec= on %s (flags=%llx)\n",
              services[f->srv_idx].name, f->cap_flags);
        return srv_build_error(resp, tag, "permission denied");
      }
      if (s->system_service) {
        return srv_build_error(resp, tag, "system service");
      }

      strncpy(s->exec_path, cmdbuf + 5, MAX_PATH_LEN - 1);
      s->exec_path[MAX_PATH_LEN - 1] = 0;

      print("RESURRECTION: Configured exec path for %s: %s\n", s->name, s->exec_path);

      /* Auto-start if configured */
      if (s->state == SRV_STOPPED) {
        start_service(s);
      }

      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "endpoint=")) {
      if (!f->created_ctl && !f->authenticated && !srv_request_is_owner(s))
        return srv_build_error(resp, tag, "authentication required");
      if (f->authenticated && !(f->cap_flags & CAP_ADMIN))
        return srv_build_error(resp, tag, "permission denied");
      if (s->system_service)
        return srv_build_error(resp, tag, "system service");
      srv_set_endpoint(s, cmdbuf + 9, strlen(cmdbuf + 9));
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "auto_restart=")) {
      if (!f->created_ctl && !f->authenticated && !srv_request_is_owner(s))
        return srv_build_error(resp, tag, "authentication required");
      if (f->authenticated && !(f->cap_flags & CAP_ADMIN))
        return srv_build_error(resp, tag, "permission denied");
      if (s->system_service)
        return srv_build_error(resp, tag, "system service");
      if (srv_parse_bool(cmdbuf + 13, &boolval) < 0)
        return srv_build_error(resp, tag, "invalid boolean");
      s->auto_restart = boolval;
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "uses_pipe=")) {
      if (!f->created_ctl && !f->authenticated && !srv_request_is_owner(s))
        return srv_build_error(resp, tag, "authentication required");
      if (f->authenticated && !(f->cap_flags & CAP_ADMIN))
        return srv_build_error(resp, tag, "permission denied");
      if (s->system_service)
        return srv_build_error(resp, tag, "system service");
      if (s->state == SRV_RUNNING || s->state == SRV_STARTING)
        return srv_build_error(resp, tag, "service running");
      if (srv_parse_bool(cmdbuf + 10, &boolval) < 0)
        return srv_build_error(resp, tag, "invalid boolean");
      s->uses_pipe = boolval;
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "heartbeat_ms=")) {
      if (!f->created_ctl && !f->authenticated && !srv_request_is_owner(s))
        return srv_build_error(resp, tag, "authentication required");
      if (f->authenticated && !(f->cap_flags & CAP_ADMIN))
        return srv_build_error(resp, tag, "permission denied");
      if (s->system_service)
        return srv_build_error(resp, tag, "system service");
      if (srv_parse_heartbeat_timeout(cmdbuf + 13, &heartbeat_ns) < 0)
        return srv_build_error(resp, tag, "invalid heartbeat");
      s->heartbeat_timeout_ns = heartbeat_ns;
      srv_touch_heartbeat(s);
      return srv_build_rwrite(resp, tag, count);
    }

    if (srv_has_prefix(cmdbuf, "cmd=")) {
      if (!f->authenticated) {
        return srv_build_error(resp, tag, "authentication required");
      }
      if (s->system_service) {
        return srv_build_error(resp, tag, "system service");
      }

      char cmd[16];
      cmdlen = strlen(cmdbuf + 4);
      if (cmdlen >= (int)sizeof(cmd))
        cmdlen = sizeof(cmd) - 1;
      memcpy(cmd, cmdbuf + 4, cmdlen);
      cmd[cmdlen] = 0;

      if (strcmp(cmd, "start") == 0) {
        if (!(f->cap_flags & (CAP_ADMIN | CAP_EXEC)))
          return srv_build_error(resp, tag, "permission denied");
        if (srv_start_blocked(s))
          return srv_build_error(resp, tag, "system lockdown");
        start_service(s);
      } else if (strcmp(cmd, "stop") == 0) {
        if (!(f->cap_flags & CAP_ADMIN))
          return srv_build_error(resp, tag, "permission denied");
        stop_service(s);
      } else if (strcmp(cmd, "restart") == 0) {
        if (!(f->cap_flags & CAP_ADMIN))
          return srv_build_error(resp, tag, "permission denied");
        if (srv_start_blocked(s))
          return srv_build_error(resp, tag, "system lockdown");
        restart_service(s);
      } else {
        return srv_build_error(resp, tag, "unknown cmd");
      }

      return srv_build_rwrite(resp, tag, count);
    }

    if (!f->authenticated) {
      return srv_build_error(resp, tag, "authentication required");
    }

    /* Authenticated write - Proxying logic would go here.
       For now, just acknowledge. */
    (void)offset;
    return srv_build_rwrite(resp, tag, count);
  }

  if (f->type == FID_RESCTL) {
    CapToken tok;

    ensure_resurrection_keys();
    f->auth_reply_len = 0;

    if (count == sizeof(CapIssueRequest)) {
      CapIssueRequest *ireq = (CapIssueRequest *)data;
      char service_name[CAP_SERVICE_NAME_LEN + 1];

      if (ireq->op != CAPREQ_ISSUE)
        return srv_build_error(resp, tag, "invalid auth request");
      srv_copy_issue_service_name(service_name, sizeof(service_name), ireq->service);
      if (srv_issue_token_for_service(service_name, ireq->client_id, ireq->flags,
                                      &tok) != 0)
        return srv_build_error(resp, tag, "permission denied");
      memcpy(f->auth_reply, &tok, sizeof(tok));
      f->auth_reply_len = sizeof(tok);
      return srv_build_rwrite(resp, tag, count);
    }

    if (count == sizeof(CapRefreshRequest)) {
      CapRefreshRequest *rreq = (CapRefreshRequest *)data;

      if (rreq->op != CAPREQ_REFRESH)
        return srv_build_error(resp, tag, "invalid auth request");
      if (srv_refresh_token_from_request(rreq, &tok) != 0)
        return srv_build_error(resp, tag, "permission denied");
      memcpy(f->auth_reply, &tok, sizeof(tok));
      f->auth_reply_len = sizeof(tok);
      return srv_build_rwrite(resp, tag, count);
    }

    if (count == sizeof(CapBlindRequest))
      return srv_build_error(resp, tag, "blind signing disabled");

    return srv_build_error(resp, tag, "invalid auth request");
  }

  if (f->type == FID_ENDPOINT) {
    Service *s = &services[f->srv_idx];
    (void)offset;
    if (s->system_service)
      return srv_build_error(resp, tag, "system service");
    if (!srv_request_is_owner(s))
      return srv_build_error(resp, tag, "permission denied");
    srv_set_endpoint(s, (char *)data, (int)count);
    return srv_build_rwrite(resp, tag, count);
  }

  return srv_build_error(resp, tag, "cannot write");
}

/* Helpers for JSON formatting */
static int get_proc_pid2(int pid, uuid_t *out);
static void uuid_to_str(const uuid_t *u, char *out) {
  const char *hex = "0123456789abcdef";
  for (int i = 0; i < 16; i++) {
    out[i * 2] = hex[u->data[i] >> 4];
    out[i * 2 + 1] = hex[u->data[i] & 0x0f];
  }
  out[32] = '\0';
}

static void hex_to_str(const uchar *in, int inlen, char *out) {
  const char *hex = "0123456789abcdef";
  for (int i = 0; i < inlen; i++) {
    out[i * 2] = hex[in[i] >> 4];
    out[i * 2 + 1] = hex[in[i] & 0x0f];
  }
  out[inlen * 2] = '\0';
}

static int format_service_json(Service *s, char *buf, int len) {
  char pid2_str[33];
  char owner_pid2_str[33] = "00000000000000000000000000000000";
  char code_hash_str[65];
  char cap_hash_str[65];
  char ppid2_str[33] = "00000000000000000000000000000000";

  uuid_to_str(&s->pid2, pid2_str);
  if (s->owner_pid2_valid)
    uuid_to_str(&s->owner_pid2, owner_pid2_str);
  hex_to_str(s->service_hash, 32, code_hash_str);
  hex_to_str(s->cap_service_hash, 32, cap_hash_str);

  /* Read directly from kernel procfs; resurrection should not depend on nsd. */
  int ppid = 0;
  char path[64];
  snprint(path, sizeof(path), "#p/%d/ppid", s->pid);
  int fd = sys_open(path, 0); /* OREAD */
  if (fd >= 0) {
    char tmp[16];
    long n = sys_read(fd, tmp, sizeof(tmp) - 1);
    sys_close(fd);
    if (n > 0) {
      tmp[n] = 0;
      ppid = atoi(tmp);
      uuid_t p_uuid;
      if (get_proc_pid2(ppid, &p_uuid) == 0) {
        uuid_to_str(&p_uuid, ppid2_str);
      }
    }
  }

  return snprint(
      buf, len,
      "{\n"
      "  \"identity\": {\n"
      "    \"name\": \"%s\",\n"
      "    \"owner_pid2\": \"%s\",\n"
      "    \"pid\": %d,\n"
      "    \"last_pid\": %d,\n"
      "    \"pid2\": \"%s\",\n"
      "    \"ppid\": %d,\n"
      "    \"ppid2\": \"%s\",\n"
      "    \"code_hash\": \"%s\",\n"
      "    \"cap_service_hash\": \"%s\"\n"
      "  },\n"
      "  \"state\": {\n"
      "    \"current\": \"%s\",\n"
      "    \"path\": \"%s\",\n"
      "    \"endpoint\": \"%s\"\n"
      "  },\n"
      "  \"policy\": {\n"
      "    \"auto_restart\": %s,\n"
      "    \"critical\": %s,\n"
      "    \"system_service\": %s,\n"
      "    \"caps_revoked\": %s,\n"
      "    \"uses_pipe\": %s,\n"
      "    \"heartbeat_timeout_ms\": %llu\n"
      "  },\n"
      "  \"telemetry\": {\n"
      "    \"restarts_total\": %d,\n"
      "    \"restarts_window\": %d,\n"
      "    \"last_restart_nsec\": %llu,\n"
      "    \"last_heartbeat_nsec\": %llu,\n"
      "    \"missed_heartbeats\": %d\n"
      "  },\n"
      "  \"9p\": {\n"
      "    \"qid_path\": %llu,\n"
      "    \"qid_vers\": %u,\n"
      "    \"qid_type\": %u\n"
      "  },\n"
      "  \"supervisor\": {\n"
      "    \"lockdown\": %s,\n"
      "    \"lockdown_reason\": \"%s\",\n"
      "    \"lockdown_pid\": %d,\n"
      "    \"lockdown_since_nsec\": %llu\n"
      "  }\n"
      "}\n",
      s->name, owner_pid2_str, s->pid, s->last_pid, pid2_str, ppid, ppid2_str,
      code_hash_str, cap_hash_str,
      srv_state_name(s->state), s->exec_path, s->endpoint,
      s->auto_restart ? "true" : "false", s->critical ? "true" : "false",
      s->system_service ? "true" : "false", s->caps_revoked ? "true" : "false",
      s->uses_pipe ? "true" : "false",
      (unsigned long long)(s->heartbeat_timeout_ns / 1000000ULL), s->restarts,
      s->restarts_in_window, (unsigned long long)s->last_restart,
      (unsigned long long)s->last_heartbeat, s->missed_heartbeats,
      (unsigned long long)s->qid.path, s->qid.vers, (unsigned int)s->qid.type,
      system_lockdown ? "true" : "false",
      srv_lockdown_reason_name(system_lockdown_reason), system_lockdown_pid,
      (unsigned long long)system_lockdown_since);
}

/*
 * Handle Tread: Read service status
 */
static u32int srv_handle_read(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  pos += 4;
  u64int offset = get_u64(req + pos);
  pos += 8;
  u32int count = get_u32(req + pos);

  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "unknown fid");

  if (f->type == FID_ROOT) {
    /* Reading root directory: list all services */
    static uchar dirbuf[4096];
    uint dpos = 0;
    const char *special = ".resurrection\nctl\n";
    int special_len = strlen(special);

    for (int i = 0; i < special_len && dpos < 4000; i++)
      dirbuf[dpos++] = special[i];

    for (int i = 0; i < num_services && dpos < 4000; i++) {
      Service *s = &services[i];
      if (!service_slot_active(s))
        continue;
      int namelen = strlen(s->name);
      char endpoint_name[MAX_NAME_LEN + 10];
      int endpoint_len;

      /* Simplified directory entry format: just names separated by newlines */
      for (int j = 0; j < namelen && dpos < 4000; j++)
        dirbuf[dpos++] = s->name[j];
      dirbuf[dpos++] = '\n';

      srv_endpoint_name(s, endpoint_name, sizeof(endpoint_name));
      endpoint_len = strlen(endpoint_name);
      for (int j = 0; j < endpoint_len && dpos < 4000; j++)
        dirbuf[dpos++] = endpoint_name[j];
      dirbuf[dpos++] = '\n';
    }

    /* Handle offset */
    if (offset >= dpos)
      return srv_build_rread(resp, tag, dirbuf, 0);

    u32int avail = dpos - (u32int)offset;
    if (count > avail)
      count = avail;

    return srv_build_rread(resp, tag, dirbuf + offset, count);
  }

  if (f->type == FID_ROOTCTL) {
    static char ctlbuf[1024];
    int dpos = srv_format_root_ctl(ctlbuf, sizeof(ctlbuf));

    if (offset >= (u64int)dpos)
      return srv_build_rread(resp, tag, (uchar *)ctlbuf, 0);
    if (count > (u32int)(dpos - (int)offset))
      count = dpos - (u32int)offset;
    return srv_build_rread(resp, tag, (uchar *)ctlbuf + offset, count);
  }

  if (f->type == FID_RESCTL) {
    uchar *src;
    u32int len;

    if (f->auth_reply_len != 0) {
      src = f->auth_reply;
      len = f->auth_reply_len;
    } else {
      ensure_resurrection_keys();
      src = resurrection_pubkey;
      len = sizeof(resurrection_pubkey);
    }

    if (offset >= len)
      return srv_build_rread(resp, tag, src, 0);
    if (count > len - (u32int)offset)
      count = len - (u32int)offset;
    return srv_build_rread(resp, tag, src + offset, count);
  }

  if (f->type == FID_SERVICE_DIR) {
    /* List sub-files: ctl, status, data */
    static uchar dirbuf[256];
    uint dpos = 0;
    const char *entries[] = {"ctl", "status", "data"};
    for (int i = 0; i < 3; i++) {
      int len = strlen(entries[i]);
      if (dpos + len + 1 < 256) {
        memcpy(dirbuf + dpos, entries[i], len);
        dpos += len;
        dirbuf[dpos++] = '\n';
      }
    }

    if (offset >= dpos)
      return srv_build_rread(resp, tag, dirbuf, 0);
    u32int avail = dpos - (u32int)offset;
    if (count > avail)
      count = avail;
    return srv_build_rread(resp, tag, dirbuf + offset, count);
  }

  if (f->type == FID_CTL) {
    Service *s = &services[f->srv_idx];
    static char ctlbuf[1024];
    int dpos = srv_format_ctl(s, ctlbuf, sizeof(ctlbuf));

    if (offset >= (u64int)dpos)
      return srv_build_rread(resp, tag, (uchar *)ctlbuf, 0);
    if (count > (u32int)(dpos - (int)offset))
      count = dpos - (u32int)offset;
    return srv_build_rread(resp, tag, (uchar *)ctlbuf + offset, count);
  }

  if (f->type == FID_STATUS) {
    /* Reading service status in JSON format */
    Service *s = &services[f->srv_idx];
    static char statusbuf[2048]; /* Increased size for JSON */
    int dpos = format_service_json(s, statusbuf, sizeof(statusbuf));

    /* Handle offset */
    if (offset >= (u64int)dpos)
      return srv_build_rread(resp, tag, (uchar *)statusbuf, 0);

    u32int avail = dpos - (u32int)offset;
    if (count > avail)
      count = avail;

    return srv_build_rread(resp, tag, (uchar *)statusbuf + offset, count);
  }

  if (f->type == FID_DATA) {
    /* Raw Binary Struct */
    Service *s = &services[f->srv_idx];
    if (offset >= sizeof(Service))
      return srv_build_rread(resp, tag, (uchar *)s, 0);
    u32int avail = sizeof(Service) - (u32int)offset;
    if (count > avail)
      count = avail;
    return srv_build_rread(resp, tag, (uchar *)s + offset, count);
  }

  if (f->type == FID_ENDPOINT) {
    Service *s = &services[f->srv_idx];
    u32int len = (u32int)strlen(s->endpoint);

    if (offset >= len)
      return srv_build_rread(resp, tag, (uchar *)s->endpoint, 0);

    if (count > len - (u32int)offset)
      count = len - (u32int)offset;
    return srv_build_rread(resp, tag, (uchar *)s->endpoint + offset, count);
  }

  return srv_build_error(resp, tag, "cannot read");
}

/*
 * Handle Tstat: Get file/service metadata
 */
static u32int srv_handle_stat(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "unknown fid");

  if (f->type == FID_ROOT) {
    /* Root directory */
    return srv_build_rstat(resp, tag, &f->qid, "srv", 0, DMDIR | 0555);
  }

  if (f->type == FID_ROOTCTL) {
    char ctlbuf[1024];
    int n = srv_format_root_ctl(ctlbuf, sizeof(ctlbuf));
    return srv_build_rstat(resp, tag, &f->qid, "ctl", n, 0644);
  }

  if (f->type == FID_SERVICE_DIR) {
    /* Service directory */
    Service *s = &services[f->srv_idx];
    return srv_build_rstat(resp, tag, &f->qid, s->name, 0, DMDIR | 0555);
  }

  if (f->type == FID_CTL) {
    Service *s = &services[f->srv_idx];
    char ctlbuf[1024];
    int n = srv_format_ctl(s, ctlbuf, sizeof(ctlbuf));
    return srv_build_rstat(resp, tag, &f->qid, "ctl", n, 0644);
  }

  if (f->type == FID_RESCTL) {
    return srv_build_rstat(resp, tag, &f->qid, ".resurrection",
                           sizeof(resurrection_pubkey), 0666);
  }

  if (f->type == FID_STATUS) {
    return srv_build_rstat(resp, tag, &f->qid, "status", 0, 0444);
  }

  if (f->type == FID_DATA) {
    return srv_build_rstat(resp, tag, &f->qid, "data", sizeof(Service), 0444);
  }

  if (f->type == FID_ENDPOINT) {
    char endpoint_name[MAX_NAME_LEN + 10];
    Service *s = &services[f->srv_idx];
    srv_endpoint_name(s, endpoint_name, sizeof(endpoint_name));
    return srv_build_rstat(resp, tag, &f->qid, endpoint_name,
                           strlen(s->endpoint), 0664);
  }

  return srv_build_error(resp, tag, "cannot stat");
}

/*
 * Handle Tclunk: Release a fid
 */
static u32int srv_handle_clunk(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (f)
    srv_free_fid(f);

  return srv_build_rclunk(resp, tag);
}

static u32int srv_handle_remove(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  unsigned short tag = get_u16(req + 5);
  SrvFid *f = srv_lookup_fid(fid);
  Service *s;

  if (!f)
    return srv_build_error(resp, tag, "unknown fid");

  switch (f->type) {
  case FID_ROOTCTL:
  case FID_RESCTL:
    return srv_build_error(resp, tag, "remove not allowed");
  case FID_ENDPOINT:
    s = &services[f->srv_idx];
    if (s->system_service)
      return srv_build_error(resp, tag, "system service");
    if (!srv_request_is_owner(s))
      return srv_build_error(resp, tag, "permission denied");
    s->endpoint[0] = 0;
    srv_free_fid(f);
    return srv_build_rremove(resp, tag);
  case FID_CTL:
  case FID_STATUS:
  case FID_DATA:
  case FID_SERVICE_DIR:
    s = &services[f->srv_idx];
    if (s->system_service)
      return srv_build_error(resp, tag, "system service");
    if (!srv_request_is_owner(s))
      return srv_build_error(resp, tag, "permission denied");
    if (s->state == SRV_RUNNING || s->state == SRV_STARTING)
      stop_service(s);
    srv_free_service_fids(f->srv_idx);
    service_slot_clear(s);
    srv_trim_trailing_slots();
    return srv_build_rremove(resp, tag);
  case FID_AUTH:
    srv_free_fid(f);
    return srv_build_rremove(resp, tag);
  default:
    return srv_build_error(resp, tag, "remove not allowed");
  }
}

static u32int srv_handle_wstat(uchar *req, uchar *resp) {
  uint pos = 7;
  u32int fid = get_u32(req + pos);
  u16int nstat;
  unsigned short tag = get_u16(req + 5);
  SrvFid *f;
  Service *s;
  char newname[MAX_NAME_LEN];
  int r;

  pos += 4;
  nstat = get_u16(req + pos);
  pos += 2;
  f = srv_lookup_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "unknown fid");
  if (get_u32(req) < pos + nstat)
    return srv_build_error(resp, tag, "short stat");

  switch (f->type) {
  case FID_SERVICE_DIR:
  case FID_ENDPOINT:
    s = &services[f->srv_idx];
    if (!srv_request_is_owner(s))
      return srv_build_error(resp, tag, "permission denied");
    r = srv_wstat_name(req + pos, nstat, newname, sizeof(newname));
    if (r < 0)
      return srv_build_error(resp, tag, "bad stat");
    if (newname[0] == 0)
      return srv_build_error(resp, tag, "name required");
    if (f->type == FID_ENDPOINT) {
      char svc_name[MAX_NAME_LEN];
      if (srv_endpoint_service_name(newname, svc_name) < 0)
        return srv_build_error(resp, tag, "endpoint rename requires .endpoint");
      strncpy(newname, svc_name, sizeof(newname) - 1);
      newname[sizeof(newname) - 1] = 0;
    }
    r = srv_rename_service(s, newname);
    if (r == -1)
      return srv_build_error(resp, tag, "system service");
    if (r == -2)
      return srv_build_error(resp, tag, "service running");
    if (r == -3)
      return srv_build_error(resp, tag, "invalid name");
    if (r == -4)
      return srv_build_error(resp, tag, "service exists");
    s->qid.vers++;
    return srv_build_rwstat(resp, tag);
  default:
    return srv_build_error(resp, tag, "wstat not supported");
  }
}

/*
 * Main 9P server dispatcher for /srv
 */
static u32int srv_dispatch(uchar *req, uchar *resp) {
  uchar type = req[4]; /* Message type after size[4] */
  unsigned short tag = get_u16(req + 5);

  switch (type) {
  case P9_Tversion:
    return srv_handle_version(req, resp);
  case P9_Tflush:
    return srv_handle_flush(req, resp);
  case P9_Tattach:
    return srv_handle_attach(req, resp);
  case P9_Twalk:
    return srv_handle_walk(req, resp);
  case P9_Tcreate:
    return srv_handle_create(req, resp);
  case P9_Topen:
    return srv_handle_open(req, resp);
  case P9_Tauth:
    return srv_handle_auth(req, resp);
  case P9_Tread:
    return srv_handle_read(req, resp);
  case P9_Twrite:
    return srv_handle_write(req, resp);
  case P9_Tstat:
    return srv_handle_stat(req, resp);
  case P9_Tclunk:
    return srv_handle_clunk(req, resp);
  case P9_Tremove:
    return srv_handle_remove(req, resp);
  case P9_Twstat:
    return srv_handle_wstat(req, resp);
  default:
    return srv_build_error(resp, tag, "operation not supported");
  }
}

/* ========== Little-Endian Helpers ========== */

void put_u32(uchar *p, uint val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
}

void put_u16(uchar *p, unsigned short val) {
  p[0] = val;
  p[1] = val >> 8;
}

void put_u64(uchar *p, uvlong val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
  p[4] = val >> 32;
  p[5] = val >> 40;
  p[6] = val >> 48;
  p[7] = val >> 56;
}

unsigned short get_u16(const uchar *p) {
  return (unsigned short)p[0] | ((unsigned short)p[1] << 8);
}

uint get_u32(const uchar *p) {
  return (uint)p[0] | ((uint)p[1] << 8) | ((uint)p[2] << 16) |
         ((uint)p[3] << 24);
}

uvlong get_u64(const uchar *p) {
  return (uvlong)p[0] | ((uvlong)p[1] << 8) | ((uvlong)p[2] << 16) |
         ((uvlong)p[3] << 24) | ((uvlong)p[4] << 32) | ((uvlong)p[5] << 40) |
         ((uvlong)p[6] << 48) | ((uvlong)p[7] << 56);
}

/* ========== Console Output ========== */

#ifndef UNIT_TEST
/* Simple print - handles format args using vsnprint from liblux */
int print(const char *fmt, ...) {
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
#endif

/* ========== Utility Functions ========== */

/*
 * Compute Blake2b hash of a file
 * Returns 0 on success, -1 on failure
 */
static int do_compute_file_hash(const char *path, uchar *output_hash) {
  crypto_blake2b_ctx ctx;
  uchar read_buf[256]; // Read in 256-byte chunks
  int fd;
  long n;

  fd = do_open(path, 0 /* OREAD */);
  if (fd < 0) {
    print("RESURRECTION: do_compute_file_hash: Failed to open ");
    print(path);
    print("\n");
    return -1;
  }

  crypto_blake2b_init(&ctx, MAX_HASH_LEN); // Hash length matches service_hash

  while ((n = do_read(fd, (char *)read_buf, sizeof(read_buf))) > 0) {
    crypto_blake2b_update(&ctx, read_buf, n);
  }

  do_close(fd);

  if (n < 0) {
    print("RESURRECTION: do_compute_file_hash: Read error from ");
    print(path);
    print("\n");
    return -1;
  }

  crypto_blake2b_final(&ctx, output_hash);
  return 0;
}

/* ========== Service Registry ========== */

static Service *find_service(const char *name) {
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i]))
      continue;
    if (strcmp(services[i].name, name) == 0) {
      return &services[i];
    }
  }
  return 0;
}

static int register_service(const char *name, const char *exec_path,
                            const char *hash, int critical) {
  int idx;
  Service *svc;

  lock(&srv_lock);
  if (find_service(name)) { // find_service already locks/unlocks
    print("RESURRECTION: Service already registered: ");
    print(name);
    print("\n");
    unlock(&srv_lock);
    return -1;
  }

  idx = -1;
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i])) {
      idx = i;
      break;
    }
  }
  if (idx < 0) {
    if (num_services >= MAX_SERVICES) {
      print("RESURRECTION: Max services reached\n");
      unlock(&srv_lock);
      return -1;
    }
    idx = num_services++;
  }

  svc = &services[idx];
  service_slot_clear(svc);
  strncpy(svc->name, name, MAX_NAME_LEN - 1);
  svc->name[MAX_NAME_LEN - 1] = 0;
  srv_refresh_cap_service_hash(svc);
  strncpy(svc->exec_path, exec_path, MAX_PATH_LEN - 1);
  if (hash)
    strncpy((char *)svc->service_hash, hash, MAX_HASH_LEN - 1);
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

  unlock(&srv_lock);
  return 0;
}

/* ========== Process Control via 9P ========== */

/*
 * Execute a program (replaces current process image)
 * This is called by the child after fork
 */
/*
 * Check if a process is still running
 * Opens /proc/PID/status and reads it
 * Returns: 1 if running, 0 if exited/crashed
 */
static int is_process_running(u32int pid) {
  char path[MAX_PATH_LEN];
  int fd;

  /* Construct /proc/PID/status path */
  // It will look like /proc/123/status
  char pid_str[16]; // Max 10 digits for u32int + null terminator
  int i = 0;
  if (pid == 0) {
    pid_str[i++] = '0';
  } else {
    u32int temp_pid = pid;
    while (temp_pid > 0) {
      pid_str[i++] = '0' + (temp_pid % 10);
      temp_pid /= 10;
    }
    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
      char temp = pid_str[j];
      pid_str[j] = pid_str[i - 1 - j];
      pid_str[i - 1 - j] = temp;
    }
  }
  pid_str[i] = '\0';

  strncpy(path, "#p/", MAX_PATH_LEN - 1);
  strncpy(path + strlen(path), pid_str, MAX_PATH_LEN - strlen(path) - 1);
  strncpy(path + strlen(path), "/status", MAX_PATH_LEN - strlen(path) - 1);
  path[MAX_PATH_LEN - 1] = '\0'; // Ensure null termination

  fd = do_open(path, 0 /* OREAD */);
  if (fd >= 0) {
    do_close(fd);
    return 1; // Process status file opened, so it's running
  }
  return 0; // Failed to open, likely not running
}

/*
 * Kill a process by writing to /proc/PID/ctl
 */
static void do_kill(u32int pid) {
  char path[MAX_PATH_LEN];
  int fd;
  const char *kill_cmd = "kill";

  print_num("RESURRECTION: Killing PID ", pid, "\n");

  /* Construct /proc/PID/ctl path */
  char pid_str[16];
  int i = 0;
  if (pid == 0) {
    pid_str[i++] = '0';
  } else {
    u32int temp_pid = pid;
    while (temp_pid > 0) {
      pid_str[i++] = '0' + (temp_pid % 10);
      temp_pid /= 10;
    }
    for (int j = 0; j < i / 2; j++) {
      char temp = pid_str[j];
      pid_str[j] = pid_str[i - 1 - j];
      pid_str[i - 1 - j] = temp;
    }
  }
  pid_str[i] = '\0';

  strncpy(path, "#p/", MAX_PATH_LEN - 1);
  strncpy(path + strlen(path), pid_str, MAX_PATH_LEN - strlen(path) - 1);
  strncpy(path + strlen(path), "/ctl", MAX_PATH_LEN - strlen(path) - 1);
  path[MAX_PATH_LEN - 1] = '\0';

  fd = do_open(path, 1 /* OWRITE */);
  if (fd >= 0) {
    do_write(fd, kill_cmd, strlen(kill_cmd));
    do_close(fd);
  } else {
    print("RESURRECTION: Failed to open ");
    print(path);
    print(" for killing PID ");
    print_num("", pid, "\n");
  }
}

/*
 * Open a file
 * Returns: fd on success, -1 on failure
 */
static int do_open(const char *path, int mode) {
  return sys_open((char *)path, mode);
}

/*
 * Read from a file
 * Returns: bytes read, -1 on error
 */
static int do_read(int fd, char *buf, int count) {
  return (int)sys_read(fd, buf, count);
}

/*
 * Close a file
 */
static void do_close(int fd) {
  sys_close(fd);
}

static void format_proc_pid2_path(char *path, int npath, int pid) {
  static char prefix[] = "#p/";
  static char suffix[] = "/pid2";
  char digits[16];
  int ndigits = 0;
  int i = 0;

  if (npath <= 0)
    return;

  while (prefix[i] != '\0' && i < npath - 1) {
    path[i] = prefix[i];
    i++;
  }

  if (pid <= 0) {
    if (i < npath - 1)
      path[i++] = '0';
  } else {
    while (pid > 0 && ndigits < (int)sizeof(digits)) {
      digits[ndigits++] = (char)('0' + (pid % 10));
      pid /= 10;
    }
    while (ndigits > 0 && i < npath - 1)
      path[i++] = digits[--ndigits];
  }

  for (int j = 0; suffix[j] != '\0' && i < npath - 1; j++)
    path[i++] = suffix[j];
  path[i] = '\0';
}

static int get_proc_pid2(int pid, uuid_t *out) {
  char path[64];
  format_proc_pid2_path(path, sizeof(path), pid);
  int fd = sys_open(path, 0);
  if (fd < 0)
    return -1;
  long n = sys_read(fd, out->data, 16);
  sys_close(fd);
  return (n == 16) ? 0 : -1;
}

static void start_service(Service *svc) {
  if (svc->state == SRV_RUNNING) {
    print("RESURRECTION: %s already running\n", svc->name);
    return;
  }

  if (srv_start_blocked(svc)) {
    print("RESURRECTION: start blocked by lockdown for %s\n", svc->name);
    svc->state = SRV_STOPPED;
    return;
  }

  print("RESURRECTION: Starting %s...\n", svc->name);

  /* BINARY HASH VERIFICATION */
  int hash_configured = 0;
  for (int i = 0; i < MAX_HASH_LEN; i++) {
    if (svc->service_hash[i] != 0) {
      hash_configured = 1;
      break;
    }
  }

  if (hash_configured) {
    uchar computed_hash[MAX_HASH_LEN];
    if (do_compute_file_hash(svc->exec_path, computed_hash) != 0) {
      print("RESURRECTION: Failed to compute hash for %s\n", svc->exec_path);
      svc->state = SRV_FAILED;
      return;
    }

    if (memcmp(computed_hash, svc->service_hash, MAX_HASH_LEN) != 0) {
      print("RESURRECTION: Binary hash mismatch for %s - ABORTING START\n",
            svc->exec_path);
      svc->state = SRV_FAILED;
      return;
    }
    print("RESURRECTION: Binary hash verified for %s\n", svc->exec_path);
  }

  svc->state = SRV_STARTING;

  int pfd[2] = {0, 0};
  char fd_arg[32] = {0};

  if (svc->uses_pipe) {
    print("RESURRECTION: Creating pipe for %s...\n", svc->name);
    if (sys_pipe(pfd) < 0) {
      print("RESURRECTION: Pipe failed for %s\n", svc->name);
      svc->state = SRV_FAILED;
      return;
    }
    snprint(fd_arg, sizeof(fd_arg), "%d", pfd[1]);
  }

  /* SECURE SPAWN - Uses sys_spawn() instead of fork+exec
   * This prevents child from accessing parent's stack data via COW
   * Critical for resurrection server security!
   */
  char *argv[4];
  argv[0] = svc->name;
  if (svc->uses_pipe) {
    argv[1] = fd_arg;
    argv[2] = 0;
  } else {
    argv[1] = 0;
  }

  int pid = sys_spawn(svc->exec_path, argv);

  if (pid < 0) {
    print("RESURRECTION: Spawn failed for %s\n", svc->name);
    svc->state = SRV_FAILED;
    if (svc->uses_pipe) {
      sys_close(pfd[0]);
      sys_close(pfd[1]);
    }
    return;
  }

  /* Parent continues - close pipe ends we don't need */
  if (svc->uses_pipe) {
    sys_close(pfd[1]); /* Close write end */
    /* pfd[0] (read end) is currently ignored/leaked in this model */
    sys_close(pfd[0]); /* Prevent leak for now */
  }

  svc->pid = pid;
  svc->state = SRV_RUNNING;
  srv_touch_heartbeat(svc);

  /* Capture PID2 for secure tracking */
  if (get_proc_pid2(pid, &svc->pid2) < 0) {
    print("RESURRECTION: Warning: Could not read PID2 for %s (pid %d)\n",
          svc->name, pid);
    memset(&svc->pid2, 0, 16);
  }

  print("RESURRECTION: Started %s (PID %d)\n", svc->name, pid);
}

static void stop_service(Service *svc) {
  if (svc->state != SRV_RUNNING && svc->state != SRV_STARTING) {
    print("RESURRECTION: %s not running\n", svc->name);
    return;
  }

  if (svc->pid > 0) {
    do_kill(svc->pid);
  }

  svc->state = SRV_STOPPED;
  svc->pid = 0;

  print("RESURRECTION: Stopped %s\n", svc->name);
}

static void restart_service(Service *svc) {
  /* Rate limiting */
  u64int current_time = nsec();

  if (current_time - svc->last_restart > RESTART_WINDOW_NS) {
    svc->restarts_in_window = 0; // Reset count if outside window
  }

  svc->restarts_in_window++;
  svc->last_restart = current_time; // Update last restart time

  if (svc->restarts_in_window > 5) {
    print("RESURRECTION: %s exceeded restart limit, marking FAILED\n", svc->name);
    svc->auto_restart = 0;
    svc->state = SRV_FAILED;
    return;
  }

  svc->restarts++;

  print("RESURRECTION: Restarting %d %s\n", svc->restarts, svc->name);

  if (svc->state == SRV_RUNNING || svc->state == SRV_STARTING)
    stop_service(svc);

  /* Restart from known good state (binary path with verified hash) */
  start_service(svc);
}

/* ========== Registry Management ========== */

/* ========== Registry Management ========== */

/* ========== Registry Management ========== */

/* Allocates a new service entry. Caller must hold lock if needed. */
int srv_create_entry(const char *name, int pid) {
  int idx;
  Service *s;

  /* Check if exists */
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i]))
      continue;
    if (strcmp(services[i].name, name) == 0)
      return -1; /* Exists */
  }

  idx = -1;
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i])) {
      idx = i;
      break;
    }
  }
  if (idx < 0) {
    if (num_services >= MAX_SERVICES)
      return -1;
    idx = num_services++;
  }

  s = &services[idx];
  service_slot_clear(s);
  strncpy(s->name, name, MAX_NAME_LEN - 1);
  s->name[MAX_NAME_LEN - 1] = 0;
  srv_refresh_cap_service_hash(s);
  s->pid = pid;
  s->state = (pid > 0) ? SRV_RUNNING : SRV_STOPPED;
  /* Generate a QID */
  s->qid.type = QTDIR; /* Directory */
  s->qid.vers = 0;
  s->qid.path = (idx + 1) << 8; /* Path unique ID (shifted) */

  return idx;
}

/* ========== Service Monitoring ========== */

static void watchdog_loop(void) {
  print("RESURRECTION: Watchdog thread started\n");
  char path[128];
  uchar statbuf[256];

  while (running) {
    do_sleep_ms(5000); /* 5 seconds */

    lock(&srv_lock);
    u64int now = nsec();

    for (int i = 0; i < num_services; i++) {
      Service *s = &services[i];
      if (!service_slot_active(s))
        continue;
      if (s->state != SRV_RUNNING)
        continue;

      /* Check /srv/NAME */
      snprint(path, sizeof(path), "/srv/%s", s->name);

      /* Unlock during IO to avoid holding lock while blocking */
      int pid = s->pid; /* Snapshot PID */
      uuid_t expected_pid2;
      u64int heartbeat_timeout = s->heartbeat_timeout_ns;
      memcpy(&expected_pid2, &s->pid2, 16);
      unlock(&srv_lock);

      /* SECURE WATCHDOG: Verify PID2 identity before checking health */
      int alive = 1;
      uuid_t current_pid2;

      if (get_proc_pid2(pid, &current_pid2) < 0) {
        alive = 0; /* Process gone */
      } else if (memcmp(expected_pid2.data, current_pid2.data, 16) != 0) {
        alive = 0; /* Identity mismatch */
      }

      int ret = -1;
      if (alive && heartbeat_timeout == 0) {
        ret = sys_stat(path, statbuf, sizeof(statbuf));
      }

      lock(&srv_lock);
      /* Re-verify service state/identity */
      if (s->state != SRV_RUNNING || s->pid != pid) {
        continue; /* Service changed while we were stating */
      }

      if (!alive) {
        print("RESURRECTION: Watchdog detected dead process %s (PID %d)\n",
              s->name, pid);
        s->missed_heartbeats = 3; /* Force restart */
      } else if (s->heartbeat_timeout_ns > 0) {
        if (now - s->last_heartbeat > s->heartbeat_timeout_ns) {
          s->missed_heartbeats++;
          print("RESURRECTION: Watchdog stale heartbeat for %s (%d/3)\n",
                s->name, s->missed_heartbeats);
        } else {
          s->missed_heartbeats = 0;
        }
      } else if (ret < 0) {
        s->missed_heartbeats++;
        print("RESURRECTION: Watchdog missed heartbeat for %s (%d/3)\n",
              s->name, s->missed_heartbeats);
      } else {
        s->missed_heartbeats = 0;
        s->last_heartbeat = now;
      }

      if (s->missed_heartbeats >= 3) {
        print("RESURRECTION: Watchdog KILLING hung service %s\n", s->name);
        stop_service(s);
        if (s->auto_restart) {
          restart_service(s);
        }
      }
    }
    unlock(&srv_lock);
  }
}

static void monitor_services(void) {
  print("RESURRECTION: Entering dynamic monitoring loop\n");

  char status[128];

  while (running) {
    /* No static registry polling anymore.
       We just wait for children to die (if we launched any),
       or wait for 9P events (which happen in other thread? No, srv_loop is
       single threaded). Wait, srv_loop calls dispatch... where does
       monitor_services run?

       Ah, main calls do_rfork.
       Child -> srv_loop.
       Parent -> monitor_services.

       So Parent monitors processes. Child monitors 9P.

       If a service "announces itself" by Tcreate, it happens in Child
       (srv_loop). Parent needs to access `services` array. We used RFMEM
       ("int pid = do_rfork(RFPROC | RFMEM);"), so memory is shared. We need
       locking.
    */

    /* Wait for child events */
    int pid = sys_wait(); /* Note: status buffer not supported yet */

    if (pid < 0) {
      /* Sleep to avoid busy loop - 100ms between checks */
      do_sleep_ms(100);
      continue;
    }

    lock(&srv_lock);

    print_num("RESURRECTION: Child died PID ", pid, " Status: ");
    print(status);
    print("\n");

    /* Find which service it was */
    for (int i = 0; i < num_services; i++) {
      Service *svc = &services[i];
      if (!service_slot_active(svc))
        continue;
      if (svc->pid == pid) {
        svc->state = SRV_CRASHED;
        svc->last_pid = svc->pid; /* Save PID for distress correlation */
        svc->pid = 0;
        if (svc->auto_restart) {
          /* Logic for restart?
             If it registered itself, we might not know how to restart it
             unless it wrote its exec_path to the file?
          */
          if (svc->exec_path[0]) {
            restart_service(svc);
          }
        }
        break;
      }
    }
    unlock(&srv_lock);
  }
}

/* ========== Wave 7 Distress Signal Handlers ========== */

/*
 * Find a service by its PID
 */
static Service *find_service_by_pid(u32int pid) {
  for (int i = 0; i < num_services; i++) {
    if (!service_slot_active(&services[i]))
      continue;
    /* Match either current running PID or the most recently crashed PID */
    if (services[i].pid == pid || services[i].last_pid == pid) {
      return &services[i];
    }
  }
  return 0; /* Not found - may be unmanaged process */
}

/*
 * Handle a distress event from the kernel
 *
 * Actions based on reason:
 * - MEMORY_CORRUPT, VAULT_BREACH: Quarantine (terminate, no restart)
 * - CAP_VIOLATION, BORROW_FAULT: Terminate, may restart if not repeated
 * - PANIC_IMMINENT, RESOURCE_EXHAUST: Restart service
 * - USER_ABORT: Clean stop
 */
static void handle_distress(DistressEvent *ev) {
  Service *svc;

  lock(&srv_lock);
  svc = find_service_by_pid(ev->pid);

  switch (ev->reason) {
  case DISTRESS_MEMORY_CORRUPT:
  case DISTRESS_VAULT_BREACH:
    /* Critical: terminate immediately, don't restart (quarantine) */
    print("DISTRESS: CRITICAL pid=%d reason=%d - QUARANTINE\n", ev->pid, ev->reason);
    if (ev->reason == DISTRESS_VAULT_BREACH)
      srv_enter_lockdown(ev->reason, ev->pid);
    if (svc) {
      svc->auto_restart = 0; /* Prevent restart of compromised service */
      svc->caps_revoked = 1;
      stop_service(svc);
    } else {
      print("DISTRESS: SYSTEM INTEGRITY WARNING - Unmanaged process %d breached vault!\n", ev->pid);
    }
    break;

  case DISTRESS_CAP_VIOLATION:
  case DISTRESS_BORROW_FAULT:
    /* Security: terminate, may restart if not repeated */
    print("DISTRESS: SECURITY pid=%d reason=%d\n", ev->pid, ev->reason);
    if (svc) {
      svc->caps_revoked = 1;
      stop_service(svc);
      /* Service might restart if auto_restart is still enabled */
    }
    break;

  case DISTRESS_PANIC_IMMINENT:
  case DISTRESS_RESOURCE_EXHAUST:
    /* Operational: restart service */
    print("DISTRESS: OPERATIONAL pid=%d reason=%d - RESTART\n", ev->pid, ev->reason);
    if (svc) {
      restart_service(svc);
    }
    break;

  case DISTRESS_USER_ABORT:
    /* Normal: clean stop */
    print("DISTRESS: USER_ABORT pid=%d\n", ev->pid);
    if (svc) {
      stop_service(svc);
    }
    break;

  default:
    print("DISTRESS: UNKNOWN reason=%d pid=%d\n", ev->reason, ev->pid);
    break;
  }

  unlock(&srv_lock);
}

/*
 * Distress monitoring loop - runs in separate thread
 *
 * Opens /dev/distress and blocks on read for events.
 * This is the main Wave 7 reception point.
 */
static void distress_loop(void) {
  DistressEvent ev;
  int fd;
  int n;
  int announced_unavailable;

  fd = -1;
  announced_unavailable = 0;

  while (running) {
    if (fd < 0) {
      if (!announced_unavailable)
        print("DISTRESS: Opening /dev/distress\n");

      fd = do_open("/dev/distress", 0); /* OREAD = 0 */
      if (fd < 0) {
        if (!announced_unavailable) {
          print("DISTRESS: Cannot open /dev/distress - retrying\n");
          announced_unavailable = 1;
        }
        do_sleep_ms(1000);
        continue;
      }

      announced_unavailable = 0;
      print("DISTRESS: Monitoring for Wave 7 signals\n");
    }

    /* Blocking read - kernel will wake us when event available */
    n = do_read(fd, (char *)&ev, sizeof(ev));

    if (n == sizeof(ev)) {
      handle_distress(&ev);
    } else if (n < 0) {
      do_close(fd);
      fd = -1;
      do_sleep_ms(1000);
    }
    /* n == 0 means spurious wakeup, just loop */
  }

  if (fd >= 0)
    do_close(fd);
}

/* Mount flags */
/* Mount flags and Rfork flags moved to top */

/* ... (previous code) ... */

/* ========== 9P Server Loop ========== */

/* Helper for writing to fd */
static int do_write(int fd, const void *buf, int count) {
  return (int)sys_write(fd, (void *)buf, count);
}

static void srv_loop(int fd);
static void srv_loop_ring_raw(void) {
  static uchar tx_data[8192];
  IpcChannel *chan;
  int fd_ctl;

  chan = segattach(0, srv_ring_path, nil, 4096);
  if (chan == (void *)-1) {
    print("RESURRECTION: failed to attach service ring\n");
    return;
  }

  fd_ctl = sys_open(srv_ctl_path, OWRITE);
  print("RESURRECTION: 9P service ring loop starting\n");

  for (;;) {
    int responded = 0;
    int spins = 0;
    u32int head;
    u64int page_handle;
    BatchHeader *batch;
    uchar *ptr;

    while (chan->submission.head == chan->submission.tail) {
      if (spins < 10000) {
        spins++;
        continue;
      }
      sys_sleep(1);
      spins = 0;
    }

    head = chan->submission.head;
    page_handle = chan->submission.pages[head & RING_MASK];
    batch = (BatchHeader *)page_handle;
    if (batch->magic != BATCH_PAGE_MAGIC)
      goto next_batch;

    ptr = (uchar *)batch + BATCH_DATA_START;
    for (int i = 0; i < batch->num_messages; i++) {
      u16int msg_len;
      uchar *msg_ptr;
      u32int resp_len;

      msg_len = *(u16int *)ptr;
      ptr += 2;
      msg_ptr = ptr;
      if (msg_len == 0)
        break;

      srv_set_current_pid2(&batch->uuids[i]);
      lock(&srv_lock);
      resp_len = srv_dispatch(msg_ptr, tx_data);
      unlock(&srv_lock);
      srv_clear_current_pid2();

      if (resp_len > 0) {
        memcpy(msg_ptr, tx_data, resp_len);
        *(u16int *)(msg_ptr - 2) = (u16int)resp_len;
        responded = 1;
      }

      ptr = msg_ptr + msg_len;
    }

  next_batch:
    chan->submission.head++;
    if (responded) {
      u32int c_tail = chan->completion.tail;
      chan->completion.pages[c_tail & RING_MASK] = page_handle;
      chan->completion.tail++;
      if (fd_ctl >= 0)
        sys_write(fd_ctl, "kickreply", 9);
    }
  }
}

static void srv_setup(void) {
  int chan_id, fd_clone, fd_ctl;
  char buf[32];
  char data_path[64];
  int n;

  print("RESURRECTION: srv_setup: entering...\n");

  fd_clone = sys_open("#X/clone", OREAD);
  if (fd_clone < 0) {
    print("RESURRECTION: #X clone failed\n");
    return;
  }
  n = (int)sys_read(fd_clone, buf, sizeof(buf) - 1);
  if (n <= 0) {
    print("RESURRECTION: failed to read exchange channel id\n");
    sys_close(fd_clone);
    return;
  }
  buf[n] = 0;
  chan_id = atoi(buf);

  make_exchange_path(srv_ctl_path, sizeof(srv_ctl_path), chan_id, "ctl");
  make_exchange_path(srv_ring_path, sizeof(srv_ring_path), chan_id, "ipcring");

  fd_ctl = sys_open(srv_ctl_path, OWRITE);
  sys_close(fd_clone);
  if (fd_ctl < 0) {
    print("RESURRECTION: failed to open exchange ctl\n");
    return;
  }
  if (sys_write(fd_ctl, "mode service", 12) != 12) {
    print("RESURRECTION: failed to set exchange mode\n");
    sys_close(fd_ctl);
    return;
  }
  sys_close(fd_ctl);

  make_exchange_path(data_path, sizeof(data_path), chan_id, "data");
  srv_fd = sys_open(data_path, ORDWR);
  if (srv_fd < 0) {
    print("RESURRECTION: failed to open exchange data channel\n");
    return;
  }

  /*
   * Fork a child to serve the exchange-backed 9P ring before publishing the
   * root. Publishing blocks until a server is listening.
   */
  child_ready = 0;
  int srv_pid = sys_rfork_stack(RFPROC | RFMEM | RFFDG | RFNOWAIT,
                                srv_stack + sizeof(srv_stack),
                                srv_loop_shim, nil);
  if (srv_pid < 0) {
    print("RESURRECTION: srv_setup: rfork failed\n");
    sys_close(srv_fd);
    srv_fd = -1;
    return;
  }

  /* Parent: wait for child to signal it's running srv_loop. */
  while (!child_ready)
    ;

  print("RESURRECTION: srv_setup: about to publish /srv root\n");
  if (sys_nsroot_publish_raw(srv_fd, "/srv", "") < 0) {
    print("RESURRECTION: /srv publish failed\n");
    sys_close(srv_fd);
    srv_fd = -1;
    return;
  }
  print("RESURRECTION: srv_setup: publish succeeded\n");

  /* Signal that 9P is running in a child - main should not call srv_loop */
  print("RESURRECTION: srv_setup: /srv mounted, 9P server running in child\n");
}

static void srv_loop(int fd) {
  /* Use static buffers for freestanding 9P server thread */
  static uchar rx_data[8192];
  static uchar tx_data[8192];

  print("RESURRECTION: 9P Server Loop Starting\n");

  while (1) {
    /* Read message from kernel */
    int n = sys_read(fd, (char *)rx_data, 8192);
    if (n < 0) {
      print("RESURRECTION: 9P Read Error\n");
      break;
    }
    if (n == 0) {
      /* EOF? Pipe closed? */
      break;
    }

    /* Dispatch */
    /* Check locking: 9P handlers read/write services.
       We need to lock `srv_lock` around dispatch if it touches services.
       Most handlers do.
    */
    srv_clear_current_pid2();
    lock(&srv_lock);
    u32int resp_len = srv_dispatch(rx_data, tx_data);
    unlock(&srv_lock);

    // Silence unused function warnings for now
    (void)stop_service;
    (void)is_process_running;
    (void)register_service;
    (void)strcpy;
    (void)keys_initialized;

    if (resp_len > 0) {
      /* Write response */
      sys_write(fd, tx_data, resp_len);
    }
  }
}

/* ========== Main Entry ========== */

#ifndef UNIT_TEST
int main(void) {
  print("RESURRECTION: ============================================\n");
  print("RESURRECTION: === RESURRECTION SERVER STARTING ===\n");
  print("RESURRECTION: ============================================\n");
  print("RESURRECTION: About to call srv_setup...\n");
  ensure_resurrection_keys();
  srv_capture_bootstrap_admin();

  /* Start 9P /srv server for service management */
  print("RESURRECTION: Setting up /srv 9P server...\n");
  srv_setup();

  print("RESURRECTION: /srv setup completed, about to start services...\n");

  /* Start managed services */
  print("RESURRECTION: ============================================\n");
  print("RESURRECTION: === STARTING MANAGED SERVICES ===\n");
  print("RESURRECTION: ============================================\n");

  int idx;

  /* Nsd - Namespace daemon (infrastructure) */
  idx = srv_create_entry("nsd", 0);
  if (idx >= 0) {
    Service *s = &services[idx];
    strncpy(s->exec_path, "#/./boot/nsd", MAX_PATH_LEN - 1);
    s->auto_restart = 1;
    s->critical = 1;
    s->system_service = 1;
    s->uses_pipe = 0;
    start_service(s);
  }

  /* Envd - Environment daemon (infrastructure) */
  idx = srv_create_entry("envd", 0);
  if (idx >= 0) {
    Service *s = &services[idx];
    strncpy(s->exec_path, "#/./boot/envd", MAX_PATH_LEN - 1);
    s->auto_restart = 1;
    s->critical = 1;
    s->system_service = 1;
    s->uses_pipe = 0;
    start_service(s);
  }

  /* Procd - Process manager */
  idx = srv_create_entry("procd", 0);
  if (idx >= 0) {
    Service *s = &services[idx];
    strncpy(s->exec_path, "#/./boot/procd", MAX_PATH_LEN - 1);
    s->auto_restart = 1;
    s->critical = 1;
    s->system_service = 1;
    s->uses_pipe = 0;
    start_service(s);
  }

  /* HAL */
  idx = srv_create_entry("hal", 0);
  if (idx >= 0) {
    Service *s = &services[idx];
    strncpy(s->exec_path, "#/./boot/hal", MAX_PATH_LEN - 1);
    s->auto_restart = 1;
    s->critical = 1;
    s->system_service = 1;
    s->uses_pipe = 1;
    start_service(s);
  }

  /* Sophia */
  idx = srv_create_entry("sophia", 0);
  if (idx >= 0) {
    Service *s = &services[idx];
    strncpy(s->exec_path, "#/./boot/sophia", MAX_PATH_LEN - 1);
    s->auto_restart = 1;
    s->critical = 1;
    s->system_service = 1;
    s->uses_pipe = 0;
    start_service(s);
  }

  print("RESURRECTION: ============================================\n");
  print("RESURRECTION: === ALL SERVICES STARTED ===\n");
  print("RESURRECTION: ============================================\n");

  /* Spawn distress signal monitoring thread */
  child_ready = 0;
  int distress_pid = sys_rfork_stack(RFPROC | RFMEM | RFFDG | RFNOWAIT,
                                     distress_stack + sizeof(distress_stack),
                                     distress_loop_shim, nil);
  if (distress_pid < 0) {
    print("RESURRECTION: Distress monitor rfork failed\n");
  } else {
    int ready_seen = 0;
    for (int tries = 0; tries < 100; tries++) {
      if (child_ready) {
        ready_seen = 1;
        break;
      }
      sys_sleep(1);
    }
    if (ready_seen)
      print("RESURRECTION: Distress monitor started\n");
    else
      print("RESURRECTION: Distress monitor launched (no ready ack)\n");
  }

  /* Spawn watchdog thread */
  child_ready = 0;
  int watchdog_pid = sys_rfork_stack(RFPROC | RFMEM | RFFDG | RFNOWAIT,
                                     watchdog_stack + sizeof(watchdog_stack),
                                     watchdog_loop_shim, nil);
  if (watchdog_pid < 0) {
    print("RESURRECTION: Watchdog rfork failed\n");
  } else {
    int ready_seen = 0;
    for (int tries = 0; tries < 100; tries++) {
      if (child_ready) {
        ready_seen = 1;
        break;
      }
      sys_sleep(1);
    }
    if (ready_seen)
      print("RESURRECTION: Watchdog monitor started\n");
    else
      print("RESURRECTION: Watchdog monitor launched (no ready ack)\n");
  }

  /* Keep wait/restart logic in the process that launched services. */
  print("RESURRECTION: Service monitor running in supervisor process\n");
  monitor_services();
  return 0;
}
#endif
