/*
 * Sartfs Journal - Append-Only Persistence Layer
 *
 * The journal is the source of truth. It is an append-only log
 * of [UUIDv8, RecordData] entries. The in-memory index is rebuilt
 * by replaying this log on startup.
 */

#include "dat.h"

/* liblux syscall wrappers */
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern long sys_seek(int fd, long offset, int whence);
extern uvlong sys_nsec(void);
extern void *memset(void *dst, int c, unsigned long n);
extern void *memmove(void *dst, const void *src, unsigned long n);
extern int memcmp(const void *s1, const void *s2, unsigned long n);

#define P9_IO_CHUNK 3584

/* Forward declarations */
static u64int compute_checksum(JournalEntry *entry);
static int read_header(int fd, JournalHeader *hdr);
static int write_header(int fd, JournalHeader *hdr);

/* Global journal state */
static SartStore *g_store = nil;

static void journal_log(const char *msg) {
  long len = 0;
  while (msg[len])
    len++;
  sys_write(2, (void *)msg, len);
}

static long journal_pread_full(int fd, void *buf, long n, long offset) {
  u64int base = g_store ? g_store->journal_base : 0;
  if (sys_seek(fd, base + offset, 0) < 0)
    return -1;
  long total = 0;
  while (n > 0) {
    long chunk = n > P9_IO_CHUNK ? P9_IO_CHUNK : n;
    long r = sys_read(fd, buf, chunk);
    if (r < 0)
      return r;
    total += r;
    if (r != chunk)
      break;
    buf = (u8int *)buf + r;
    n -= r;
  }
  return total;
}

static long journal_pwrite_full(int fd, const void *buf, long n, long offset) {
  u64int base = g_store ? g_store->journal_base : 0;
  if (sys_seek(fd, base + offset, 0) < 0)
    return -1;
  long total = 0;
  while (n > 0) {
    long chunk = n > P9_IO_CHUNK ? P9_IO_CHUNK : n;
    long r = sys_write(fd, (void *)buf, chunk);
    if (r < 0)
      return r;
    total += r;
    if (r != chunk)
      break;
    buf = (const u8int *)buf + r;
    n -= r;
  }
  return total;
}

/*
 * journal_init - Initialize the journal subsystem
 *
 * Opens the journal file and reads or creates the header.
 * Returns 0 on success, negative on error.
 */
int journal_init(SartStore *store) {
  JournalHeader hdr;
  long n;

  if (store == nil || store->journal_fd < 0)
    return SART_ERR_IO;

  g_store = store;

  journal_log("journal_init: reading header\n");
  /* Try to read existing header */
  n = journal_pread_full(store->journal_fd, &hdr, sizeof(hdr), 0);
  journal_log("journal_init: header read complete\n");

  if (n == sizeof(hdr) && hdr.magic == SART_JOURNAL_MAGIC) {
    /* Existing journal - validate and use */
    if (hdr.version != SART_VERSION)
      return SART_ERR_CORRUPT;

    store->journal_pos = hdr.last_entry;
    store->entry_count = hdr.entry_count;
    return SART_OK;
  }
  if (n < 0) {
    journal_log("journal_init: pread failed\n");
  }

  /* New journal - initialize header */
  memset(&hdr, 0, sizeof(hdr));
  hdr.magic = SART_JOURNAL_MAGIC;
  hdr.version = SART_VERSION;
  hdr.entry_count = 0;
  hdr.first_entry = sizeof(JournalHeader);
  hdr.last_entry = sizeof(JournalHeader);

  n = journal_pwrite_full(store->journal_fd, &hdr, sizeof(hdr), 0);
  if (n != sizeof(hdr)) {
    journal_log("journal_init: pwrite failed\n");
    return SART_ERR_IO;
  }

  store->journal_pos = sizeof(JournalHeader);
  store->entry_count = 0;

  return SART_OK;
}

/*
 * journal_append_blob - Append a content registration entry
 */
int journal_append_blob(UUIDv8 *id, RecordData *rec) {
  JournalEntry entry;
  long n;
  u64int offset;

  if (g_store == nil || id == nil || rec == nil)
    return SART_ERR_IO;
  memset(&entry, 0, sizeof(entry));
  entry.magic = SART_ENTRY_MAGIC;
  entry.type = JENT_BLOB;
  entry.version = SART_VERSION;
  entry.timestamp = sys_nsec();
  memmove(&entry.u.blob.id, id, sizeof(UUIDv8));
  memmove(&entry.u.blob.rec, rec, sizeof(RecordData));

  entry.checksum = compute_checksum(&entry);
  offset = g_store->journal_pos;
  n = journal_pwrite_full(g_store->journal_fd, &entry, sizeof(entry), offset);
  if (n != sizeof(entry))
    return SART_ERR_IO;

  g_store->journal_pos += sizeof(entry);
  g_store->entry_count++;

  return SART_OK;
}

/*
 * journal_append_edge - Append a namespace relationship entry
 */
