#include "../../lib/liblux/inc/lux.h"
#include "../../lib/liblux/inc/server9p.h"
#include "../../lib/liblux/src/lux_bootstrap.h"

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
#ifndef MBEFORE
#define MBEFORE 1
#endif
#ifndef MAFTER
#define MAFTER 2
#endif
#ifndef MCREATE
#define MCREATE 4
#endif
#ifndef DMDIR
#define DMDIR 0x80000000U
#endif
#ifndef QTDIR
#define QTDIR 0x80
#endif
#ifndef QTFILE
#define QTFILE 0x00
#endif
#ifndef STATFIXLEN
#define STATFIXLEN (2 + 13 + 5 * 2 + 4 * 4 + 8)
#endif
#ifndef VERSION9P
#define VERSION9P "9P2000"
#endif
#ifndef NOTAG
#define NOTAG ((ushort)~0U)
#endif
#ifndef NOFID
#define NOFID ((u32int)~0U)
#endif

enum {
  NsdMaxNodes = 128,
  NsdMaxMounts = 32,
  NsdMaxName = 63,
  NsdMaxEndpoint = 127,
  NsdMaxRelPath = 255,
  NsdMaxCtl = 256,
  NsdMaxRpc = 8192,
  NsdMaxResolveDepth = 8,
  NsdServerStack = 16384,
};

typedef struct MountSpec MountSpec;
typedef struct NsdMount NsdMount;
typedef struct NsdNode NsdNode;
typedef struct NsdFidState NsdFidState;
typedef struct RemoteConn RemoteConn;

enum {
  NodeDir,
  NodeCtl,
  NodeMounts,
};

enum {
  MountNone,
  MountEndpoint,
  MountBind,
};

enum {
  StateLocal,
  StateRemote,
  StateBind,
};

struct MountSpec {
  const char *endpoint_file;
  const char *mountpoint;
  int mounted;
  char endpoint[NsdMaxEndpoint + 1];
};

struct NsdMount {
  int used;
  int kind;
  int flags;
  char endpoint[NsdMaxEndpoint + 1];
  char aname[NsdMaxName + 1];
  char target[NsdMaxRelPath + 1];
  Qid root_qid;
  int have_root_qid;
};

struct NsdNode {
  int used;
  int kind;
  int fixed;
  char name[NsdMaxName + 1];
  u64int qid_path;
  NsdNode *parent;
  NsdNode *child;
  NsdNode *sibling;
  NsdMount *mount;
};

struct NsdFidState {
  int kind;
  NsdNode *node;
  char relpath[NsdMaxRelPath + 1];
  Qid qid;
};

struct RemoteConn {
  int fd;
  ushort next_tag;
  Qid root_qid;
};

static MountSpec mounts[] = {
    {"/srv/procd.endpoint", "/proc", 0, {0}},
    {"/srv/envd.endpoint", "/env", 0, {0}},
    {"/srv/sophia.endpoint", "/mnt/sophia", 0, {0}},
};

static NsdNode nodes[NsdMaxNodes];
static NsdMount mount_table[NsdMaxMounts];
static NsdNode *root_node;
static volatile u64int next_qid_path = 1;
static volatile int vfs_lock;
static volatile int child_ready;
static volatile int rescan_requested;
static uchar server_stack[NsdServerStack] __attribute__((aligned(16)));
static char nsd_ring_path[64];

static void p9_attach(Req *r);
static void p9_walk(Req *r);
static void p9_open(Req *r);
static void p9_create(Req *r);
static void p9_read(Req *r);
static void p9_write(Req *r);
static void p9_stat(Req *r);
static void p9_remove(Req *r);
static void p9_clunk(Req *r);

static Srv nsd_srv = {
    .attach = p9_attach,
    .walk = p9_walk,
    .open = p9_open,
    .create = p9_create,
    .read = p9_read,
    .write = p9_write,
    .stat = p9_stat,
    .remove = p9_remove,
    .clunk = p9_clunk,
};

static int
nsd_strlen(const char *s)
{
  int n;

  for (n = 0; s[n] != 0; n++)
    ;
  return n;
}

static int
nsd_streq(const char *a, const char *b)
{
  int i;

  for (i = 0; a[i] != 0 && b[i] != 0; i++) {
    if (a[i] != b[i])
      return 0;
  }
  return a[i] == 0 && b[i] == 0;
}

static int
nsd_startswith(const char *s, const char *prefix)
{
  int i;

  for (i = 0; prefix[i] != 0; i++) {
    if (s[i] != prefix[i])
      return 0;
  }
  return 1;
}

static int
nsd_atoi(const char *s)
{
  int sign, v, i;

  sign = 1;
  v = 0;
  i = 0;
  if (s[0] == '-') {
    sign = -1;
    i++;
  }
  for (; s[i] != 0; i++) {
    if (s[i] < '0' || s[i] > '9')
      break;
    v = v * 10 + (s[i] - '0');
  }
  return sign * v;
}

static int
nsd_has_char(const char *s, char ch)
{
  int i;

  for (i = 0; s[i] != 0; i++) {
    if (s[i] == ch)
      return 1;
  }
  return 0;
}

static void
nsd_print(const char *s)
{
  sys_write(1, (void *)s, nsd_strlen(s));
}

static void
nsd_print2(const char *a, const char *b)
{
  nsd_print(a);
  nsd_print(b);
}

static void
make_exchange_path(char *buf, int buflen, int chan_id, const char *suffix)
{
  snprint(buf, buflen, "#X/%d/%s", chan_id, suffix);
}

static void
vfs_lock_acquire(void)
{
  while (__sync_lock_test_and_set(&vfs_lock, 1) != 0)
    sys_sleep(1);
}

static void
vfs_lock_release(void)
{
  __sync_lock_release(&vfs_lock);
}

static int
path_under_mnt(const char *path)
{
  if (!nsd_startswith(path, "/mnt"))
    return 0;
  return path[4] == 0 || path[4] == '/';
}

static int
path_exact_root_publishable(const char *path)
{
  return nsd_streq(path, "/proc") || nsd_streq(path, "/env");
}

static int
relative_from_mnt(const char *path, char *out, int outlen)
{
  const char *src;
  int i;

  if (outlen <= 0)
    return -1;

  if (path_under_mnt(path)) {
    src = path + 4;
    if (*src == '/')
      src++;
  } else if (path[0] != '/') {
    src = path;
  } else {
    return -1;
  }

  for (i = 0; src[i] != 0 && i < outlen - 1; i++)
    out[i] = src[i];
  out[i] = 0;
  if (src[i] != 0)
    return -1;
  return 0;
}

static int
absolute_under_mnt(const char *path, char *out, int outlen)
{
  char rel[NsdMaxRelPath + 1];

  if (relative_from_mnt(path, rel, sizeof(rel)) < 0)
    return -1;
  if (rel[0] == 0)
    return snprint(out, outlen, "/mnt") < outlen ? 0 : -1;
  return snprint(out, outlen, "/mnt/%s", rel) < outlen ? 0 : -1;
}

