/*
 * Sartfs Block I/O Layer
 *
 * Synchronous block I/O implementation compatible with HJFS semantics.
 * Provides getfree/getbuf/putbuf without libthread dependencies.
 *
 * This is a simplified version for single-threaded freestanding execution.
 * The full HJFS uses async I/O via Plan 9 channels - we use direct
 * pread/pwrite.
 */

#include "dat.h"

/* liblux syscall wrappers */
extern long sys_pread(int fd, void *buf, long n, long offset);
extern long sys_pwrite(int fd, void *buf, long n, long offset);
extern void *memset(void *dst, int c, unsigned long n);
extern void *memmove(void *dst, const void *src, unsigned long n);

/*
 * Block Buffer Cache
 *
 * Simple LRU-style cache for block buffers.
 * Much simpler than HJFS's full buffer cache.
 */
#define NBUF 64        /* Number of cached buffers */
#define BLK_FREE 0x01  /* Block is in free list */
#define BLK_DIRTY 0x02 /* Block needs to be written back */
#define BLK_VALID 0x04 /* Block contains valid data */

typedef struct BlkBuf {
  u8int data[BLOCK]; /* Block data - 4KB */
  u64int blkno;      /* Block number on disk */
  int flags;         /* Status flags */
  int refcnt;        /* Reference count */
  int type;          /* Block type (TRAW, TREF, etc.) */
} BlkBuf;

/* Device context */
typedef struct BlkDev {
  int fd;             /* File descriptor */
  u64int size;        /* Total blocks */
  u64int fstart;      /* Start of freelist blocks */
  u64int fend;        /* End of freelist blocks */
  u64int next_scan;   /* Next block to scan for free */
  BlkBuf cache[NBUF]; /* Buffer cache */
} BlkDev;

/* Global device instance */
static BlkDev g_dev;
static int g_dev_initialized = 0;

/*
 * blkio_init - Initialize block I/O layer
 *
 * fd: Open file descriptor for block device
 * size: Total number of blocks on device
 *
 * Block 0 is reserved for superblock.
 * Returns 0 on success, -1 on error.
 */
int blkio_init(int fd, u64int size) {
  if (g_dev_initialized)
    return 0;

  memset(&g_dev, 0, sizeof(g_dev));
  g_dev.fd = fd;
  g_dev.size = size;
  g_dev.fstart = 1; /* Block 0 is superblock */
  g_dev.fend = size;
  g_dev.next_scan = 1;

  /* Initialize buffer cache */
  for (int i = 0; i < NBUF; i++) {
    g_dev.cache[i].blkno = ~0ULL;
    g_dev.cache[i].flags = 0;
    g_dev.cache[i].refcnt = 0;
  }

  g_dev_initialized = 1;
  return 0;
}

/*
 * find_buf - Find buffer in cache or allocate new one
 */
static BlkBuf *find_buf(u64int blkno) {
  int i, oldest = 0;

  /* Look for existing buffer */
  for (i = 0; i < NBUF; i++) {
    if (g_dev.cache[i].blkno == blkno && (g_dev.cache[i].flags & BLK_VALID)) {
      return &g_dev.cache[i];
    }
  }

  /* Find free or LRU buffer */
  for (i = 0; i < NBUF; i++) {
    if (g_dev.cache[i].refcnt == 0) {
      /* Write back if dirty */
      if (g_dev.cache[i].flags & BLK_DIRTY) {
        sys_pwrite(g_dev.fd, g_dev.cache[i].data, BLOCK,
                   g_dev.cache[i].blkno * BLOCK);
        g_dev.cache[i].flags &= ~BLK_DIRTY;
      }
      oldest = i;
      break;
    }
  }

  /* Evict and reuse */
  BlkBuf *b = &g_dev.cache[oldest];
  if (b->flags & BLK_DIRTY) {
    sys_pwrite(g_dev.fd, b->data, BLOCK, b->blkno * BLOCK);
  }

  b->blkno = blkno;
  b->flags = 0;
  b->type = TRAW;

  return b;
}

/*
 * getbuf - Get a buffer for a block
 *
 * blkno: Block number to get
 * type: Expected block type (TRAW, TREF, TDENTRY, etc.)
 * nodata: If 1, don't read from disk (for new blocks)
 *
 * Returns buffer pointer, or nil on error.
 */
BlkBuf *blkio_getbuf(u64int blkno, int type, int nodata) {
  BlkBuf *b;
  long n;

  if (!g_dev_initialized)
    return nil;

  if (blkno >= g_dev.size)
    return nil;

  b = find_buf(blkno);
  if (b == nil)
    return nil;

  /* Read from disk if needed */
  if (!(b->flags & BLK_VALID) && !nodata) {
    n = sys_pread(g_dev.fd, b->data, BLOCK, blkno * BLOCK);
    if (n != BLOCK) {
      b->flags = 0;
      b->blkno = ~0ULL;
      return nil;
    }
    b->flags |= BLK_VALID;
  } else if (nodata) {
    /* Zero out for new block */
    memset(b->data, 0, BLOCK);
    b->flags |= BLK_VALID;
  }

  b->type = type;
  b->refcnt++;

  return b;
}

