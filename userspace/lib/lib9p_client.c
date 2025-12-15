/*
 * lib9p_client.c - Pure 9P Client Library for Lux9
 *
 * Direct exchange page access implementation.
 * Writes Fcall messages directly to the exchange page and
 * rings doorbell to trigger kernel processing.
 */

#include "lib9p_client.h"
#include <stdarg.h>
#include <u.h>

/* Explicit prototypes matching libc.h/fcall.h EXACTLY to avoid conflicts */
extern void *memmove(void *dest, void *src, ulong n); /* const removed */
extern void *memset(void *s, int c, ulong n);
extern char *strncpy(char *dest, char *src, long n); /* long, not ulong */
extern char *strcpy(char *dest, char *src);          /* const removed */
extern char *strrchr(char *s, int c);                /* const removed */

extern uint convM2S(uchar *ap, uint nap, Fcall *f);
extern uint convS2M(Fcall *f, uchar *ap, uint nap);
extern uint convM2D(uchar *ap, uint nap, Dir *d, char *strs);
extern uint convD2M(Dir *d, uchar *ap, uint nap);

/* Exchange page pointers (set by p9_init) */
static volatile P9Control *ctl;
static uchar *req_buf;
static uchar *rep_buf;

/* Fid allocation */
static u32int next_fid = 1;
static u32int fid_alloc_bitmap[8] = {0}; /* 256 fids max */

/* Error string buffer */
static char errstr_buf[128] = "";

/* Tag counter */
static ushort next_tag = 1;

/*
 * Trigger kernel processing via doorbell
 * Uses a lightweight syscall instruction to notify kernel
 */
static void ring_doorbell(void) {
  ctl->doorbell = 1;
  /*
   * Trigger trap to kernel via memory fence + hint
   * The kernel's trap handler checks doorbell on every syscall entry
   */
  __asm__ volatile("mfence\n\t"
                   "int $0x40" /* Custom doorbell interrupt vector */
                   ::
                       : "memory");
}

/*
 * Wait for kernel to complete processing
 */
static int wait_complete(void) {
  volatile int timeout = 1000000;

  while (ctl->status != P9_STATUS_COMPLETE && ctl->status != P9_STATUS_ERROR) {
    if (--timeout == 0) {
      strcpy(errstr_buf, "9P timeout");
      return -1;
    }
    __asm__ volatile("pause" ::: "memory");
  }

  if (ctl->status == P9_STATUS_ERROR) {
    strcpy(errstr_buf, "9P error");
    return -1;
  }

  return 0;
}

/*
 * Transact a single 9P message
 * Returns 0 on success, -1 on error
 */
static int p9_transact(Fcall *t, Fcall *r) {
  uint n;

  /* Assign tag */
  t->tag = next_tag++;
  if (next_tag == NOTAG)
    next_tag = 1;

  /* Serialize request to exchange page */
  n = convS2M(t, req_buf, P9_REQUEST_SIZE);
  if (n == 0) {
    strcpy(errstr_buf, "convS2M failed");
    return -1;
  }

  /* Update control block */
  ctl->req_head = 0;
  ctl->req_tail = n;
  ctl->req_seq++;
  ctl->status = P9_STATUS_IDLE;

  /* Ring doorbell to notify kernel */
  ring_doorbell();

  /* Wait for completion */
  if (wait_complete() < 0)
    return -1;

  /* Parse response */
  memset(r, 0, sizeof(*r));
  if (convM2S(rep_buf + ctl->rep_head, ctl->rep_tail - ctl->rep_head, r) == 0) {
    strcpy(errstr_buf, "convM2S failed");
    return -1;
  }

  /* Check for error response */
  if (r->type == Rerror) {
    if (r->ename)
      strncpy(errstr_buf, r->ename, sizeof(errstr_buf) - 1);
    else
      strcpy(errstr_buf, "unknown error");
    return -1;
  }

  return 0;
}

/*
 * Initialize 9P client
 */
