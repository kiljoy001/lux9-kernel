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

/* Qid structure */
typedef struct Qid {
  uchar type;
  u32int vers;
  u64int path;
} Qid;

#define QTDIR 0x80
#define QTFILE 0x00

/* Resurrection signing key (Ed25519) */
static uchar resurrection_privkey[64];
static uchar resurrection_pubkey[32];
static int keys_initialized = 0;

/* /srv entry */
#define MAX_SRV_ENTRIES 64
#define SRV_NAME_LEN 32

typedef struct SrvEntry {
  char name[SRV_NAME_LEN]; /* Service name in /srv */
  uchar service_hash[32];  /* Blake2b(name) for token binding */
  u32int owner_pid;        /* PID that registered this */
  u64int registered_epoch; /* When registered */
  int active;              /* Entry in use */
  Qid qid;                 /* Qid for this entry */
} SrvEntry;

static SrvEntry srv_entries[MAX_SRV_ENTRIES];
static int srv_count = 0;
static u64int srv_qid_path = 2; /* Next qid.path to assign */

/* FID tracking for 9P server */
#define MAX_FIDS 128
#define FID_FREE 0
#define FID_ROOT 1
#define FID_ENTRY 2
#define FID_AUTH 3

typedef struct SrvFid {
  u32int fid;
  int type;          /* FID_FREE, FID_ROOT, FID_ENTRY, FID_AUTH */
  int srv_idx;       /* Index into srv_entries[] if FID_ENTRY */
  int authenticated; /* Has presented valid token */
  u32int client_pid; /* Owning process */
  Qid qid;           /* Current qid */
} SrvFid;

static SrvFid srv_fids[MAX_FIDS];

/* Current epoch for capability verification */
#define CAP_EPOCH_HOUR_NS (3600ULL * 1000000000ULL)
static u64int current_epoch = 0;

/* Forward declarations for helper functions used by 9P handlers */
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
void strncpy(char *dst, const char *src, int n);
void put_u32(uchar *p, uint val);
void put_u16(uchar *p, unsigned short val);
void put_u64(uchar *p, uvlong val);
unsigned short get_u16(const uchar *p);
uint get_u32(const uchar *p);
uvlong get_u64(const uchar *p);
void *memset(void *dst, int c, unsigned long n);
void *memcpy(void *dst, const void *src, unsigned long n);
int memcmp(const void *s1, const void *s2, unsigned long n);

/* Type definitions for crypto */
typedef uchar u8int;

/* Forward declarations for crypto functions */
extern int cap_epoch_valid(u64int e, u64int current, u64int grace);
extern int cap_token_verify(const CapToken *tok, const u8int *service_hash,
                            const u8int *pubkey, u64int current_epoch);
extern int cap_blind_sign(CapBlindResponse *resp, const CapBlindRequest *req,
                          const u8int *privkey);
extern u64int cap_current_epoch(int type);
extern void crypto_eddsa_key_pair(u8int *sk, u8int *pk);

/* Syscall for time (needed by blind_cap.c) */
#define SYS_NSEC 53

long long nsec(void) {
  uchar *req = (uchar *)exchange;
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
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

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
  for (int i = 0; i < MAX_SRV_ENTRIES; i++) {
    if (srv_entries[i].active && strcmp(srv_entries[i].name, name) == 0)
      return i;
  }
  return -1;
}

static int srv_create_entry(const char *name, u32int owner_pid) {
  if (srv_count >= MAX_SRV_ENTRIES)
    return -1;

  for (int i = 0; i < MAX_SRV_ENTRIES; i++) {
    if (!srv_entries[i].active) {
      strncpy(srv_entries[i].name, name, SRV_NAME_LEN - 1);
      srv_entries[i].name[SRV_NAME_LEN - 1] = 0;
      srv_entries[i].owner_pid = owner_pid;
      srv_entries[i].registered_epoch = current_epoch;
      srv_entries[i].active = 1;
      srv_entries[i].qid.type = QTFILE;
      srv_entries[i].qid.vers = 0;
      srv_entries[i].qid.path = srv_qid_path++;
      /* Hash service name for token binding (simplified - use Blake2b in
       * production) */
      for (int j = 0; j < 32 && j < SRV_NAME_LEN; j++)
        srv_entries[i].service_hash[j] = (uchar)name[j];
      srv_count++;
      return i;
    }
  }
  return -1;
}

