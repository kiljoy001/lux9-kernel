/*
 * Sartfs Journal - Append-Only Persistence Layer
 *
 * The journal is the source of truth. It is an append-only log
 * of [UUIDv8, RecordData] entries. The in-memory index is rebuilt
 * by replaying this log on startup.
 */

#include "dat.h"

/* liblux syscall wrappers */
extern long sys_pread(int fd, void *buf, long n, long offset);
extern long sys_pwrite(int fd, void *buf, long n, long offset);
extern uvlong sys_nsec(void);
extern void *memset(void *dst, int c, unsigned long n);
extern void *memmove(void *dst, const void *src, unsigned long n);
extern int memcmp(const void *s1, const void *s2, unsigned long n);

/* Forward declarations */
static u64int compute_checksum(JournalEntry *entry);
static int read_header(int fd, JournalHeader *hdr);
static int write_header(int fd, JournalHeader *hdr);

/* Global journal state */
static SartStore *g_store = nil;

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

  /* Try to read existing header */
  n = sys_pread(store->journal_fd, &hdr, sizeof(hdr), 0);

  if (n == sizeof(hdr) && hdr.magic == SART_JOURNAL_MAGIC) {
    /* Existing journal - validate and use */
    if (hdr.version != SART_VERSION)
      return SART_ERR_CORRUPT;

    store->journal_pos = hdr.last_entry;
    store->entry_count = hdr.entry_count;
    return SART_OK;
  }

  /* New journal - initialize header */
  memset(&hdr, 0, sizeof(hdr));
  hdr.magic = SART_JOURNAL_MAGIC;
  hdr.version = SART_VERSION;
  hdr.entry_count = 0;
  hdr.first_entry = sizeof(JournalHeader);
  hdr.last_entry = sizeof(JournalHeader);

  n = sys_pwrite(store->journal_fd, &hdr, sizeof(hdr), 0);
  if (n != sizeof(hdr))
    return SART_ERR_IO;

  store->journal_pos = sizeof(JournalHeader);
  store->entry_count = 0;

  return SART_OK;
}

/*
 * journal_append - Append a new entry to the journal
 *
 * Serializes the UUIDv8 and RecordData, computes checksum,
 * and appends to the journal file.
 *
 * Returns 0 on success, negative on error.
 */
int journal_append(UUIDv8 *id, RecordData *rec) {
  JournalEntry entry;
  JournalHeader hdr;
  long n;
  u64int offset;

  if (g_store == nil || id == nil || rec == nil)
    return SART_ERR_IO;

  /* Build the entry */
  memset(&entry, 0, sizeof(entry));
  entry.magic = SART_ENTRY_MAGIC;
  entry.version = SART_VERSION;
  memmove(&entry.id, id, sizeof(UUIDv8));
  memmove(&entry.rec, rec, sizeof(RecordData));

  /* Get timestamp via nsec syscall */
  entry.timestamp = sys_nsec();

  /* Compute integrity checksum */
  entry.checksum = compute_checksum(&entry);

  /* Append to journal */
  offset = g_store->journal_pos;
  n = sys_pwrite(g_store->journal_fd, &entry, sizeof(entry), offset);
  if (n != sizeof(entry))
    return SART_ERR_IO;

  /* Update header */
  n = sys_pread(g_store->journal_fd, &hdr, sizeof(hdr), 0);
  if (n != sizeof(hdr))
    return SART_ERR_IO;

  hdr.entry_count++;
  hdr.last_entry = offset + sizeof(entry);

  n = sys_pwrite(g_store->journal_fd, &hdr, sizeof(hdr), 0);
  if (n != sizeof(hdr))
    return SART_ERR_IO;

  /* Update local state */
  g_store->journal_pos = hdr.last_entry;
  g_store->entry_count = hdr.entry_count;

  return SART_OK;
}

/*
 * journal_replay - Replay journal and invoke callback per entry
 *
 * Reads the journal from start to end, validating each entry
 * and invoking the callback function for every valid entry.
 * Used to rebuild the in-memory index on startup.
 *
 * Returns number of entries processed, or negative on error.
 */
int journal_replay(void (*callback)(UUIDv8 *id, RecordData *rec, void *ctx),
                   void *ctx) {
  JournalHeader hdr;
  JournalEntry entry;
  u64int offset;
  u64int checksum;
  int count = 0;
  long n;

  if (g_store == nil || callback == nil)
    return SART_ERR_IO;

  /* Read header */
  n = sys_pread(g_store->journal_fd, &hdr, sizeof(hdr), 0);
  if (n != sizeof(hdr) || hdr.magic != SART_JOURNAL_MAGIC)
    return SART_ERR_CORRUPT;

  /* Iterate through entries */
  offset = hdr.first_entry;
  while (offset < hdr.last_entry) {
    n = sys_pread(g_store->journal_fd, &entry, sizeof(entry), offset);
    if (n != sizeof(entry))
      break;

    /* Validate entry */
    if (entry.magic != SART_ENTRY_MAGIC)
      break;

    /* Verify checksum */
    checksum = entry.checksum;
    entry.checksum = 0;
    if (compute_checksum(&entry) != checksum) {
      /* Corrupted entry - stop replay */
      break;
    }
    entry.checksum = checksum;

    /* Invoke callback */
    callback(&entry.id, &entry.rec, ctx);
    count++;

    offset += sizeof(entry);
  }

  return count;
}

/*
 * journal_sync - Ensure all journal data is persisted
 *
 * This is a no-op in single-threaded synchronous mode,
 * but provided for API completeness.
 */
int journal_sync(void) {
  /* In synchronous mode, writes are already persisted */
  return SART_OK;
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
  long n = sys_pread(fd, hdr, sizeof(*hdr), 0);
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
  long n = sys_pwrite(fd, hdr, sizeof(*hdr), 0);
  if (n != sizeof(*hdr))
    return SART_ERR_IO;
  return SART_OK;
}
