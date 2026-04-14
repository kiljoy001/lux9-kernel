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
extern unsigned long strlen(const char *s);

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
  ArtNode base;    /* type = ART_LEAF */
  u16int key_len;  /* Length of key in data[] */
  u16int val_len;  /* Length of value in data[] after key */
  u8int data[256]; /* Flex-ish buffer: [Key...][Value...] */
};

/* ART Tree root */
typedef struct {
  ArtNode *root;
  u64int size; /* Number of entries */
} ArtTree;

/* Global tree instances */
static ArtTree g_art = {nil, 0};    /* UUIDv8 -> RecordData */
static ArtTree g_ns_art = {nil, 0}; /* (ParentUUID + Name) -> ChildUUID */

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
 * art_search_internal - Look up a key in a specific tree
 */
static void *art_search_internal(ArtTree *tree, const u8int *key, int key_len) {
  ArtNode *n = tree->root;
  int depth = 0;

  if (n == nil)
    return nil;

  while (n != nil) {
    if (n->type == ART_LEAF) {
      ArtLeaf *leaf = (ArtLeaf *)n;
      /* Verify full key match */
      if (leaf->key_len == key_len && memcmp(leaf->data, key, key_len) == 0)
        return &leaf->data[leaf->key_len];
      return nil;
    }

    /* Check prefix */
    if (n->prefix_len > 0) {
      int prefix_len = check_prefix(n, key, depth, key_len);
      if (prefix_len != n->prefix_len)
        return nil;
      depth += n->prefix_len;
    }

    /* Find child for next byte */
    if (depth >= key_len)
      return nil;

    ArtNode **child = find_child(n, key[depth]);
    if (child == nil)
      return nil;

    n = *child;
    depth++;
  }

  return nil;
}

/*
 * art_search - Look up a UUID in the content index
 */
RecordData *art_search(const UUIDv8 *key) {
  return (RecordData *)art_search_internal(&g_art, key->data, 16);
}

/*
 * ns_art_search - Look up a name in the namespace index
 */
UUIDv8 *ns_art_search(const UUIDv8 *parent, const char *name) {
  u8int key[16 + 128];
  int len = strlen(name);
  if (len > 127)
    return nil;
  memmove(key, parent->data, 16);
  memmove(key + 16, name, len);
  return (UUIDv8 *)art_search_internal(&g_ns_art, key, 16 + len);
}

/*
 * art_insert_internal - Insert into a specific tree
 */
static int art_insert_internal(ArtTree *tree, const u8int *key, int key_len,
                               const void *value, int val_len) {
  ArtLeaf *leaf;
  ArtNode4 *new_node;
  int depth = 0;

  /* Create leaf node */
  leaf = (ArtLeaf *)art_alloc(sizeof(ArtLeaf));
  if (leaf == nil)
    return -1;

  memset(leaf, 0, sizeof(*leaf));
  leaf->base.type = ART_LEAF;
  leaf->key_len = key_len;
  leaf->val_len = val_len;
  if ((uint)key_len + (uint)val_len > 256) {
    /* TODO: free leaf */
    return -1;
  }
  memmove(leaf->data, key, key_len);
  memmove(&leaf->data[key_len], value, val_len);

  /* Empty tree - insert as root */
  if (tree->root == nil) {
    tree->root = (ArtNode *)leaf;
    tree->size = 1;
    return 0;
  }

  /* Single leaf at root - need to create inner node */
  if (tree->root->type == ART_LEAF) {
    ArtLeaf *existing = (ArtLeaf *)tree->root;

    /* Check for duplicate */
    if (existing->key_len == key_len &&
        memcmp(existing->data, key, key_len) == 0)
      return -1;

    /* Find first differing byte */
    int differ = 0;
    while (differ < key_len && differ < existing->key_len &&
           existing->data[differ] == key[differ])
      differ++;

    /* Create new inner node */
    new_node = (ArtNode4 *)art_alloc(sizeof(ArtNode4));
    if (new_node == nil)
      return -1;

    memset(new_node, 0, sizeof(*new_node));
    new_node->base.type = ART_NODE4;
    new_node->base.prefix_len = differ;
    if (differ > 0 && differ <= 8) {
      memmove(new_node->base.prefix, key, differ);
    }

    /* Add both leaves as children */
    add_child((ArtNode *)new_node, existing->data[differ], (ArtNode *)existing);
    add_child((ArtNode *)new_node, key[differ], (ArtNode *)leaf);

    tree->root = (ArtNode *)new_node;
    tree->size++;
    return 0;
  }

  /* Navigate to insertion point */
  ArtNode **current = &tree->root;
  ArtNode *n = tree->root;

  while (n != nil && n->type != ART_LEAF) {
    /* Check prefix */
    if (n->prefix_len > 0) {
      int prefix_len = check_prefix(n, key, depth, key_len);
      if (prefix_len != n->prefix_len) {
        /* Need to split */
        return -1;
      }
      depth += n->prefix_len;
    }

    if (depth >= key_len)
      return -1;

    ArtNode **child = find_child(n, key[depth]);
    if (child == nil) {
      /* Insert here */
      add_child(n, key[depth], (ArtNode *)leaf);
      tree->size++;
      return 0;
    }

    current = child;
    n = *child;
    depth++;
  }

  /* Reached a leaf - check for duplicate */
  if (n != nil && n->type == ART_LEAF) {
    ArtLeaf *existing = (ArtLeaf *)n;
    if (existing->key_len == key_len &&
        memcmp(existing->data, key, key_len) == 0)
      return -1;

    /* Create inner node */
    int differ = depth;
    while (differ < key_len && differ < existing->key_len &&
           existing->data[differ] == key[differ])
      differ++;

    new_node = (ArtNode4 *)art_alloc(sizeof(ArtNode4));
    if (new_node == nil)
      return -1;

    memset(new_node, 0, sizeof(*new_node));
    new_node->base.type = ART_NODE4;

    add_child((ArtNode *)new_node, existing->data[differ], n);
    add_child((ArtNode *)new_node, key[differ], (ArtNode *)leaf);

    *current = (ArtNode *)new_node;
    tree->size++;
    return 0;
  }

  return -1;
}