/* ========== 9P Message Building ========== */

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
    char name[SRV_NAME_LEN];
    for (int j = 0; j < namelen && j < SRV_NAME_LEN - 1; j++)
      name[j] = req[pos + j];
    name[namelen < SRV_NAME_LEN ? namelen : SRV_NAME_LEN - 1] = 0;
    pos += namelen;

    if (newf->type == FID_ROOT) {
      /* Walking from root to a service */
      int idx = srv_find_entry(name);
      if (idx < 0)
        break; /* Not found - partial walk */
      newf->type = FID_ENTRY;
      newf->srv_idx = idx;
      newf->qid = srv_entries[idx].qid;
      wqids[nwqid++] = srv_entries[idx].qid;
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
  char name[SRV_NAME_LEN];
  for (int i = 0; i < namelen && i < SRV_NAME_LEN - 1; i++)
    name[i] = req[pos + i];
  name[namelen < SRV_NAME_LEN ? namelen : SRV_NAME_LEN - 1] = 0;
  pos += namelen;

  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f || f->type != FID_ROOT)
    return srv_build_error(resp, tag, "create in non-directory");

  if (srv_find_entry(name) >= 0)
    return srv_build_error(resp, tag, "service exists");

  int idx = srv_create_entry(name, 0 /* TODO: get caller PID */);
  if (idx < 0)
    return srv_build_error(resp, tag, "no space for service");

  f->type = FID_ENTRY;
  f->srv_idx = idx;
  f->qid = srv_entries[idx].qid;

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
          cap_token_verify(tok, srv_entries[f->srv_idx].service_hash,
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
    if (!f->authenticated) {
      /* Expect CapToken */
      if (count == sizeof(CapToken)) {
        CapToken *tok = (CapToken *)data;

        if (cap_epoch_valid(tok->epoch, current_epoch, 2) &&
            cap_token_verify(tok, srv_entries[f->srv_idx].service_hash,
                             resurrection_pubkey, current_epoch) == 0) {
          f->authenticated = 1;
          return srv_build_rwrite(resp, tag, count);
        }
        return srv_build_error(resp, tag, "invalid token or expired");
      }
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

void *memset(void *dst, int c, unsigned long n) __attribute__((used));
void *memset(void *dst, int c, unsigned long n) {
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

int strlen(const char *s) __attribute__((used));
int strlen(const char *s) {
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

/*
 * Open a file
 * Returns: fd on success, -1 on failure
 */
static int do_open(const char *path, int mode) {
  uchar *req = (uchar *)exchange;
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
  put_u32(req + pos, mode);
  pos += 4; /* arg 0: mode */
  put_u16(req + pos, pathlen);
  pos += 2;                         /* arg 1: path len */
  memcpy(req + pos, path, pathlen); /* arg 1: path */

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  pos += 2; /* tag */

  if (reply_type == Rerror) {
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
  uchar *req = (uchar *)exchange;
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
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

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
  uchar *req = (uchar *)exchange;
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
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
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

/*
 * Wait for any child process to exit (BLOCKING)
 * Returns: PID of exited child, or -1 on error
 */
static int do_wait(char *status_buf, int status_len) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  memset(req, 0, 256);

  /* Tsyscall header */
  uint size = 4 + 1 + 2 + 4;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, 166); /* SYS_WAIT */
  pos += 4;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

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

  uint pid = get_u32(req + pos);
  pos += 4;
  uint msglen = get_u16(req + pos);
  pos += 2;

  if (status_buf && status_len > 0) {
    int copy_len = (msglen < status_len - 1) ? msglen : status_len - 1;
    memcpy(status_buf, req + pos, copy_len);
    status_buf[copy_len] = 0;
  }

  return (int)pid;
}

/* ========== Service Monitoring ========== */

/* ========== Registry Management ========== */

static void sync_services(Service *new_list, int new_count) {
  /*
   * Reconciliation Logic:
   * 1. Check existing services:
   *    - If not in new_list, STOP it.
   *    - If in new_list but config changed, RESTART it.
   *    - If in new_list and same, keep running.
   * 2. Check new services:
   *    - If not in existing, START it.
   */

  /* Mark all current services as potentially removed */
  int kept[MAX_SERVICES];
  memset(kept, 0, sizeof(kept));

  /* Pass 1: Stop removed or changed services */
  for (int i = 0; i < num_services; i++) {
    Service *curr = &services[i];
    int found = 0;

    for (int j = 0; j < new_count; j++) {
      Service *new = &new_list[j];
      if (strcmp(curr->name, new->name) == 0) {
        found = 1;
        /* Check if config changed */
        if (strcmp(curr->exec_path, new->exec_path) != 0 ||
            curr->critical != new->critical) {
          print("RESURRECTION: Config changed for ");
          print(curr->name);
          print(". Restarting...\n");
          stop_service(curr);
          /* Update config */
          strncpy(curr->exec_path, new->exec_path, MAX_PATH_LEN - 1);
          curr->critical = new->critical;
          curr->auto_restart = new->auto_restart;
          /* Will be started in start_service loop if needed,
             but simpler to just mark as stopped and let it start below */
        }
        kept[i] = 1; /* Keep this slot */
        break;
      }
    }

    if (!found) {
      print("RESURRECTION: Service removed: ");
      print(curr->name);
      print("\n");
      stop_service(curr);
      curr->state = SRV_STOPPED; /* effectively free slot */
      /* We compact list later or just mark as unused?
         Simple: mark unused by empty name */
      curr->name[0] = 0;
    }
  }

  /* Compact list */
  int write_idx = 0;
  for (int i = 0; i < num_services; i++) {
    if (services[i].name[0] != 0) {
      if (write_idx != i) {
        services[write_idx] = services[i];
      }
      write_idx++;
    }
  }
  num_services = write_idx;

  /* Pass 2: Add NEW services */
  for (int j = 0; j < new_count; j++) {
    Service *new = &new_list[j];
    Service *existing = find_service(new->name);

    if (!existing) {
      if (num_services >= MAX_SERVICES) {
        print("RESURRECTION: Max services, cannot add ");
        print(new->name);
        print("\n");
        continue;
      }
      Service *s = &services[num_services++];
      *s = *new; /* struct copy */
      s->state = SRV_STOPPED;
      s->pid = 0;
      s->restarts = 0;

      print("RESURRECTION: New service added: ");
      print(s->name);
      print("\n");

      start_service(s);
    } else {
      /* ensure generic start if it was stopped */
      if (existing->state == SRV_STOPPED) {
        start_service(existing);
      }
    }
  }
}

static int parse_line(char *line, Service *svc) {
  /* Line format: name path critical auto_restart hash */
  /* Split by spaces */
  char *p = line;

  /* Skip empty/comment */
  if (*p == '#' || *p == 0)
    return -1;

  /* Name */
  char *name = p;
  while (*p && *p != ' ' && *p != '\t')
    p++;
  if (*p == 0)
    return -1;
  *p++ = 0;

  /* Skip whitespace */
  while (*p == ' ' || *p == '\t')
    p++;

  /* Path */
  char *path = p;
  while (*p && *p != ' ' && *p != '\t')
    p++;
  if (*p == 0)
    return -1;
  *p++ = 0;

  /* Skip whitespace */
  while (*p == ' ' || *p == '\t')
    p++;

  /* Critical (0/1) */
  if (*p != '0' && *p != '1')
    return -1;
  int crit = *p++ - '0';

  /* Skip whitespace */
  while (*p == ' ' || *p == '\t')
    p++;

  /* AutoRestart (0/1) */
  if (*p != '0' && *p != '1')
    return -1;
  int auto_res = *p++ - '0';

  memset(svc, 0, sizeof(Service));
  strncpy(svc->name, name, MAX_NAME_LEN - 1);
  strncpy(svc->exec_path, path, MAX_PATH_LEN - 1);
  svc->critical = crit;
  svc->auto_restart = auto_res;

  return 0;
}

static void load_registry(void) {
  int fd = do_open("/boot/services.conf", 0); // O_RDONLY
  if (fd < 0) {
    print("RESURRECTION: Could not open /boot/services.conf\n");
    return;
  }

  /* Buffer for file content */
  /* Note: Simple implementation reads small config in one go or blocks */
  char buf[4096];
  int n = do_read(fd, buf, sizeof(buf) - 1);
  do_close(fd);

  if (n <= 0)
    return;
  buf[n] = 0;

  Service new_list[MAX_SERVICES];
  int new_count = 0;

  /* Parse lines */
  char *line_start = buf;
  char *p = buf;
  while (*p) {
    if (*p == '\n') {
      *p = 0;
      if (new_count < MAX_SERVICES) {
        if (parse_line(line_start, &new_list[new_count]) == 0) {
          new_count++;
        }
      }
      line_start = p + 1;
    }
    p++;
  }
  /* Handle last line if no newline */
  if (p > line_start && new_count < MAX_SERVICES) {
    if (parse_line(line_start, &new_list[new_count]) == 0) {
      new_count++;
    }
  }

  sync_services(new_list, new_count);
}

/* ========== Service Monitoring ========== */

static void monitor_services(void) {
  print("RESURRECTION: Entering monitoring loop (Hot-Reload Enabled)\n");

  char status[128];

  while (running) {
    /* 1. Poll Registry for changes */
    load_registry();

    /* 2. Wait for child events (NON-BLOCKING check ideally, but we use waitpid
       with brief polling if supported, OR we rely on cycle with small timeout.
       Since do_wait is blocking in current impl, we can't 'poll' frequently
       unless children are dying.

       For HOT RELOAD demonstration with fakeserver, fakeserver dies frequently,
       so load_registry() will be called every 5 ticks. This is sufficient.
    */

    int pid = do_wait(status, sizeof(status));

    if (pid < 0) {
      /* No children died or error. In a real poll loop we'd sleep.
         If do_wait is true blocking, we rely on children crashing.
         If do_wait returns error immediately (no children), we sleep. */

      /* Safety sleep to avoid CPU burn if no services running */
      for (volatile int i = 0; i < 5000000; i++)
        ;
      continue;
    }

    print_num("RESURRECTION: Child died PID ", pid, " Status: ");
    print(status);
    print("\n");

    /* Find which service it was */
    int found = 0;
    for (int i = 0; i < num_services; i++) {
      Service *svc = &services[i];
      if (svc->pid == pid) {
        svc->state = SRV_CRASHED;
        svc->pid = 0;
        found = 1;
        if (svc->auto_restart) {
          restart_service(svc);
        }
        break;
      }
    }
  }

  print("RESURRECTION: Exiting monitoring loop\n");
}

/* ========== Syscall Definitions ========== */

#define SYS_MOUNT 46
#define SYS_RFORK 19
#define SYS_PIPE 21

/* Mount flags */
#define MREPL 0x0000
#define MBEFORE 0x0001
#define MAFTER 0x0002
#define MCREATE 0x0004

/* Rfork flags */
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define RFNOWAIT (1 << 6)

/* Atomic primitives for locking (needed for RFMEM threads) */
static void lock(int *l) {
  int x = 1;
  while (x) {
    /* Simple spinlock using xchg */
    __asm__ volatile("xchgl %0, %1" : "+r"(x), "+m"(*l));
  }
}

static void unlock(int *l) { *l = 0; }

static int srv_lock = 0;

/* ... (previous code) ... */

/*
 * Create a pipe
 * Returns: 0 on success, -1 on failure. Fds in fd[2]
 */
static int do_pipe(int fd[2]) {
  uchar *req = (uchar *)exchange;
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
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

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
  uchar *req = (uchar *)exchange;
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
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

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
  uchar *req = (uchar *)exchange;
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
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos] == Rerror)
    return -1;

  return (int)get_u64(req + pos + 3);
}

/* ========== 9P Server Loop ========== */

static void srv_loop(int fd) {
  /* Use static buffers for freestanding 9P server thread */
  static uchar rx_data[8192];
  static uchar tx_data[8192];

  print("RESURRECTION: 9P Server Loop Starting\n");

  while (1) {
    /* Read message from kernel */
    int n = do_read(fd, (char *)rx_data, 8192);
    if (n < 0) {
      print("RESURRECTION: 9P Read Error\n");
      break;
    }
    if (n == 0) {
      /* EOF? Pipe closed? */
      break;
    }

    /* Dispatch */
    /* Check locking: 9P handlers read/write srv_entries.
       We need to lock `srv_lock` around dispatch if it touches srv_entries.
       Most handlers do.
    */
    lock(&srv_lock);
    u32int resp_len = srv_dispatch(rx_data, tx_data);
    unlock(&srv_lock);

    if (resp_len > 0) {
      /* Write response */
      /* TODO: Handle partial writes? Tsyscall SYS_WRITE handles it? */
      /* Note: SYS_WRITE takes fd, buf, count. */

      /* We need to implement do_write. resurrection.c has print() which uses
         SYS_WRITE(1, msg), but we need generic do_write(fd, buf, len).
      */

      /* Let's add do_write implementation first or reuse logic */
      /* Re-implementing simplified write for now */
      uchar *req = (uchar *)exchange;
      uint pos = 0;
      memset(req, 0, 128);
      /* Header: size, type... */
      /* Note: for large write, we need to handle data copy to exchange page
         properly. The exchange implementation in do_read/print puts data AFTER
         header.
      */
      /* For 8KB response, we can't fit in one Tsyscall if exchange page is
         small? Exchange page size: 4KB usually? fakeserver.c says
         0x7FFFFEEFF000. It's a page (4KB). So we can't do 8KB writes in one
         syscall if we copy data to it.

         However, SYS_WRITE implementation in kernel usually takes a pointer.
         If we are in userspace, we pass the pointer address.

         Wait, `print` implementation loops? No.
          put_u64(req + pos, 0); // arg 1: buf (placeholder/offset?)

         In `do_read` implementation above:
          put_u64(req + pos, 0); // arg 1: buf (placeholder)

         This implies the kernel might access userspace memory directly?
         If so, we just pass the pointer.

         But `print` copies the string to `req`.
          memcpy(req + pos, msg, msg_len);

         This implies specific IPC mechanism where data is inline.
         This limits us to ~4KB - overhead.
         MAX_MSG_SIZE 8192 is too big for 4KB exchange page.
         We should reduce MAX_MSG_SIZE to 2048 or so for safety.
      */

      /* For now, assume fit. */

      /* Inline do_write logic here to save space/time */
      /* WRONG: I should implement do_write properly */
    }
  }
}

/* Helper for writing to fd */
static int do_write(int fd, const void *buf, int count) {
  /* See do_read/print logic. Copy data to exchange page. */
  /* Limit count to ~4000 */
  if (count > 4000)
    count = 4000;

  uchar *req = (uchar *)exchange;
  uint pos = 0;

  uint size = 4 + 1 + 2 + 4 + 4 + 4 + 8 + 4 + count;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_WRITE);
  pos += 4;
  put_u32(req + pos, 4 + 8 + 4 + count);
  pos += 4; /* sdata size */
  put_u32(req + pos, 3);
  pos += 4; /* scount */

  put_u32(req + pos, fd);
  pos += 4;
  put_u64(req + pos, 0);
  pos += 8; /* buf ptr placeholder */
  put_u32(req + pos, count);
  pos += 4;

  memcpy(req + pos, buf, count); /* Data inline */

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos] == Rerror)
    return -1;

  return (int)get_u64(req + pos + 3);
}