static int
canonical_target_path(const char *path, char *out, int outlen)
{
  if (path == nil || outlen <= 0)
    return -1;
  if (path[0] == '/')
    return snprint(out, outlen, "%s", path) < outlen ? 0 : -1;
  return absolute_under_mnt(path, out, outlen);
}

static u32int
unpack32(uchar *p)
{
  return (u32int)p[0] | ((u32int)p[1] << 8) | ((u32int)p[2] << 16) |
         ((u32int)p[3] << 24);
}

static int
read_full(int fd, uchar *buf, int len)
{
  int got, n;

  got = 0;
  while (got < len) {
    n = (int)sys_read(fd, buf + got, len - got);
    if (n <= 0)
      return -1;
    got += n;
  }
  return 0;
}

static NsdNode *
node_alloc_locked(const char *name, int kind, int fixed)
{
  int i;
  NsdNode *n;

  for (i = 0; i < NsdMaxNodes; i++) {
    if (nodes[i].used)
      continue;
    n = &nodes[i];
    memset(n, 0, sizeof(*n));
    n->used = 1;
    n->kind = kind;
    n->fixed = fixed;
    n->qid_path = next_qid_path++;
    strncpy(n->name, name, NsdMaxName);
    n->name[NsdMaxName] = 0;
    return n;
  }
  return nil;
}

static NsdMount *
mount_alloc_locked(void)
{
  int i;

  for (i = 0; i < NsdMaxMounts; i++) {
    if (mount_table[i].used)
      continue;
    memset(&mount_table[i], 0, sizeof(mount_table[i]));
    mount_table[i].used = 1;
    return &mount_table[i];
  }
  return nil;
}

static void
mount_free_locked(NsdMount *mnt)
{
  if (mnt != nil)
    memset(mnt, 0, sizeof(*mnt));
}

static int
node_is_dir(NsdNode *n)
{
  return n != nil && n->kind == NodeDir;
}

static void
node_add_child_locked(NsdNode *parent, NsdNode *child)
{
  child->parent = parent;
  child->sibling = parent->child;
  parent->child = child;
}

static NsdNode *
node_find_child_locked(NsdNode *parent, const char *name)
{
  NsdNode *n;

  for (n = parent->child; n != nil; n = n->sibling) {
    if (nsd_streq(n->name, name))
      return n;
  }
  return nil;
}

static NsdNode *
node_walk_one_locked(NsdNode *start, const char *name)
{
  if (nsd_streq(name, "."))
    return start;
  if (nsd_streq(name, ".."))
    return start->parent != nil ? start->parent : start;
  if (!node_is_dir(start))
    return nil;
  return node_find_child_locked(start, name);
}

static NsdNode *
ensure_relative_dir_locked(const char *relpath, int fixed)
{
  char path[NsdMaxRelPath + 1];
  char *p, *next;
  NsdNode *cur, *child;

  if (snprint(path, sizeof(path), "%s", relpath) >= sizeof(path))
    return nil;

  cur = root_node;
  p = path;
  while (*p == '/')
    p++;
  while (*p != 0) {
    next = p;
    while (*next != 0 && *next != '/')
      next++;
    if (*next != 0)
      *next++ = 0;
    if (*p != 0) {
      child = node_find_child_locked(cur, p);
      if (child == nil) {
        child = node_alloc_locked(p, NodeDir, fixed);
        if (child == nil)
          return nil;
        node_add_child_locked(cur, child);
      }
      if (!node_is_dir(child))
        return nil;
      cur = child;
    }
    p = next;
    while (*p == '/')
      p++;
  }
  return cur;
}

static NsdNode *
find_relative_node_locked(const char *relpath)
{
  char path[NsdMaxRelPath + 1];
  char *p, *next;
  NsdNode *cur;

  if (snprint(path, sizeof(path), "%s", relpath) >= sizeof(path))
    return nil;
  if (path[0] == 0)
    return root_node;

  cur = root_node;
  p = path;
  while (*p == '/')
    p++;
  while (*p != 0) {
    next = p;
    while (*next != 0 && *next != '/')
      next++;
    if (*next != 0)
      *next++ = 0;
    cur = node_walk_one_locked(cur, p);
    if (cur == nil)
      return nil;
    p = next;
    while (*p == '/')
      p++;
  }
  return cur;
}

static int
relpath_join(const char *base, const char *name, char *out, int outlen)
{
  if (base[0] == 0)
    return snprint(out, outlen, "%s", name) < outlen ? 0 : -1;
  return snprint(out, outlen, "%s/%s", base, name) < outlen ? 0 : -1;
}

static void
relpath_parent(char *path)
{
  int i;

  for (i = nsd_strlen(path); i > 0; i--) {
    if (path[i - 1] == '/') {
      path[i - 1] = 0;
      return;
    }
  }
  path[0] = 0;
}

static int
compose_bind_target(const NsdMount *mnt, const char *relpath, char *out, int outlen)
{
  if (mnt == nil || mnt->kind != MountBind)
    return -1;
  if (relpath[0] == 0)
    return snprint(out, outlen, "%s", mnt->target) < outlen ? 0 : -1;
  return snprint(out, outlen, "%s/%s", mnt->target, relpath) < outlen ? 0 : -1;
}

static void
node_qid_locked(NsdNode *n, Qid *q)
{
  if (n->mount != nil && n->mount->have_root_qid) {
    *q = n->mount->root_qid;
    return;
  }
  mkqid(q, n->qid_path, 0, n->kind == NodeDir ? QTDIR : QTFILE);
}

static NsdFidState *
state_alloc(void)
{
  void *p;

  if (pebble_alloc(sizeof(NsdFidState), &p) < 0)
    return nil;
  memset(p, 0, sizeof(NsdFidState));
  return (NsdFidState *)p;
}

static void
state_free(NsdFidState *st)
{
  if (st != nil)
    pebble_free(st);
}

static NsdFidState *
state_clone(NsdFidState *src)
{
  NsdFidState *dst;

  dst = state_alloc();
  if (dst == nil)
    return nil;
  memcpy(dst, src, sizeof(*dst));
  return dst;
}

static void
state_init_local(NsdFidState *st, NsdNode *node)
{
  memset(st, 0, sizeof(*st));
  st->kind = StateLocal;
  st->node = node;
  vfs_lock_acquire();
  node_qid_locked(node, &st->qid);
  vfs_lock_release();
}

static void
state_init_remote(NsdFidState *st, NsdNode *node, const char *relpath, Qid *qid)
{
  memset(st, 0, sizeof(*st));
  st->kind = StateRemote;
  st->node = node;
  strncpy(st->relpath, relpath, NsdMaxRelPath);
  st->relpath[NsdMaxRelPath] = 0;
  st->qid = *qid;
}

static void
state_init_bind(NsdFidState *st, NsdNode *node, const char *relpath, Qid *qid)
{
  memset(st, 0, sizeof(*st));
  st->kind = StateBind;
  st->node = node;
  strncpy(st->relpath, relpath, NsdMaxRelPath);
  st->relpath[NsdMaxRelPath] = 0;
  st->qid = *qid;
}

static NsdFidState *
state_from_fid(Req *r)
{
  if (r->fid != nil && r->fid->aux != nil)
    return (NsdFidState *)r->fid->aux;
  return nil;
}

