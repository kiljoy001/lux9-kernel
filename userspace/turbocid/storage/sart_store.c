/*
 * Sartfs Store - Immutable Content-Addressed Storage
 *
 * Core storage engine implementing:
 *   - sart_write_immutable(): Write data, get content-derived UUIDv8
 *   - sart_read(): Read data by UUIDv8 identity
 *   - sart_rebuild_index(): Replay journal to rebuild index
 *
 * Features:
 *   - Real BLAKE2b hashing via Monocypher
 *   - Content deduplication (same content → existing UUID)
 *   - Adaptive Radix Tree index for O(k) lookups
 *   - Proper HJFS block allocation via getfree/getbuf/putbuf
 */

#include "dat.h"

/* liblux syscall wrappers */
extern long sys_pread(int fd, void *buf, long n, long offset);
extern long sys_pwrite(int fd, void *buf, long n, long offset);
extern uvlong sys_nsec(void);
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

/* ART index functions (from sart_art.c) */
extern void art_init(void);
extern RecordData *art_search(const UUIDv8 *key);
extern int art_insert(const UUIDv8 *key, const RecordData *value);
extern int art_exists(const UUIDv8 *key);
extern u64int art_size(void);
extern void art_clear(void);

/* Forward declarations */
int sart_rebuild_index(void);
int sart_add_edge(UUIDv8 *parent, const char *name, UUIDv8 *child);

/*
 * Monocypher BLAKE2b (from monocypher.h)
 *
 * void crypto_blake2b(uint8_t *hash, size_t hash_size,
 *                     const uint8_t *message, size_t message_size);
 */
extern void crypto_blake2b(u8int *hash, unsigned long hash_size,
                           const u8int *message, unsigned long message_size);

extern int tlsh_hash(const u8int *data, u64int len, u8int tlsh_out[35]);

/* Global store instance */
static SartStore g_store_instance;
static int g_initialized = 0;
static volatile int g_store_lock = 0;
static u64int g_disk_base_override = 0;
static u64int g_journal_base_override = 0;
static int g_base_override_set = 0;

void sart_set_backing_offsets(u64int disk_base, u64int journal_base) {
  g_disk_base_override = disk_base;
  g_journal_base_override = journal_base;
  g_base_override_set = 1;
}

/* Simple spinlock */
static void acquire_lock(volatile int *lock) {
  while (__sync_lock_test_and_set(lock, 1)) {
    // busy wait
    while (*lock)
      ;
  }
}

static void release_lock(volatile int *lock) { __sync_lock_release(lock); }

/* Debug helper */
extern long sys_write(int fd, void *buf, long n);
static void debug_print(const char *msg) {
  // long len = 0;
  // while(msg[len]) len++;
  // sys_write(1, (void*)msg, len);
}

static void init_log(const char *msg) {
  long len = 0;
  while (msg[len])
    len++;
  sys_write(2, (void *)msg, len);
}

/*
 * Content hash table for deduplication
 * Maps content hash → UUIDv8
 */
#define DEDUP_HASH_SIZE 256
typedef struct DedupEntry {
  u8int content_hash[32];
  UUIDv8 uuid;
  int valid;
} DedupEntry;

static DedupEntry g_dedup_table[DEDUP_HASH_SIZE];

/*
 * hash_to_bucket - Map 32-byte hash to bucket index
 */
static int hash_to_bucket(const u8int *hash) {
  /* Use first bytes for bucket, XOR for collision reduction */
  return (hash[0] ^ hash[1] ^ hash[2] ^ hash[3]) % DEDUP_HASH_SIZE;
}

/*
 * dedup_lookup - Check if content hash already exists
 * Returns pointer to existing UUID if found, nil otherwise
 */
