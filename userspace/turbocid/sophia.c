#include "../lib/liblux/inc/lux.h"
#include "../lib/liblux/inc/server9p.h"
#include "storage/dat.h"

#define DMDIR 0x80000000
#define OREAD 0
#define OWRITE 1
#define ORDWR 2
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
static int g_shared_backing = 0;

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

#define MAX_NAME_LEN 128

/* Helper to free a SophiaNode */
static void free_node(SophiaNode *node) {
  if (node)
    free(node);
}

/* Bootstrap the FS with persistent Sartfs backend */
static void sophia_bootstrap(void) {
  /* disk_fd = 3, journal_fd = 4 assigned by init */
  int disk_fd = 3;
  int journal_fd = 4;
  if (g_shared_backing) {
    sart_set_backing_offsets(0, 1024 * SART_BLOCK_SIZE);
  }
  int rc = sart_init(disk_fd, journal_fd, 1024);
  if (rc != SART_OK) {
    char disk_path[] = "/sophia.disk";
    char journal_path[] = "/sophia.journal";

    disk_fd = sys_open(disk_path, ORDWR);
    if (disk_fd < 0) {
      disk_fd = sys_create(disk_path, ORDWR, 0666);
    }

    journal_fd = sys_open(journal_path, ORDWR);
    if (journal_fd < 0) {
      journal_fd = sys_create(journal_path, ORDWR, 0666);
    }
    if (disk_fd >= 0 && journal_fd >= 0) {
      rc = sart_init(disk_fd, journal_fd, 1024);
    }
  }
  if (rc != SART_OK) {
    char disk_path[] = "/tmp/sophia.disk";
    char journal_path[] = "/tmp/sophia.journal";

    disk_fd = sys_open(disk_path, ORDWR);
    if (disk_fd < 0) {
      disk_fd = sys_create(disk_path, ORDWR, 0666);
    }

    journal_fd = sys_open(journal_path, ORDWR);
    if (journal_fd < 0) {
      journal_fd = sys_create(journal_path, ORDWR, 0666);
    }
    if (disk_fd >= 0 && journal_fd >= 0) {
      rc = sart_init(disk_fd, journal_fd, 1024);
    }
  }
  if (rc != SART_OK) {
    char err[] = "Sophia: FATAL - sart_init failed\n";
    sys_write(2, err, sizeof(err) - 1);
    sys_exit("sart_init failure");
  }
  {
    char ok[] = "Sophia: sart_init ok\n";
    sys_write(2, ok, sizeof(ok) - 1);
  }

  /* Root Node bootstrap */
  root_node = malloc(sizeof(SophiaNode));
  if (!root_node) {
    char err[] = "Sophia: FATAL - failed to allocate root node\n";
    sys_write(2, err, sizeof(err) - 1);
    sys_exit("root allocation failure");
  }
  memset(root_node, 0, sizeof(SophiaNode));
  root_node->mode = DMDIR | 0755;
  /* Root CID is all zeros by default */
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
  /* Portable QID encoding - use first 8 bytes of root CID */
  r->fid->qid.path = ((u64int)root_node->cid.data[0]) |
                     ((u64int)root_node->cid.data[1] << 8) |
                     ((u64int)root_node->cid.data[2] << 16) |
                     ((u64int)root_node->cid.data[3] << 24) |
                     ((u64int)root_node->cid.data[4] << 32) |
                     ((u64int)root_node->cid.data[5] << 40) |
                     ((u64int)root_node->cid.data[6] << 48) |
                     ((u64int)root_node->cid.data[7] << 56);
  r->fid->qid.vers = 0;

  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void sophia_walk(Req *r) {
  SophiaNode *curr = r->fid->aux;
  SophiaNode *next = nil;
  SophiaNode *prev = nil;
  int i;

  /* Clone FID if newfid is specified */
  if (r->newfid) {
    r->newfid->aux = curr;
    r->newfid->qid = r->fid->qid;
  }

  if (r->ifcall.nwname > 0) {
    for (i = 0; i < r->ifcall.nwname; i++) {
      next = edge_lookup(curr, r->ifcall.wname[i]);
      if (!next) {
        /* Partial walk - free any intermediate nodes we allocated */
        if (i > 0 && prev && prev != r->fid->aux) {
          free_node(prev);
        }

        if (i == 0) {
          srv_respond(r, "file not found");
        } else {
          r->ofcall.nwqid = i;
          srv_respond(r, nil);
        }
        return;
      }

      /* Free the previous intermediate node (but not the original FID node) */
      if (i > 0 && curr != r->fid->aux) {
        free_node(prev);
      }

      prev = curr;
      curr = next;

      /* Portable QID encoding */
      r->ofcall.wqid[i].type = (curr->mode & DMDIR) ? QTDIR : QTFILE;
      r->ofcall.wqid[i].path =
          ((u64int)curr->cid.data[0]) | ((u64int)curr->cid.data[1] << 8) |
          ((u64int)curr->cid.data[2] << 16) |
          ((u64int)curr->cid.data[3] << 24) |
          ((u64int)curr->cid.data[4] << 32) |
          ((u64int)curr->cid.data[5] << 40) |
          ((u64int)curr->cid.data[6] << 48) | ((u64int)curr->cid.data[7] << 56);
      r->ofcall.wqid[i].vers = 0;
    }

    /* Assign the final walked node to the appropriate FID */
    if (r->newfid) {
      r->newfid->aux = curr;
      r->newfid->qid = r->ofcall.wqid[r->ifcall.nwname - 1];
    } else {
      /* Free the old node before replacing it */
      if (r->fid->aux != root_node && r->fid->aux != curr) {
        free_node(r->fid->aux);
      }
      r->fid->aux = curr;
      r->fid->qid = r->ofcall.wqid[r->ifcall.nwname - 1];
    }
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
  if (nlen > MAX_NAME_LEN - 1)
    nlen = MAX_NAME_LEN - 1;
  memmove(node->name, name, nlen);
  node->name[nlen] = '\0';
  node->mode = perm;
  node->rec = art_search(&node->cid);

  /* Update FID to point to newly created file/directory */
  /* Note: We keep parent in parent FID, but update qid to child */
  r->fid->qid.type = (perm & DMDIR) ? QTDIR : QTFILE;
  r->fid->qid.path =
      ((u64int)node->cid.data[0]) | ((u64int)node->cid.data[1] << 8) |
      ((u64int)node->cid.data[2] << 16) | ((u64int)node->cid.data[3] << 24) |
      ((u64int)node->cid.data[4] << 32) | ((u64int)node->cid.data[5] << 40) |
      ((u64int)node->cid.data[6] << 48) | ((u64int)node->cid.data[7] << 56);
  r->fid->qid.vers = 0;

  /* Free old parent node (unless it's root) and update FID aux */
  if (r->fid->aux != root_node) {
    free_node(r->fid->aux);
  }
  r->fid->aux = node;

  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

/* Context for directory read iteration */
typedef struct {
  uchar *buf;
  uint bufsize;
  uint offset;
  uint current;
  uint count;
} DirReadCtx;

/* Callback for ns_art_iterate */
static void dir_read_callback(const UUIDv8 *parent, const char *name,
                              const UUIDv8 *child, void *ctx_ptr) {
  DirReadCtx *ctx = (DirReadCtx *)ctx_ptr;
  extern uint convD2M(Dir * d, uchar * buf, uint nbuf);
  extern uint sizeD2M(Dir * d);
  extern RecordData *art_search(const UUIDv8 *key);

  /* Skip entries until we reach the requested offset */
  if (ctx->current < ctx->offset) {
    ctx->current++;
    return;
  }

  /* Check if buffer is full */
  if (ctx->count >= ctx->bufsize) {
    return;
  }

  /* Look up child metadata */
  RecordData *rec = art_search(child);

  /* Build Dir entry */
  Dir d;
  memset(&d, 0, sizeof(Dir));
  d.qid.type = (rec && (rec->perms & DMDIR)) ? QTDIR : QTFILE;
  d.qid.path = ((u64int)child->data[0]) | ((u64int)child->data[1] << 8) |
               ((u64int)child->data[2] << 16) | ((u64int)child->data[3] << 24) |
               ((u64int)child->data[4] << 32) | ((u64int)child->data[5] << 40) |
               ((u64int)child->data[6] << 48) | ((u64int)child->data[7] << 56);
  d.qid.vers = 0;
  d.mode = rec ? rec->perms : 0755;
  d.length = rec ? rec->size : 0;
  d.atime = rec ? rec->atime : 0;
  d.mtime = rec ? rec->mtime : 0;
  d.name = (char *)name;
  d.uid = "root";
  d.gid = "root";
  d.muid = "root";

  /* Encode Dir entry into buffer */
  uint sz = convD2M(&d, ctx->buf + ctx->count, ctx->bufsize - ctx->count);
  if (sz > 0) {
    ctx->count += sz;
    ctx->current++;
  }
}

static void sophia_read(Req *r) {
  SophiaNode *node = r->fid->aux;
  extern long sart_read(UUIDv8 * id, void *buf, u64int len);
  extern int ns_art_iterate(
      const UUIDv8 *parent,
      void (*callback)(const UUIDv8 *, const char *, const UUIDv8 *, void *),
      void *ctx);

  if (node->mode & DMDIR) {
    /* Directory read using namespace iteration */
    DirReadCtx ctx;
    ctx.buf = (uchar *)r->ofcall.data;
    ctx.bufsize = r->ifcall.count;
    ctx.offset = r->ifcall.offset / 1; /* Offset is in entry count, not bytes */
    ctx.current = 0;
    ctx.count = 0;

    /* Iterate over children and generate Dir entries */
    int n = ns_art_iterate(&node->cid, dir_read_callback, &ctx);

    if (n < 0) {
      srv_respond(r, "directory read failed");
      return;
    }

    r->ofcall.count = ctx.count;
    srv_respond(r, nil);
    return;
  }

  /* File read */
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

static void sophia_remove(Req *r) {
  SophiaNode *node = r->fid->aux;
  extern int ns_art_remove(const UUIDv8 *parent, const char *name);

  /* Prevent removal of root directory */
  if (node == root_node) {
    srv_respond(r, "cannot remove root directory");
    return;
  }

  /* For immutable storage, "remove" means unlinking from namespace */
  /* The actual data blocks remain (garbage collection is separate) */

  /* Remove the namespace edge */
  int rc = ns_art_remove(&node->parent_cid, node->name);
  if (rc != 0) {
    srv_respond(r, "file not found or cannot remove");
    return;
  }

  /* Successful removal */
  srv_respond(r, nil);
}

static void sophia_open(Req *r) {
  SophiaNode *node = r->fid->aux;

  /* Validate mode flags */
  int mode = r->ifcall.mode;
  if ((node->mode & DMDIR) && (mode & 3) != 0) {
    srv_respond(r, "cannot open directory for read/write");
    return;
  }

  r->fid->omode = mode;
  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void sophia_clunk(Req *r) {
  SophiaNode *node = r->fid->aux;

  /* Free the node when FID is closed */
  if (node && node != root_node) {
    free_node(node);
    r->fid->aux = nil;
  }

  srv_respond(r, nil);
}

static void sophia_stat(Req *r) {
  SophiaNode *node = r->fid->aux;
  extern uint convD2M(Dir * d, uchar * buf, uint nbuf);
  extern uint sizeD2M(Dir * d);

  Dir d;
  memset(&d, 0, sizeof(Dir));

  d.qid.type = (node->mode & DMDIR) ? QTDIR : QTFILE;
  /* Portable QID path encoding - use first 8 bytes */
  d.qid.path =
      ((u64int)node->cid.data[0]) | ((u64int)node->cid.data[1] << 8) |
      ((u64int)node->cid.data[2] << 16) | ((u64int)node->cid.data[3] << 24) |
      ((u64int)node->cid.data[4] << 32) | ((u64int)node->cid.data[5] << 40) |
      ((u64int)node->cid.data[6] << 48) | ((u64int)node->cid.data[7] << 56);
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
  if (!r->ofcall.stat) {
    srv_respond(r, "out of memory");
    return;
  }
  r->ofcall.nstat = convD2M(&d, (uchar *)r->ofcall.stat, sz);

  srv_respond(r, nil);
  /* Note: stat buffer is freed by srv_respond */
}

int main(int argc, char **argv) {
  /* Debug: Print startup message */
  char msg[] = "Sophia: starting up\n";
  sys_write(2, msg, sizeof(msg) - 1);

  int pipe_fd = -1;
  g_shared_backing = 0;
  if (argc > 1) {
    int v = 0;
    char *p = argv[1];
    while (*p >= '0' && *p <= '9') {
      v = v * 10 + (*p - '0');
      p++;
    }
    pipe_fd = v;
    if (*p == ',') {
      p++;
      g_shared_backing = (*p != '0' && *p != '\0');
    }
  }
  if (argc > 1) {
    char prefix[] = "Sophia: argv1=";
    sys_write(2, prefix, sizeof(prefix) - 1);
    char *p = argv[1];
    long len = 0;
    while (p[len])
      len++;
    sys_write(2, p, len);
    sys_write(2, "\n", 1);
  }
  {
    char shared_msg[] = "Sophia: shared backing = 0\n";
    if (g_shared_backing)
      shared_msg[26] = '1';
    sys_write(2, shared_msg, sizeof(shared_msg) - 1);
  }

  sophia_bootstrap();

  Srv s = {
      .attach = sophia_attach,
      .walk = sophia_walk,
      .open = sophia_open,
      .read = sophia_read,
      .write = sophia_write,
      .create = sophia_create,
      .clunk = sophia_clunk,
      .remove = sophia_remove,
      .stat = sophia_stat,
  };

  {
    char srv_init_msg[] = "Sophia: calling srv_init\n";
    sys_write(2, srv_init_msg, sizeof(srv_init_msg) - 1);
  }
  srv_init(&s);
  {
    char srv_init_done[] = "Sophia: srv_init done\n";
    sys_write(2, srv_init_done, sizeof(srv_init_done) - 1);
  }

  char srv_msg[] = "Sophia: entering srv_loop\n";
  sys_write(2, srv_msg, sizeof(srv_msg) - 1);

  if (argc > 1 && pipe_fd >= 0) {
    srv_loop(&s, pipe_fd, pipe_fd);
  } else {
    /* Default: use stdin/stdout for testing */
    srv_loop(&s, 0, 1);
  }
  return 0;
}