static void
fid_replace_state(Fid *fid, NsdFidState *state)
{
  NsdFidState *old;

  if (fid == nil)
    return;
  old = (NsdFidState *)fid->aux;
  fid->aux = state;
  if (state != nil)
    fid->qid = state->qid;
  if (old != nil && old != state)
    state_free(old);
}

static int
publishable_read(const char *path, char *buf, int len)
{
  int fd, n, i;

  if (len <= 1)
    return -1;
  fd = sys_open((char *)path, OREAD);
  if (fd < 0)
    return -1;
  n = (int)sys_read(fd, buf, len - 1);
  sys_close(fd);
  if (n <= 0)
    return -1;
  buf[n] = 0;
  for (i = 0; buf[i] != 0; i++) {
    if (buf[i] == '\n' || buf[i] == '\r') {
      buf[i] = 0;
      break;
    }
  }
  return 0;
}

static int
split_path_elems(char *path, char *elems[], int maxelems)
{
  int n;
  char *p, *start;

  n = 0;
  p = path;
  while (*p == '/')
    p++;
  while (*p != 0) {
    if (n >= maxelems)
      return -1;
    start = p;
    while (*p != 0 && *p != '/')
      p++;
    if (*p != 0)
      *p++ = 0;
    elems[n++] = start;
    while (*p == '/')
      p++;
  }
  return n;
}

static int
remote_rpc(RemoteConn *conn, Fcall *tx, Fcall *rx, uchar *rxbuf, int rxbuflen)
{
  uchar txbuf[NsdMaxRpc];
  int n;
  u32int msglen;

  memset(rx, 0, sizeof(*rx));
  n = (int)convS2M(tx, txbuf, sizeof(txbuf));
  if (n <= 0)
    return -1;
  if (sys_write(conn->fd, txbuf, n) != n)
    return -1;
  if (read_full(conn->fd, rxbuf, 4) < 0)
    return -1;
  msglen = unpack32(rxbuf);
  if (msglen < 7 || msglen > (u32int)rxbuflen)
    return -1;
  if (read_full(conn->fd, rxbuf + 4, (int)msglen - 4) < 0)
    return -1;
  if (convM2S(rxbuf, msglen, rx) != msglen)
    return -1;
  if (rx->type == Rerror)
    return -1;
  return 0;
}

static void
remote_conn_close(RemoteConn *conn)
{
  if (conn->fd >= 0)
    sys_close(conn->fd);
  conn->fd = -1;
}

static int
remote_conn_open(RemoteConn *conn, NsdMount *mnt)
{
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];

  memset(conn, 0, sizeof(*conn));
  conn->fd = sys_open(mnt->endpoint, ORDWR);
  if (conn->fd < 0)
    return -1;
  conn->next_tag = 1;

  memset(&tx, 0, sizeof(tx));
  tx.type = Tversion;
  tx.tag = NOTAG;
  tx.msize = NsdMaxRpc;
  tx.version = VERSION9P;
  if (remote_rpc(conn, &tx, &rx, rxbuf, sizeof(rxbuf)) < 0) {
    remote_conn_close(conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Tattach;
  tx.tag = conn->next_tag++;
  tx.fid = 1;
  tx.afid = NOFID;
  tx.uname = "nsd";
  tx.aname = mnt->aname[0] != 0 ? mnt->aname : "";
  if (remote_rpc(conn, &tx, &rx, rxbuf, sizeof(rxbuf)) < 0) {
    remote_conn_close(conn);
    return -1;
  }
  conn->root_qid = rx.qid;
  return 0;
}

static int
remote_conn_walk_path(RemoteConn *conn, const char *relpath, Qid *qid_out)
{
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  char path[NsdMaxRelPath + 1];
  char *names[MAXWELEM];
  int nname, i, chunk, off;
  Qid last;

  last = conn->root_qid;
  if (relpath == nil || relpath[0] == 0) {
    if (qid_out != nil)
      *qid_out = last;
    return 0;
  }
  if (snprint(path, sizeof(path), "%s", relpath) >= sizeof(path))
    return -1;
  nname = split_path_elems(path, names, MAXWELEM);
  if (nname < 0)
    return -1;

  off = 0;
  while (off < nname) {
    chunk = nname - off;
    if (chunk > MAXWELEM)
      chunk = MAXWELEM;
    memset(&tx, 0, sizeof(tx));
    tx.type = Twalk;
    tx.tag = conn->next_tag++;
    tx.fid = 1;
    tx.newfid = 1;
    tx.nwname = chunk;
    for (i = 0; i < chunk; i++)
      tx.wname[i] = names[off + i];
    if (remote_rpc(conn, &tx, &rx, rxbuf, sizeof(rxbuf)) < 0)
      return -1;
    if (rx.nwqid != chunk)
      return -1;
    last = rx.wqid[chunk - 1];
    off += chunk;
  }

  if (qid_out != nil)
    *qid_out = last;
  return 0;
}

static int
remote_query_qid(NsdMount *mnt, const char *relpath, Qid *qid_out)
{
  RemoteConn conn;
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, relpath, qid_out);
  remote_conn_close(&conn);
  return rc;
}

static int
remote_open_path(NsdMount *mnt, const char *relpath, int mode, Qid *qid_out,
                 u32int *iounit_out)
{
  RemoteConn conn;
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, relpath, nil);
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Topen;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  tx.mode = mode & 3;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  remote_conn_close(&conn);
  if (rc < 0)
    return -1;
  if (qid_out != nil)
    *qid_out = rx.qid;
  if (iounit_out != nil)
    *iounit_out = rx.iounit;
  return 0;
}

static int
remote_create_path(NsdMount *mnt, const char *parent_rel, const char *name,
                   int mode, u32int perm, Qid *qid_out, u32int *iounit_out)
{
  RemoteConn conn;
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, parent_rel, nil);
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Tcreate;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  tx.name = (char *)name;
  tx.perm = perm;
  tx.mode = mode & 3;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  remote_conn_close(&conn);
  if (rc < 0)
    return -1;
  if (qid_out != nil)
    *qid_out = rx.qid;
  if (iounit_out != nil)
    *iounit_out = rx.iounit;
  return 0;
}

static int
remote_read_path(NsdMount *mnt, const char *relpath, int mode, vlong offset,
                 u32int count, char *buf, u32int *outcount)
{
  RemoteConn conn;
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, relpath, nil);
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Topen;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  tx.mode = mode >= 0 ? (mode & 3) : OREAD;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Tread;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  tx.offset = offset;
  tx.count = count;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  remote_conn_close(&conn);
  if (rc < 0)
    return -1;
  if (rx.count > count)
    return -1;
  if (rx.count > 0)
    memcpy(buf, rx.data, rx.count);
  if (outcount != nil)
    *outcount = rx.count;
  return 0;
}

