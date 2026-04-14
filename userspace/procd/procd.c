#include <server9p.h>
#include <stdarg.h>

/* Missing constants from lux.h/libc.h */
#define QTDIR 0x80
#define QTFILE 0x00
#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define MREPL 0
#define MCREATE 4
#define SEEKSET 0
#define DMDIR 0x80000000U

/* Forward declarations */
static void p9_attach(Req *r);
static void p9_walk(Req *r);
static void p9_open(Req *r);
static void p9_create(Req *r);
static void p9_read(Req *r);
static void p9_write(Req *r);
static void p9_clunk(Req *r);
static void p9_remove(Req *r);
static void p9_stat(Req *r);
static void p9_wstat(Req *r);
static void p9_flush(Req *r);
static void fill_stat(Dir *d, u64int qpath);
static int procd_snprint(char *buf, int n, char *fmt, u32int val);
static ulong procd_strtoul(const char *s, char **endp, int base);

/* 9P Qid types */
enum {
  Qroot,    /* /proc directory */
  Qprocdir, /* /proc/<pid> directory */
  Qmem,     /* /proc/<pid>/mem */
  Qnote,    /* /proc/<pid>/note */
  Qstatus,  /* /proc/<pid>/status */
  Qctl,     /* /proc/<pid>/ctl */
  Qpid2,    /* /proc/<pid>/pid2 */
  Qppid,    /* /proc/<pid>/ppid */
};

#define QPATH(pid, type) (((u64int)(pid) << 8) | (type))
#define QPID(path) ((u32int)((path) >> 8))
#define QTYPE(path) ((path) & 0xFF)

typedef struct ProcFile {
  const char *name;
  int type;
} ProcFile;

static ProcFile proc_files[] = {
    {"ctl", Qctl},
    {"mem", Qmem},
    {"note", Qnote},
    {"status", Qstatus},
    {"pid2", Qpid2},
    {"ppid", Qppid},
};

/* Library stubs to avoid libc.h conflict */
extern long sys_write(int fd, void *buf, long n);
extern long sys_seek(int fd, long offset, int whence);
extern ulong strlen(const char *s);
extern void *malloc(ulong size);
extern uint convD2M(Dir *d, uchar *buf, uint nbuf);
extern uint sizeD2M(Dir *d);

static int procd_streq(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    if (a[i] != b[i])
      return 0;
    i++;
  }
  return a[i] == b[i];
}

static u16int procd_gbit16(const uchar *p) {
  return (u16int)(p[0] | (p[1] << 8));
}

static int write_endpoint_file(const char *path, const char *endpoint) {
  char buf[128];
  int fd;
  int i = 0;

  while (endpoint[i] && i < (int)sizeof(buf) - 2) {
    buf[i] = endpoint[i];
    i++;
  }
  buf[i++] = '\n';

  fd = sys_create((char *)path, OWRITE, 0666);
  if (fd < 0)
    fd = sys_open((char *)path, OWRITE);
  if (fd < 0)
    return -1;
  if (sys_write(fd, buf, i) != i) {
    sys_close(fd);
    return -1;
  }
  sys_close(fd);
  return 0;
}

static void make_path(char *buf, char *prefix, int id, char *suffix) {
  int i = 0;

  while (*prefix)
    buf[i++] = *prefix++;
  if (id == 0)
    buf[i++] = '0';
  else {
    char tmp[16];
    int j = 0;
    int v = id;
    while (v > 0) {
      tmp[j++] = (v % 10) + '0';
      v /= 10;
    }
    while (j > 0)
      buf[i++] = tmp[--j];
  }
  while (*suffix)
    buf[i++] = *suffix++;
  buf[i] = '\0';
}

static void make_proc_path(char *buf, int pid, const char *suffix) {
  if (pid <= 0) {
    buf[0] = '#';
    buf[1] = 'p';
    buf[2] = '\0';
    return;
  }
  make_path(buf, "#p/", pid, (char *)suffix);
}

static const char *proc_qtype_name(int type) {
  for (int i = 0; i < (int)(sizeof(proc_files) / sizeof(proc_files[0])); i++) {
    if (proc_files[i].type == type)
      return proc_files[i].name;
  }
  return nil;
}

