/*
 * Sartfs Adaptive Radix Tree (ART) Index
 *
 * High-performance index for content-addressed lookups.
 * O(k) lookup where k is key length (16 bytes for UUIDv8).
 *
 * Implementation based on "The Adaptive Radix Tree: ARTful Indexing
 * for Main-Memory Databases" by Viktor Leis et al.
 */

#include "dat.h"

extern void *memset(void *dst, int c, unsigned long n);
extern void *memmove(void *dst, const void *src, unsigned long n);

/* Local memcmp implementation */
static int memcmp(const void *s1, const void *s2, unsigned long n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  unsigned long i;
  for (i = 0; i < n; i++) {
    if (p1[i] < p2[i])
      return -1;
    if (p1[i] > p2[i])
      return 1;
  }
  return 0;
}

/*
 * ART Node Types
 *
 * Node4:   Up to 4 children, linear search
 * Node16:  Up to 16 children, SIMD-friendly
 * Node48:  Up to 48 children, indirect keys
 * Node256: Full 256-way fan-out
 */

typedef enum {
  ART_NODE4 = 0,
  ART_NODE16,
  ART_NODE48,
  ART_NODE256,
  ART_LEAF
} ArtNodeType;

/* Forward declarations */
typedef struct ArtNode ArtNode;
typedef struct ArtNode4 ArtNode4;
typedef struct ArtNode16 ArtNode16;
typedef struct ArtNode48 ArtNode48;
typedef struct ArtNode256 ArtNode256;
typedef struct ArtLeaf ArtLeaf;

/* Base node header */
struct ArtNode {
  u8int type;
  u8int num_children;
  u16int prefix_len;
  u8int prefix[8]; /* Compressed path prefix */
};

/* Node4: 4-way node with linear search */
struct ArtNode4 {
  ArtNode base;
  u8int keys[4];
  ArtNode *children[4];
};

/* Node16: 16-way node */
struct ArtNode16 {
  ArtNode base;
  u8int keys[16];
  ArtNode *children[16];
};

/* Node48: 48-way node with index array */
struct ArtNode48 {
  ArtNode base;
  u8int child_idx[256];
  ArtNode *children[48];
};

/* Node256: Full fanout */
struct ArtNode256 {
  ArtNode base;
  ArtNode *children[256];
};

/* Leaf node holding actual data */
struct ArtLeaf {
  ArtNode base;     /* type = ART_LEAF */
  UUIDv8 key;       /* Full key for verification */
  RecordData value; /* The stored record */
};

/* ART Tree root */
typedef struct {
  ArtNode *root;
  u64int size; /* Number of entries */
} ArtTree;

/* Global tree instance */
static ArtTree g_art = {nil, 0};

/*
 * Simple memory allocation (using static pool for freestanding)
 * In production, replace with pebble_alloc
 */
#define ART_POOL_SIZE (1024 * 64) /* 64KB pool */
static u8int art_pool[ART_POOL_SIZE];
static ulong art_pool_used = 0;

static void *art_alloc(ulong size) {
  void *ptr;
  /* Align to 8 bytes */
  size = (size + 7) & ~7UL;
  if (art_pool_used + size > ART_POOL_SIZE)
    return nil;
  ptr = &art_pool[art_pool_used];
  art_pool_used += size;
  return ptr;
}

/*
 * Helper: Check prefix match
 */
static int check_prefix(ArtNode *n, const u8int *key, int depth, int key_len) {
  int idx;
  int max_cmp =
      (n->prefix_len < (key_len - depth)) ? n->prefix_len : (key_len - depth);

  for (idx = 0; idx < max_cmp; idx++) {
    if (n->prefix[idx] != key[depth + idx])
      return idx;
  }
  return idx;
}

/*
 * Helper: Find child in node
 */
static ArtNode **find_child(ArtNode *n, u8int key_byte) {
  int i;

  switch (n->type) {
  case ART_NODE4: {
    ArtNode4 *node = (ArtNode4 *)n;
    for (i = 0; i < n->num_children; i++) {
      if (node->keys[i] == key_byte)
        return &node->children[i];
    }
    break;
  }
  case ART_NODE16: {
    ArtNode16 *node = (ArtNode16 *)n;
    for (i = 0; i < n->num_children; i++) {
      if (node->keys[i] == key_byte)
        return &node->children[i];
    }
    break;
  }
  case ART_NODE48: {
    ArtNode48 *node = (ArtNode48 *)n;
    i = node->child_idx[key_byte];
    if (i != 0)
      return &node->children[i - 1];
    break;
  }
  case ART_NODE256: {
    ArtNode256 *node = (ArtNode256 *)n;
    if (node->children[key_byte])
      return &node->children[key_byte];
    break;
  }
  }
  return nil;
}

/*
 * Helper: Add child to node
 */
static void add_child(ArtNode *n, u8int key_byte, ArtNode *child) {
  switch (n->type) {
  case ART_NODE4: {
    ArtNode4 *node = (ArtNode4 *)n;
    if (n->num_children < 4) {
      node->keys[n->num_children] = key_byte;
      node->children[n->num_children] = child;
      n->num_children++;
    }
    /* TODO: Grow to Node16 if full */
    break;
  }
  case ART_NODE16: {
    ArtNode16 *node = (ArtNode16 *)n;
    if (n->num_children < 16) {
      node->keys[n->num_children] = key_byte;
      node->children[n->num_children] = child;
      n->num_children++;
    }
    /* TODO: Grow to Node48 if full */
    break;
  }
  case ART_NODE48: {
    ArtNode48 *node = (ArtNode48 *)n;
    if (n->num_children < 48) {
      int idx = n->num_children;
      node->child_idx[key_byte] = idx + 1;
      node->children[idx] = child;
      n->num_children++;
    }
    /* TODO: Grow to Node256 if full */
    break;
  }
  case ART_NODE256: {
    ArtNode256 *node = (ArtNode256 *)n;
    node->children[key_byte] = child;
    n->num_children++;
    break;
  }
  }
}