static int
remote_write_path(NsdMount *mnt, const char *relpath, int mode, vlong offset,
                  char *buf, u32int count, u32int *outcount)
{
  RemoteConn conn;
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, relpath, nil);
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Topen;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  tx.mode = mode >= 0 ? (mode & 3) : OWRITE;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Twrite;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  tx.offset = offset;
  tx.count = count;
  tx.data = buf;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  remote_conn_close(&conn);
  if (rc < 0)
    return -1;
  if (outcount != nil)
    *outcount = rx.count;
  return 0;
}

static int
remote_stat_path(NsdMount *mnt, const char *relpath, uchar *statbuf, uint *nstat)
{
  RemoteConn conn;
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, relpath, nil);
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Tstat;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  remote_conn_close(&conn);
  if (rc < 0)
    return -1;
  memcpy(statbuf, rx.stat, rx.nstat);
  if (nstat != nil)
    *nstat = rx.nstat;
  return 0;
}

static int
remote_remove_path(NsdMount *mnt, const char *relpath)
{
  RemoteConn conn;
  Fcall tx, rx;
  uchar rxbuf[NsdMaxRpc];
  int rc;

  if (remote_conn_open(&conn, mnt) < 0)
    return -1;
  rc = remote_conn_walk_path(&conn, relpath, nil);
  if (rc < 0) {
    remote_conn_close(&conn);
    return -1;
  }

  memset(&tx, 0, sizeof(tx));
  tx.type = Tremove;
  tx.tag = conn.next_tag++;
  tx.fid = 1;
  rc = remote_rpc(&conn, &tx, &rx, rxbuf, sizeof(rxbuf));
  remote_conn_close(&conn);
  return rc;
}

static int resolve_absolute_state(const char *abs_path, NsdFidState *out,
                                  int depth);

static int
resolve_bind_state(NsdFidState *src, NsdFidState *out, int depth)
{
  char target[NsdMaxCtl];

  if (depth > NsdMaxResolveDepth)
    return -1;
  if (src->node == nil || src->node->mount == nil ||
      src->node->mount->kind != MountBind)
    return -1;
  if (compose_bind_target(src->node->mount, src->relpath, target, sizeof(target)) < 0)
    return -1;
  return resolve_absolute_state(target, out, depth + 1);
}

static int
resolve_concrete_state(NsdFidState *src, NsdFidState *out, int depth)
{
  NsdFidState cur, next;

  if (src == nil || out == nil)
    return -1;
  cur = *src;
  while (cur.kind == StateBind) {
    if (resolve_bind_state(&cur, &next, depth) < 0)
      return -1;
    cur = next;
    depth++;
    if (depth > NsdMaxResolveDepth)
      return -1;
  }
  *out = cur;
  return 0;
}

static int
bind_query_qid(NsdNode *node, const char *relpath, Qid *qid_out, int depth)
{
  NsdFidState resolved;
  NsdFidState concrete;
  char target[NsdMaxCtl];

  if (depth > NsdMaxResolveDepth)
    return -1;
  if (node == nil || node->mount == nil || node->mount->kind != MountBind)
    return -1;
  if (compose_bind_target(node->mount, relpath, target, sizeof(target)) < 0)
    return -1;
  if (resolve_absolute_state(target, &resolved, depth + 1) < 0)
    return -1;
  if (resolve_concrete_state(&resolved, &concrete, depth + 1) < 0)
    return -1;
  *qid_out = concrete.qid;
  return 0;
}

static int
walk_one_state(NsdFidState *src, const char *name, NsdFidState *dst, int depth)
{
  NsdNode *child;
  NsdMount *mnt;
  char rel[NsdMaxRelPath + 1];
  Qid qid;

  if (src == nil || dst == nil)
    return -1;

  switch (src->kind) {
  case StateLocal:
    if (nsd_streq(name, ".")) {
      *dst = *src;
      return 0;
    }
    if (nsd_streq(name, "..")) {
      state_init_local(dst, src->node->parent != nil ? src->node->parent : src->node);
      return 0;
    }
    if (!node_is_dir(src->node))
      return -1;
    vfs_lock_acquire();
    child = node_find_child_locked(src->node, name);
    if (child != nil) {
      mnt = child->mount;
      node_qid_locked(child, &qid);
    } else {
      mnt = nil;
    }
    vfs_lock_release();
    if (child == nil)
      return -1;
    if (mnt == nil) {
      state_init_local(dst, child);
      return 0;
    }
    if (mnt->kind == MountEndpoint) {
      if (remote_query_qid(mnt, "", &qid) < 0)
        return -1;
      state_init_remote(dst, child, "", &qid);
      return 0;
    }
    if (mnt->kind == MountBind) {
      if (bind_query_qid(child, "", &qid, depth + 1) < 0)
        return -1;
      state_init_bind(dst, child, "", &qid);
      return 0;
    }
    return -1;

  case StateRemote:
    if (nsd_streq(name, ".")) {
      *dst = *src;
      return 0;
    }
    if (nsd_streq(name, "..")) {
      if (src->relpath[0] == 0) {
        state_init_local(dst,
                         src->node->parent != nil ? src->node->parent : src->node);
        return 0;
      }
      strncpy(rel, src->relpath, sizeof(rel) - 1);
      rel[sizeof(rel) - 1] = 0;
      relpath_parent(rel);
      if (remote_query_qid(src->node->mount, rel, &qid) < 0)
        return -1;
      state_init_remote(dst, src->node, rel, &qid);
      return 0;
    }
    if (relpath_join(src->relpath, name, rel, sizeof(rel)) < 0)
      return -1;
    if (remote_query_qid(src->node->mount, rel, &qid) < 0)
      return -1;
    state_init_remote(dst, src->node, rel, &qid);
    return 0;

  case StateBind:
    if (nsd_streq(name, ".")) {
      *dst = *src;
      return 0;
    }
    if (nsd_streq(name, "..")) {
      if (src->relpath[0] == 0) {
        state_init_local(dst,
                         src->node->parent != nil ? src->node->parent : src->node);
        return 0;
      }
      strncpy(rel, src->relpath, sizeof(rel) - 1);
      rel[sizeof(rel) - 1] = 0;
      relpath_parent(rel);
      if (bind_query_qid(src->node, rel, &qid, depth + 1) < 0)
        return -1;
      state_init_bind(dst, src->node, rel, &qid);
      return 0;
    }
    if (relpath_join(src->relpath, name, rel, sizeof(rel)) < 0)
      return -1;
    if (bind_query_qid(src->node, rel, &qid, depth + 1) < 0)
      return -1;
    state_init_bind(dst, src->node, rel, &qid);
    return 0;
  }

  return -1;
}

static int
resolve_absolute_state(const char *abs_path, NsdFidState *out, int depth)
{
  char rel[NsdMaxRelPath + 1];
  char path[NsdMaxRelPath + 1];
  char *names[MAXWELEM];
  int nname, i;
  NsdFidState cur, next;

  if (depth > NsdMaxResolveDepth)
    return -1;
  if (relative_from_mnt(abs_path, rel, sizeof(rel)) < 0)
    return -1;
  state_init_local(&cur, root_node);
  if (rel[0] == 0) {
    *out = cur;
    return 0;
  }
  if (snprint(path, sizeof(path), "%s", rel) >= sizeof(path))
    return -1;
  nname = split_path_elems(path, names, MAXWELEM);
  if (nname < 0)
    return -1;
  for (i = 0; i < nname; i++) {
    if (walk_one_state(&cur, names[i], &next, depth + 1) < 0)
      return -1;
    cur = next;
  }
  *out = cur;
  return 0;
}

