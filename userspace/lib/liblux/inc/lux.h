#ifndef _LIBLUX_H_
#define _LIBLUX_H_

#include <stdarg.h> // For va_list
#include <stddef.h> // For NULL

/* Basic userspace types (compatible with kernel types from u.h/portlib.h) */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;

typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;

typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;

typedef u32int Rune; /* UTF-8 code point */

#ifndef nil
#define nil ((void *)0)
#endif

typedef struct {
  u8int data[16];
} uuid_t;

#ifndef offsetof
#define offsetof(s, m) ((ulong)(&(((s *)0)->m)))
#endif

static inline void uuid_copy(uuid_t *dst, const uuid_t *src) {
  for (int i = 0; i < 16; i++)
    dst->data[i] = src->data[i];
}

// Plan 9 standard constants
#define MAXWELEM 16
#define P9_MSG_SIZE 0xF00 /* 3840 bytes for message */
#define P9_REQUEST_OFFSET 0x000
#define P9_CONTROL_OFFSET 0xF00
#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL

extern uintptr lux_exchange_base;
static inline uintptr lux_exchange_page(void) {
  uintptr base = lux_exchange_base != 0 ? lux_exchange_base : EXCHANGE_PAGE_ADDR;
  volatile u32int *trace = (u32int *)(base + P9_MSG_SIZE - 16);
  trace[0] = 0x3050584C; /* "LXP0" */
  trace[1] = (u32int)base;
  return base;
}

/* Userspace compatible Qid structure */
typedef struct Qid {
  uchar type;
  u32int vers;
  u64int path;
} Qid;

/* Ring IPC Definitions */
#define RING_SIZE 128
#define RING_MASK 127

typedef struct IpcPageRing {
  volatile u32int head;
  volatile u32int tail;
  u32int mask;
  u32int flags;
  u64int pages[RING_SIZE];
} IpcPageRing;

typedef struct IpcChannel {
  u32int magic; /* 0x52494E47 "RING" */
  u32int status;
  IpcPageRing submission;
  IpcPageRing completion;
} IpcChannel;

#define BATCH_PAGE_MAGIC 0xB47C4831

typedef struct BatchHeader {
  u16int num_messages;
  u16int used_bytes;
  u32int magic;
  u64int nonce;
  uuid_t uuids[120]; /* Per-message caller identity */
} BatchHeader;

#define BATCH_DATA_START sizeof(BatchHeader)

/* Userspace compatible Dir structure */
#ifndef _DIR_H_
#define _DIR_H_
typedef struct Dir {
  ushort type;
  uint dev;
  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
} Dir;
#endif

/* Userspace compatible Fcall structure */
typedef struct Fcall {
  uchar type;
  u32int fid;
  ushort tag;
  union {
    struct {
      u32int msize;
      char *version;
    };
    struct {
      ushort oldtag;
    };
    struct {
      char *ename;
    };
    struct {
      Qid qid;
      u32int iounit;
    };
    struct {
      Qid aqid;
    };
    struct {
      u32int afid;
      char *uname;
      char *aname;
    };
    struct {
      u32int perm;
      char *name;
      uchar mode;
    };
    struct {
      u32int newfid;
      ushort nwname;
      char *wname[MAXWELEM];
    };
    struct {
      ushort nwqid;
      Qid wqid[MAXWELEM];
    };
    struct {
      vlong offset;
      u32int count;
      char *data;
    };
    struct {
      ushort nstat;
      uchar *stat;
    };
    struct {
      u32int scallnr;
      u32int sflags;
      uchar *sdata;
      u32int scount;
      u64int retval;
    };
    struct {
      u32int flags;
      u32int pid;
    };
    struct {
      char *path;
      char **argv;
      u32int argc;
      char *args[16];
    };
    struct {
      u64int addr;
    };
    struct {
      char *oldpath;
      u32int fd;
    };
    struct {
      u32int fid0;
      u32int fid1;
    };
    struct {
      int whence;
    };
    struct {
      u64int handler;
    };
  };
} Fcall;