static int proxy_open_proc_path(u64int qpath, int mode, char *path_out, int nout) {
  int type = QTYPE(qpath);
  int pid = (int)QPID(qpath);
  const char *name;

  if (type == Qroot)
    make_proc_path(path_out, 0, "");
  else if (type == Qprocdir)
    make_proc_path(path_out, pid, "");
  else {
    name = proc_qtype_name(type);
    if (name == nil)
      return -1;
    make_proc_path(path_out, pid, "/");
    if ((int)strlen(path_out) + (int)strlen(name) + 1 > nout)
      return -1;
    {
      int off = (int)strlen(path_out);
      int i = 0;
      while (name[i] != '\0' && off < nout - 1)
        path_out[off++] = name[i++];
      path_out[off] = '\0';
    }
  }

  if (path_out[0] == '\0')
    return -1;

  (void)nout;
  return sys_open(path_out, mode);
}

static int proxy_path_exists(u64int qpath) {
  char path[64];
  int fd;
  int type = QTYPE(qpath);

  fd = proxy_open_proc_path(qpath, type == Qctl ? OWRITE : OREAD, path,
                            sizeof(path));
  if (fd < 0 && type == Qctl)
    fd = proxy_open_proc_path(qpath, ORDWR, path, sizeof(path));
  if (fd < 0)
    return 0;
  sys_close(fd);
  return 1;
}

static int proxy_read(Req *r, u64int qpath) {
  char path[64];
  int fd;
  int n;

  fd = proxy_open_proc_path(qpath, OREAD, path, sizeof(path));
  if (fd < 0)
    return -1;
  if (r->ifcall.offset > 0 && sys_seek(fd, (long)r->ifcall.offset, SEEKSET) < 0) {
    sys_close(fd);
    return -1;
  }
  n = (int)sys_read(fd, r->ofcall.data, (long)r->ifcall.count);
  sys_close(fd);
  return n;
}

static int proxy_write(Req *r, u64int qpath) {
  char path[64];
  int fd;
  int n;

  fd = proxy_open_proc_path(qpath, OWRITE, path, sizeof(path));
  if (fd < 0)
    fd = proxy_open_proc_path(qpath, ORDWR, path, sizeof(path));
  if (fd < 0)
    return -1;
  if (r->ifcall.offset != 0 && sys_seek(fd, (long)r->ifcall.offset, SEEKSET) < 0) {
    sys_close(fd);
    return -1;
  }
  n = (int)sys_write(fd, r->ifcall.data, (long)r->ifcall.count);
  sys_close(fd);
  return n;
}

static ulong proxy_length(u64int qpath) {
  char path[64];
  char buf[256];
  int fd;
  long n;
  ulong total = 0;
  int type = QTYPE(qpath);

  if (type == Qroot || type == Qprocdir || type == Qctl || type == Qmem)
    return 0;

  fd = proxy_open_proc_path(qpath, OREAD, path, sizeof(path));
  if (fd < 0)
    return 0;
  while ((n = sys_read(fd, buf, sizeof(buf))) > 0)
    total += (ulong)n;
  sys_close(fd);
  return total;
}

static int append_dir_entry(Req *r, Dir *d, int *used, vlong *offset_cursor) {
  uint sz;
  uint wrote;

  sz = sizeD2M(d);
  if (*offset_cursor + (vlong)sz <= r->ifcall.offset) {
    *offset_cursor += (vlong)sz;
    return 0;
  }
  if (*used + (int)sz > (int)r->ifcall.count)
    return -1;

  wrote = convD2M(d, (uchar *)r->ofcall.data + *used,
                  (uint)((int)r->ifcall.count - *used));
  if (wrote != sz)
    return -1;
  *used += (int)wrote;
  *offset_cursor += (vlong)wrote;
  return 1;
}

static int parse_pid_name(const char *name) {
  int pid = 0;

  if (name == nil || name[0] == '\0')
    return -1;
  while (*name) {
    if (*name < '0' || *name > '9')
      return -1;
    pid = pid * 10 + (*name - '0');
    name++;
  }
  return pid > 0 ? pid : -1;
}