static UUIDv8 *dedup_lookup(const u8int *content_hash) {
  int bucket = hash_to_bucket(content_hash);
  int i;

  /* Linear probe for matches */
  for (i = 0; i < DEDUP_HASH_SIZE; i++) {
    int idx = (bucket + i) % DEDUP_HASH_SIZE;
    if (!g_dedup_table[idx].valid)
      continue;
    if (memcmp(g_dedup_table[idx].content_hash, content_hash, 32) == 0)
      return &g_dedup_table[idx].uuid;
  }
  return nil;
}

/*
 * dedup_insert - Add content hash → UUID mapping
 */
static void dedup_insert(const u8int *content_hash, const UUIDv8 *uuid) {
  int bucket = hash_to_bucket(content_hash);
  int i;

  /* Find empty slot with linear probe */
  for (i = 0; i < DEDUP_HASH_SIZE; i++) {
    int idx = (bucket + i) % DEDUP_HASH_SIZE;
    if (!g_dedup_table[idx].valid) {
      memmove(g_dedup_table[idx].content_hash, content_hash, 32);
      memmove(&g_dedup_table[idx].uuid, uuid, sizeof(UUIDv8));
      g_dedup_table[idx].valid = 1;
      return;
    }
  }
  /* Table full - silently ignore (dedup is optimization, not required) */
}

extern int blkio_init(int fd, u64int size, u64int base);
extern int blkio_getfree(u64int *r);
extern int blkio_putfree(u64int r);
extern void *blkio_getbuf(u64int blkno, int type, int nodata);
extern void blkio_putbuf(void *b);
extern void blkio_mark_dirty(void *b);
extern void blkio_sync(void);
extern long blkio_read_block(u64int blkno, void *buf, u64int len);
extern long blkio_write_block(u64int blkno, const void *buf, u64int len);

/* Journal Layer Functions */
extern int journal_init(SartStore *store);
extern int journal_append_blob(UUIDv8 *id, RecordData *rec);
extern int journal_append_edge(UUIDv8 *parent, const char *name, UUIDv8 *child);
extern int journal_replay(void (*on_blob)(UUIDv8 *, RecordData *, void *),
                          void (*on_edge)(UUIDv8 *, const char *, UUIDv8 *,
                                          void *),
                          void *ctx);

/* BlkBuf structure from sart_blkio.c */
typedef struct {
  u8int data[BLOCK];
  u64int blkno;
  int flags;
  int refcnt;
  int type;
} BlkBuf;

/*
 * hjfs_alloc_block - Allocate a free block using HJFS-style allocator
 */
static int hjfs_alloc_block(u64int *block_addr) {
  return blkio_getfree(block_addr) > 0 ? SART_OK : SART_ERR_FULL;
}

/*
 * hjfs_write_block - Write data to a block using buffer cache
 */
static int hjfs_write_block(u64int block_addr, const void *data, u64int len) {
  BlkBuf *buf;

  /* Get buffer for write (nodata=1 since we're overwriting) */
  buf = (BlkBuf *)blkio_getbuf(block_addr, TRAW, 1);
  if (buf == nil)
    return SART_ERR_IO;

  /* Copy data to buffer */
  u64int copylen = (len > BLOCK) ? BLOCK : len;
  memmove(buf->data, data, copylen);

  /* Mark dirty and release */
  blkio_mark_dirty(buf);
  blkio_putbuf(buf);

  /* Handle multi-block writes */
  if (len > BLOCK) {
    /* Recursive call for remaining data */
    return hjfs_write_block(block_addr + 1, (const u8int *)data + BLOCK,
                            len - BLOCK);
  }

  return SART_OK;
}

/*
 * hjfs_read_block - Read data from a block using buffer cache
 */
static long hjfs_read_block(u64int block_addr, void *buf, u64int len) {
  BlkBuf *blkbuf;
  u64int copylen;
  long total = 0;

  while (len > 0) {
    /* Get buffer (read from disk if needed) */
    blkbuf = (BlkBuf *)blkio_getbuf(block_addr, TRAW, 0);
    if (blkbuf == nil)
      return total > 0 ? total : SART_ERR_IO;

    /* Copy from buffer */
    copylen = (len > BLOCK) ? BLOCK : len;
    memmove(buf, blkbuf->data, copylen);

    /* Release buffer */
    blkio_putbuf(blkbuf);

    buf = (u8int *)buf + copylen;
    len -= copylen;
    total += copylen;
    block_addr++;
  }

  return total;
}