static int
ensure_dir_path(const char *path)
{
  char rel[NsdMaxRelPath + 1];
  NsdNode *n;

  if (!path_under_mnt(path))
    return -1;

  if (relative_from_mnt(path, rel, sizeof(rel)) < 0)
    return -1;

  vfs_lock_acquire();
  n = ensure_relative_dir_locked(rel, 0);
  vfs_lock_release();
  return n != nil ? 0 : -1;
}

static int
publish_root_endpoint(const char *endpoint, const char *mountpoint, const char *aname)
{
  int fd;
  int rc;

  fd = sys_open((char *)endpoint, ORDWR);
  if (fd < 0)
    return -1;
  rc = sys_nsroot_publish_raw(fd, (char *)mountpoint, (char *)(aname ? aname : ""));
  sys_close(fd);
  return rc;
}

static int
install_endpoint_mount(const char *endpoint, const char *mountpoint, int flags,
                       const char *aname)
{
  char rel[NsdMaxRelPath + 1];
  NsdMount probe;
  NsdNode *node;
  NsdMount *mnt;
  Qid root_qid;

  if (path_exact_root_publishable(mountpoint))
    return publish_root_endpoint(endpoint, mountpoint, aname);
  if (!path_under_mnt(mountpoint))
    return -1;
  if ((flags & (MBEFORE | MAFTER)) != 0)
    return -1;
  if (relative_from_mnt(mountpoint, rel, sizeof(rel)) < 0)
    return -1;

  memset(&probe, 0, sizeof(probe));
  probe.kind = MountEndpoint;
  strncpy(probe.endpoint, endpoint, NsdMaxEndpoint);
  probe.endpoint[NsdMaxEndpoint] = 0;
  if (aname != nil) {
    strncpy(probe.aname, aname, NsdMaxName);
    probe.aname[NsdMaxName] = 0;
  }
  if (remote_query_qid(&probe, "", &root_qid) < 0)
    return -1;

  vfs_lock_acquire();
  node = ensure_relative_dir_locked(rel, 0);
  if (node == nil) {
    vfs_lock_release();
    return -1;
  }
  if (node->mount == nil) {
    node->mount = mount_alloc_locked();
    if (node->mount == nil) {
      vfs_lock_release();
      return -1;
    }
  }
  mnt = node->mount;
  memset(mnt, 0, sizeof(*mnt));
  mnt->used = 1;
  mnt->kind = MountEndpoint;
  mnt->flags = flags;
  strncpy(mnt->endpoint, endpoint, NsdMaxEndpoint);
  mnt->endpoint[NsdMaxEndpoint] = 0;
  if (aname != nil) {
    strncpy(mnt->aname, aname, NsdMaxName);
    mnt->aname[NsdMaxName] = 0;
  }
  mnt->root_qid = root_qid;
  mnt->have_root_qid = 1;
  vfs_lock_release();
  return 0;
}

static int
install_endpoint_file_mount(const char *endpoint_file, const char *mountpoint,
                            int flags, const char *aname)
{
  char endpoint[NsdMaxEndpoint + 1];

  if (publishable_read(endpoint_file, endpoint, sizeof(endpoint)) < 0)
    return -1;
  return install_endpoint_mount(endpoint, mountpoint, flags, aname);
}

static int
install_bind_mount(const char *src, const char *dst, int flags)
{
  char abs_src[NsdMaxCtl];
  char abs_dst[NsdMaxCtl];
  char rel[NsdMaxRelPath + 1];
  NsdFidState resolved;
  NsdFidState concrete;
  NsdNode *node;
  NsdMount *mnt;

  if ((flags & (MBEFORE | MAFTER)) != 0)
    return -1;
  if (canonical_target_path(src, abs_src, sizeof(abs_src)) < 0)
    return -1;
  if (canonical_target_path(dst, abs_dst, sizeof(abs_dst)) < 0)
    return -1;
  if (!path_under_mnt(abs_src) || !path_under_mnt(abs_dst))
    return -1;
  if (resolve_absolute_state(abs_src, &resolved, 0) < 0)
    return -1;
  if (resolve_concrete_state(&resolved, &concrete, 0) < 0)
    return -1;
  if (relative_from_mnt(abs_dst, rel, sizeof(rel)) < 0)
    return -1;

  vfs_lock_acquire();
  node = ensure_relative_dir_locked(rel, 0);
  if (node == nil) {
    vfs_lock_release();
    return -1;
  }
  if (node->mount == nil) {
    node->mount = mount_alloc_locked();
    if (node->mount == nil) {
      vfs_lock_release();
      return -1;
    }
  }
  mnt = node->mount;
  memset(mnt, 0, sizeof(*mnt));
  mnt->used = 1;
  mnt->kind = MountBind;
  mnt->flags = flags;
  strncpy(mnt->target, abs_src, NsdMaxRelPath);
  mnt->target[NsdMaxRelPath] = 0;
  mnt->root_qid = concrete.qid;
  mnt->have_root_qid = 1;
  vfs_lock_release();
  return 0;
}

static int
remove_mount_target(const char *path)
{
  char rel[NsdMaxRelPath + 1];
  NsdNode *node;

  if (path_exact_root_publishable(path))
    return sys_nsroot_unpublish_raw((char *)path);
  if (!path_under_mnt(path))
    return -1;
  if (relative_from_mnt(path, rel, sizeof(rel)) < 0)
    return -1;

  vfs_lock_acquire();
  node = find_relative_node_locked(rel);
  if (node == nil || node->mount == nil) {
    vfs_lock_release();
    return -1;
  }
  mount_free_locked(node->mount);
  node->mount = nil;
  vfs_lock_release();
  return 0;
}

static void
build_ctl_text(char *buf, int buflen)
{
  snprint(buf, buflen,
          "nsd /mnt ctl\n"
          "commands:\n"
          "mkdir <path>\n"
          "mount <endpoint> <path> [flags] [aname]\n"
          "mountfile <endpoint-file> <path> [flags] [aname]\n"
          "bind <src> <dst> <flags>\n"
          "unmount <name> <old>\n"
          "rescan\n");
}

static void
build_mounts_text(char *buf, int buflen)
{
  int i, n;

  n = snprint(buf, buflen, "nsd mounts\n");
  for (i = 0; i < (int)(sizeof(mounts) / sizeof(mounts[0])); i++) {
    if (n >= buflen)
      break;
    n += snprint(buf + n, buflen - n, "%s %s %s",
                 mounts[i].mounted ? "mounted" : "pending", mounts[i].mountpoint,
                 mounts[i].endpoint_file);
    if (mounts[i].mounted)
      n += snprint(buf + n, buflen - n, " %s", mounts[i].endpoint);
    n += snprint(buf + n, buflen - n, "\n");
  }
}