static int parse_root_dir_pid(const uchar *entry, int dirlen) {
  char name[32];
  int namelen;
  int i;

  if (dirlen < 43)
    return -1;
  namelen = (int)procd_gbit16(entry + 41);
  if (namelen <= 0 || 43 + namelen > dirlen || namelen >= (int)sizeof(name))
    return -1;
  for (i = 0; i < namelen; i++)
    name[i] = (char)entry[43 + i];
  name[namelen] = '\0';
  return parse_pid_name(name);
}

static int read_root_dir(Req *r) {
  uchar raw[1024];
  Dir out;
  int fd;
  long nread;
  int used;
  vlong offset_cursor;

  fd = sys_open("#p", OREAD);
  if (fd < 0)
    return -1;

  used = 0;
  offset_cursor = 0;
  while ((nread = sys_read(fd, raw, sizeof(raw))) > 0) {
    long off = 0;

    while (off + 2 <= nread) {
      int dirlen = (int)(2 + procd_gbit16(raw + off));
      int pid = -1;

      if (dirlen <= 2 || off + dirlen > nread) {
        sys_close(fd);
        return -1;
      }
      pid = parse_root_dir_pid(raw + off, dirlen);
      if (pid > 0) {
        fill_stat(&out, QPATH(pid, Qprocdir));
        if (append_dir_entry(r, &out, &used, &offset_cursor) < 0) {
          sys_close(fd);
          return used;
        }
      }
      off += dirlen;
    }
  }

  sys_close(fd);
  return nread < 0 ? -1 : used;
}

static int read_proc_dir(Req *r, int pid) {
  Dir d;
  int used = 0;
  vlong offset_cursor = 0;

  for (int i = 0; i < (int)(sizeof(proc_files) / sizeof(proc_files[0])); i++) {
    u64int qpath = QPATH(pid, proc_files[i].type);

    if (!proxy_path_exists(qpath))
      continue;
    fill_stat(&d, qpath);
    if (append_dir_entry(r, &d, &used, &offset_cursor) < 0)
      return used;
  }
  return used;
}

static void fill_stat(Dir *d, u64int qpath) {
  static char root_name[] = "proc";
  static char mem_name[] = "mem";
  static char note_name[] = "note";
  static char status_name[] = "status";
  static char ctl_name[] = "ctl";
  static char pid2_name[] = "pid2";
  static char ppid_name[] = "ppid";
  static char user_name[] = "lux";
  static char pid_name[16];
  int type = QTYPE(qpath);

  memset(d, 0, sizeof(*d));
  mkqid(&d->qid, qpath, 0, (type == Qroot || type == Qprocdir) ? QTDIR : QTFILE);
  if (type == Qroot || type == Qprocdir)
    d->mode = DMDIR | 0555;
  else if (type == Qstatus || type == Qpid2 || type == Qppid)
    d->mode = 0444;
  else
    d->mode = 0666;
  d->length = proxy_length(qpath);
  d->uid = user_name;
  d->gid = user_name;
  d->muid = user_name;

  if (type == Qroot)
    d->name = root_name;
  else if (type == Qprocdir) {
    procd_snprint(pid_name, sizeof(pid_name), "", QPID(qpath));
    d->name = pid_name;
  } else if (type == Qmem)
    d->name = mem_name;
  else if (type == Qnote)
    d->name = note_name;
  else if (type == Qstatus)
    d->name = status_name;
  else if (type == Qctl)
    d->name = ctl_name;
  else if (type == Qpid2)
    d->name = pid2_name;
  else
    d->name = ppid_name;
}