/*
 * sart_init - Initialize the storage engine
 *
 * disk_fd: File descriptor for raw disk storage
 * journal_fd: File descriptor for journal file
 * total_blocks: Total available blocks on disk
 *
 * Returns 0 on success, negative on error.
 */
int sart_init(int disk_fd, int journal_fd, u64int total_blocks) {
  int rc;

  if (g_initialized)
    return SART_OK;

  memset(&g_store_instance, 0, sizeof(g_store_instance));
  g_store_instance.disk_fd = disk_fd;
  g_store_instance.journal_fd = journal_fd;
  g_store_instance.total_blocks = total_blocks;
  g_store_instance.next_block = 1; /* Block 0 reserved */
  if (g_base_override_set) {
    g_store_instance.disk_base = g_disk_base_override;
    g_store_instance.journal_base = g_journal_base_override;
  }

  /* Initialize block I/O layer */
  init_log("sart_init: blkio_init start\n");
  rc = blkio_init(disk_fd, total_blocks, g_store_instance.disk_base);
  if (rc != 0) {
    init_log("sart_init: blkio_init failed\n");
    return SART_ERR_IO;
  }
  init_log("sart_init: blkio_init ok\n");

  /* Initialize ART index */
  art_init();
  init_log("sart_init: art_init ok\n");

  /* Initialize dedup table */
  memset(g_dedup_table, 0, sizeof(g_dedup_table));

  /* Initialize journal */
  init_log("sart_init: journal_init start\n");
  rc = journal_init(&g_store_instance);
  if (rc != SART_OK) {
    init_log("sart_init: journal_init failed\n");
    return rc;
  }
  init_log("sart_init: journal_init ok\n");

  /* Rebuild index from journal if entries exist */
  if (g_store_instance.entry_count > 0) {
    rc = sart_rebuild_index();
    if (rc < 0) {
      init_log("sart_init: rebuild_index failed\n");
      return rc;
    }
  }

  g_initialized = 1;
  return SART_OK;
}

/*
 * generate_uuid_from_record - Hash RecordData to produce UUIDv8
 *
 * The identity is derived from the metadata, making it content-addressed.
 * Uses Monocypher BLAKE2b for cryptographic hashing.
 */
static void generate_uuid_from_record(RecordData *rec, UUIDv8 *id) {
  u8int hash[32];
  char msg[128];

  debug_print("DEBUG: generate_uuid_from_record: crypto_blake2b\n");
  /* Hash the entire RecordData structure with BLAKE2b */
  crypto_blake2b(hash, 32, (const u8int *)rec, sizeof(RecordData));

  debug_print(msg);

  /* Use first 16 bytes as UUID, setting version/variant bits */
  memmove(id->data, hash, 16);

  debug_print("DEBUG: generate_uuid_from_record: setting bits\n");
  /* Set UUIDv8 version (bits 48-51 = 0b1000) */
  id->data[6] = (id->data[6] & 0x0F) | 0x80;

  /* Set variant (bits 64-65 = 0b10) */
  id->data[8] = (id->data[8] & 0x3F) | 0x80;
  debug_print("DEBUG: generate_uuid_from_record: done\n");
}

/*
 * sart_write_immutable - Write data and return content-derived identity
 */