void p9_init(void) {
  /* Exchange page is pre-mapped by kernel at fixed address */
  uintptr base = EXCHANGE_PAGE_ADDR;

  ctl = (volatile P9Control *)(base + P9_CONTROL_OFFSET);
  req_buf = (uchar *)(base + P9_REQUEST_OFFSET);
  rep_buf = (uchar *)(base + P9_REPLY_OFFSET);

  /* Reset state */
  next_fid = 1;
  next_tag = 1;
  memset(fid_alloc_bitmap, 0, sizeof(fid_alloc_bitmap));
  errstr_buf[0] = 0;
}

/*
 * Allocate a new fid
 */
int p9_allocfid(void) {
  for (int i = 0; i < 256; i++) {
    int word = i / 32;
    int bit = i % 32;
    if ((fid_alloc_bitmap[word] & (1U << bit)) == 0) {
      fid_alloc_bitmap[word] |= (1U << bit);
      return i + 1; /* Fid 0 reserved */
    }
  }
  strcpy(errstr_buf, "no free fids");
  return -1;
}

/*
 * Free a fid
 */
void p9_freefid(int fid) {
  if (fid < 1 || fid > 256)
    return;
  int idx = fid - 1;
  int word = idx / 32;
  int bit = idx % 32;
  fid_alloc_bitmap[word] &= ~(1U << bit);
}

/*
 * Attach to a path
 * Returns fid on success, -1 on error
 */
int p9_attach(char *path) {
  Fcall t = {0}, r = {0};
  int fid;

  fid = p9_allocfid();
  if (fid < 0)
    return -1;

  t.type = Tattach;
  t.fid = (u32int)fid;
  t.afid = NOFID;
  t.uname = ""; /* Pebble session carries identity */
  t.aname = path;

  if (p9_transact(&t, &r) < 0) {
    p9_freefid(fid);
    return -1;
  }

  return fid;
}

/*
 * Walk from fid to name, storing result in newfid
 * Returns 0 on success, -1 on error
 */
int p9_walk(int fid, char *name, int newfid) {
  Fcall t = {0}, r = {0};

  t.type = Twalk;
  t.fid = (u32int)fid;
  t.newfid = (u32int)newfid;

  if (name && name[0]) {
    t.nwname = 1;
    t.wname[0] = name;
  } else {
    t.nwname = 0;
  }

  if (p9_transact(&t, &r) < 0)
    return -1;

  return 0;
}

/*
 * Open a fid
 * Returns iounit on success, -1 on error
 */
int p9_open(int fid, int mode) {
  Fcall t = {0}, r = {0};

  t.type = Topen;
  t.fid = (u32int)fid;
  t.mode = (uchar)mode;

  if (p9_transact(&t, &r) < 0)
    return -1;

  return (int)r.iounit;
}

/*
 * Create a file
 * Returns 0 on success, -1 on error
 */
int p9_create(int fid, char *name, int perm, int mode) {
  Fcall t = {0}, r = {0};

  t.type = Tcreate;
  t.fid = (u32int)fid;
  t.name = name;
  t.perm = (u32int)perm;
  t.mode = (uchar)mode;

  if (p9_transact(&t, &r) < 0)
    return -1;

  return 0;
}

/*
 * Read from fid at offset
 * Returns bytes read, -1 on error
 */
long p9_read(int fid, void *buf, long count, vlong offset) {
  Fcall t = {0}, r = {0};

  if (count > P9_REPLY_SIZE - IOHDRSZ)
    count = P9_REPLY_SIZE - IOHDRSZ;

  t.type = Tread;
  t.fid = (u32int)fid;
  t.offset = offset;
  t.count = (u32int)count;

  if (p9_transact(&t, &r) < 0)
    return -1;

  /* Copy data to user buffer */
  if (r.count > 0 && r.data)
    memmove(buf, r.data, r.count);

  return (long)r.count;
}

/*
 * Write to fid at offset
 * Returns bytes written, -1 on error
 */
long p9_write(int fid, void *buf, long count, vlong offset) {
  Fcall t = {0}, r = {0};

  if (count > P9_REQUEST_SIZE - IOHDRSZ)
    count = P9_REQUEST_SIZE - IOHDRSZ;

  t.type = Twrite;
  t.fid = (u32int)fid;
  t.offset = offset;
  t.count = (u32int)count;
  t.data = (char *)buf;

  if (p9_transact(&t, &r) < 0)
    return -1;

  return (long)r.count;
}

