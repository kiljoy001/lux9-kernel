#ifndef _FCALL_H_
#define _FCALL_H_

#ifndef __FRAMAC__
#pragma src "/sys/src/libc/9sys"
#pragma lib "libc.a"
#endif

#define VERSION9P "9P2000"

#define MAXWELEM 16

typedef struct Fcall {
  uchar type;
  u32int fid;
  ushort tag;
  union {
    struct {
      u32int msize;  /* Tversion, Rversion */
      char *version; /* Tversion, Rversion */
    };
    struct {
      ushort oldtag; /* Tflush */
    };
    struct {
      char *ename; /* Rerror */
    };
    struct {
      Qid qid;       /* Rattach, Ropen, Rcreate */
      u32int iounit; /* Ropen, Rcreate */
    };
    struct {
      Qid aqid; /* Rauth */
    };
    struct {
      u32int afid; /* Tauth, Tattach */
      char *uname; /* Tauth, Tattach */
      char *aname; /* Tauth, Tattach */
    };
    struct {
      u32int perm; /* Tcreate */
      char *name;  /* Tcreate */
      uchar mode;  /* Tcreate, Topen */
    };
    struct {
      u32int newfid;         /* Twalk */
      ushort nwname;         /* Twalk */
      char *wname[MAXWELEM]; /* Twalk */
    };
    struct {
      ushort nwqid;       /* Rwalk */
      Qid wqid[MAXWELEM]; /* Rwalk */
    };
    struct {
      vlong offset; /* Tread, Twrite */
      u32int count; /* Tread, Twrite, Rread */
      char *data;   /* Twrite, Rread */
    };
    struct {
      ushort nstat; /* Twstat, Rstat */
      uchar *stat;  /* Twstat, Rstat */
    };
    struct {
      u32int scallnr; /* Tsyscall */
      u32int sflags;  /* Tsyscall flags/category */
      uchar *sdata;   /* Tsyscall, Rsyscall */
      u32int scount;  /* Tsyscall, Rsyscall */
      u64int retval;  /* Rsyscall */
    };
    /* Tsys* message fields */
    struct {
      u32int flags; /* Tsysfork (rfork flags), Tsysbind, Tsysmount */
      u32int pid;   /* Rsysfork, Rsyswait */
    };
    struct {
      char *path;  /* Tsysexec - executable path */
      char **argv; /* Tsysexec - argument array */
      u32int argc; /* Tsysexec - argument count */
    };
    struct {
      u64int addr; /* Tsysbrk, Rsysbrk - memory address */
    };
    struct {
      char *oldpath; /* Tsysbind, Tsysmount, Tsysunmount - old path */
      u32int fd;     /* Tsysmount - file descriptor */
    };
    struct {
      u32int fid0; /* Rsyspipe - first pipe fid */
      u32int fid1; /* Rsyspipe - second pipe fid */
    };
    struct {
      int whence; /* Tsysseek - seek type (SEEK_SET, etc.) */
    };
    struct {
      u64int handler; /* Tsysnotify - notification handler address */
    };
  };
} Fcall;

#define GBIT8(p) (((uchar *)(p))[0])
#define GBIT16(p) (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8))
#define GBIT32(p)                                                              \
  (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) |  \
   (((uchar *)(p))[3] << 24))
#define GBIT64(p)                                                              \
  ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) |                     \
            (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) |           \
   ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) |                    \
             (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24))            \
    << 32))

#define PBIT8(p, v)                                                            \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
  } while (0)
#define PBIT16(p, v)                                                           \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
    ((uchar *)(p))[1] = (uchar)((v) >> 8);                                     \
  } while (0)
#define PBIT32(p, v)                                                           \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
    ((uchar *)(p))[1] = (uchar)((v) >> 8);                                     \
    ((uchar *)(p))[2] = (uchar)((v) >> 16);                                    \
    ((uchar *)(p))[3] = (uchar)((v) >> 24);                                    \
  } while (0)
#define PBIT64(p, v)                                                           \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
    ((uchar *)(p))[1] = (uchar)((v) >> 8);                                     \
    ((uchar *)(p))[2] = (uchar)((v) >> 16);                                    \
    ((uchar *)(p))[3] = (uchar)((v) >> 24);                                    \
    ((uchar *)(p))[4] = (uchar)((v) >> 32);                                    \
    ((uchar *)(p))[5] = (uchar)((v) >> 40);                                    \
    ((uchar *)(p))[6] = (uchar)((v) >> 48);                                    \
    ((uchar *)(p))[7] = (uchar)((v) >> 56);                                    \
  } while (0)

