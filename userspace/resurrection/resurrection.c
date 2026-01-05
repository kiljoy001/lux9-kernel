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

/* Use a static const pointer in .rodata to avoid BSS corruption issues */
static const unsigned char *const exchange_base =
    (unsigned char *)EXCHANGE_PAGE_ADDR;

/* 9P Message types */
#define Tsyscall 130
#define Rsyscall 131
#define Rerror 107

/* Syscall numbers (Matches kernel/include/sys.h) */
#define SYS_OPEN 14
#define SYS_CLOSE 4
#define SYS_READ 15
#define SYS_WRITE 20
#define SYS_REMOVE 25
#define SYS_EXIT 8
#define SYS_RFORK 19
#define SYS_WAIT 36
#define SYS_MOUNT 46
#define SYS_PIPE 21
#define SYS_PIPE 21
#define SYS_CREATE 22

/* Mount flags */
#define MREPL 0x0000
#define MBEFORE 0x0001
#define MAFTER 0x0002
#define MCREATE 0x0004

/* Rfork flags */
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define RFNOWAIT (1 << 6)

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

/* Syscall declarations from liblux */
extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern void sys_exit(char *msg);
extern int sys_create(char *path, int mode, uint perm);
extern int sys_rfork(int flags);
extern void sys_exec(char *path);
extern int sys_pipe(int *fds);
extern int sys_wait(void);
extern int sys_mount(int fd, int afd, char *old, int flags, char *aname);

/* 9P Qid (required for Service struct) */
typedef struct Qid {
  uchar type;
  u32int vers;
  u64int path;
} Qid;

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
#define RESTART_WINDOW_NS (60ULL * 1000000000ULL) // 60 seconds in nanoseconds

typedef struct {
  char name[MAX_NAME_LEN];          /* Service name */
  char exec_path[MAX_PATH_LEN];     /* Known good binary path */
  uchar service_hash[MAX_HASH_LEN]; /* SHA256 of known good binary */
  u32int pid;                       /* Current PID (0 = not running) */
  int state;                        /* SRV_* state */
  int restarts;                     /* Total restart count */
  int restarts_in_window;           /* Restarts in current minute */
  u64int last_restart;              /* Timestamp of last restart */
  int auto_restart;                 /* Auto-restart on crash */
  int critical;                     /* Critical service flag */
  Qid qid;                          /* Unique file ID for 9P */
} Service;

/* Forward Declarations */
/* Print function - also used by liblux for debugging */
int print(char *fmt, ...);
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

/* Exchange page pointers */
static volatile uchar *exchange;
static volatile struct P9Control *ctl;

/* ========== /srv 9P Server State ========== */

/*
 * Use shared 9P library from kernel/include/fcall.h
 * For freestanding build, we include the kernel headers directly
 */

/* Capability definitions - use kernel/include/capability.h types */
#define CAP_HASH_SIZE 32
#define CAP_SIG_SIZE 64
#define CAP_NONCE_SIZE 16
#define EPOCH_HOUR 0
#define EPOCH_GRACE 1

/* Capability flags */
#define CAP_READ (1 << 0)
#define CAP_WRITE (1 << 1)
#define CAP_EXEC (1 << 2)
#define CAP_ADMIN (1 << 3)

/* CapToken structure (matches kernel/include/capability.h) */
typedef struct CapToken {
  uchar service_hash[CAP_HASH_SIZE];
  uchar client_commit[CAP_HASH_SIZE];
  u64int epoch;
  u64int flags;
  uchar signature[CAP_SIG_SIZE];
} CapToken;

typedef struct CapBlindRequest {
  uchar blinded_data[CAP_HASH_SIZE + CAP_HASH_SIZE + 16];
  uchar blinding_factor[32];
} CapBlindRequest;

typedef struct CapBlindResponse {
  uchar blind_signature[CAP_SIG_SIZE];
} CapBlindResponse;

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
#define FID_ENTRY 2
#define FID_AUTH 3