// Fcall message types
enum {
  Tversion = 100,
  Rversion,
  Tauth = 102,
  Rauth,
  Tattach = 104,
  Rattach,
  Terror = 106, /* illegal */
  Rerror,
  Tflush = 108,
  Rflush,
  Twalk = 110,
  Rwalk,
  Topen = 112,
  Ropen,
  Tcreate = 114,
  Rcreate,
  Tread = 116,
  Rread,
  Twrite = 118,
  Rwrite,
  Tclunk = 120,
  Rclunk,
  Tremove = 122,
  Rremove,
  Tstat = 124,
  Rstat,
  Twstat = 126,
  Rwstat,
  Tmax,

  /* Lux9 custom message types */
  Texec = 128,
  Rexec,

  /* Lux9 syscall message types */
  Tsyscall = 130,
  Rsyscall,

  /* Specific syscall wrappers */
  Tsysopen = 132,
  Rsysopen,
  Tsyscreate = 134,
  Rsyscreate,
  Tsysread = 136,
  Rsysread,
  Tsyswrite = 138,
  Rsyswrite,
  Tsysclose = 140,
  Rsysclose,
  Tsyspread = 142,
  Rsyspread,
  Tsyspwrite = 144,
  Rsyspwrite,
  Tsysremove = 146,
  Rsysremove,

  Tsysstat = 148,
  Rsysstat,
  Tsysfstat = 150,
  Rsysfstat,
  Tsyswstat = 152,
  Rsyswstat,
  Tsysfwstat = 154,
  Rsysfwstat,

  Tsysfork = 160,
  Rsysfork,
  Tsysexec = 162,
  Rsysexec,
  Tsysexit = 164,
  Rsysexit,
  Tsyswait = 166,
  Rsyswait,
  Tsysbrk = 168,
  Rsysbrk,
  Tsyssleep = 170,
  Rsyssleep,
  Tsysspawn = 172, /* spawn(path, argv) -> pid - SECURE fork+exec primitive */
  Rsysspawn,
  /*
   * Compatibility aliases: some callers/documentation use Tsyspawn/Rsyspawn.
   * Keep both spellings mapped to the same wire values.
   */
  Tsyspawn = Tsysspawn,
  Rsyspawn = Rsysspawn,

  Tsysbind = 180,
  Rsysbind,
  Tsysmount = 182,
  Rsysmount,
  Tsysunmount = 184,
  Rsysunmount,
  Tsyschdir = 186,
  Rsyschdir,

  Tsysdup = 190,
  Rsysdup,
  Tsyspipe = 192,
  Rsyspipe,
  Tsysfd2path = 194,
  Rsysfd2path,

  Tsysseek = 200,
  Rsysseek,
  Tsysnotify = 202,
  Rsysnotify,
  Tsysalarm = 204,
  Rsysalarm,

  Tsysmax,
};

/* Syscall Numbers */
enum {
  SYS_OPEN = 1,
  SYS_CLOSE,
  SYS_READ,
  SYS_WRITE,
  SYS_PREAD,
  SYS_PWRITE,
  SYS_CREATE,
  SYS_REMOVE = 25,
  SYS_SEGATTACH = 30,
  SYS_EXIT,
  SYS_FORK,
  SYS_STAT,
  SYS_WSTAT,
  SYS_RFORK = 19,
  SYS_PIPE = 21,
  SYS_SEEK = 39,
  SYS_MOUNT = 46,
  SYS_NSEC = 53,
  SYS_BRK = 55,
  SYS_WASM_COMPILE = 160,
  SYS_WASM_EXECUTE = 161,
  SYS_WASM_DESTROY = 162,
  SYS_GETPID2 = 66,
  SYS_EXCHANGE_ALLOC = 67,
  SYS_EXCHANGE_FREE = 68,
  SYS_EXCHANGE_PUBLISH = 69,
  SYS_EXCHANGE_SUBSCRIBE = 70,
  SYS_EXCHANGE_UNSUBSCRIBE = 71,
  SYS_EXCHANGE_RECEIVE = 72,
  SYS_NSROOT_PUBLISH = 73,
  SYS_NSROOT_UNPUBLISH = 74,
  SYS_EXCHANGE_PREPARE = 75,
  SYS_EXCHANGE_PREPARE_RANGE = 76,
  SYS_EXCHANGE_ACCEPT = 77,
  SYS_EXCHANGE_CANCEL = 78,
  SYS_EXCHANGE_TRANSFER = 79,
  SYS_WAIT = 166
};