int journal_append_edge(UUIDv8 *parent, const char *name, UUIDv8 *child) {
  JournalEntry entry;
  long n;
  u64int offset;

  if (g_store == nil || parent == nil || name == nil || child == nil)
    return SART_ERR_IO;
  memset(&entry, 0, sizeof(entry));
  entry.magic = SART_ENTRY_MAGIC;
  entry.type = JENT_EDGE;
  entry.version = SART_VERSION;
  entry.timestamp = sys_nsec();
  memmove(&entry.u.edge.parent_id, parent, sizeof(UUIDv8));
  memmove(&entry.u.edge.child_id, child, sizeof(UUIDv8));
  for (int i = 0; i < 127 && name[i]; i++)
    entry.u.edge.name[i] = name[i];

  entry.checksum = compute_checksum(&entry);
  offset = g_store->journal_pos;
  n = journal_pwrite_full(g_store->journal_fd, &entry, sizeof(entry), offset);
  if (n != sizeof(entry))
    return SART_ERR_IO;

  g_store->journal_pos += sizeof(entry);
  g_store->entry_count++;

  return SART_OK;
}

/*
 * journal_replay - Replay journal and invoke callbacks based on entry type
 */
int journal_replay(void (*on_blob)(UUIDv8 *id, RecordData *rec, void *ctx),
                   void (*on_edge)(UUIDv8 *parent, const char *name,
                                   UUIDv8 *child, void *ctx),
                   void *ctx) {
  JournalHeader hdr;
  JournalEntry entry;
  u64int offset;
  u64int checksum;
  int count = 0;
  long n;

  if (g_store == nil)
    return SART_ERR_IO;

  n = journal_pread_full(g_store->journal_fd, &hdr, sizeof(hdr), 0);
  if (n != sizeof(hdr) || hdr.magic != SART_JOURNAL_MAGIC)
    return SART_ERR_CORRUPT;

  offset = hdr.first_entry;
  while (1) {
    n = journal_pread_full(g_store->journal_fd, &entry, sizeof(entry), offset);
    /* Stop at partial read or EOF */
    if (n != sizeof(entry))
      break;

    /* Stop at invalid magic (unwritten space) */
    if (entry.magic != SART_ENTRY_MAGIC)
      break;

    /* Stop at corruption */
    checksum = entry.checksum;
    entry.checksum = 0;
    if (compute_checksum(&entry) != checksum)
      break;
    entry.checksum = checksum;

    if (entry.type == JENT_BLOB && on_blob) {
      on_blob(&entry.u.blob.id, &entry.u.blob.rec, ctx);
    } else if (entry.type == JENT_EDGE && on_edge) {
      on_edge(&entry.u.edge.parent_id, entry.u.edge.name,
              &entry.u.edge.child_id, ctx);
    }

    count++;
    offset += sizeof(entry);
  }

  /* Update in-memory state to reflect what we actually found */
  g_store->journal_pos = offset;
  g_store->entry_count = count;

  return count;
}

/*
 * journal_sync - Ensure all journal data is persisted
 *
 * This is a no-op in single-threaded synchronous mode,
 * but provided for API completeness.
 */
int journal_sync(void) {
  JournalHeader hdr;
  
  if (g_store == nil) return SART_ERR_IO;

  /* Read current header template */
  if (read_header(g_store->journal_fd, &hdr) != SART_OK)
      return SART_ERR_IO;

  /* Update with current state */
  hdr.entry_count = g_store->entry_count;
  hdr.last_entry = g_store->journal_pos;

  /* Write back */
  return write_header(g_store->journal_fd, &hdr);
}

/*
 * journal_get_count - Return number of entries in journal
 */
u64int journal_get_count(void) {
  if (g_store == nil)
    return 0;
  return g_store->entry_count;
}

/*
 * compute_checksum - XOR fold integrity check
 *
 * Simple but effective for detecting corruption.
 * Computes XOR of all 64-bit words in the entry.
 */
static u64int compute_checksum(JournalEntry *entry) {
  u64int *words = (u64int *)entry;
  u64int checksum = 0;
  int nwords = (sizeof(JournalEntry) - sizeof(u64int)) / sizeof(u64int);
  int i;

  /* XOR all words except the checksum field itself */
  for (i = 0; i < nwords; i++) {
    /* Skip the checksum field (last u64int) */
    checksum ^= words[i];
  }

  return checksum;
}

/*
 * Helper: read journal header
 */
static int read_header(int fd, JournalHeader *hdr) {
  long n = journal_pread_full(fd, hdr, sizeof(*hdr), 0);
  if (n != sizeof(*hdr))
    return SART_ERR_IO;
  if (hdr->magic != SART_JOURNAL_MAGIC)
    return SART_ERR_CORRUPT;
  return SART_OK;
}

/*
 * Helper: write journal header
 */
static int write_header(int fd, JournalHeader *hdr) {
  long n = journal_pwrite_full(fd, hdr, sizeof(*hdr), 0);
  if (n != sizeof(*hdr))
    return SART_ERR_IO;
  return SART_OK;
}
