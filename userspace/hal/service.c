#include <server9p.h>
#include <stdarg.h>

#ifndef QTDIR
#define QTDIR 0x80
#endif
#ifndef QTFILE
#define QTFILE 0x00
#endif
#ifndef DMDIR
#define DMDIR 0x80000000U
#endif
#ifndef OREAD
#define OREAD 0
#endif
#ifndef OWRITE
#define OWRITE 1
#endif
#ifndef ORDWR
#define ORDWR 2
#endif
#define SEEKSET 0
#define HAL_PCI_BUS_PATH "#J/bus"

enum {
  Qroot = 1,
  Qctl,
  Qfamilies,
  Qpcidir,
  Qpcibus,
  Qpcictl,
};

extern long sys_write(int fd, void *buf, long n);
extern ulong strlen(const char *s);
extern int print(char *fmt, ...);
extern int pebble_alloc(ulong size, void **addr);
extern int pebble_free(void *addr);
extern int family_registry_count(void);
extern int family_registry_snapshot(char *buf, int nbuf);
extern int pcifamily_refresh(void);
extern int pcifamily_device_count(void);

static int hal_streq(const char *a, const char *b) {
  int i = 0;

  while (a[i] && b[i]) {
    if (a[i] != b[i])
      return 0;
    i++;
  }
  return a[i] == b[i];
}

static int hal_parse_int(const char *s) {
  int v = 0;

  if (s == nil || *s == 0)
    return -1;
  while (*s >= '0' && *s <= '9') {
    v = v * 10 + (*s - '0');
    s++;
  }
  return v;
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
  buf[i] = 0;
}

static int post_srv_path(const char *name, const char *endpoint_path) {
  char path[64];
  int fd_srv;
  int n;
  long wrote;

  if (snprint(path, sizeof(path), "#s/%s", name) >= (int)sizeof(path))
    return -1;

  fd_srv = sys_create(path, OWRITE, 0666);
  print("HAL: post create %s -> %d\n", path, fd_srv);
  if (fd_srv < 0) {
    fd_srv = sys_open(path, OWRITE);
    print("HAL: post open %s -> %d\n", path, fd_srv);
  }
  if (fd_srv < 0)
    return -1;

  n = (int)strlen(endpoint_path);
  wrote = sys_write(fd_srv, (void *)endpoint_path, n);
  print("HAL: post write %s <= %s (%ld)\n", path, endpoint_path, wrote);
  if (n <= 0 || wrote != n) {
    sys_close(fd_srv);
    return -1;
  }
  sys_close(fd_srv);
  return 0;
}

static long read_text_path(const char *path, void *buf, long n, vlong offset) {
  int fd;
  long rv;

  fd = sys_open((char *)path, OREAD);
  if (fd < 0)
    return -1;
  if (offset > 0 && sys_seek(fd, (long)offset, SEEKSET) < 0) {
    sys_close(fd);
    return -1;
  }
  rv = sys_read(fd, buf, n);
  sys_close(fd);
  return rv;
}

static ulong text_path_length(const char *path) {
  char buf[256];
  int fd;
  long n;
  ulong total = 0;

  fd = sys_open((char *)path, OREAD);
  if (fd < 0)
    return 0;
  while ((n = sys_read(fd, buf, sizeof(buf))) > 0)
    total += (ulong)n;
  sys_close(fd);
  return total;
}

static int append_dir_entry(Req *r, Dir *d, int *used, vlong *cursor) {
  uint sz;
  uint wrote;

  sz = sizeD2M(d);
  if (*cursor + (vlong)sz <= r->ifcall.offset) {
    *cursor += (vlong)sz;
    return 0;
  }
  if (*used + (int)sz > (int)r->ifcall.count)
    return -1;

  wrote = convD2M(d, (uchar *)r->ofcall.data + *used,
                  (uint)((int)r->ifcall.count - *used));
  if (wrote != sz)
    return -1;
  *used += (int)wrote;
  *cursor += (vlong)wrote;
  return 1;
}

static int format_root_ctl(char *buf, int nbuf) {
  return snprint(buf, nbuf,
                 "service hal_pci\n"
                 "transport exchange-ring\n"
                 "families %d\n"
                 "pci_devices %d\n"
                 "commands: rescan\n",
                 family_registry_count(), pcifamily_device_count());
}

static int format_pci_ctl(char *buf, int nbuf) {
  return snprint(buf, nbuf,
                 "family pci\n"
                 "devices %d\n"
                 "commands: rescan\n",
                 pcifamily_device_count());
}