#define BIT8SZ 1
#define BIT16SZ 2
#define BIT32SZ 4
#define BIT64SZ 8
#define QIDSZ (BIT8SZ + BIT32SZ + BIT64SZ)

/* STATFIXLEN includes leading 16-bit count */
/* The count, however, excludes itself; total size is BIT16SZ+count */
#define STATFIXLEN                                                             \
  (BIT16SZ + QIDSZ + 5 * BIT16SZ + 4 * BIT32SZ +                               \
   1 * BIT64SZ) /* amount of fixed length data in a stat buffer */

#define NOTAG (ushort) ~0U /* Dummy tag */
#define NOFID (u32int) ~0U /* Dummy fid */
#define IOHDRSZ 24         /* ample room for Twrite/Rread header (iounit) */

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

  /* Lux9 syscall message types - for pure 9P message passing */
  /* Generic Syscall Message: Tsyscall (130) - kept for backwards compatibility
   */
  Tsyscall = 130,
  Rsyscall,

  /* Specific syscall wrappers - provide syscall-like semantics over 9P */
  /* I/O Operations */
  Tsysopen = 132, /* open(path, mode) -> fid */
  Rsysopen,
  Tsyscreate = 134, /* create(path, perm, mode) -> fid */
  Rsyscreate,
  Tsysread = 136, /* read(fid, offset, count) -> data */
  Rsysread,
  Tsyswrite = 138, /* write(fid, offset, data) -> count */
  Rsyswrite,
  Tsysclose = 140, /* close(fid) */
  Rsysclose,
  Tsyspread = 142, /* pread(fid, offset, count) -> data */
  Rsyspread,
  Tsyspwrite = 144, /* pwrite(fid, offset, data) -> count */
  Rsyspwrite,
  Tsysremove = 146, /* remove(path) */
  Rsysremove,

  /* File Info Operations */
  Tsysstat = 148, /* stat(path) -> Dir */
  Rsysstat,
  Tsysfstat = 150, /* fstat(fid) -> Dir */
  Rsysfstat,
  Tsyswstat = 152, /* wstat(path, Dir) */
  Rsyswstat,
  Tsysfwstat = 154, /* fwstat(fid, Dir) */
  Rsysfwstat,

  /* Process Control */
  Tsysfork = 160, /* rfork(flags) -> pid */
  Rsysfork,
  Tsysexec = 162, /* exec(path, argv) */
  Rsysexec,
  Tsysexit = 164, /* exits(status) */
  Rsysexit,
  Tsyswait = 166, /* wait() -> Waitmsg */
  Rsyswait,
  Tsysbrk = 168, /* brk(addr) -> addr */
  Rsysbrk,
  Tsyssleep = 170, /* sleep(millisecs) */
  Rsyssleep,

  /* Namespace Operations */
  Tsysbind = 180, /* bind(name, old, flags) */
  Rsysbind,
  Tsysmount = 182, /* mount(fd, afd, old, flags, aname) */
  Rsysmount,
  Tsysunmount = 184, /* unmount(name, old) */
  Rsysunmount,
  Tsyschdir = 186, /* chdir(path) */
  Rsyschdir,

  /* FD Operations */
  Tsysdup = 190, /* dup(oldfd, newfd) -> fid */
  Rsysdup,
  Tsyspipe = 192, /* pipe(fd[2]) -> fid[2] */
  Rsyspipe,
  Tsysfd2path = 194, /* fd2path(fid) -> path */
  Rsysfd2path,

  /* Misc Operations */
  Tsysseek = 200, /* seek(fid, offset, type) -> offset */
  Rsysseek,
  Tsysnotify = 202, /* notify(handler) */
  Rsysnotify,
  Tsysalarm = 204, /* alarm(millisecs) -> previous */
  Rsysalarm,

  Tsysmax,
};

uint convM2S(uchar *, uint, Fcall *);
uint convS2M(Fcall *, uchar *, uint);
uint sizeS2M(Fcall *);

int statcheck(uchar *abuf, uint nbuf);
uint convM2D(uchar *, uint, Dir *, char *);
uint convD2M(Dir *, uchar *, uint);
uint sizeD2M(Dir *);

int fcallfmt(Fmt *);
int dirfmt(Fmt *);
int dirmodefmt(Fmt *);

int read9pmsg(int, void *, uint);

#ifndef __FRAMAC__
#pragma varargck type "F" Fcall *
#pragma varargck type "M" ulong
#pragma varargck type "D" Dir *
#endif

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

#endif /* _FCALL_H_ */