/*
 * putbuf - Release a buffer
 *
 * If dirty, write back to disk.
 */
void blkio_putbuf(BlkBuf *b) {
  if (b == nil)
    return;

  if (b->refcnt > 0)
    b->refcnt--;

  /* Write back if dirty and unreferenced */
  if (b->refcnt == 0 && (b->flags & BLK_DIRTY)) {
    sys_pwrite(g_dev.fd, b->data, BLOCK, b->blkno * BLOCK);
    b->flags &= ~BLK_DIRTY;
  }
}

/*
 * blkio_mark_dirty - Mark buffer as dirty (needs writeback)
 */
void blkio_mark_dirty(BlkBuf *b) {
  if (b != nil)
    b->flags |= BLK_DIRTY;
}

/*
 * Reference Counting for Block Allocation
 *
 * We use a simple bitmap instead of HJFS's 24-bit refcount.
 * This is sufficient for append-only immutable storage.
 */
#define BITMAP_BLOCKS 16 /* Blocks for allocation bitmap */
static u8int g_alloc_bitmap[BITMAP_BLOCKS * BLOCK];
static int g_bitmap_loaded = 0;

/*
 * load_bitmap - Load allocation bitmap from disk
 */
static int load_bitmap(void) {
  long n;

  if (g_bitmap_loaded)
    return 0;

  /* Bitmap stored at blocks 1..BITMAP_BLOCKS */
  for (int i = 0; i < BITMAP_BLOCKS; i++) {
    n = sys_pread(g_dev.fd, &g_alloc_bitmap[i * BLOCK], BLOCK, (1 + i) * BLOCK);
    if (n != BLOCK) {
      /* New disk - zero initialize */
      memset(&g_alloc_bitmap[i * BLOCK], 0, BLOCK);
    }
  }

  /* Mark system blocks as used */
  for (int i = 0; i <= BITMAP_BLOCKS; i++) {
    g_alloc_bitmap[i / 8] |= (1 << (i % 8));
  }

  g_bitmap_loaded = 1;
  return 0;
}

/*
 * save_bitmap - Save allocation bitmap to disk
 */
static int save_bitmap(void) {
  for (int i = 0; i < BITMAP_BLOCKS; i++) {
    sys_pwrite(g_dev.fd, &g_alloc_bitmap[i * BLOCK], BLOCK, (1 + i) * BLOCK);
  }
  return 0;
}

/*
 * blkio_getfree - Allocate a free block
 *
 * r: Output block number
 *
 * Returns 1 on success, -1 on error (disk full).
 */
int blkio_getfree(u64int *r) {
  u64int i, start;

  if (!g_dev_initialized)
    return -1;

  load_bitmap();

  /* Start from where we left off */
  start = g_dev.next_scan;
  i = start;

  do {
    /* Skip system blocks */
    if (i <= BITMAP_BLOCKS) {
      i = BITMAP_BLOCKS + 1;
      continue;
    }

    /* Check if block is free */
    if ((g_alloc_bitmap[i / 8] & (1 << (i % 8))) == 0) {
      /* Mark as used */
      g_alloc_bitmap[i / 8] |= (1 << (i % 8));
      save_bitmap();

      *r = i;
      g_dev.next_scan = i + 1;
      return 1;
    }

    i++;
    if (i >= g_dev.size)
      i = BITMAP_BLOCKS + 1;

  } while (i != start);

  /* Disk full */
  return -1;
}

/*
 * blkio_putfree - Free a block
 *
 * r: Block number to free
 *
 * Returns 1 on success.
 */
int blkio_putfree(u64int r) {
  if (!g_dev_initialized)
    return -1;

  if (r <= BITMAP_BLOCKS || r >= g_dev.size)
    return -1;

  load_bitmap();

  /* Clear bit */
  g_alloc_bitmap[r / 8] &= ~(1 << (r % 8));
  save_bitmap();

  return 1;
}

/*
 * blkio_sync - Flush all dirty buffers to disk
 */
void blkio_sync(void) {
  for (int i = 0; i < NBUF; i++) {
    if (g_dev.cache[i].flags & BLK_DIRTY) {
      sys_pwrite(g_dev.fd, g_dev.cache[i].data, BLOCK,
                 g_dev.cache[i].blkno * BLOCK);
      g_dev.cache[i].flags &= ~BLK_DIRTY;
    }
  }

  if (g_bitmap_loaded)
    save_bitmap();
}

/*
 * blkio_read_block - Direct block read
 *
 * For use when you don't want caching.
 */
long blkio_read_block(u64int blkno, void *buf, u64int len) {
  if (!g_dev_initialized || blkno >= g_dev.size)
    return -1;

  return sys_pread(g_dev.fd, buf, len, blkno * BLOCK);
}

/*
 * blkio_write_block - Direct block write
 *
 * For use when you don't want caching.
 */
long blkio_write_block(u64int blkno, const void *buf, u64int len) {
  if (!g_dev_initialized || blkno >= g_dev.size)
    return -1;

  return sys_pwrite(g_dev.fd, (void *)buf, len, blkno * BLOCK);
}

/*
 * blkio_get_dev_size - Return device size in blocks
 */
u64int blkio_get_dev_size(void) { return g_dev_initialized ? g_dev.size : 0; }
