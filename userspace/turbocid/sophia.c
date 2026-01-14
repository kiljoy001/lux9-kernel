#include "../lib/liblux/inc/lux.h"
#include "../lib/liblux/inc/server9p.h"
#include "storage/dat.h"

#define DMDIR 0x80000000
#define QTDIR 0x80
#define QTFILE 0x00
#define nil ((void *)0)

/* liblux wrappers */
void *malloc(ulong size) {
  void *p = nil;
  if (pebble_alloc(size, &p) < 0)
    return nil;
  return p;
}

void free(void *ptr) { pebble_free(ptr); }

extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dest, const void *src, ulong n);
extern ulong strlen(const char *s);
extern long sys_write(int fd, void *buf, long n);

/* Sartfs Externs */
extern int sart_init(int disk_fd, int journal_fd, u64int total_blocks);
extern RecordData *art_search(const UUIDv8 *key);
extern UUIDv8 *ns_art_search(const UUIDv8 *parent, const char *name);
extern int ns_art_insert(const UUIDv8 *parent, const char *name,
                         const UUIDv8 *child);
extern int sart_write_immutable(void *data, u64int len, u32int perms,
                                UUIDv8 *id_out);
extern int sart_add_edge(UUIDv8 *parent, const char *name, UUIDv8 *child);
extern int sart_mkdir(UUIDv8 *parent, const char *name, u32int perms,
                      UUIDv8 *id_out);

/*
 * SophiaNode: A node in the DAG.
 */
typedef struct SophiaNode {
  UUIDv8 cid;        /* The Pointer (UUIDv8 Content Identity) */
  UUIDv8 parent_cid; /* Stable parent anchor */
  char name[128];    /* Name in parent directory */
  RecordData *rec;   /* Persistent metadata (if JENT_BLOB) */
  u32int mode;       /* QID type | permissions */
  void *data;        /* Cached data pointer */
} SophiaNode;

#define EDGE_KEY_MAX 256
static SophiaNode *root_node;

/*
 * Edge Index Lookup
 * Maps (ParentUUID + Name) -> SophiaNode*
 */
static SophiaNode *edge_lookup(SophiaNode *parent, char *name) {
  UUIDv8 *child_id = ns_art_search(&parent->cid, name);
  if (child_id == nil)
    return nil;

  RecordData *rec = art_search(child_id);
  if (rec == nil) {
    /* Potential inconsistency handled by fallback */
  }

  SophiaNode *node = malloc(sizeof(SophiaNode));
  if (node == nil)
    return nil;

  memset(node, 0, sizeof(SophiaNode));
  memmove(&node->cid, child_id, 16);
  memmove(&node->parent_cid, &parent->cid, 16);

  int nlen = strlen(name);
  if (nlen > 127)
    nlen = 127;
  memmove(node->name, name, nlen);
  node->name[nlen] = '\0';

  node->rec = rec;
  if (rec)
    node->mode = rec->perms;
  else
    node->mode = DMDIR | 0755;

  return node;
}

/* Bootstrap the FS with persistent Sartfs backend */
static void sophia_bootstrap(void) {
  /* disk_fd = 3, journal_fd = 4 assigned by init */
  int rc = sart_init(3, 4, 1024);
  if (rc != SART_OK) {
    char err[] = "Sophia: sart_init failed, continuing anyway\n";
    sys_write(2, err, sizeof(err) - 1);
  }

  /* Root Node bootstrap */
  root_node = malloc(sizeof(SophiaNode));
  if (root_node) {
    memset(root_node, 0, sizeof(SophiaNode));
    root_node->mode = DMDIR | 0755;
    /* Root CID is all zeros by default */
  }
}

/* ========================================================================= */
/* 9P CALLBACKS                                                              */
/* ========================================================================= */