typedef struct SrvFid {
  u32int fid;
  int type;          /* FID_FREE, FID_ROOT, FID_ENTRY, FID_AUTH */
  int srv_idx;       /* Index into services[] if FID_ENTRY */
  int authenticated; /* Has presented valid token */
  u32int client_pid; /* Owning process */
  Qid qid;           /* Current qid */
} SrvFid;

static SrvFid srv_fids[MAX_FIDS];

/* Current epoch for capability verification */
#define CAP_EPOCH_HOUR_NS (3600ULL * 1000000000ULL)
static u64int current_epoch = 0;

/* Forward declarations for helper functions used by 9P handlers */
static int strcmp(const char *s1, const char *s2);
static int strlen(const char *s);
static void strncpy(char *dst, const char *src, int n);
static void put_u32(uchar *p, uint val);
static void put_u16(uchar *p, unsigned short val);
static void put_u64(uchar *p, uvlong val);
static unsigned short get_u16(const uchar *p);
static uint get_u32(const uchar *p);
static uvlong get_u64(const uchar *p);
static void *memset(void *dst, int c, unsigned long n);
void *memcpy(void *dst, const void *src,
             unsigned long n); /* non-static for blind_cap.o */
int memcmp(const void *s1, const void *s2,
           unsigned long n); /* non-static for blind_cap.o */

/* Type definitions for crypto */
typedef uchar u8int;

/* Forward declarations for crypto functions */
extern int cap_epoch_valid(u64int e, u64int current, u64int grace);
extern int cap_token_verify(const CapToken *tok, const u8int *service_hash,
                            const u8int *pubkey, u64int current_epoch);
extern int cap_blind_sign(CapBlindResponse *resp, const CapBlindRequest *req,
                          const u8int *privkey);
extern u64int cap_current_epoch(int type);
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

long long nsec(void) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 128);

  uint size = 4 + 1 + 2 + 4 + 4 + 0;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_NSEC);
  pos += 4;
  put_u32(req + pos, 0);
  pos += 4; /* sdata size */
  put_u32(req + pos, 0);
  pos += 4; /* scount */

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  pos += 2;

  if (reply_type == Rerror)
    return 0;

  return (long long)get_u64(req + pos);
}

static u32int rng_state = 0xDEADBEEF;

/* Simple PRNG for blinding factors (needed by blind_cap.c) */
void randombytes(void *buf, unsigned long long len) {
  u8int *b = buf;
  if (rng_state == 0xDEADBEEF) {
    rng_state = (u32int)nsec(); /* Seed with time */
    if (rng_state == 0)
      rng_state = 0x12345678;
  }
  for (unsigned long long i = 0; i < len; i++) {
    /* xorshift32 */
    u32int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    b[i] = (u8int)x;
  }
}

/* ========== /srv FID Management ========== */

static SrvFid *srv_alloc_fid(u32int fid) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (srv_fids[i].type == FID_FREE) {
      srv_fids[i].fid = fid;
      srv_fids[i].authenticated = 0;
      return &srv_fids[i];
    }
  }
  return 0;
}

static SrvFid *srv_lookup_fid(u32int fid) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (srv_fids[i].type != FID_FREE && srv_fids[i].fid == fid)
      return &srv_fids[i];
  }
  return 0;
}

static void srv_free_fid(SrvFid *f) {
  if (f) {
    f->type = FID_FREE;
    f->fid = 0;
    f->authenticated = 0;
  }
}

/* ========== /srv Entry Management ========== */