/*
 * art_init - Initialize the ART index
 */
void art_init(void) {
  memset(&g_art, 0, sizeof(g_art));
  art_pool_used = 0;
  memset(art_pool, 0, sizeof(art_pool));
}

/*
 * art_search - Look up a key in the tree
 *
 * Returns pointer to RecordData if found, nil otherwise.
 * O(k) where k = key length (16 bytes for UUIDv8)
 */
RecordData *art_search(const UUIDv8 *key) {
  ArtNode *n = g_art.root;
  int depth = 0;
  int key_len = 16;

  if (n == nil)
    return nil;

  while (n != nil) {
    if (n->type == ART_LEAF) {
      ArtLeaf *leaf = (ArtLeaf *)n;
      /* Verify full key match */
      if (UUID_EQUAL(&leaf->key, key))
        return &leaf->value;
      return nil;
    }

    /* Check prefix */
    if (n->prefix_len > 0) {
      int prefix_len = check_prefix(n, key->data, depth, key_len);
      if (prefix_len != n->prefix_len)
        return nil;
      depth += n->prefix_len;
    }

    /* Find child for next byte */
    if (depth >= key_len)
      return nil;

    ArtNode **child = find_child(n, key->data[depth]);
    if (child == nil)
      return nil;

    n = *child;
    depth++;
  }

  return nil;
}

/*
 * art_insert - Insert a key-value pair
 *
 * Returns 0 on success, -1 on failure (duplicate or OOM).
 */
int art_insert(const UUIDv8 *key, const RecordData *value) {
  ArtLeaf *leaf;
  ArtNode4 *new_node;
  int depth = 0;
  int key_len = 16;

  /* Create leaf node */
  leaf = (ArtLeaf *)art_alloc(sizeof(ArtLeaf));
  if (leaf == nil)
    return -1;

  memset(leaf, 0, sizeof(*leaf));
  leaf->base.type = ART_LEAF;
  memmove(&leaf->key, key, sizeof(UUIDv8));
  memmove(&leaf->value, value, sizeof(RecordData));

  /* Empty tree - insert as root */
  if (g_art.root == nil) {
    g_art.root = (ArtNode *)leaf;
    g_art.size = 1;
    return 0;
  }

  /* Single leaf at root - need to create inner node */
  if (g_art.root->type == ART_LEAF) {
    ArtLeaf *existing = (ArtLeaf *)g_art.root;

    /* Check for duplicate */
    if (UUID_EQUAL(&existing->key, key))
      return -1;

    /* Find first differing byte */
    int differ = 0;
    while (differ < key_len && existing->key.data[differ] == key->data[differ])
      differ++;

    /* Create new inner node */
    new_node = (ArtNode4 *)art_alloc(sizeof(ArtNode4));
    if (new_node == nil)
      return -1;

    memset(new_node, 0, sizeof(*new_node));
    new_node->base.type = ART_NODE4;
    new_node->base.prefix_len = differ;
    if (differ > 0 && differ <= 8) {
      memmove(new_node->base.prefix, key->data, differ);
    }

    /* Add both leaves as children */
    add_child((ArtNode *)new_node, existing->key.data[differ],
              (ArtNode *)existing);
    add_child((ArtNode *)new_node, key->data[differ], (ArtNode *)leaf);

    g_art.root = (ArtNode *)new_node;
    g_art.size++;
    return 0;
  }

  /* Navigate to insertion point */
  ArtNode **current = &g_art.root;
  ArtNode *n = g_art.root;

  while (n != nil && n->type != ART_LEAF) {
    /* Check prefix */
    if (n->prefix_len > 0) {
      int prefix_len = check_prefix(n, key->data, depth, key_len);
      if (prefix_len != n->prefix_len) {
        /* Need to split */
        /* Simplified: just fail for now */
        return -1;
      }
      depth += n->prefix_len;
    }

    if (depth >= key_len)
      return -1;

    ArtNode **child = find_child(n, key->data[depth]);
    if (child == nil) {
      /* Insert here */
      add_child(n, key->data[depth], (ArtNode *)leaf);
      g_art.size++;
      return 0;
    }

    current = child;
    n = *child;
    depth++;
  }

  /* Reached a leaf - check for duplicate */
  if (n != nil && n->type == ART_LEAF) {
    ArtLeaf *existing = (ArtLeaf *)n;
    if (UUID_EQUAL(&existing->key, key))
      return -1; /* Duplicate */

    /* Create inner node */
    int differ = depth;
    while (differ < key_len && existing->key.data[differ] == key->data[differ])
      differ++;

    new_node = (ArtNode4 *)art_alloc(sizeof(ArtNode4));
    if (new_node == nil)
      return -1;

    memset(new_node, 0, sizeof(*new_node));
    new_node->base.type = ART_NODE4;

    add_child((ArtNode *)new_node, existing->key.data[differ], n);
    add_child((ArtNode *)new_node, key->data[differ], (ArtNode *)leaf);

    *current = (ArtNode *)new_node;
    g_art.size++;
    return 0;
  }

  return -1;
}

/*
 * art_exists - Check if key exists
 */
int art_exists(const UUIDv8 *key) { return art_search(key) != nil; }

/*
 * art_size - Return number of entries
 */
u64int art_size(void) { return g_art.size; }

/*
 * art_clear - Clear all entries
 */
void art_clear(void) {
  g_art.root = nil;
  g_art.size = 0;
  art_pool_used = 0;
}