/* rfork flags */
enum {
  RFNAMEG = (1 << 0),
  RFENVG = (1 << 1),
  RFFDG = (1 << 2),
  RFNOTEG = (1 << 3),
  RFPROC = (1 << 4),
  RFMEM = (1 << 5),
  RFNOWAIT = (1 << 6),
  RFCNAMEG = (1 << 10),
  RFCENVG = (1 << 11),
  RFCFDG = (1 << 12),
  RFREND = (1 << 13),
  RFNOMNT = (1 << 14)
};

typedef struct SpawnxFdAction {
  int op;
  int fd;
  int arg;
} SpawnxFdAction;

typedef struct SpawnxBindAction {
  char *oldpath;
  char *newpath;
  int flags;
} SpawnxBindAction;

typedef struct SpawnxSpec {
  int rfork_flags;

  SpawnxFdAction *fd_actions;
  u32int nfd_actions;

  SpawnxBindAction *bind_actions;
  u32int nbind_actions;

  int do_mount;
  int mount_fd;
  int mount_afd;
  char *mount_old;
  int mount_flags;
  char *mount_aname;
} SpawnxSpec;

enum {
  SPAWNX_FD_CLOSE = 1,
  SPAWNX_FD_DUP2 = 2
};

// Basic Pebble types
typedef struct PebbleWhite {
  ulong token;
  uintptr size;
} PebbleWhite;

typedef struct PebbleBlue {
  ulong tokens;
} PebbleBlue;

/* Exchange Pool Types */
#ifndef LUX_EXCHANGE_TYPES_DEFINED
#define LUX_EXCHANGE_TYPES_DEFINED
typedef struct {
  uchar uuid[16];
  uchar hash[32];
  ulong size;
  uint type;
  uint perms;
} ExchangeCapability;

/* IPC Notification Structure */
typedef struct {
  uchar message_id[16];
  uchar topic_uuid[16];
  ExchangeCapability *capability;
  int delivered_count;
  int ack_count;
} Notification;
#endif

#define EXCHANGE_PAGE_SIZE 0x1000UL

typedef struct ExchangePagePool {
  uchar *base;
  ulong len;
  ulong npages;
} ExchangePagePool;

/* Userspace Syscall Wrappers */
int sys_open(char *path, int mode);
int sys_close(int fd);
long sys_read(int fd, void *buf, long n);
long sys_write(int fd, void *buf, long n);
long sys_pwrite(int fd, void *buf, long n, long offset);
long sys_pread(int fd, void *buf, long n, long offset);
void sys_exit(char *msg);
int sys_create(char *path, int mode, uint perm);
int sys_rfork(int flags);
int sys_rfork_stack(int flags, void *stack_top, void (*func)(void *), void *arg);
int sys_exec(char *path, char *argv[]);
int sys_spawn(char *path, char *argv[]);
int sys_spawnx(char *path, char *argv[], SpawnxSpec *spec);
char *sys_exec_error(void);
int sys_pipe(int *fds);
long sys_seek(int fd, long offset, int whence);
int sys_wait(void);
uvlong sys_nsec(void);
int sys_stat(char *path, uchar *buf, int nbuf);
int sys_wstat(char *path, uchar *buf, int nbuf);
int sys_mount(int fd, int afd, char *old, int flags, char *aname);
int sys_srv_publish(char *path, int fd);
int sys_sleep(long ms);
int sys_getpid2(uuid_t *out, ulong len);
int sys_bind(char *old, char *newname, int flags);
int sys_dup(int oldfd, int newfd);
void *segattach(int attr, char *spec, void *addr, ulong len);