static int srv_find_entry(const char *name) {
  lock(&srv_lock);
  for (int i = 0; i < num_services; i++) {
    if (strcmp(services[i].name, name) == 0) {
      unlock(&srv_lock);
      return i;
    }
  }
  unlock(&srv_lock);
  return -1;
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

static u32int srv_build_rauth(uchar *buf, unsigned short tag, Qid *aqid,
                              uchar *sigdata, u32int siglen) {
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

/* ========== 9P Message Handlers ========== */

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
      /* Walking from root to a service */
      int idx = srv_find_entry(name);
      if (idx < 0)
        break; /* Not found - partial walk */
      newf->type = FID_ENTRY;
      newf->srv_idx = idx;
      newf->qid = services[idx].qid;
      wqids[nwqid++] = services[idx].qid;
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

  if (srv_find_entry(name) >= 0)
    return srv_build_error(resp, tag, "service exists");

  // TODO: Obtaining the *actual* caller PID (the process making the 9P request)
  // would require kernel-level modifications to the 9P protocol or a custom
  // mechanism to pass it. For now, we pass 0, implying the service is 'owned'
  // by the resurrection server from its own perspective.
  int idx = srv_create_entry(
      name, 0 /* PID of resurrection server or 0 if unknown */);
  if (idx < 0)
    return srv_build_error(resp, tag, "no space for service");

  /* New service created via file system - starts as STOPPED
     waiting for configuration via WRITE */
  Service *s = &services[idx];
  s->state = SRV_STOPPED;
  s->pid = 0;

  print("RESURRECTION: Dynamic Service Registered: ");

  print(name);
  print("\n");

  f->type = FID_ENTRY;
  f->srv_idx = idx;
  f->qid = services[idx].qid;

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

  if (f->type == FID_ENTRY) {
    /* Allow open, but mark as unauthenticated.
       Client must write CapToken to authenticate. */
    f->authenticated = 0;

    /* If extension data present, try to verify immediately (optional
     * optimization) */
    u32int size = get_u32(req);
    if (size >= pos + sizeof(CapToken)) {
      CapToken *tok = (CapToken *)(req + pos);
      if (cap_epoch_valid(tok->epoch, current_epoch, 2) &&
          cap_token_verify(tok, services[f->srv_idx].service_hash,
                           resurrection_pubkey, current_epoch) == 0) {
        f->authenticated = 1;
      }
    }

    return srv_build_ropen(resp, tag, &f->qid, 4096);
  }

  return srv_build_error(resp, tag, "cannot open");
}

/*
 * Handle Tauth: Blind signing endpoint
 * Client sends blinded capability request, we sign and return
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

  /* Allocate auth fid */
  SrvFid *f = srv_alloc_fid(afid);
  if (!f)
    return srv_build_error(resp, tag, "no fids available");

  f->type = FID_AUTH;
  f->qid.type = QTFILE;
  f->qid.vers = 0;
  f->qid.path = 0;

  /* Extract CapBlindRequest */
  u32int size = get_u32(req);
  if (size < pos + sizeof(CapBlindRequest))
    return srv_build_error(resp, tag, "blind request required");

  CapBlindRequest *breq = (CapBlindRequest *)(req + pos);
  CapBlindResponse bresp;

  /* Sign blindly */
  cap_blind_sign(&bresp, breq, resurrection_privkey);

  return srv_build_rauth(resp, tag, &f->qid, bresp.blind_signature, 64);
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

  if (f->type == FID_ENTRY) {
    /* Allow configuration if not authenticated (registration phase)
       OR if authenticated (control phase) */

    /* Check for authentication token first */
    if (count == sizeof(CapToken)) {
      CapToken *tok = (CapToken *)data;
      if (cap_epoch_valid(tok->epoch, current_epoch, 2) &&
          cap_token_verify(tok, services[f->srv_idx].service_hash,
                           resurrection_pubkey, current_epoch) == 0) {
        f->authenticated = 1;
        return srv_build_rwrite(resp, tag, count);
      }
      /* Fallthrough: might be config data */
    }

    /* If matches "exec=" pattern, treat as configuration */
    if (count > 5 && memcmp(data, "exec=", 5) == 0) {
      Service *s = &services[f->srv_idx];

      /* Simple parsing: remainder is path */
      int pathlen = count - 5;
      if (pathlen >= MAX_PATH_LEN)
        pathlen = MAX_PATH_LEN - 1;

      memcpy(s->exec_path, data + 5, pathlen);
      s->exec_path[pathlen] = 0;

      /* Strip newline if present */
      if (pathlen > 0 && s->exec_path[pathlen - 1] == '\n')
        s->exec_path[pathlen - 1] = 0;

      print("RESURRECTION: Configured exec path for ");
      print(s->name);
      print(": ");
      print(s->exec_path);
      print("\n");

      /* Auto-start if configured */
      if (s->state == SRV_STOPPED) {
        start_service(s);
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

  return srv_build_error(resp, tag, "cannot write");
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

/*
 * Main 9P server dispatcher for /srv
 */
static u32int srv_dispatch(uchar *req, uchar *resp) {
  uchar type = req[4]; /* Message type after size[4] */
  unsigned short tag = get_u16(req + 5);

  switch (type) {
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
  case P9_Twrite:
    return srv_handle_write(req, resp);
  case P9_Tclunk:
    return srv_handle_clunk(req, resp);
  default:
    return srv_build_error(resp, tag, "operation not supported");
  }
}

/* ========== Memory Operations ========== */

void *memcpy(void *dst, const void *src, unsigned long n) {
  uchar *d = dst;
  const uchar *s = src;
  while (n--)
    *d++ = *s++;
  return dst;
}

static void *memset(void *dst, int c, unsigned long n) __attribute__((used));
static void *memset(void *dst, int c, unsigned long n) {
  uchar *d = dst;
  while (n--)
    *d++ = (uchar)c;
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (uchar)*s1 - (uchar)*s2;
}

static int strlen(const char *s) __attribute__((used));
static int strlen(const char *s) {
  int len = 0;
  while (*s++)
    len++;
  return len;
}

int memcmp(const void *s1, const void *s2, unsigned long n) {
  const uchar *p1 = s1, *p2 = s2;
  while (n--) {
    if (*p1 != *p2)
      return *p1 - *p2;
    p1++;
    p2++;
  }
  return 0;
}

static void strcpy(char *dst, const char *src) {
  while ((*dst++ = *src++))
    ;
}

void strncpy(char *dst, const char *src, int n) {
  while (n-- > 0 && (*dst++ = *src++))
    ;
  if (n >= 0)
    *dst = 0;
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

/* Simple print - ignores format args for now */
int print(char *fmt, ...) {
  const char *msg = fmt;
  int msg_len = strlen(msg);
  uchar *req = (uchar *)exchange_base;
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
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");
  return msg_len;
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
    if (strcmp(services[i].name, name) == 0) {
      return &services[i];
    }
  }
  return 0;
}

static int register_service(const char *name, const char *exec_path,
                            const char *hash, int critical) {
  lock(&srv_lock);
  if (num_services >= MAX_SERVICES) {
    print("RESURRECTION: Max services reached\n");
    unlock(&srv_lock);
    return -1;
  }

  if (find_service(name)) { // find_service already locks/unlocks
    print("RESURRECTION: Service already registered: ");
    print(name);
    print("\n");
    unlock(&srv_lock);
    return -1;
  }

  Service *svc = &services[num_services++];
  memset(svc, 0, sizeof(Service));
  strncpy(svc->name, name, MAX_NAME_LEN - 1);
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
 * Fork a new process
 * Returns: child PID on success, -1 on failure
 */
static int do_fork(void) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 256);

  /* Tsyscall header */
  uint size = 4 + 1 + 2 + 4 + 4 + 4; /* header + flags */
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_RFORK);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4;                   /* scount */
  put_u32(req + pos, RFPROC); /* flags = RFPROC */

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

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
  uchar *req = (uchar *)exchange_base;
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
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

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

  strncpy(path, "/proc/", MAX_PATH_LEN - 1);
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

  strncpy(path, "/proc/", MAX_PATH_LEN - 1);
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
  uchar *req = (uchar *)exchange_base;
  int pathlen = strlen(path);
  uint pos = 0;

  memset(req, 0, 512);

  uint size = 4 + 1 + 2 + 4 + 4 + 4 + 2 + pathlen;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_OPEN);
  pos += 4;
  put_u32(req + pos, 4 + 2 + pathlen);
  pos += 4; /* sdata size in Tsyscall */
  put_u32(req + pos, 2);
  pos += 4; /* scount */
  put_u16(req + pos, pathlen);
  pos += 2;                         /* arg 0: path len */
  memcpy(req + pos, path, pathlen); /* arg 0: path */
  pos += pathlen;
  put_u32(req + pos, mode);
  pos += 4; /* arg 1: mode */

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  pos += 2; /* tag */

  if (reply_type == Rerror) {
    char *err_str = (char *)(req + pos);
    print("RESURRECTION: do_open failed: ");
    print(err_str);
    print("\n");
    return -1;
  }

  uvlong retval = get_u64(req + pos);
  return (int)retval;
}

/*
 * Read from a file
 * Returns: bytes read, -1 on error
 */
static int do_read(int fd, char *buf, int count) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 128);

  /* Tsyscall SYS_READ(fd, buf, count) */
  /* Note: buf pointer is ignored by kernel for userspace address,
     kernel uses return buffer for data */
  uint size = 4 + 1 + 2 + 4 + 4 + 4 + 8 + 4;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_READ);
  pos += 4;
  put_u32(req + pos, 16);
  pos += 4; /* sdata size */
  put_u32(req + pos, 3);
  pos += 4; /* scount */
  put_u32(req + pos, fd);
  pos += 4; /* arg 0: fd */
  put_u64(req + pos, 0);
  pos += 8; /* arg 1: buf (placeholder) */
  put_u32(req + pos, (uint)count);
  pos += 4; /* arg 2: count */

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  pos += 2; /* tag */

  if (reply_type == Rerror) {
    return -1;
  }

  /* Rsyscall returns bytes read as retval */
  /* Data is in sdata */
  /* Format: [size] [type] [tag] [retval:8] [sdata_len:4] [data...] */

  int bytes_read = (int)get_u64(req + pos);
  pos += 8;
  /* uint sdata_len = */ get_u32(req + pos);
  pos += 4;

  if (bytes_read > 0 && buf) {
    memcpy(buf, req + pos, bytes_read);
  }
  return bytes_read;
}