/*
 * Stat a fid
 * Returns 0 on success, -1 on error
 */
int p9_stat(int fid, Dir *d) {
  Fcall t = {0}, r = {0};

  t.type = Tstat;
  t.fid = (u32int)fid;

  if (p9_transact(&t, &r) < 0)
    return -1;

  /* Parse stat buffer into Dir */
  if (r.stat && r.nstat > 0) {
    char strs[256];
    if (convM2D((uchar *)r.stat, r.nstat, d, strs) == 0) {
      strcpy(errstr_buf, "convM2D failed");
      return -1;
    }
  }

  return 0;
}

/*
 * Write stat to fid
 * Returns 0 on success, -1 on error
 */
int p9_wstat(int fid, Dir *d) {
  Fcall t = {0}, r = {0};
  uchar statbuf[256];
  uint n;

  n = convD2M(d, statbuf, sizeof(statbuf));
  if (n == 0) {
    strcpy(errstr_buf, "convD2M failed");
    return -1;
  }

  t.type = Twstat;
  t.fid = (u32int)fid;
  t.stat = (void *)statbuf;
  t.nstat = (ushort)n;

  if (p9_transact(&t, &r) < 0)
    return -1;

  return 0;
}

/*
 * Remove file at fid
 * Returns 0 on success, -1 on error
 */
int p9_remove(int fid) {
  Fcall t = {0}, r = {0};

  t.type = Tremove;
  t.fid = (u32int)fid;

  if (p9_transact(&t, &r) < 0)
    return -1;

  /* Fid is implicitly clunked after remove */
  p9_freefid(fid);

  return 0;
}

/*
 * Clunk (close) a fid
 */
void p9_clunk(int fid) {
  Fcall t = {0}, r = {0};

  t.type = Tclunk;
  t.fid = (u32int)fid;

  p9_transact(&t, &r); /* Ignore errors */
  p9_freefid(fid);
}

/*
 * Get last error string
 */
char *p9_errstr(void) { return errstr_buf; }

/* === High-level convenience functions === */

/*
 * Open a file by path
 * Returns fid on success, -1 on error
 */
int p9_openfile(char *path, int mode) {
  int fid;

  fid = p9_attach(path);
  if (fid < 0)
    return -1;

  if (p9_open(fid, mode) < 0) {
    p9_clunk(fid);
    return -1;
  }

  return fid;
}

/*
 * Create a file at path
 * Returns fid on success, -1 on error
 */
int p9_createfile(char *path, int perm, int mode) {
  int fid, dirfid;
  char *name, *dir;
  char pathbuf[256];

  /* Split path into directory and name */
  strncpy(pathbuf, path, sizeof(pathbuf) - 1);
  pathbuf[sizeof(pathbuf) - 1] = 0;

  name = strrchr(pathbuf, '/');
  if (name) {
    *name++ = 0;
    dir = pathbuf;
    if (*dir == 0)
      dir = "/";
  } else {
    name = pathbuf;
    dir = ".";
  }

  /* Attach to directory */
  dirfid = p9_attach(dir);
  if (dirfid < 0)
    return -1;

  /* Create file */
  if (p9_create(dirfid, name, perm, mode) < 0) {
    p9_clunk(dirfid);
    return -1;
  }

  return dirfid; /* After create, fid is open to new file */
}

/*
 * Read entire file by path
 * Returns bytes read, -1 on error
 */
long p9_readfile(char *path, void *buf, long count) {
  int fid;
  long n;

  fid = p9_openfile(path, 0); /* OREAD = 0 */
  if (fid < 0)
    return -1;

  n = p9_read(fid, buf, count, 0);
  p9_clunk(fid);

  return n;
}

/*
 * Write to file by path
 * Returns bytes written, -1 on error
 */
long p9_writefile(char *path, void *buf, long count) {
  int fid;
  long n;

  fid = p9_openfile(path, 1); /* OWRITE = 1 */
  if (fid < 0)
    return -1;

  n = p9_write(fid, buf, count, 0);
  p9_clunk(fid);

  return n;
}