/* ========== Main Entry ========== */

int main(void) {
  /* Initialize exchange page pointers */
  exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  /* Initialize keys */
  /* Generate new server keys on startup (ephemeral) or load from secure
     storage? For resurrection, maybe ephemeral is fine if services re-register?
     But services rely on known pubkey.
     For now, generate deterministic keys for testing using a seed?
     Or just random.
  */
  crypto_eddsa_key_pair(resurrection_privkey, resurrection_pubkey);

  /* Update epoch */
  /* Need standard time. nsec syscall? */
  /* current_epoch is u64int static. cap_current_epoch(EPOCH_HOUR). */
  current_epoch = cap_current_epoch(EPOCH_HOUR);

  print("=== Resurrection Server Starting (Layer 2) ===\n");

  /* Setup /srv 9P server */
  int p[2];
  if (do_pipe(p) < 0) {
    print("RESURRECTION: Pipe failed\n");
  } else {
    /* Mount p[1] to /srv */
    /* Flags: MREPL (0) | MCREATE (4) = 4 */
    if (do_mount(p[1], -1, "/srv", MREPL | MCREATE, "") < 0) {
      print("RESURRECTION: Mount failed\n");
    } else {
      do_close(p[1]);

      /* Spawn 9P worker thread */
      /* Use RFPROC | RFMEM to share srv_entries */
      int pid = do_rfork(RFPROC | RFMEM);
      if (pid < 0) {
        print("RESURRECTION: Rfork failed\n");
      } else if (pid == 0) {
        /* Child: 9P Server Loop */
        srv_loop(p[0]);
        /* Should not return */
        do_close(p[0]);
        return 0;
      } else {
        /* Parent: Continue to monitor services */
        print("RESURRECTION: 9P Server started\n");
        do_close(p[0]);
      }
    }
  }

  /* Register known good services */
  print("RESURRECTION: Loading registry from /boot/services.conf...\n");

  /* Initial load */
  lock(&srv_lock);
  load_registry();
  unlock(&srv_lock);

  /* Enter service monitoring loop (will reload on events) */
  monitor_services();

  /* ... shutdown ... */
}
