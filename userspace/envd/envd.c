#include "../lib/liblux/inc/lux.h"
#include "../lib/liblux/inc/server9p.h"

/* Missing constants from lux.h/libc.h */
#ifndef QTDIR
#define QTDIR 0x80
#endif
#ifndef QTFILE
#define QTFILE 0x00
#endif
#ifndef DMDIR
#define DMDIR 0x80000000
#endif
#ifndef STATFIXLEN
#define STATFIXLEN (2 + 13 + 5 * 2 + 4 * 4 + 8)
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
#ifndef MREPL
#define MREPL 0
#endif
#ifndef MCREATE
#define MCREATE 4
#endif

/* Pebble-based Memory Allocation */
static void *env_malloc(ulong size) {
  void *p = nil;
  if (pebble_alloc(size, &p) < 0)
    return nil;
  return p;
}

static void env_free(void *ptr) {
  if (ptr)
    pebble_free(ptr);
}

static char *env_strdup(const char *s) {
  usize n = strlen(s);
  char *d = env_malloc(n + 1);
  if (d) {
    for (usize i = 0; i <= n; i++)
      d[i] = s[i];
  }
  return d;
}

/* Environment Variable Storage */
typedef struct EnvVar EnvVar;
struct EnvVar {
  char *name;
  char *value;
  int len;
  EnvVar *next;
  u64int qid_path;
  uuid_t owner;
};

static EnvVar *env_list = nil;
static u64int next_qid = 1;

/* Forward declarations */
static void p9_attach(Req *r);
static void p9_walk(Req *r);
static void p9_open(Req *r);
static void p9_read(Req *r);
static void p9_write(Req *r);
static void p9_stat(Req *r);
static void p9_create(Req *r);
static void p9_remove(Req *r);

/* Qid types */
enum {
  Qroot = 0,
};

/* Env Helpers */
static int uuid_equal(uuid_t *a, uuid_t *b) {
  for (int i = 0; i < 16; i++)
    if (a->data[i] != b->data[i])
      return 0;
  return 1;
}

static EnvVar *find_var(const char *name, uuid_t *owner) {
  for (EnvVar *e = env_list; e; e = e->next) {
    if (strcmp(e->name, (char *)name) == 0 && uuid_equal(&e->owner, owner))
      return e;
  }
  return nil;
}

static EnvVar *find_var_by_qid(u64int path) {
  for (EnvVar *e = env_list; e; e = e->next) {
    if (e->qid_path == path)
      return e;
  }
  return nil;
}

/* 9P Handlers */

static void p9_attach(Req *r) {
  mkqid(&r->ofcall.qid, Qroot, 0, QTDIR);
  r->fid->qid = r->ofcall.qid;
  srv_respond(r, nil);
}

static void p9_walk(Req *r) {
  if (r->ifcall.nwname == 0) {
    r->ofcall.nwqid = 0;
    srv_respond(r, nil);
    return;
  }

  if (r->fid->qid.path != Qroot) {
    srv_respond(r, "cannot walk from file");
    return;
  }

  char *name = r->ifcall.wname[0];
  EnvVar *e = find_var(name, &r->pid2);
  if (e) {
    mkqid(&r->ofcall.wqid[0], e->qid_path, 0, QTFILE);
    r->ofcall.nwqid = 1;
    srv_respond(r, nil);
  } else {
    srv_respond(r, "file not found");
  }
}

static void p9_open(Req *r) {
  if (r->fid->qid.path == Qroot) {
    if (r->ifcall.mode != OREAD) {
      srv_respond(r, "permission denied");
      return;
    }
  }
  srv_respond(r, nil);
}

static void p9_read(Req *r) {
  if (r->fid->qid.path == Qroot) {
    /* Directory listing */
    uchar *data = (uchar *)r->ofcall.data;
    int n = 0;
    int max = r->ifcall.count;
    Dir d = {0};
    vlong off = 0;

    for (EnvVar *e = env_list; e; e = e->next) {
      if (!uuid_equal(&e->owner, &r->pid2))
        continue;

      d.name = e->name;
      d.qid.path = e->qid_path;
      d.qid.type = QTFILE;
      d.mode = 0666;
      d.length = e->len;
      d.uid = "lux";
      d.gid = "lux";
      d.muid = "lux";

      uint sz = sizeD2M(&d);
      if (off >= r->ifcall.offset) {
        if (n + sz > max)
          break;
        n += convD2M(&d, data + n, max - n);
      }
      off += sz;
    }

    r->ofcall.count = n;
    srv_respond(r, nil);
    return;
  }

  EnvVar *e = find_var_by_qid(r->fid->qid.path);
  if (!e) {
    srv_respond(r, "entry disappeared");
    return;
  }

  if (r->ifcall.offset >= (vlong)e->len) {
    r->ofcall.count = 0;
  } else {
    int count = (int)r->ifcall.count;
    if (count > e->len - (int)r->ifcall.offset)
      count = e->len - (int)r->ifcall.offset;
    memcpy(r->ofcall.data, e->value + (int)r->ifcall.offset, count);
    r->ofcall.count = count;
  }
  srv_respond(r, nil);
}

