/*
 * Red-Black Tree Implementation
 *
 * Intrusive RB-tree based on Linux kernel design.
 * Provides O(log n) insert, delete, and search operations.
 *
 * Usage:
 *   struct my_node {
 *       struct rb_node rb;
 *       int key;
 *       // ... other data
 *   };
 *
 *   struct rb_root mytree = RB_ROOT;
 *
 *   // Insert: caller provides comparison and linking
 *   // Search: caller walks tree with comparison
 *   // Delete: rb_erase(&node->rb, &mytree)
 */

#ifndef RBTREE_H
#define RBTREE_H

/* Use kernel's uintptr if available, otherwise define it */
#ifndef uintptr
typedef unsigned long uintptr;
#endif

#ifndef nil
#define nil ((void *)0)
#endif

/*
 * Red-Black tree node - embed this in your structure
 */
struct rb_node {
  uintptr __rb_parent_color; /* Parent pointer + color bit */
  struct rb_node *rb_right;
  struct rb_node *rb_left;
};

/*
 * Red-Black tree root
 */
struct rb_root {
  struct rb_node *rb_node;
};

#define RB_ROOT                                                                \
  (struct rb_root) { nil }

/* Color encoding in parent pointer's low bit */
#define RB_RED 0
#define RB_BLACK 1

#define __rb_parent(pc) ((struct rb_node *)(pc & ~3))
#define __rb_color(pc) ((pc) & 1)
#define __rb_is_black(pc) __rb_color(pc)
#define __rb_is_red(pc) (!__rb_color(pc))
#define rb_color(rb) __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb) __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb) __rb_is_black((rb)->__rb_parent_color)

#define rb_parent(r) ((struct rb_node *)((r)->__rb_parent_color & ~3))

#define rb_entry(ptr, type, member)                                            \
  ((type *)((char *)(ptr) - (uintptr)(&((type *)0)->member)))

#define RB_EMPTY_ROOT(root) ((root)->rb_node == nil)
#define RB_EMPTY_NODE(node) ((node)->__rb_parent_color == (uintptr)(node))
#define RB_CLEAR_NODE(node) ((node)->__rb_parent_color = (uintptr)(node))

/* Core operations */
void rb_insert_color(struct rb_node *node, struct rb_root *root);
void rb_erase(struct rb_node *node, struct rb_root *root);

/* Link a node into the tree (before calling rb_insert_color) */
static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link) {
  node->__rb_parent_color = (uintptr)parent;
  node->rb_left = node->rb_right = nil;

  *rb_link = node;
}

/* Set parent and color */
static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p,
                                       int color) {
  rb->__rb_parent_color = (uintptr)p | (uintptr)color;
}

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p) {
  rb->__rb_parent_color = rb_color(rb) | (uintptr)p;
}

static inline void rb_set_black(struct rb_node *rb) {
  rb->__rb_parent_color |= RB_BLACK;
}

static inline void rb_set_red(struct rb_node *rb) {
  rb->__rb_parent_color &= ~1UL;
}

/* Tree navigation */
struct rb_node *rb_first(const struct rb_root *root);
struct rb_node *rb_last(const struct rb_root *root);
struct rb_node *rb_next(const struct rb_node *node);
struct rb_node *rb_prev(const struct rb_node *node);

/* Replace a node in the tree (for updates) */
void rb_replace_node(struct rb_node *victim, struct rb_node *new_node,
                     struct rb_root *root);

#endif /* RBTREE_H */
