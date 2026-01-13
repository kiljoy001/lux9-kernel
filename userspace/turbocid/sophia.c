#include <libc.h>
#include <server9p.h>
#include <u.h>

/* ========================================================================= */
/* ADAPTIVE RADIX TREE (ART) - Edge Index Implementation                     */
/* ========================================================================= */

typedef enum { NODE4 = 1, NODE16 = 2, NODE48 = 3, NODE256 = 4 } NodeType;

typedef struct ArtNode {
  u8int type;
  u8int num_children;
  u8int partial_len;
  uchar partial[8]; /* Prefix compression (simplified) */
} ArtNode;

/* Node4: Small node with 4 children */
typedef struct Node4 {
  ArtNode n;
  uchar keys[4];
  ArtNode *children[4];
} Node4;

/* Node16: Node with 16 children (SIMD-friendly layout) */
typedef struct Node16 {
  ArtNode n;
  uchar keys[16];
  ArtNode *children[16];
} Node16;

/* Node48: Node with 48 children (Indirect index) */
typedef struct Node48 {
  ArtNode n;
  uchar child_index[256];
  ArtNode *children[48];
} Node48;

/* Node256: Full node (Direct lookup) */
typedef struct Node256 {
  ArtNode n;
  ArtNode *children[256];
} Node256;

/* Leaf Node: Holds the SophiaNode pointer */
/* We differentiate leaves by tagging the pointer or checking type */
typedef struct ArtLeaf {
  ArtNode n;
  void *value; /* SophiaNode* */
} ArtLeaf;

/* Root of the Global Edge Index */
static ArtNode *edge_index_root = nil;

/* Helper: Check if node is leaf */
static int is_leaf(ArtNode *n) {
  return n ==
         nil; /* Simplified: Leaves are stored in parent pointers differently?
                 For this Lite impl, we'll assume a specific Type 0 for Leaf. */
}

/*
 * ART Search
 * Key: ParentCID (16 bytes) + EdgeName (N bytes)
 */
static void *art_search(ArtNode *n, uchar *key, int len, int depth) {
  if (n == nil)
    return nil;

  /* Handle Leaf */
  if (n->type == 0) {
    /* Verify full key match if needed, but for now return value */
    return ((ArtLeaf *)n)->value;
  }

  /* Check Prefix (Simplified: skip for now or assume match) */
  /* In full ART, we compare n->partial with key[depth]... */

  /* Find Child */
  uchar c = key[depth];
  ArtNode **child = nil;

  switch (n->type) {
  case NODE4: {
    Node4 *n4 = (Node4 *)n;
    for (int i = 0; i < n4->n.num_children; i++) {
      if (n4->keys[i] == c) {
        child = &n4->children[i];
        break;
      }
    }
    break;
  }
  case NODE16: {
    Node16 *n16 = (Node16 *)n;
    /* TODO: SIMD here */
    for (int i = 0; i < n16->n.num_children; i++) {
      if (n16->keys[i] == c) {
        child = &n16->children[i];
        break;
      }
    }
    break;
  }
  case NODE48: {
    Node48 *n48 = (Node48 *)n;
    int idx = n48->child_index[c];
    if (idx != 0) { /* 0 means empty in simplified mapping? Need offset */
      child = &n48->children[idx - 1];
    }
    break;
  }
  case NODE256: {
    Node256 *n256 = (Node256 *)n;
    child = &n256->children[c];
    break;
  }
  }

  if (child && *child) {
    /* If we reached end of key, the child should be a leaf */
    if (depth == len - 1) {
      /* If *child is a leaf, return its value */
      if ((*child)->type == 0)
        return ((ArtLeaf *)*child)->value;
      /* Else key is prefix of another key */
      return nil;
    }
    return art_search(*child, key, len, depth + 1);
  }

  return nil;
}