static int walk_one(u64int cur, const char *name, Qid *qid) {
  int pid;
  char *endp;

  if (procd_streq(name, ".") || name[0] == '\0') {
    mkqid(qid, cur, 0, (QTYPE(cur) == Qroot || QTYPE(cur) == Qprocdir) ? QTDIR : QTFILE);
    return 0;
  }

  if (procd_streq(name, "..")) {
    if (QTYPE(cur) == Qroot)
      mkqid(qid, QPATH(0, Qroot), 0, QTDIR);
    else if (QTYPE(cur) == Qprocdir)
      mkqid(qid, QPATH(0, Qroot), 0, QTDIR);
    else
      mkqid(qid, QPATH(QPID(cur), Qprocdir), 0, QTDIR);
    return 0;
  }

  if (QTYPE(cur) == Qroot) {
    pid = (int)procd_strtoul(name, &endp, 10);
    if (*endp == '\0' && pid > 0 && proxy_path_exists(QPATH(pid, Qprocdir))) {
      mkqid(qid, QPATH(pid, Qprocdir), 0, QTDIR);
      return 0;
    }
    return -1;
  }

  if (QTYPE(cur) == Qprocdir) {
    for (int i = 0; i < (int)(sizeof(proc_files) / sizeof(proc_files[0])); i++) {
      if (procd_streq(name, proc_files[i].name) &&
          proxy_path_exists(QPATH(QPID(cur), proc_files[i].type))) {
        mkqid(qid, QPATH(QPID(cur), proc_files[i].type), 0, QTFILE);
        return 0;
      }
    }
  }

  return -1;
}

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

int fprint(int fd, const char *fmt, ...) {
  char buf[1024];
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprint(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n > 0) {
    sys_write(fd, buf, n);
  }
  return n;
}

static ulong procd_strtoul(const char *s, char **endp, int base) {
  ulong val = 0;
  (void)base;
  while (*s >= '0' && *s <= '9') {
    val = val * 10 + (*s - '0');
    s++;
  }
  if (endp)
    *endp = (char *)s;
  return val;
}

/* 9P Handlers */

static void p9_attach(Req *r) {
  mkqid(&r->ofcall.qid, QPATH(0, Qroot), 0, QTDIR);
  r->fid->qid = r->ofcall.qid;
  srv_respond(r, nil);
}

static void p9_walk(Req *r) {
  u64int cur;
  Qid qid;
  int i;

  if (r->ifcall.nwname == 0) {
    r->ofcall.nwqid = 0;
    srv_respond(r, nil);
    return;
  }

  cur = r->fid->qid.path;
  for (i = 0; i < (int)r->ifcall.nwname; i++) {
    if (walk_one(cur, r->ifcall.wname[i], &qid) < 0)
      break;
    r->ofcall.wqid[i] = qid;
    r->ofcall.nwqid = (u16int)(i + 1);
    cur = qid.path;
  }

  if (r->ofcall.nwqid == 0) {
    srv_respond(r, "file not found");
    return;
  }
  srv_respond(r, nil);
}

static void p9_open(Req *r) {
  int type = QTYPE(r->fid->qid.path);
  int mode = r->ifcall.mode & 3;
  char path[64];
  int fd;

  if ((type == Qroot || type == Qprocdir) && mode != OREAD) {
    srv_respond(r, "permission denied");
    return;
  }

  if (type != Qroot && type != Qprocdir) {
    fd = proxy_open_proc_path(r->fid->qid.path, mode, path, sizeof(path));
    if (fd < 0) {
      srv_respond(r, "permission denied");
      return;
    }
    sys_close(fd);
  }

  r->ofcall.qid = r->fid->qid;
  r->ofcall.iounit = 0;
  srv_respond(r, nil);
}

static void p9_create(Req *r) { srv_respond(r, "create not supported"); }

// Minimal snprint for procd
static int procd_snprint(char *buf, int n, char *fmt, u32int val) {
  // Only handles %ud for now
  char *p = buf;
  char temp[16];
  int i = 0;
  while (val > 0) {
    temp[i++] = '0' + (val % 10);
    val /= 10;
  }
  if (i == 0)
    temp[i++] = '0';
  while (i > 0 && n > 1) {
    *p++ = temp[--i];
    n--;
  }
  *p = '\0';
  return p - buf;
}