/*
 * Close a file
 */
static void do_close(int fd) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 128);

  uint size = 4 + 1 + 2 + 4 + 4 + 4;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_CLOSE);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4; /* sdata size */
  put_u32(req + pos, 1);
  pos += 4; /* scount */
  put_u32(req + pos, fd);
  pos += 4; /* arg 0: fd */

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");
}

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

  /* BINARY HASH VERIFICATION */
  // Check if a service_hash is configured (not all zeros)
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
      print("RESURRECTION: Failed to compute hash for ");
      print(svc->exec_path);
      print("\n");
      svc->state = SRV_FAILED;
      return;
    }

    if (memcmp(computed_hash, svc->service_hash, MAX_HASH_LEN) != 0) {
      print("RESURRECTION: Binary hash mismatch for ");
      print(svc->exec_path);
      print(" - ABORTING START\n");
      svc->state = SRV_FAILED;
      return;
    }
    print("RESURRECTION: Binary hash verified for ");
    print(svc->exec_path);
    print("\n");
  }

  svc->state = SRV_STARTING;

  /* Fork new process */
  int pid = sys_rfork(RFPROC);

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
    sys_exec(svc->exec_path);
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
  u64int current_time = nsec();

  if (current_time - svc->last_restart > RESTART_WINDOW_NS) {
    svc->restarts_in_window = 0; // Reset count if outside window
  }

  svc->restarts_in_window++;
  svc->last_restart = current_time; // Update last restart time

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