static void
respond_text(Req *r, const char *text)
{
  int len, count;

  len = nsd_strlen(text);
  if (r->ifcall.offset >= (vlong)len) {
    r->ofcall.count = 0;
    srv_respond(r, nil);
    return;
  }
  count = (int)r->ifcall.count;
  if (count > len - (int)r->ifcall.offset)
    count = len - (int)r->ifcall.offset;
  memcpy(r->ofcall.data, text + (int)r->ifcall.offset, count);
  r->ofcall.count = count;
  srv_respond(r, nil);
}

static void
respond_dynamic_text(Req *r, void (*builder)(char *, int))
{
  char buf[1024];

  memset(buf, 0, sizeof(buf));
  builder(buf, sizeof(buf));
  respond_text(r, buf);
}

static void
fill_dir(Dir *d, NsdNode *n)
{
  Qid qid;
  int isdir;

  memset(d, 0, sizeof(*d));
  d->name = n == root_node ? "mnt" : n->name;
  node_qid_locked(n, &qid);
  d->qid = qid;
  isdir = (qid.type & QTDIR) != 0;
  d->mode = (isdir ? DMDIR : 0) |
            (n->kind == NodeCtl ? 0666 : (n->kind == NodeMounts ? 0444 : 0777));
  d->uid = "lux";
  d->gid = "lux";
  d->muid = "lux";
}

static void
respond_dir_read(Req *r, NsdNode *dir)
{
  uchar *data;
  NsdNode *n;
  vlong off;
  int max, used, sz;
  Dir d;

  data = (uchar *)r->ofcall.data;
  max = (int)r->ifcall.count;
  used = 0;
  off = 0;

  vfs_lock_acquire();
  for (n = dir->child; n != nil; n = n->sibling) {
    fill_dir(&d, n);
    sz = (int)sizeD2M(&d);
    if (off >= r->ifcall.offset) {
      if (used + sz > max)
        break;
      used += (int)convD2M(&d, data + used, max - used);
    }
    off += sz;
  }
  vfs_lock_release();

  r->ofcall.count = used;
  srv_respond(r, nil);
}

static int
ctl_command(Req *r, const char *cmd, int len)
{
  char buf[NsdMaxCtl];
  char *argv[6];
  int argc;
  char *p;
  char abs_path[NsdMaxCtl];
  int flags;
  char *aname;

  (void)r;
  if (len <= 0 || len >= (int)sizeof(buf))
    return -1;
  memcpy(buf, cmd, len);
  buf[len] = 0;

  argc = 0;
  p = buf;
  while (*p != 0 && argc < (int)(sizeof(argv) / sizeof(argv[0]))) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
      p++;
    if (*p == 0)
      break;
    argv[argc++] = p;
    while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
      p++;
    if (*p != 0)
      *p++ = 0;
  }

  if (argc == 0)
    return -1;

  if (nsd_streq(argv[0], "rescan")) {
    rescan_requested = 1;
    return len;
  }

  if (argc >= 2 && nsd_streq(argv[0], "mkdir")) {
    if (absolute_under_mnt(argv[1], abs_path, sizeof(abs_path)) < 0)
      return -1;
    return ensure_dir_path(abs_path) < 0 ? -1 : len;
  }

  if (argc >= 3 && nsd_streq(argv[0], "mount")) {
    if (canonical_target_path(argv[2], abs_path, sizeof(abs_path)) < 0)
      return -1;
    flags = argc >= 4 ? nsd_atoi(argv[3]) : (MREPL | MCREATE);
    aname = argc >= 5 ? argv[4] : "";
    return install_endpoint_mount(argv[1], abs_path, flags, aname) < 0 ? -1
                                                                        : len;
  }

  if (argc >= 3 && nsd_streq(argv[0], "mountfile")) {
    if (canonical_target_path(argv[2], abs_path, sizeof(abs_path)) < 0)
      return -1;
    flags = argc >= 4 ? nsd_atoi(argv[3]) : (MREPL | MCREATE);
    aname = argc >= 5 ? argv[4] : "";
    return install_endpoint_file_mount(argv[1], abs_path, flags, aname) < 0 ? -1
                                                                             : len;
  }

  if (argc >= 4 && nsd_streq(argv[0], "bind"))
    return install_bind_mount(argv[1], argv[2], nsd_atoi(argv[3])) < 0 ? -1
                                                                        : len;

  if (argc >= 3 && nsd_streq(argv[0], "unmount")) {
    if (canonical_target_path(argv[2], abs_path, sizeof(abs_path)) < 0)
      return -1;
    return remove_mount_target(abs_path) < 0 ? -1 : len;
  }

  return -1;
}

static int
local_open_state(NsdFidState *st, Req *r)
{
  NsdNode *n;

  n = st->node;
  if (n->kind == NodeDir && r->ifcall.mode != OREAD)
    return -1;
  if (n->kind == NodeMounts && r->ifcall.mode != OREAD)
    return -1;
  r->ofcall.qid = st->qid;
  r->ofcall.iounit = 8192;
  return 0;
}

static int
local_create_state(NsdFidState *st, Req *r)
{
  NsdNode *parent, *child;
  NsdFidState *fidst;

  parent = st->node;
  if (parent == nil || !node_is_dir(parent))
    return -1;
  if (r->ifcall.name == nil || r->ifcall.name[0] == 0 ||
      nsd_has_char(r->ifcall.name, '/'))
    return -1;
  if ((r->ifcall.perm & DMDIR) == 0)
    return -1;

  vfs_lock_acquire();
  child = node_find_child_locked(parent, r->ifcall.name);
  if (child != nil) {
    vfs_lock_release();
    return -1;
  }
  child = node_alloc_locked(r->ifcall.name, NodeDir, 0);
  if (child != nil)
    node_add_child_locked(parent, child);
  vfs_lock_release();
  if (child == nil)
    return -1;

  fidst = state_from_fid(r);
  if (fidst == nil)
    return -1;
  state_init_local(fidst, child);
  r->ofcall.qid = fidst->qid;
  r->ofcall.iounit = 8192;
  return 0;
}

static int
local_read_state(NsdFidState *st, Req *r)
{
  NsdNode *n;

  n = st->node;
  if (n->kind == NodeDir) {
    respond_dir_read(r, n);
    return 1;
  }
  if (n->kind == NodeCtl) {
    respond_dynamic_text(r, build_ctl_text);
    return 1;
  }
  if (n->kind == NodeMounts) {
    respond_dynamic_text(r, build_mounts_text);
    return 1;
  }
  return -1;
}

static int
local_write_state(NsdFidState *st, Req *r)
{
  int rc;

  if (st->node->kind != NodeCtl)
    return -1;
  rc = ctl_command(r, r->ifcall.data, (int)r->ifcall.count);
  if (rc < 0)
    return -1;
  r->ofcall.count = r->ifcall.count;
  return 0;
}

static int
local_stat_state(NsdFidState *st, Req *r)
{
  Dir d;
  uchar statbuf[STATFIXLEN + 256];
  uint nstat;
  void *copy;

  vfs_lock_acquire();
  fill_dir(&d, st->node);
  vfs_lock_release();
  nstat = convD2M(&d, statbuf, sizeof(statbuf));
  if (nstat == 0)
    return -1;
  if (pebble_alloc(nstat, &copy) < 0)
    return -1;
  memcpy(copy, statbuf, nstat);
  r->ofcall.stat = copy;
  r->ofcall.nstat = nstat;
  return 0;
}