static void sophia_attach(Req *r) {
  if (!root_node)
    sophia_bootstrap();

  /* Bind the FID to the Root Node */
  r->fid->aux = root_node;
  r->fid->qid.type = QTDIR;
  r->fid->qid.path = *(u64int *)root_node->cid.data;
  r->fid->qid.vers = 0;

  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void sophia_walk(Req *r) {
  SophiaNode *curr = r->fid->aux;
  SophiaNode *next;
  int i;

  if (r->newfid) {
    r->newfid->aux = curr;
    r->newfid->qid = r->fid->qid;
  }

  if (r->ifcall.nwname > 0) {
    for (i = 0; i < r->ifcall.nwname; i++) {
      next = edge_lookup(curr, r->ifcall.wname[i]);
      if (!next) {
        if (i == 0)
          srv_respond(r, "file not found");
        else {
          r->ofcall.nwqid = i;
          srv_respond(r, nil);
        }
        return;
      }

      curr = next;
      r->ofcall.wqid[i].type = (curr->mode & DMDIR) ? QTDIR : QTFILE;
      r->ofcall.wqid[i].path = *(u64int *)curr->cid.data;
      r->ofcall.wqid[i].vers = 0;
    }

    if (r->newfid)
      r->newfid->aux = curr;
    else
      r->fid->aux = curr;
  }

  r->ofcall.nwqid = r->ifcall.nwname;
  srv_respond(r, nil);
}

static void sophia_create(Req *r) {
  SophiaNode *parent = r->fid->aux;
  char *name = r->ifcall.name;
  u32int perm = r->ifcall.perm;
  UUIDv8 child_id;
  int rc;

  if (perm & DMDIR) {
    rc = sart_mkdir(&parent->cid, name, perm, &child_id);
  } else {
    rc = sart_write_immutable(nil, 0, perm, &child_id);
    if (rc == 0) {
      rc = sart_add_edge(&parent->cid, name, &child_id);
    }
  }

  if (rc != 0) {
    srv_respond(r, "create failed");
    return;
  }

  SophiaNode *node = malloc(sizeof(SophiaNode));
  if (!node) {
    srv_respond(r, "out of memory");
    return;
  }
  memset(node, 0, sizeof(SophiaNode));
  memmove(&node->cid, &child_id, 16);
  memmove(&node->parent_cid, &parent->cid, 16);
  int nlen = strlen(name);
  if (nlen > 127)
    nlen = 127;
  memmove(node->name, name, nlen);
  node->name[nlen] = '\0';
  node->mode = perm;
  node->rec = art_search(&node->cid);

  r->fid->aux = node;
  r->fid->qid.type = (perm & DMDIR) ? QTDIR : QTFILE;
  r->fid->qid.path = (u64int)node->rec; /* Temporary path */
  r->fid->qid.vers = 0;

  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void sophia_read(Req *r) {
  SophiaNode *node = r->fid->aux;
  extern long sart_read(UUIDv8 * id, void *buf, u64int len);

  if (node->mode & DMDIR) {
    srv_respond(r, "directory read not implemented");
    return;
  }

  long n = sart_read(&node->cid, r->ofcall.data, r->ifcall.count);
  if (n < 0) {
    srv_respond(r, "read failed");
    return;
  }

  r->ofcall.count = n;
  srv_respond(r, nil);
}

static void sophia_write(Req *r) {
  SophiaNode *node = r->fid->aux;
  if (node->mode & DMDIR) {
    srv_respond(r, "cannot write to directory");
    return;
  }

  UUIDv8 new_cid;
  int rc = sart_write_immutable(r->ifcall.data, r->ifcall.count, node->mode,
                                &new_cid);
  if (rc != 0) {
    srv_respond(r, "write failed");
    return;
  }

  rc = sart_add_edge(&node->parent_cid, node->name, &new_cid);
  if (rc != 0) {
    srv_respond(r, "namespace update failed");
    return;
  }

  memmove(&node->cid, &new_cid, 16);
  node->rec = art_search(&node->cid);

  r->ofcall.count = r->ifcall.count;
  srv_respond(r, nil);
}

static void sophia_stat(Req *r) {
  SophiaNode *node = r->fid->aux;
  extern uint convD2M(Dir * d, uchar * buf, uint nbuf);
  extern uint sizeD2M(Dir * d);

  Dir d;
  memset(&d, 0, sizeof(Dir));

  d.qid.type = (node->mode & DMDIR) ? QTDIR : QTFILE;
  d.qid.path = *(u64int *)node->cid.data;
  d.mode = node->mode;
  d.length = node->rec ? node->rec->size : 0;
  d.atime = node->rec ? node->rec->atime : 0;
  d.mtime = node->rec ? node->rec->mtime : 0;
  d.name = node->name;
  d.uid = "root";
  d.gid = "root";
  d.muid = "root";

  uint sz = sizeD2M(&d);
  r->ofcall.stat = malloc(sz);
  if (r->ofcall.stat) {
    r->ofcall.nstat = convD2M(&d, (uchar *)r->ofcall.stat, sz);
  }

  srv_respond(r, nil);
}

int main(int argc, char **argv) {
  /* Debug: Print startup message */
  char msg[] = "Sophia: starting up\n";
  sys_write(2, msg, sizeof(msg) - 1);

  sophia_bootstrap();

  Srv s = {
      .attach = sophia_attach,
      .walk = sophia_walk,
      .read = sophia_read,
      .write = sophia_write,
      .create = sophia_create,
      .stat = sophia_stat,
  };

  srv_init(&s);

  char srv_msg[] = "Sophia: entering srv_loop\n";
  sys_write(2, srv_msg, sizeof(srv_msg) - 1);

  if (argc > 1) {
    int fd = 0;
    char *p = argv[1];
    while (*p >= '0' && *p <= '9') {
      fd = fd * 10 + (*p - '0');
      p++;
    }
    srv_loop(&s, fd, fd);
  } else {
    /* Default: use stdin/stdout for testing */
    srv_loop(&s, 0, 1);
  }
  return 0;
}