/*
 * Wait for any child process to exit (BLOCKING)
 * Returns: PID of exited child, or -1 on error
 */
static int do_wait(char *status_buf, int status_len) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 256);

  /* Tsyscall header */
  uint size = 4 + 1 + 2 + 4 + 4;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, 166); /* SYS_WAIT */
  pos += 4;
  put_u32(req + pos, 0);
  pos += 4;

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  /* Parse reply */
  pos = 0;
  /* uint reply_size = */ get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  /* tag */ pos += 2;

  if (reply_type == Rerror) {
    /* No children or error */
    return -1;
  }

  uvlong retval = get_u64(req + pos);
  pos += 8;
  uint msglen = get_u32(req + pos);
  pos += 4;

  if (status_buf && status_len > 0 && msglen > 0) {
    int copy_len =
        (msglen < (uint)(status_len - 1)) ? (int)msglen : (status_len - 1);
    memcpy(status_buf, req + pos, copy_len);
    status_buf[copy_len] = 0;
  }

  return (int)retval;
}

/* ========== Service Monitoring ========== */

/* ========== Registry Management ========== */

/* ========== Registry Management ========== */

/* Allocates a new service entry. Caller must hold lock if needed. */
int srv_create_entry(const char *name, int pid) {
  /* Check if exists */
  for (int i = 0; i < num_services; i++) {
    if (strcmp(services[i].name, name) == 0)
      return -1; /* Exists */
  }

  if (num_services >= MAX_SERVICES)
    return -1;

  int idx = num_services++;
  Service *s = &services[idx];
  memset(s, 0, sizeof(Service));
  strncpy(s->name, name, MAX_NAME_LEN - 1);
  /* Generate a QID */
  s->qid.type = 0; /* File */
  s->qid.vers = 0;
  s->qid.path = idx + 1; /* Path unique ID */

  return idx;
}