/* Helper: Insert Leaf (Simplified - Only NODE256 support for scaffold) */
static void art_insert(ArtNode **root, uchar *key, int len, void *value) {
  if (*root == nil) {
    /* Create Root as Node256 for simplicity */
    Node256 *n = malloc(sizeof(Node256));
    memset(n, 0, sizeof(Node256));
    n->n.type = NODE256;
    *root = (ArtNode *)n;
  }

  ArtNode *curr = *root;
  for (int i = 0; i < len; i++) {
    uchar c = key[i];

    if (curr->type == NODE256) {
      Node256 *n256 = (Node256 *)curr;
      if (i == len - 1) {
        /* Insert Value at Leaf */
        ArtLeaf *l = malloc(sizeof(ArtLeaf));
        l->n.type = 0;
        l->value = value;
        n256->children[c] = (ArtNode *)l;
      } else {
        /* Create intermediate node if missing */
        if (n256->children[c] == nil) {
          Node256 *next = malloc(sizeof(Node256));
          memset(next, 0, sizeof(Node256));
          next->n.type = NODE256;
          n256->children[c] = (ArtNode *)next;
        }
        curr = n256->children[c];
      }
    }
    /* TODO: Handle node expansion (4->16->48->256) */
  }
}

/* ========================================================================= */
/* SOPHIAFS: The Semantic DAG Filesystem                                     */
/* ========================================================================= */

/*
 * TurboCID: 128-bit UUIDv8 acting as the unique handle
 */
typedef struct TurboCID {
  u64int hi;
  u64int lo;
} TurboCID;

/*
 * Semantic Record: The Soul of the File
 * Contains the immutable properties used for indexing.
 */
typedef struct SemanticRecord {
  u8int true_name[32]; /* BLAKE3 Hash - The Identity */
  u8int tlsh[35];      /* TLSH Digest - The Similarity Vector */
  u32int type_hash;    /* Magic/MIME Hash */
  u64int size;         /* Physical Size */
} SemanticRecord;

/*
 * SophiaNode: A node in the DAG.
 */
typedef struct SophiaNode {
  TurboCID cid;             /* The Pointer (UUIDv8) */
  SemanticRecord *semantic; /* The Data/Soul */
  u32int mode;
  void *data_ref;
} SophiaNode;

/*
 * EdgeKey: The key for the Adaptive Radix Tree (Edge Index).
 * Format: [ParentCID (16 bytes)] [EdgeName (variable)]
 */
#define EDGE_KEY_MAX 256

/* Global State */
static SophiaNode *root_node;

/*
 * Edge Index Lookup
 * Maps (ParentCID + Name) -> ChildNode*
 */
static SophiaNode *edge_lookup(SophiaNode *parent, char *name) {
  uchar key[EDGE_KEY_MAX];
  int klen = 0;

  /* Construct Key: ParentCID (16 bytes) | Name */
  memmove(key, &parent->cid, 16);
  klen += 16;

  int nlen = strlen(name);
  if (klen + nlen > EDGE_KEY_MAX)
    return nil;

  memmove(key + 16, name, nlen);
  klen += nlen;

  return (SophiaNode *)art_search(edge_index_root, key, klen, 0);
}

/* Bootstrap the FS with some nodes */
static void sophia_bootstrap(void) {
  /* Root Node */
  root_node = malloc(sizeof(SophiaNode));
  memset(root_node, 0, sizeof(SophiaNode));
  root_node->mode = DMDIR | 0755;
  /* Root CID = 0 */

  /* "ctl" File */
  SophiaNode *ctl = malloc(sizeof(SophiaNode));
  ctl->mode = 0666;
  ctl->cid.lo = 1;

  /* "semantic" Directory */
  SophiaNode *sem = malloc(sizeof(SophiaNode));
  sem->mode = DMDIR | 0555;
  sem->cid.lo = 2;

  /* Insert Edges: Root -> "ctl", Root -> "semantic" */
  uchar key[EDGE_KEY_MAX];

  /* Edge: Root + "ctl" */
  memset(key, 0, 16); /* Root CID is 0 */
  memmove(key + 16, "ctl", 3);
  art_insert(&edge_index_root, key, 16 + 3, ctl);

  /* Edge: Root + "semantic" */
  memset(key, 0, 16);
  memmove(key + 16, "semantic", 8);
  art_insert(&edge_index_root, key, 16 + 8, sem);
}