int sart_write_immutable(void *data, u64int len, u32int perms, UUIDv8 *id_out) {
  RecordData rec;
  u8int content_hash[32];
  UUIDv8 *existing;
  u64int block_addr;
  u64int blocks_needed;
  int rc;

  acquire_lock(&g_store_lock);
  debug_print("DEBUG: sart_write_immutable enter\n");

  if (!g_initialized || id_out == nil) {
    release_lock(&g_store_lock);
    return SART_ERR_IO;
  }

  /* Step 1: Hash content for deduplication */
  if (data && len > 0) {
    debug_print("DEBUG: hashing content\n");
    crypto_blake2b(content_hash, 32, (const u8int *)data, len);
  } else {
    /* Empty content hash */
    memset(content_hash, 0, 32);
    len = 0;
  }

  /* Step 2: Check dedup table (only for files with content) */
  if (len > 0) {
    debug_print("DEBUG: dedup lookup\n");
    existing = dedup_lookup(content_hash);
    if (existing != nil) {
      memmove(id_out, existing, sizeof(UUIDv8));
      release_lock(&g_store_lock);
      return SART_OK;
    }
  }

  /* Step 3: Allocate blocks */
  if (len > 0) {
    debug_print("DEBUG: allocating blocks\n");
    blocks_needed = (len + SART_BLOCK_SIZE - 1) / SART_BLOCK_SIZE;
    rc = hjfs_alloc_block(&block_addr);
    if (rc != SART_OK) {
      release_lock(&g_store_lock);
      return rc;
    }
    g_store_instance.next_block += (blocks_needed - 1);

    /* Step 4: Write data to disk */
    debug_print("DEBUG: writing blocks\n");
    rc = hjfs_write_block(block_addr, data, len);
    if (rc != SART_OK) {
      release_lock(&g_store_lock);
      return rc;
    }
  } else {
    block_addr = (u64int)-1; /* Sentinel for no data */
  }

  /* Step 5: Build RecordData */
  debug_print("DEBUG: building record\n");
  memset(&rec, 0, sizeof(rec));
  rec.block_addr = block_addr;
  rec.size = len;
  rec.perms = perms;
  rec.atime = sys_nsec();
  rec.mtime = sys_nsec();

  /* Initialize delta fields as FULL (not delta-compressed) */
  rec.type = 0;         /* FULL */
  rec.delta_format = 0; /* No delta */
  rec.delta_size = 0;
  rec.version = 1; /* First version */
  rec.chain_depth = 0;
  rec.write_timestamp = (u32int)(sys_nsec() / 1000000000ULL); /* Seconds */

  /* Compute content hash (BLAKE2b) */
  crypto_blake2b(rec.data_hash, 32, (const u8int *)data, len);

  if (len > 0 && data) {
    debug_print("DEBUG: tlsh hashing\n");
    if (tlsh_hash((const u8int *)data, len, rec.tlsh) != 0) {
      memset(rec.tlsh, 0, sizeof(rec.tlsh));
    }
  }

  /* Step 6: Generate UUID */
  debug_print("DEBUG: generating uuid\n");
  generate_uuid_from_record(&rec, id_out);

  char uuid_dbg[128];
  debug_print(uuid_dbg);
  for (int k = 0; k < 16; k++) {
    debug_print(uuid_dbg);
  }
  debug_print("\n");

  /* Step 7: Persist */
  debug_print("DEBUG: journal append\n");
  rc = journal_append_blob(id_out, &rec);
  if (rc != SART_OK) {
    release_lock(&g_store_lock);
    return rc;
  }

  /* Step 8: Index */
  debug_print("DEBUG: art insert\n");
  art_insert(id_out, &rec);
  if (len > 0)
    dedup_insert(content_hash, id_out);

  debug_print("DEBUG: sart_write_immutable success\n");
  release_lock(&g_store_lock);
  return SART_OK;
}

/*
 * sart_mkdir - Create a persistent directory
 */
int sart_mkdir(UUIDv8 *parent, const char *name, u32int perms, UUIDv8 *id_out) {
  int rc;
  rc = sart_write_immutable(nil, 0, perms | DMDIR, id_out);
  if (rc != SART_OK)
    return rc;

  return sart_add_edge(parent, name, id_out);
}