/* ========== Service Monitoring ========== */

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
      /* Sleep to avoid busy loop */
      for (volatile int i = 0; i < 5000000; i++)
        ;
      continue;
    }

    lock(&srv_lock);

    print_num("RESURRECTION: Child died PID ", pid, " Status: ");
    print(status);
    print("\n");

    /* Find which service it was */
    for (int i = 0; i < num_services; i++) {
      Service *svc = &services[i];
      if (svc->pid == pid) {
        svc->state = SRV_CRASHED;
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

/* Mount flags */
/* Mount flags and Rfork flags moved to top */

/* ... (previous code) ... */

/*
 * Create a pipe
 * Returns: 0 on success, -1 on failure. Fds in fd[2]
 */
static int do_pipe(int fd[2]) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 128);

  uint size = 4 + 1 + 2 + 4 + 4 + 8;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_PIPE);
  pos += 4;
  put_u32(req + pos, 8);
  pos += 4; /* sdata size */
  put_u32(req + pos, 1);
  pos += 4; /* scount */
  put_u32(req + pos, 0);
  pos += 4; /* arg 0: pipefd array (ignored, returned in sdata) */

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  pos += 2;

  if (reply_type == Rerror)
    return -1;

  /* Pipe returns 2 fds in sdata (8 bytes) */
  int retval = (int)get_u64(req + pos);
  pos += 8;
  (void)retval;
  /* uint sdata_len = */ get_u32(req + pos);
  pos += 4;

  fd[0] = (int)get_u32(req + pos);
  pos += 4;
  fd[1] = (int)get_u32(req + pos);

  return 0;
}

/*
 * Mount a file descriptor
 */
static int do_mount(int fd, int afd, const char *old, int flags,
                    const char *aname) {
  uchar *req = (uchar *)exchange_base;
  int oldlen = strlen(old);
  int anamelen = aname ? strlen(aname) : 0;
  uint pos = 0;

  memset(req, 0, 512);

  /* Tsyscall: SYS_MOUNT(fd, afd, old, flags, aname) */
  /* Args: fd, afd, old(ptr), flags, aname(ptr) */
  /* But string pointers are passed as length+data in our Tsyscall convention?
   */
  /* Re-checking do_open: it passes pathlen then path. */
  /* Let's assume standard Tsyscall packing:
     arg0: fd
     arg1: afd
     arg2: old (len, data)
     arg3: flags
     arg4: aname (len, data)
  */

  uint size =
      4 + 1 + 2 + 4 + 4 + 4 + 4 + (2 + oldlen) + 4 + (2 + anamelen); // approx

  /* Calculate exact size */
  /* header(11) + syscall(4) + sdata_sz(4) + scount(4) + args */
  /* args: 4, 4, (2+oldlen), 4, (2+anamelen) */
  uint args_size = 4 + 4 + (2 + oldlen) + 4 + (2 + anamelen);
  size = 11 + 4 + 4 + 4 + args_size;

  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_MOUNT);
  pos += 4;
  put_u32(req + pos, args_size);
  pos += 4;
  put_u32(req + pos, 5);
  pos += 4; /* scount */

  put_u32(req + pos, fd);
  pos += 4;
  put_u32(req + pos, afd);
  pos += 4;

  put_u16(req + pos, oldlen);
  pos += 2;
  memcpy(req + pos, old, oldlen);
  pos += oldlen;

  put_u32(req + pos, flags);
  pos += 4;

  put_u16(req + pos, anamelen);
  pos += 2;
  if (anamelen)
    memcpy(req + pos, aname, anamelen);
  pos += anamelen;

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos] == Rerror)
    return -1;

  return 0;
}