int sys_print(const char *fmt, ...);
int snprint(char *buf, int len, const char *fmt, ...);
int vsnprint(char *buf, int len, const char *fmt, va_list args);

/* Exchange Pool Syscalls */
int sys_exchange_prepare(uintptr vaddr, ExchangeCapability *out_cap);
int sys_exchange_prepare_range(uintptr vaddr, ulong len,
                               ExchangeCapability *handles);
int sys_exchange_accept(const ExchangeCapability *cap, uintptr dest_vaddr,
                        int prot);
int sys_exchange_cancel(const ExchangeCapability *cap);
int sys_exchange_transfer(int from_pid, int to_pid,
                          const ExchangeCapability *cap, uintptr dest_vaddr);
ExchangeCapability *sys_exchange_alloc(void);
int sys_exchange_free(ExchangeCapability *cap);
ExchangeCapability *sys_exchange_publish(char *topic, void *data, ulong len);
int sys_exchange_subscribe(char *topic);
int sys_exchange_unsubscribe(char *topic);
Notification *sys_exchange_receive(void);

/* Exchange helpers */
int exchange_prepare(uintptr vaddr, ExchangeCapability *out_cap);
int exchange_prepare_range(uintptr vaddr, ulong len,
                           ExchangeCapability *handles);
int exchange_accept(const ExchangeCapability *cap, uintptr dest_vaddr,
                    int prot);
int exchange_cancel(const ExchangeCapability *cap);
int exchange_transfer(int from_pid, int to_pid,
                      const ExchangeCapability *cap, uintptr dest_vaddr);
int exchange_pool_init(ExchangePagePool *pool, void *base, ulong len);
ulong exchange_pool_npages(const ExchangePagePool *pool);
void *exchange_pool_page(const ExchangePagePool *pool, ulong index);
int exchange_pool_index(const ExchangePagePool *pool, uintptr addr,
                        ulong *out_index);
int exchange_pool_prepare(const ExchangePagePool *pool, ulong index,
                          ExchangeCapability *out_cap);
int exchange_pool_prepare_addr(const ExchangePagePool *pool, uintptr addr,
                               ExchangeCapability *out_cap);
int exchange_pool_accept(const ExchangePagePool *pool, ulong index,
                         const ExchangeCapability *cap, int prot);
int exchange_pool_accept_addr(const ExchangePagePool *pool, uintptr addr,
                              const ExchangeCapability *cap, int prot);

/* MsgOrd / Kinetic Defense */
typedef struct {
  uchar type;
  ushort tag;
  u32int scallnr;
  u32int sflags;
  uchar *sdata;
  u32int scount;
} MsgOrdFcall;
int msgord_submit(char *path, MsgOrdFcall *t);

int pow_solve(u64int context, int difficulty, u64int *nonce_out);

/* Pebble */
int pebble_alloc(ulong size, void **addr);
int pebble_free(void *addr);
static inline void *pebble_alloc_zero(ulong size) {
  void *p;
  if (pebble_alloc(size, &p) < 0)
    return nil;
  for (ulong i = 0; i < size; i++)
    ((uchar *)p)[i] = 0;
  return p;
}
int pebble_increase_budget(ulong size, u64int nonce);

/* Standard string functions (implemented in liblux/src/string.c) */
void *memset(void *dst, int c, ulong n);
void *memcpy(void *dst, const void *src, ulong n);
void *memmove(void *dst, const void *src, ulong n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, ulong n);
int memcmp(const void *s1, const void *s2, ulong n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, ulong n);
ulong strlen(const char *s);
char *strdup(const char *s);
int atoi(const char *s);

/* 9P Conversion functions (implemented in liblux/conv*) */
uint convM2S(uchar *, uint, Fcall *);
uint convS2M(Fcall *, uchar *, uint);
uint convD2M(Dir *, uchar *, uint);
Dir *convM2D(uchar *, uint, Dir *, char *);
uint sizeS2M(Fcall *);
uint sizeD2M(Dir *);

#endif