/*
 * sart_read - Read data by UUIDv8 identity
 *
 * id: The content-derived identity to look up
 * buf: Buffer to read into
 * len: Maximum bytes to read
 *
 * Returns bytes read on success, negative on error.
 */
long sart_read(UUIDv8 *id, void *buf, u64int len) {
  RecordData *rec;
  u64int read_len;

  acquire_lock(&g_store_lock);

  if (!g_initialized || id == nil || buf == nil) {
    release_lock(&g_store_lock);
    return SART_ERR_IO;
  }

  /* Look up in ART index - O(k) */
  rec = art_search(id);
  if (rec == nil) {
    release_lock(&g_store_lock);
    return SART_ERR_NOTFOUND;
  }

  /* Determine read length */
  read_len = (len < rec->size) ? len : rec->size;

  /* Read from disk via HJFS */
  long ret = hjfs_read_block(rec->block_addr, buf, read_len);

  release_lock(&g_store_lock);
  return ret;
}

/*
 * sart_exists - Check if UUIDv8 exists in store
 */
int sart_exists(UUIDv8 *id) { return art_exists(id); }

/*
 * sart_get_metadata - Get RecordData for a UUIDv8
 */
int sart_get_metadata(UUIDv8 *id, RecordData *rec_out) {
  RecordData *rec;

  if (!g_initialized || id == nil || rec_out == nil)
    return SART_ERR_IO;

  rec = art_search(id);
  if (rec == nil)
    return SART_ERR_NOTFOUND;

  memmove(rec_out, rec, sizeof(RecordData));
  return SART_OK;
}

/*
 * on_blob_callback - Rebuild content index
 */
static void on_blob_callback(UUIDv8 *id, RecordData *rec, void *ctx) {
  (void)ctx;
  u64int end_block =
      rec->block_addr + (rec->size + SART_BLOCK_SIZE - 1) / SART_BLOCK_SIZE;
  if (end_block >= g_store_instance.next_block)
    g_store_instance.next_block = end_block + 1;
  art_insert(id, rec);
  dedup_insert(rec->data_hash, id);
}

/*
 * on_edge_callback - Rebuild namespace index
 */
static void on_edge_callback(UUIDv8 *parent, const char *name, UUIDv8 *child,
                             void *ctx) {
  (void)ctx;
  extern int ns_art_insert(const UUIDv8 *parent, const char *name,
                           const UUIDv8 *child);
  ns_art_insert(parent, name, child);
}

/*
 * sart_rebuild_index - Replay journal to rebuild in-memory structures
 */
int sart_rebuild_index(void) {
  art_clear();
  memset(g_dedup_table, 0, sizeof(g_dedup_table));
  return journal_replay(on_blob_callback, on_edge_callback, nil);
}

/*
 * sart_add_edge - Persistent directory relationship
 */
int sart_add_edge(UUIDv8 *parent, const char *name, UUIDv8 *child) {
  if (!g_initialized)
    return SART_ERR_IO;
  return journal_append_edge(parent, name, child);
}

/*
 * sart_get_entry_count - Return total number of stored entries
 */
u64int sart_get_entry_count(void) { return art_size(); }

/*
 * sart_verify_content - Verify content matches stored hash
 *
 * Re-reads content, recomputes BLAKE2b, compares with stored hash.
 * Returns 1 if valid, 0 if corrupted.
 */
int sart_verify_content(UUIDv8 *id) {
  RecordData *rec;
  u8int buf[SART_BLOCK_SIZE];
  u8int computed_hash[32];
  long n;

  rec = art_search(id);
  if (rec == nil)
    return 0;

  /* Read content */
  n = hjfs_read_block(rec->block_addr, buf,
                      rec->size < SART_BLOCK_SIZE ? rec->size
                                                  : SART_BLOCK_SIZE);
  if (n <= 0)
    return 0;

  /* Recompute hash */
  crypto_blake2b(computed_hash, 32, buf, n);

  /* Compare with stored hash */
  return memcmp(computed_hash, rec->data_hash, 32) == 0;
}