static void fill_stat(Dir *d, u64int qpath) {
  static char user_name[] = "lux";
  static char root_name[] = "hal_pci";
  static char ctl_name[] = "ctl";
  static char families_name[] = "families";
  static char pci_name[] = "pci";
  static char bus_name[] = "bus";
  char tmp[256];
  int len = 0;

  memset(d, 0, sizeof(*d));
  mkqid(&d->qid, qpath, 0,
        (qpath == Qroot || qpath == Qpcidir) ? QTDIR : QTFILE);

  if (qpath == Qroot || qpath == Qpcidir)
    d->mode = DMDIR | 0555;
  else if (qpath == Qctl || qpath == Qpcictl)
    d->mode = 0664;
  else
    d->mode = 0444;

  if (qpath == Qctl) {
    len = format_root_ctl(tmp, sizeof(tmp));
    d->length = len > 0 ? len : 0;
    d->name = ctl_name;
  } else if (qpath == Qfamilies) {
    len = family_registry_snapshot(tmp, sizeof(tmp));
    d->length = len > 0 ? len : 0;
    d->name = families_name;
  } else if (qpath == Qpcibus) {
    d->length = text_path_length(HAL_PCI_BUS_PATH);
    d->name = bus_name;
  } else if (qpath == Qpcictl) {
    len = format_pci_ctl(tmp, sizeof(tmp));
    d->length = len > 0 ? len : 0;
    d->name = ctl_name;
  } else if (qpath == Qpcidir) {
    d->length = 0;
    d->name = pci_name;
  } else {
    d->length = 0;
    d->name = root_name;
  }

  d->uid = user_name;
  d->gid = user_name;
  d->muid = user_name;
}

static int walk_one(u64int cur, const char *name, Qid *qid) {
  if (hal_streq(name, ".") || name[0] == '\0') {
    mkqid(qid, cur, 0, (cur == Qroot || cur == Qpcidir) ? QTDIR : QTFILE);
    return 0;
  }

  if (hal_streq(name, "..")) {
    if (cur == Qroot)
      mkqid(qid, Qroot, 0, QTDIR);
    else if (cur == Qpcidir)
      mkqid(qid, Qroot, 0, QTDIR);
    else if (cur == Qpcibus || cur == Qpcictl)
      mkqid(qid, Qpcidir, 0, QTDIR);
    else
      mkqid(qid, Qroot, 0, QTDIR);
    return 0;
  }

  if (cur == Qroot) {
    if (hal_streq(name, "ctl")) {
      mkqid(qid, Qctl, 0, QTFILE);
      return 0;
    }
    if (hal_streq(name, "families")) {
      mkqid(qid, Qfamilies, 0, QTFILE);
      return 0;
    }
    if (hal_streq(name, "pci")) {
      mkqid(qid, Qpcidir, 0, QTDIR);
      return 0;
    }
    return -1;
  }

  if (cur == Qpcidir) {
    if (hal_streq(name, "bus")) {
      mkqid(qid, Qpcibus, 0, QTFILE);
      return 0;
    }
    if (hal_streq(name, "ctl")) {
      mkqid(qid, Qpcictl, 0, QTFILE);
      return 0;
    }
  }

  return -1;
}

static int read_dir(Req *r, u64int dirq) {
  Dir d;
  int used = 0;
  vlong cursor = 0;

  if (dirq == Qroot) {
    fill_stat(&d, Qctl);
    if (append_dir_entry(r, &d, &used, &cursor) < 0)
      return used;
    fill_stat(&d, Qfamilies);
    if (append_dir_entry(r, &d, &used, &cursor) < 0)
      return used;
    fill_stat(&d, Qpcidir);
    if (append_dir_entry(r, &d, &used, &cursor) < 0)
      return used;
    return used;
  }

  if (dirq == Qpcidir) {
    fill_stat(&d, Qpcibus);
    if (append_dir_entry(r, &d, &used, &cursor) < 0)
      return used;
    fill_stat(&d, Qpcictl);
    if (append_dir_entry(r, &d, &used, &cursor) < 0)
      return used;
    return used;
  }

  return -1;
}

static void p9_attach(Req *r) {
  mkqid(&r->ofcall.qid, Qroot, 0, QTDIR);
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
  if (r->newfid)
    r->newfid->qid = r->ofcall.wqid[r->ofcall.nwqid - 1];
  srv_respond(r, nil);
}

static void p9_open(Req *r) {
  u64int qpath = r->fid->qid.path;
  int mode = r->ifcall.mode & 3;

  if (qpath == Qroot || qpath == Qpcidir) {
    if (mode != OREAD) {
      srv_respond(r, "permission denied");
      return;
    }
  } else if (qpath != Qctl && qpath != Qpcictl && mode != OREAD) {
    srv_respond(r, "permission denied");
    return;
  }

  r->ofcall.qid = r->fid->qid;
  r->ofcall.iounit = 0;
  r->fid->omode = mode;
  srv_respond(r, nil);
}