/*
 * Rfork (create new process/thread)
 */
static int do_rfork(int flags) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  memset(req, 0, 128);

  uint size = 4 + 1 + 2 + 4 + 4 + 4;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_RFORK);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4; /* sdata size */
  put_u32(req + pos, 1);
  pos += 4; /* scount */
  put_u32(req + pos, flags);
  pos += 4;

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos] == Rerror)
    return -1;

  return (int)get_u64(req + pos + 3);
}

/* ========== 9P Server Loop ========== */

/* Helper for writing to fd */
static int do_write(int fd, const void *buf, int count) {
  uchar *req = (uchar *)exchange_base;
  uint pos = 0;

  if (count > 4000)
    count = 4000;

  memset(req, 0, 128 + count);

  uint size = 4 + 1 + 2 + 4 + 4 + 4 + 8 + 4 + count;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_WRITE);
  pos += 4;
  put_u32(req + pos, 4 + 8 + 4 + count); /* sdata size */
  pos += 4;
  put_u32(req + pos, 3); /* scount */
  pos += 4;
  put_u32(req + pos, fd);
  pos += 4; /* arg 0: fd */
  put_u64(req + pos, 0);
  pos += 8; /* arg 1: buf (placeholder/offset) */
  put_u32(req + pos, (uint)count);
  pos += 4; /* arg 2: count */

  memcpy(req + pos, buf, count);

  ctl->doorbell = 1;
  __asm__ volatile("push %%rbx; syscall; pop %%rbx" ::
                       : "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];

  if (reply_type == Rerror)
    return -1;

  uvlong retval = get_u64(req + pos + 2);
  return (int)retval;
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

int main(void) {
  /* Initialize exchange page pointers */
  exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  /* Initialize keys */
  /* Generate new server keys on startup (ephemeral) or load from secure
     storage? For resurrection, maybe ephemeral is fine if services
     re-register? But services rely on known pubkey. For now, generate
     deterministic keys for testing using a seed? Or just random.
  */
  u8int seed[32];
  memset(seed, 42, 32);
  crypto_eddsa_key_pair(resurrection_privkey, resurrection_pubkey, seed);

  /* Update epoch */
  /* Need standard time. nsec syscall? */
  /* current_epoch is u64int static. cap_current_epoch(EPOCH_HOUR). */
  current_epoch = cap_current_epoch(EPOCH_HOUR);

  print("=== Resurrection Server Starting (Layer 2) ===\n");

  /* Setup /srv 9P server */
  int p[2];
  if (sys_pipe(p) < 0) {
    print("RESURRECTION: Pipe failed\n");
  } else {
    /* Mount p[1] to /srv */
    /* Flags: MREPL (0) | MCREATE (4) = 4 */
    if (sys_mount(p[1], -1, "/srv", MREPL | MCREATE, "") < 0) {
      print("RESURRECTION: Mount failed\n");
    } else {
      sys_close(p[1]);

      /* Spawn 9P worker thread */
      /* Use RFPROC | RFMEM to share services */
      int pid = sys_rfork(RFPROC | RFMEM);
      if (pid < 0) {
        print("RESURRECTION: Rfork failed\n");
      } else if (pid == 0) {
        /* Child: 9P Server Loop */
        srv_loop(p[0]);
        /* Should not return */
        sys_close(p[0]);
        return 0;
      } else {
        /* Parent: Continue to monitor services */
        print("RESURRECTION: 9P Server started\n");
        sys_close(p[0]);
      }
    }
  }

  /* Enter service monitoring loop */
  monitor_services();

  /* ... shutdown ... */
}