/* ========================================================================= */
/* 9P CALLBACKS                                                              */
/* ========================================================================= */

static void sophia_attach(Req *r) {
  /* Create the Root Node if missing */
  if (!root_node) {
    root_node = malloc(sizeof(SophiaNode));
    memset(root_node, 0, sizeof(SophiaNode));
    root_node->mode = DMDIR | 0755;
    /* Root CID is typically all zeros or a fixed hash */
  }

  /* Bind the FID to the Root Node */
  r->fid->aux = root_node;
  r->fid->qid.type = QTDIR;
  r->fid->qid.path = 0; /* TODO: Map CID to Qid.path */
  r->fid->qid.vers = 0;

  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void sophia_walk(Req *r) {
  /*
   * 9P Walk: Traverse N names from the current FID.
   * In a DAG, this is N edge traversals.
   */
  SophiaNode *curr = r->fid->aux;
  SophiaNode *next;
  int i;

  /* Clone logic handled by server9p/liblux?
     No, server9p handles the 'newfid' allocation, but we must populate it. */
  if (r->newfid) {
    r->newfid->aux = curr;
    r->newfid->qid = r->fid->qid;
  }

  if (r->ifcall.nwname > 0) {
    for (i = 0; i < r->ifcall.nwname; i++) {
      next = edge_lookup(curr, r->ifcall.wname[i]);
      if (!next) {
        /* Walk failed at this step */
        if (i == 0)
          srv_respond(r, "file not found");
        else {
          /* Partial success */
          r->ofcall.nwqid = i;
          srv_respond(r, nil);
        }
        return;
      }

      /* Step taken */
      curr = next;

      /* Update response QID list */
      r->ofcall.wqid[i].type = (curr->mode & DMDIR) ? QTDIR : QTFILE;
      r->ofcall.wqid[i].path = curr->cid.lo; /* Hack for now */
      r->ofcall.wqid[i].vers = 0;
    }

    /* Update the new FID to point to the final node */
    if (r->newfid)
      r->newfid->aux = curr;
    else
      r->fid->aux = curr; /* Should not happen in standard 9P walk usually
                             implies clone */
  }

  r->ofcall.nwqid = r->ifcall.nwname;
  srv_respond(r, nil);
}

static void sophia_read(Req *r) {
  SophiaNode *node = r->fid->aux;

  if (node->mode & DMDIR) {
    /* Directory Read: Enumerate outgoing edges from this node */
    /* TODO: Scan Edge Index for all keys starting with [NodeCID] */
    srv_respond(r, "directory read not implemented");
    return;
  }

  /* File Read */
  srv_respond(r, "file read not implemented");
}

static void sophia_stat(Req *r) {
  SophiaNode *node = r->fid->aux;
  Dir d;
  memset(&d, 0, sizeof(Dir));

  d.qid.type = (node->mode & DMDIR) ? QTDIR : QTFILE;
  d.qid.path = node->cid.lo;
  d.mode = node->mode;
  d.length = node->size;
  d.name = "???"; /* Node doesn't know its name! The Edge knows. */
  /* Issue: 9P Stat requires a name.
     We typically cache the name in the Fid during Walk. */

  /* For now, return empty name */
  d.name = "";

  /* Serialize Dir to r->ofcall.stat */
  /* convD2M needed here */
  srv_respond(r, nil);
}

void main(int argc, char **argv) {
  sophia_bootstrap();

  Srv s = {
      .attach = sophia_attach,
      .walk = sophia_walk,
      .read = sophia_read,
      .stat = sophia_stat,
  };

  srv_init(&s);

  /* If an argument is provided, treat it as the file descriptor for the
   * connection */
  if (argc > 1) {
    int fd = 0;
    /* Simple atoi since we don't have strtol handy? or use liblux default */
    char *p = argv[1];
    while (*p >= '0' && *p <= '9') {
      fd = fd * 10 + (*p - '0');
      p++;
    }
    srv_loop(&s, fd, fd);
  } else {
    /* Default to stdin/stdout */
    srv_loop(&s, 0, 1);
  }
}