static int
local_remove_state(NsdFidState *st, Req *r)
{
  NsdNode *n, *cur, **link;

  (void)r;
  n = st->node;
  if (n == nil || n == root_node || n->fixed)
    return -1;

  vfs_lock_acquire();
  if (n->child != nil || n->mount != nil) {
    vfs_lock_release();
    return -1;
  }
  link = &n->parent->child;
  while ((cur = *link) != nil) {
    if (cur == n) {
      *link = n->sibling;
      memset(n, 0, sizeof(*n));
      vfs_lock_release();
      return 0;
    }
    link = &cur->sibling;
  }
  vfs_lock_release();
  return -1;
}

static int
state_dir_access_mode(NsdFidState *st, Fid *fid)
{
  if ((st->qid.type & QTDIR) != 0)
    return OREAD;
  if (fid != nil && fid->omode >= 0)
    return fid->omode;
  return OREAD;
}

static void
p9_attach(Req *r)
{
  NsdFidState *st;

  st = state_alloc();
  if (st == nil) {
    srv_respond(r, "out of memory");
    return;
  }
  state_init_local(st, root_node);
  fid_replace_state(r->fid, st);
  r->ofcall.qid = st->qid;
  srv_respond(r, nil);
}

static void
p9_walk(Req *r)
{
  NsdFidState *cur;
  NsdFidState next;
  NsdFidState *target_state;
  Fid *target;
  int i;

  cur = state_from_fid(r);
  if (cur == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  target = r->newfid != nil ? r->newfid : r->fid;
  if (r->ifcall.nwname == 0) {
    target_state = (target == r->fid) ? cur : state_clone(cur);
    if (target_state == nil) {
      srv_respond(r, "out of memory");
      return;
    }
    if (target != r->fid)
      fid_replace_state(target, target_state);
    target->qid = target_state->qid;
    r->ofcall.nwqid = 0;
    srv_respond(r, nil);
    return;
  }

  next = *cur;
  for (i = 0; i < r->ifcall.nwname; i++) {
    if (walk_one_state(&next, r->ifcall.wname[i], &next, 0) < 0)
      break;
    r->ofcall.wqid[r->ofcall.nwqid++] = next.qid;
  }

  if (r->ofcall.nwqid == 0) {
    srv_respond(r, "file not found");
    return;
  }

  target_state = (target == r->fid) ? cur : state_alloc();
  if (target_state == nil) {
    srv_respond(r, "out of memory");
    return;
  }
  *target_state = next;
  if (target != r->fid)
    fid_replace_state(target, target_state);
  else
    target->qid = target_state->qid;
  srv_respond(r, nil);
}

static void
p9_open(Req *r)
{
  NsdFidState *st;
  NsdFidState concrete;

  st = state_from_fid(r);
  if (st == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  if (resolve_concrete_state(st, &concrete, 0) < 0) {
    srv_respond(r, "file not found");
    return;
  }

  if (concrete.kind == StateLocal) {
    if (local_open_state(&concrete, r) < 0) {
      srv_respond(r, "open failed");
      return;
    }
  } else if (concrete.kind == StateRemote) {
    if (remote_open_path(concrete.node->mount, concrete.relpath, r->ifcall.mode,
                         &st->qid, &r->ofcall.iounit) < 0) {
      srv_respond(r, "open failed");
      return;
    }
    concrete.qid = st->qid;
    r->ofcall.qid = st->qid;
  } else {
    srv_respond(r, "open failed");
    return;
  }

  r->ofcall.qid = st->qid;
  srv_respond(r, nil);
}

static void
p9_create(Req *r)
{
  NsdFidState *st;
  NsdFidState concrete;

  st = state_from_fid(r);
  if (st == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  if (resolve_concrete_state(st, &concrete, 0) < 0) {
    srv_respond(r, "file not found");
    return;
  }

  if (concrete.kind == StateLocal) {
    if (local_create_state(&concrete, r) < 0) {
      srv_respond(r, "create failed");
      return;
    }
    r->ofcall.qid = st->qid;
    srv_respond(r, nil);
    return;
  }

  if (concrete.kind != StateRemote || r->ifcall.name == nil ||
      r->ifcall.name[0] == 0 || nsd_has_char(r->ifcall.name, '/')) {
    srv_respond(r, "create failed");
    return;
  }

  if (remote_create_path(concrete.node->mount, concrete.relpath, r->ifcall.name,
                         r->ifcall.mode, r->ifcall.perm, &st->qid,
                         &r->ofcall.iounit) < 0) {
    srv_respond(r, "create failed");
    return;
  }

  if (st->kind == StateRemote) {
    if (relpath_join(st->relpath, r->ifcall.name, st->relpath, sizeof(st->relpath)) <
        0) {
      srv_respond(r, "create failed");
      return;
    }
  } else if (st->kind == StateBind) {
    if (relpath_join(st->relpath, r->ifcall.name, st->relpath, sizeof(st->relpath)) <
        0) {
      srv_respond(r, "create failed");
      return;
    }
  }

  r->fid->qid = st->qid;
  r->ofcall.qid = st->qid;
  srv_respond(r, nil);
}

static void
p9_read(Req *r)
{
  NsdFidState *st;
  NsdFidState concrete;
  u32int count;
  int handled;

  st = state_from_fid(r);
  if (st == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  if (resolve_concrete_state(st, &concrete, 0) < 0) {
    srv_respond(r, "file not found");
    return;
  }

  if (concrete.kind == StateLocal) {
    handled = local_read_state(&concrete, r);
    if (handled > 0)
      return;
    srv_respond(r, "read not supported");
    return;
  }

  if (concrete.kind != StateRemote ||
      remote_read_path(concrete.node->mount, concrete.relpath,
                       state_dir_access_mode(&concrete, r->fid), r->ifcall.offset,
                       r->ifcall.count, r->ofcall.data, &count) < 0) {
    srv_respond(r, "read failed");
    return;
  }

  r->ofcall.count = count;
  srv_respond(r, nil);
}

static void
p9_write(Req *r)
{
  NsdFidState *st;
  NsdFidState concrete;
  u32int count;

  st = state_from_fid(r);
  if (st == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  if (resolve_concrete_state(st, &concrete, 0) < 0) {
    srv_respond(r, "file not found");
    return;
  }

  if (concrete.kind == StateLocal) {
    if (local_write_state(&concrete, r) < 0) {
      srv_respond(r, "bad ctl command");
      return;
    }
    srv_respond(r, nil);
    return;
  }

  if (concrete.kind != StateRemote ||
      remote_write_path(concrete.node->mount, concrete.relpath, r->fid->omode,
                        r->ifcall.offset, r->ifcall.data, r->ifcall.count,
                        &count) < 0) {
    srv_respond(r, "write failed");
    return;
  }

  r->ofcall.count = count;
  srv_respond(r, nil);
}

static void
p9_stat(Req *r)
{
  NsdFidState *st;
  NsdFidState concrete;
  uchar *copy;
  uchar statbuf[NsdMaxRpc];
  uint nstat;

  st = state_from_fid(r);
  if (st == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  if (resolve_concrete_state(st, &concrete, 0) < 0) {
    srv_respond(r, "file not found");
    return;
  }

  if (concrete.kind == StateLocal) {
    if (local_stat_state(&concrete, r) < 0) {
      srv_respond(r, "stat failed");
      return;
    }
    srv_respond(r, nil);
    return;
  }

  if (concrete.kind != StateRemote ||
      remote_stat_path(concrete.node->mount, concrete.relpath, statbuf, &nstat) <
          0) {
    srv_respond(r, "stat failed");
    return;
  }
  if (pebble_alloc(nstat, (void **)&copy) < 0) {
    srv_respond(r, "out of memory");
    return;
  }
  memcpy(copy, statbuf, nstat);
  r->ofcall.stat = copy;
  r->ofcall.nstat = nstat;
  srv_respond(r, nil);
}

static void
p9_remove(Req *r)
{
  NsdFidState *st;
  NsdFidState concrete;

  st = state_from_fid(r);
  if (st == nil) {
    srv_respond(r, "unknown fid");
    return;
  }

  if (resolve_concrete_state(st, &concrete, 0) < 0) {
    srv_respond(r, "file not found");
    return;
  }

  if ((st->kind == StateRemote || st->kind == StateBind) && st->relpath[0] == 0) {
    srv_respond(r, "cannot remove mount root");
    return;
  }

  if (concrete.kind == StateLocal) {
    if (local_remove_state(&concrete, r) < 0) {
      srv_respond(r, "remove failed");
      return;
    }
  } else if (concrete.kind == StateRemote) {
    if (remote_remove_path(concrete.node->mount, concrete.relpath) < 0) {
      srv_respond(r, "remove failed");
      return;
    }
  } else {
    srv_respond(r, "remove failed");
    return;
  }

  fid_replace_state(r->fid, nil);
  srv_respond(r, nil);
}

static void
p9_clunk(Req *r)
{
  fid_replace_state(r->fid, nil);
  srv_respond(r, nil);
}

static void
server_loop_shim(void *arg)
{
  (void)arg;
  child_ready = 1;
  srv_init(&nsd_srv);
  srv_loop_ring(&nsd_srv, nsd_ring_path);
  sys_exit("nsd srv loop exited");
}

static void
init_tree(void)
{
  NsdNode *ctl, *mounts_file;
  int i;
  char rel[NsdMaxRelPath + 1];

  memset(nodes, 0, sizeof(nodes));
  memset(mount_table, 0, sizeof(mount_table));
  next_qid_path = 1;

  vfs_lock_acquire();
  root_node = node_alloc_locked("mnt", NodeDir, 1);
  ctl = node_alloc_locked("ctl", NodeCtl, 1);
  mounts_file = node_alloc_locked("mounts", NodeMounts, 1);
  if (root_node != nil && ctl != nil)
    node_add_child_locked(root_node, ctl);
  if (root_node != nil && mounts_file != nil)
    node_add_child_locked(root_node, mounts_file);

  for (i = 0; i < (int)(sizeof(mounts) / sizeof(mounts[0])); i++) {
    if (!path_under_mnt(mounts[i].mountpoint))
      continue;
    if (relative_from_mnt(mounts[i].mountpoint, rel, sizeof(rel)) < 0)
      continue;
    ensure_relative_dir_locked(rel, 1);
  }
  vfs_lock_release();
}

int
main(void)
{
  int i, pending, pid, fd_clone, fd_ctl, fd_data, n, chan_id;
  char buf[32];
  char data_path[64];
  char ctl_path[64];
  char endpoint[NsdMaxEndpoint + 1];

  nsd_print("NSD: starting namespace service\n");
  init_tree();

  fd_clone = sys_open("#X/clone", OREAD);
  if (fd_clone < 0) {
    nsd_print("NSD: exchange clone failed\n");
    return 1;
  }
  n = (int)sys_read(fd_clone, buf, sizeof(buf) - 1);
  if (n <= 0) {
    nsd_print("NSD: failed to read exchange channel id\n");
    sys_close(fd_clone);
    return 1;
  }
  buf[n] = 0;
  chan_id = nsd_atoi(buf);

  make_exchange_path(ctl_path, sizeof(ctl_path), chan_id, "ctl");
  make_exchange_path(data_path, sizeof(data_path), chan_id, "data");
  make_exchange_path(nsd_ring_path, sizeof(nsd_ring_path), chan_id, "ipcring");

  fd_ctl = sys_open(ctl_path, OWRITE);
  if (fd_ctl < 0) {
    nsd_print("NSD: failed to open exchange ctl\n");
    sys_close(fd_clone);
    return 1;
  }
  if (sys_write(fd_ctl, "mode service", 12) != 12) {
    nsd_print("NSD: failed to set exchange mode\n");
    sys_close(fd_ctl);
    sys_close(fd_clone);
    return 1;
  }
  sys_close(fd_ctl);
  sys_close(fd_clone);

  fd_data = sys_open(data_path, ORDWR);
  if (fd_data < 0) {
    nsd_print("NSD: failed to open exchange data channel\n");
    return 1;
  }

  child_ready = 0;
  pid = sys_rfork_stack(RFPROC | RFMEM | RFFDG | RFNOWAIT,
                        server_stack + sizeof(server_stack), server_loop_shim,
                        nil);
  if (pid < 0) {
    nsd_print("NSD: rfork failed\n");
    return 1;
  }

  while (!child_ready)
    sys_sleep(1);

  if (sys_srv_publish("/srv/nsd", fd_data) < 0) {
    nsd_print("NSD: failed to publish /srv/nsd.endpoint\n");
    sys_close(fd_data);
    return 1;
  }

  if (sys_nsroot_publish_raw(fd_data, "/mnt", "") < 0) {
    nsd_print("NSD: failed to publish /mnt root\n");
    sys_close(fd_data);
    return 1;
  }
  nsd_print("NSD: published userspace VFS on /mnt\n");

  for (;;) {
    pending = rescan_requested ? 1 : 0;
    rescan_requested = 0;
    for (i = 0; i < (int)(sizeof(mounts) / sizeof(mounts[0])); i++) {
      if (mounts[i].mounted)
        continue;
      pending = 1;
      if (publishable_read(mounts[i].endpoint_file, endpoint, sizeof(endpoint)) <
          0)
        continue;
      if (install_endpoint_mount(endpoint, mounts[i].mountpoint, MREPL | MCREATE,
                                 "") < 0)
        continue;
      strncpy(mounts[i].endpoint, endpoint, NsdMaxEndpoint);
      mounts[i].endpoint[NsdMaxEndpoint] = 0;
      mounts[i].mounted = 1;
      nsd_print("NSD: mounted ");
      nsd_print((char *)mounts[i].mountpoint);
      nsd_print(" from ");
      nsd_print2(endpoint, "\n");
    }
    sys_sleep(pending ? 100 : 1000);
  }
}
