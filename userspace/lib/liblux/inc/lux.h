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

#ifndef offsetof
#define offsetof(s, m) ((ulong)(&(((s *)0)->m)))
#endif

// Plan 9 standard constants
#define MAXWELEM 16
#define P9_MSG_SIZE 0xF00 /* 3840 bytes for message */
#define P9_REQUEST_OFFSET 0x000
#define P9_CONTROL_OFFSET 0xF00
#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL

/* Userspace compatible Qid structure */
typedef struct Qid {
  uchar type;
  u32int vers;
  u64int path;
} Qid;

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
  SYS_WAIT = 166
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
void sys_exec(char *path, char *argv[]);
int sys_pipe(int *fds);
long sys_seek(int fd, long offset, int whence);
int sys_wait(void);
uvlong sys_nsec(void);
int sys_stat(char *path, uchar *buf, int nbuf);
int sys_wstat(char *path, uchar *buf, int nbuf);
int sys_mount(int fd, int afd, char *old, int flags, char *aname);
int sys_sleep(long ms);
int sys_getpid2(void *out, ulong len);
int sys_bind(char *old, char *newname, int flags);

int sys_print(const char *fmt, ...);

/* Exchange Pool Syscalls */
ExchangeCapability *sys_exchange_alloc(void);
int sys_exchange_free(ExchangeCapability *cap);
ExchangeCapability *sys_exchange_publish(char *topic, void *data, ulong len);
int sys_exchange_subscribe(char *topic);
int sys_exchange_unsubscribe(char *topic);
Notification *sys_exchange_receive(void);

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

#endif