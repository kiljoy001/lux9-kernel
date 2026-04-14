#ifndef SART_ART_LAYOUT_H
#define SART_ART_LAYOUT_H

#include "dat.h"

/*
 * On-Disk Adaptive Radix Tree Layout
 * 
 * Instead of pointers, we use block addresses (u64int).
 * Nodes are stored in standard 4KB blocks.
 * 
 * To save space, we can pack headers.
 */

#define ART_MAGIC 0xA571D3  /* ART IDX */

typedef enum {
    NODE_TYPE_4   = 0,
    NODE_TYPE_16  = 1,
    NODE_TYPE_48  = 2,
    NODE_TYPE_256 = 3,
    NODE_TYPE_LEAF = 4
} NodeType;

/* Common Header for all on-disk nodes */
typedef struct {
    u32int magic;       /* Integrity check */
    u8int type;         /* NodeType */
    u8int num_children; 
    u16int prefix_len;
    u8int prefix[8];    /* Optimistic path compression */
    u8int padding[16];  /* Alignment/Reserved */
} ArtHeaderDisk;

/* Node4: Fits easily in a block */
typedef struct {
    ArtHeaderDisk header;
    u8int keys[4];
    u8int padding[4];   /* Align to 8 bytes */
    u64int children[4]; /* Block Addresses */
} ArtNode4Disk;

/* Node16 */
typedef struct {
    ArtHeaderDisk header;
    u8int keys[16];
    u64int children[16];
} ArtNode16Disk;

/* Node48 */
typedef struct {
    ArtHeaderDisk header;
    u8int child_idx[256]; /* Indirection: key byte -> index in children */
    u64int children[48];
} ArtNode48Disk;

/* Node256 */
typedef struct {
    ArtHeaderDisk header;
    u64int children[256]; /* Direct mapping */
} ArtNode256Disk;

/* Leaf Node (The Payload) */
typedef struct {
    ArtHeaderDisk header;
    u32int key_len;
    u32int val_len;
    /* Followed by key bytes, then value bytes */
    u8int payload[SART_BLOCK_SIZE - sizeof(ArtHeaderDisk) - 8]; 
} ArtLeafDisk;

/* 
 * The Root Anchor
 * Stored in the Superblock or a special dedicated block 
 */
typedef struct {
    u64int root_block; /* Block address of the root node */
    u64int entry_count;
} ArtRootDisk;

#endif