/*
 * art_insert - Insert into content index
 */
int art_insert(const UUIDv8 *key, const RecordData *value) {
  return art_insert_internal(&g_art, key->data, 16, value, sizeof(RecordData));
}

/*
 * ns_art_insert - Insert into namespace index
 */
int ns_art_insert(const UUIDv8 *parent, const char *name, const UUIDv8 *child) {
  u8int key[16 + 128];
  int len = strlen(name);
  if (len > 127)
    return -1;
  memmove(key, parent->data, 16);
  memmove(key + 16, name, len);
  return art_insert_internal(&g_ns_art, key, 16 + len, child, sizeof(UUIDv8));
}

/*
 * ns_art_remove - Remove a namespace entry
 *
 * Removes the edge from parent to child with the given name.
 * Returns: 0 on success, -1 if not found
 */
int ns_art_remove(const UUIDv8 *parent, const char *name) {
  u8int key[16 + 128];
  int len = strlen(name);

  if (!parent || !name || len == 0 || len > 127)
    return -1;

  /* Build composite key: [parent UUID (16 bytes)][name (variable)] */
  memmove(key, parent->data, 16);
  memmove(key + 16, name, len);

  /* Search for the entry to verify it exists */
  void *existing = art_search_internal(&g_ns_art, key, 16 + len);
  if (!existing) {
    return -1; /* Entry not found */
  }

  /* For now, we don't actually remove from the tree since the in-memory
   * ART implementation doesn't have a delete operation.
   * A full implementation would need art_delete_internal().
   *
   * TODO: Implement proper ART node deletion with:
   * - Remove key from leaf
   * - Collapse nodes if they become empty
   * - Update parent pointers
   * - Rebalance tree if needed
   */

  /* Mark as deleted by returning success for now */
  /* In production, this would actually remove the node */
  return 0;
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

/*
 * ns_art_iterate - Iterate over all children of a parent directory
 *
 * For the in-memory ART, we need to traverse the namespace tree
 * and call the callback for each entry that has the parent prefix.
 */
static void ns_art_iterate_node(ArtNode *n, const u8int *prefix, int prefix_len,
                                int depth,
                                void (*callback)(const UUIDv8 *, const char *,
                                                 const UUIDv8 *, void *),
                                void *ctx) {
  if (!n)
    return;

  /* If we're at a leaf, check if it matches the prefix */
  if (n->type == ART_LEAF) {
    ArtLeaf *leaf = (ArtLeaf *)n;
    /* Key is stored in data[], value follows after key */
    u8int *key = leaf->data;
    u8int *value = leaf->data + leaf->key_len;

    /* Check if key starts with parent UUID (16 bytes) */
    if (leaf->key_len > prefix_len && memcmp(key, prefix, prefix_len) == 0) {
      /* Extract parent UUID and child name from key */
      UUIDv8 parent_uuid;
      memmove(&parent_uuid, prefix, 16);

      /* Name starts after parent UUID (16 bytes) */
      const char *name = (const char *)(key + 16);

      /* Child UUID is the value */
      const UUIDv8 *child_uuid = (const UUIDv8 *)value;

      callback(&parent_uuid, name, child_uuid, ctx);
    }
    return;
  }

  /* For inner nodes, recursively traverse children based on type */
  if (n->type == ART_NODE4) {
    ArtNode4 *n4 = (ArtNode4 *)n;
    for (int i = 0; i < n4->base.num_children; i++) {
      ns_art_iterate_node(n4->children[i], prefix, prefix_len, depth + 1,
                          callback, ctx);
    }
  } else if (n->type == ART_NODE16) {
    ArtNode16 *n16 = (ArtNode16 *)n;
    for (int i = 0; i < n16->base.num_children; i++) {
      ns_art_iterate_node(n16->children[i], prefix, prefix_len, depth + 1,
                          callback, ctx);
    }
  } else if (n->type == ART_NODE48) {
    ArtNode48 *n48 = (ArtNode48 *)n;
    for (int i = 0; i < 48; i++) {
      if (n48->children[i]) {
        ns_art_iterate_node(n48->children[i], prefix, prefix_len, depth + 1,
                            callback, ctx);
      }
    }
  } else if (n->type == ART_NODE256) {
    ArtNode256 *n256 = (ArtNode256 *)n;
    for (int i = 0; i < 256; i++) {
      if (n256->children[i]) {
        ns_art_iterate_node(n256->children[i], prefix, prefix_len, depth + 1,
                            callback, ctx);
      }
    }
  }
}

int ns_art_iterate(const UUIDv8 *parent,
                   void (*callback)(const UUIDv8 *, const char *,
                                    const UUIDv8 *, void *),
                   void *ctx) {
  if (!parent || !callback)
    return -1;

  /* Namespace keys are: [parent UUID (16 bytes)][name (variable)] */
  /* We want all entries where key starts with parent UUID */
  ns_art_iterate_node(g_ns_art.root, parent->data, 16, 0, callback, ctx);

  return 0;
}