static void p9_read(Req *r) {
  char buf[4096];
  long n;

  if (r->fid->qid.path == Qroot || r->fid->qid.path == Qpcidir) {
    n = read_dir(r, r->fid->qid.path);
    if (n < 0) {
      srv_respond(r, "directory read failed");
      return;
    }
    r->ofcall.count = (u32int)n;
    srv_respond(r, nil);
    return;
  }

  if (r->fid->qid.path == Qfamilies) {
    n = family_registry_snapshot(buf, sizeof(buf));
    if (n < 0) {
      srv_respond(r, "family snapshot failed");
      return;
    }
  } else if (r->fid->qid.path == Qctl) {
    n = format_root_ctl(buf, sizeof(buf));
  } else if (r->fid->qid.path == Qpcictl) {
    n = format_pci_ctl(buf, sizeof(buf));
  } else if (r->fid->qid.path == Qpcibus) {
    n = read_text_path(HAL_PCI_BUS_PATH, r->ofcall.data, (long)r->ifcall.count,
                       r->ifcall.offset);
    if (n < 0) {
      srv_respond(r, "pci bus read failed");
      return;
    }
    r->ofcall.count = (u32int)n;
    srv_respond(r, nil);
    return;
  } else {
    srv_respond(r, "file not found");
    return;
  }

  if (n < 0 || r->ifcall.offset >= n) {
    r->ofcall.count = 0;
    srv_respond(r, nil);
    return;
  }
  if (r->ifcall.offset + r->ifcall.count > (u32int)n)
    r->ofcall.count = (u32int)(n - r->ifcall.offset);
  else
    r->ofcall.count = r->ifcall.count;

  memcpy(r->ofcall.data, buf + r->ifcall.offset, r->ofcall.count);
  srv_respond(r, nil);
}

static void p9_write(Req *r) {
  char cmd[64];
  int n = (int)r->ifcall.count;

  if (r->fid->qid.path != Qctl && r->fid->qid.path != Qpcictl) {
    srv_respond(r, "permission denied");
    return;
  }
  if (n <= 0) {
    r->ofcall.count = 0;
    srv_respond(r, nil);
    return;
  }
  if (n >= (int)sizeof(cmd))
    n = (int)sizeof(cmd) - 1;
  memcpy(cmd, r->ifcall.data, n);
  cmd[n] = 0;
  while (n > 0 &&
         (cmd[n - 1] == '\n' || cmd[n - 1] == '\r' || cmd[n - 1] == ' ' ||
          cmd[n - 1] == '\t'))
    cmd[--n] = 0;

  if (hal_streq(cmd, "rescan")) {
    if (pcifamily_refresh() < 0) {
      srv_respond(r, "pci rescan failed");
      return;
    }
    print("HAL: rescanned PCI bus, %d devices\n", pcifamily_device_count());
  } else {
    srv_respond(r, "unknown control command");
    return;
  }

  r->ofcall.count = r->ifcall.count;
  srv_respond(r, nil);
}

static void p9_clunk(Req *r) {
  (void)r;
  srv_respond(r, nil);
}

static void p9_remove(Req *r) {
  (void)r;
  srv_respond(r, "remove not supported");
}

static void p9_stat(Req *r) {
  Dir d;
  uint sz;

  fill_stat(&d, r->fid->qid.path);
  sz = sizeD2M(&d);
  if (pebble_alloc(sz, (void **)&r->ofcall.stat) < 0) {
    srv_respond(r, "out of memory");
    return;
  }
  memset(r->ofcall.stat, 0, sz);
  r->ofcall.nstat = convD2M(&d, (uchar *)r->ofcall.stat, sz);
  srv_respond(r, nil);
}

static void p9_wstat(Req *r) {
  (void)r;
  srv_respond(r, "wstat not supported");
}

static void p9_flush(Req *r) {
  (void)r;
  srv_respond(r, nil);
}

int hal_service_main(void) {
  static Srv s = {
      .attach = p9_attach,
      .walk = p9_walk,
      .open = p9_open,
      .read = p9_read,
      .write = p9_write,
      .clunk = p9_clunk,
      .remove = p9_remove,
      .stat = p9_stat,
      .wstat = p9_wstat,
      .flush = p9_flush,
  };
  char buf[32];
  char path_ctl[64];
  char path_data[64];
  char path_ring[64];
  int fd_clone;
  int fd_ctl;
  int n;
  int chan_id;

  fd_clone = sys_open("#X/clone", OREAD);
  if (fd_clone < 0) {
    sys_write(1, "HAL: #X clone failed\n", 21);
    return 1;
  }
  n = (int)sys_read(fd_clone, buf, sizeof(buf) - 1);
  if (n <= 0) {
    sys_close(fd_clone);
    sys_write(1, "HAL: failed to read #X channel id\n", 34);
    return 1;
  }
  buf[n] = 0;
  chan_id = hal_parse_int(buf);
  if (chan_id < 0) {
    sys_write(1, "HAL: invalid #X channel id\n", 27);
    return 1;
  }

  make_path(path_ctl, "#X/", chan_id, "/ctl");
  fd_ctl = sys_open(path_ctl, OWRITE);
  sys_close(fd_clone);
  if (fd_ctl >= 0) {
    sys_write(fd_ctl, "mode service", 12);
    sys_close(fd_ctl);
  }

  make_path(path_data, "#X/", chan_id, "/data");
  print("HAL: service channel id %d path %s\n", chan_id, path_data);

  if (post_srv_path("hal_pci", path_data) < 0) {
    sys_write(1, "HAL: failed to post /srv/hal_pci\n", 33);
    return 1;
  }
  print("HAL: published /srv/hal_pci (%s)\n", path_data);

  srv_init(&s);
  make_path(path_ring, "#X/", chan_id, "/ipcring");
  srv_loop_ring(&s, path_ring);
  return 0;
}