static void p9_write(Req *r) {
  if (r->fid->qid.path == Qroot) {
    srv_respond(r, "cannot write to directory");
    return;
  }

  EnvVar *e = find_var_by_qid(r->fid->qid.path);
  if (!e) {
    srv_respond(r, "entry disappeared");
    return;
  }

  /* Support truncating/overwriting */
  if (r->ifcall.offset == 0) {
    if (e->value)
      env_free(e->value);
    e->value = env_malloc(r->ifcall.count);
    if (!e->value) {
      e->len = 0;
      srv_respond(r, "out of memory");
      return;
    }
    memcpy(e->value, r->ifcall.data, r->ifcall.count);
    e->len = (int)r->ifcall.count;
  } else if (r->ifcall.offset == (vlong)e->len) {
    char *new_val = env_malloc(e->len + r->ifcall.count);
    if (!new_val) {
      srv_respond(r, "out of memory");
      return;
    }
    memcpy(new_val, e->value, e->len);
    memcpy(new_val + e->len, r->ifcall.data, r->ifcall.count);
    if (e->value)
      env_free(e->value);
    e->value = new_val;
    e->len += (int)r->ifcall.count;
  } else {
    srv_respond(r, "random access write not supported");
    return;
  }

  r->ofcall.count = r->ifcall.count;
  srv_respond(r, nil);
}

static void p9_stat(Req *r) {
  EnvVar *e = find_var_by_qid(r->fid->qid.path);
  Dir d = {0};

  if (r->fid->qid.path == Qroot) {
    d.name = "/";
    d.qid.path = Qroot;
    d.qid.type = QTDIR;
    d.mode = 0755 | DMDIR;
  } else if (e) {
    d.name = e->name;
    d.qid.path = e->qid_path;
    d.qid.type = QTFILE;
    d.mode = 0666;
    d.length = e->len;
  } else {
    srv_respond(r, "file not found");
    return;
  }

  d.uid = "lux";
  d.gid = "lux";
  d.muid = "lux";

  uchar statbuf[STATFIXLEN + 256];
  uint n = convD2M(&d, statbuf, sizeof(statbuf));
  if (n > 0) {
    r->ofcall.stat = env_malloc(n);
    if (r->ofcall.stat) {
      memcpy(r->ofcall.stat, statbuf, n);
      r->ofcall.nstat = n;
      srv_respond(r, nil);
      return;
    }
  }
  srv_respond(r, "stat error");
}

static void p9_create(Req *r) {
  if (r->fid->qid.path != Qroot) {
    srv_respond(r, "cannot create in file");
    return;
  }

  if (find_var(r->ifcall.name, &r->pid2)) {
    srv_respond(r, "file already exists");
    return;
  }

  EnvVar *e = env_malloc(sizeof(EnvVar));
  if (!e) {
    srv_respond(r, "out of memory");
    return;
  }

  e->name = env_strdup(r->ifcall.name);
  e->value = nil;
  e->len = 0;
  e->qid_path = next_qid++;
  uuid_copy(&e->owner, &r->pid2);
  e->next = env_list;
  env_list = e;

  mkqid(&r->ofcall.qid, e->qid_path, 0, QTFILE);
  r->fid->qid = r->ofcall.qid;
  srv_respond(r, nil);
}

static void p9_remove(Req *r) {
  if (r->fid->qid.path == Qroot) {
    srv_respond(r, "cannot remove root");
    return;
  }

  EnvVar **p = &env_list;
  while (*p) {
    if ((*p)->qid_path == r->fid->qid.path) {
      EnvVar *e = *p;
      *p = e->next;
      env_free(e->name);
      if (e->value)
        env_free(e->value);
      env_free(e);
      srv_respond(r, nil);
      return;
    }
    p = &((*p)->next);
  }

  srv_respond(r, "file not found");
}

void make_path(char *buf, char *prefix, int id, char *suffix) {
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

int main(void) {
  static Srv s = {
      .attach = p9_attach,
      .walk = p9_walk,
      .open = p9_open,
      .read = p9_read,
      .write = p9_write,
      .stat = p9_stat,
      .create = p9_create,
      .remove = p9_remove,
  };

  /* 1. Create Exchange Channel */
  int fd_clone = sys_open("#X/clone", OREAD);
  if (fd_clone < 0)
    return 1;
  char buf[32];
  int n = sys_read(fd_clone, buf, 31);
  buf[n] = 0;
  int chan_id = atoi(buf);
  /* Keep fd_clone open until we get a reference via ctl */

  /* 2. Configure Service Mode */
  char path_ctl[64];
  make_path(path_ctl, "#X/", chan_id, "/ctl");
  int fd_ctl = sys_open(path_ctl, OWRITE);
  sys_close(fd_clone); /* Safe to close now */

  if (fd_ctl >= 0) {
    sys_write(fd_ctl, "mode service", 12);
    sys_close(fd_ctl);
  }

  /* 3. Publish #X/N/data for nsd to mount */
  char path_data[64];
  make_path(path_data, "#X/", chan_id, "/data");
  if (write_endpoint_file("/srv/envd.endpoint", path_data) < 0)
    return 1;

  /* 4. Serve Ring */
  srv_init(&s);
  char path_ring[64];
  make_path(path_ring, "#X/", chan_id, "/ipcring");
  srv_loop_ring(&s, path_ring);

  return 0;
}