static void p9_read(Req *r) {
  u64int path = r->fid->qid.path;
  int type = QTYPE(path);
  int n = 0;

  if (type != Qroot && type != Qprocdir && type != Qmem && type != Qnote &&
      type != Qstatus && type != Qctl && type != Qpid2 && type != Qppid) {
    srv_respond(r, "read error");
    return;
  }

  if (type == Qroot)
    n = read_root_dir(r);
  else if (type == Qprocdir)
    n = read_proc_dir(r, (int)QPID(path));
  else
    n = proxy_read(r, path);
  if (n < 0) {
    srv_respond(r, "read error");
    return;
  }
  r->ofcall.count = (u32int)n;
  srv_respond(r, nil);
}

static void p9_write(Req *r) {
  int n;
  int type = QTYPE(r->fid->qid.path);

  if (type != Qctl && type != Qmem && type != Qnote) {
    srv_respond(r, "write not supported");
    return;
  }
  n = proxy_write(r, r->fid->qid.path);
  if (n < 0) {
    srv_respond(r, "write failed");
    return;
  }
  r->ofcall.count = (u32int)n;
  srv_respond(r, nil);
}

static void p9_clunk(Req *r) {
  (void)r;
  srv_respond(r, nil);
}

static void p9_remove(Req *r) { srv_respond(r, "remove not supported"); }

static void p9_stat(Req *r) {
  Dir d;
  uint sz;

  fill_stat(&d, r->fid->qid.path);
  sz = sizeD2M(&d);
  r->ofcall.stat = malloc(sz);
  if (r->ofcall.stat == nil) {
    srv_respond(r, "out of memory");
    return;
  }
  r->ofcall.nstat = convD2M(&d, (uchar *)r->ofcall.stat, sz);
  srv_respond(r, nil);
}

static void p9_wstat(Req *r) { srv_respond(r, "wstat not supported"); }

static void p9_flush(Req *r) {
  (void)r;
  srv_respond(r, nil);
}

int main(void) {
  // Print startup message for troubleshooting
  const char *start_msg = "PROCD: Starting process daemon\n";
  sys_write(1, (void*)start_msg, strlen(start_msg));

  static Srv s = {
      .attach = p9_attach,
      .walk = p9_walk,
      .open = p9_open,
      .create = p9_create,
      .read = p9_read,
      .write = p9_write,
      .clunk = p9_clunk,
      .remove = p9_remove,
      .stat = p9_stat,
      .wstat = p9_wstat,
      .flush = p9_flush,
  };

  int fd_clone = sys_open("#X/clone", OREAD);
  if (fd_clone < 0) {
    const char *err_msg = "PROCD: #X clone failed\n";
    sys_write(1, (void*)err_msg, strlen(err_msg));
    return 1;
  }
  char buf[32];
  int n = (int)sys_read(fd_clone, buf, 31);
  if (n <= 0) {
    const char *err_msg = "PROCD: failed to read #X channel id\n";
    sys_write(1, (void*)err_msg, strlen(err_msg));
    return 1;
  }
  buf[n] = '\0';
  int chan_id = (int)procd_strtoul(buf, nil, 10);

  char path_ctl[64];
  make_path(path_ctl, "#X/", chan_id, "/ctl");
  int fd_ctl = sys_open(path_ctl, OWRITE);
  sys_close(fd_clone);
  if (fd_ctl >= 0) {
    sys_write(fd_ctl, "mode service", 12);
    sys_close(fd_ctl);
  }

  char path_data[64];
  make_path(path_data, "#X/", chan_id, "/data");
  if (write_endpoint_file("/srv/procd.endpoint", path_data) < 0) {
    const char *err_msg = "PROCD: failed to publish endpoint\n";
    sys_write(1, (void*)err_msg, strlen(err_msg));
    return 1;
  }

  /* Resurrection owns service supervision; procd only needs to publish its
   * endpoint for nsd and other consumers. */
  const char *ready_msg = "PROCD: endpoint published, entering service loop\n";
  sys_write(1, (void *)ready_msg, strlen(ready_msg));

  srv_init(&s);
  char path_ring[64];
  make_path(path_ring, "#X/", chan_id, "/ipcring");
  srv_loop_ring(&s, path_ring);

  return 0;
}
