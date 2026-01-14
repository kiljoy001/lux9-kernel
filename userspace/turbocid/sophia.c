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

/* Sartfs Externs */
extern int sart_init(int disk_fd, int journal_fd, u64int total_blocks);
extern RecordData *art_search(const UUIDv8 *key);
extern UUIDv8 *ns_art_search(const UUIDv8 *parent, const char *name);
extern int ns_art_insert(const UUIDv8 *parent, const char *name,
                         const UUIDv8 *child);

/*
 * SophiaNode: A node in the DAG.
 */
typedef struct SophiaNode {
  UUIDv8 cid;      /* The Pointer (UUIDv8 Content Identity) */
  RecordData *rec; /* Persistent metadata (if JENT_BLOB) */
  u32int mode;     /* QID type | permissions */
  void *data;      /* Cached data pointer */
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
    /* This is an inconsistency: edge exists but blob metadata doesn't?
     * Might happen for directories if they don't have RecordData yet.
     */
  }

  SophiaNode *node = malloc(sizeof(SophiaNode));
  if (node == nil)
    return nil;

  memset(node, 0, sizeof(SophiaNode));
  memmove(&node->cid, child_id, 16);
  node->rec = rec;
  /* If rec exists, take mode from it. Else default to dir? */
  if (rec)
    node->mode = rec->perms;
  else
    node->mode = DMDIR | 0755; /* Fallback for directories */

  return node;
}

/* Bootstrap the FS with persistent Sartfs backend */
static void sophia_bootstrap(void) {
  /* disk_fd = 3, journal_fd = 4 assigned by init */
  if (sart_init(3, 4, 1024) != SART_OK) {
    /* TODO: handle init failure */
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
  r->fid->qid.path = 0;
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
      r->ofcall.wqid[i].path = 0; /* TODO: Map UUID to path */
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

static void sophia_read(Req *r) {
  SophiaNode *node = r->fid->aux;
  if (node->mode & DMDIR) {
    srv_respond(r, "directory read not implemented");
    return;
  }
  srv_respond(r, "file read not implemented");
}

static void sophia_stat(Req *r) {
  SophiaNode *node = r->fid->aux;
  Dir d;
  memset(&d, 0, sizeof(Dir));

  d.qid.type = (node->mode & DMDIR) ? QTDIR : QTFILE;
  d.qid.path = 0;
  d.mode = node->mode;
  d.length = node->rec ? node->rec->size : 0;
  d.name = "";

  r->ofcall.stat = nil; /* TODO: implement stat serialization */
  srv_respond(r, nil);
}

int main(int argc, char **argv) {
  sophia_bootstrap();

  Srv s = {
      .attach = sophia_attach,
      .walk = sophia_walk,
      .read = sophia_read,
      .stat = sophia_stat,
  };

  srv_init(&s);

  if (argc > 1) {
    int fd = 0;
    char *p = argv[1];
    while (*p >= '0' && *p <= '9') {
      fd = fd * 10 + (*p - '0');
      p++;
    }
    srv_loop(&s, fd, fd);
  } else {
    srv_loop(&s, 0, 1);
  }
  return 0;
}
