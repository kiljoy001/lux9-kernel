/*
 * Exchange device - 9P interface for page exchange operations
 * Provides Singularity-style exchange heap semantics at page granularity
 *
 * Extended with channel pool architecture:
 * #X/clone - allocate new channel
 * #X/N/ctl - channel control
 * #X/N/ring - ring buffer control page (mmap)
 * #X/N/ipcring - IPC ring buffer control page (mmap)
 * #X/N/pool - exchange page pool (read=alloc, write=free)
 * #X/N/stats - channel statistics
 * #X/N/peers - connected peers
 */

#include "9p_router.h" /* For EXCHANGE_PAGE_ADDR */
#include "blind_ledger.h"
#include "dat.h"
#include "exchange.h"
#include "fcall.h"
#include "fns.h"
#include "hhdm.h"
#include "ipc_ring.h"
#include "mem.h"
#include "pageown.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include "uuid.h"
#include <error.h>

enum {
  /* Top level */
  Qtopdir = 0,    /* top level directory */
  Qclone = 1,     /* allocate new channel */
  Qglobalctl = 2, /* global control/stats */

  /* Channel files (path = (chan_id << 8) | subfile) */
  Qchandir = 3,  /* channel N directory */
  Qctl = 4,      /* channel control */
  Qring = 5,     /* ring buffer control page */
  Qipcring = 11, /* IPC ring buffer control page */
  Qpool = 6,     /* exchange page pool */
  Qstats = 7,    /* channel stats */
  Qpeers = 8,    /* peer list */
  Qdata = 12,    /* 9P data stream (forwarded to ring) */

  /* Legacy */
  Qexchange = 9, /* old exchange interface (deprecated) */
  Qstat = 10,    /* old statistics */
};

#define CHANQID(id, subfile) (((id) << 8) | (subfile))
#define CHANID(qid) ((qid) >> 8)
#define SUBFILE(qid) ((qid) & 0xFF)
#define ISCHANDIR(qid) (SUBFILE(qid) == Qchandir)

#define MAX_EXCHANGE_CHANNELS 256
#define DEFAULT_POOL_SIZE 16

char *Enochannel = "channel does not exist";

/* Ring Buffer Control Page Structure (4KB, mmap'd to userspace) */
typedef struct RingControl {
  u32int magic;   /* 0x52494E47 "RING" */
  u32int version; /* 1 */

  /* Submission Ring (User → Kernel) */
  struct {
    u32int head; /* Consumer index (kernel) */
    u32int tail; /* Producer index (user) */
    u32int mask; /* Ring size - 1 */
    u32int flags;
    u64int pages[120]; /* 960 bytes - Page Handles */
    uuid_t uuids[120]; /* 1920 bytes - Caller IDs */
  } submission;

  /* Completion Ring (Kernel → User) */
  struct {
    u32int head; /* Consumer index (user) */
    u32int tail; /* Producer index (kernel) */
    u32int mask;
    u32int flags;
    u64int pages[120]; /* 960 bytes */
    /* Completion doesn't strictly need UUIDs echoed back if response is
     * in-place, but ok to keep symm */
    uuid_t uuids[120]; /* 1920 bytes */
  } completion;

  /* Security */
  uuid_t session_uuid; /* UUIDv8 session identifier */

  /* Padding to 4KB: 8 + 16 + 1920 + 16 + 1920 + 16 = 3896 bytes, need 200 bytes
   * padding */
  u8int reserved[200];
} RingControl;

/* Exchange Channel - per-process communication endpoint */
typedef struct ExchangeChannel {
  Ref ref;
  Lock lock;
  Proc *owner; /* Owning process */
  int chan_id; /* Channel ID */

  /* Ring Buffer Control Page (mmap'd to userspace) */
  RingControl *ring_kaddr; /* Kernel mapping */
  UserCapability ring_cap; /* Capability for ring page */
  IpcChannel *ipc_kaddr;   /* Kernel mapping for IPC ring */
  UserCapability ipc_cap;  /* Capability for IPC ring page */

  /* Exchange Page Pool */
  UserCapability *pool_caps; /* Array of capabilities */
  uint pool_size;            /* Total pool size */
  uint pool_head;            /* Free list head */
  uint pool_tail;            /* Free list tail */
  Lock pool_lock;            /* Pool allocation lock */

  /* Channel Mode */
  enum {
    CHAN_KERNEL,  /* Kernel syscall endpoint */
    CHAN_PEER,    /* Peer IPC endpoint */
    CHAN_SERVICE, /* 9P Service endpoint (mount target) */
  } mode;

  /* Peer Connection (if mode == CHAN_PEER) */
  struct ExchangeChannel *peer; /* Connected peer channel */

  /* Statistics */
  u64int messages_sent;
  u64int messages_recv;
  u64int bytes_sent;
  u64int bytes_recv;
  u64int pool_allocs;
  u64int pool_frees;

  /* TOCTOU Protection */
  u64int last_seqno;     /* Replay protection */
  uuid_t session_uuid;   /* Per-channel UUIDv8 session ID */
  u64int ipc_last_seqno; /* IPC ring replay protection */
  Rendez data_wait;      /* Wait for data in Qdata */
  int dead;              /* Channel is dead/closed */
} ExchangeChannel;

/* Global channel table */
static ExchangeChannel *channels[MAX_EXCHANGE_CHANNELS];
static Lock channels_lock;

/* Legacy exchange control */
typedef struct Exchctl Exchctl;
struct Exchctl {
  QLock qlock;

  /* Prepared pages tracking */
  struct {
    ExchangeHandle handle;
    uintptr original_vaddr;
    Proc *owner;
    ulong time; /* time when prepared */
  } prepared[1024];
  int nprepared;
};

static Exchctl exchctl;
extern void userpmap(uintptr, uintptr, int);
extern uintptr paddr(void *);

typedef struct Dirtab Dirtab;
Dirtab exchdir[] = {
    {".", {Qtopdir, 0, QTDIR}, 0, DMDIR | 0555},
    {"clone", {Qclone, 0}, 0, 0666},
    {"ctl", {Qglobalctl, 0}, 0, 0666},
    {"exchange", {Qexchange, 0}, 0, 0666}, /* Legacy */
    {"stat", {Qstat, 0}, 0, 0444},         /* Legacy */
};

Dirtab chandir[] = {
    {".", {Qchandir, 0, QTDIR}, 0, DMDIR | 0555},
    {"ctl", {Qctl, 0}, 0, 0666},
    {"ring", {Qring, 0}, 0, 0666},
    {"ipcring", {Qipcring, 0}, 0, 0666},
    {"pool", {Qpool, 0}, 0, 0666},
    {"stats", {Qstats, 0}, 0, 0444},
    {"peers", {Qpeers, 0}, 0, 0444},
    {"data", {Qdata, 0}, 0, 0666},
};

/* Helper: compare two UserCapability structs */
/*@
  @ requires a == \null || \valid(a);
  @ requires b == \null || \valid(b);
  @ assigns \nothing;
  @*/
static int capability_equal(const ExchangeHandle *a, const ExchangeHandle *b) {
  return memcmp(a->hash, b->hash, BLIND_LEDGER_CAP_SIZE) == 0 &&
         a->size == b->size && a->type == b->type && a->perms == b->perms;
}

/* Helper: check if UserCapability is zero/invalid */
/*@
  @ requires cap == \null || \valid(cap);
  @ assigns \nothing;
  @*/
static int capability_is_zero(const ExchangeHandle *cap) {
  static const u8int zero_hash[BLIND_LEDGER_CAP_SIZE] = {0};
  return memcmp(cap->hash, zero_hash, BLIND_LEDGER_CAP_SIZE) == 0 &&
         cap->size == 0 && cap->type == 0 && cap->perms == 0;
}

/*@
  @ requires r == \null || \valid(r);
  @ requires ename == \null || \valid(ename);
  @ assigns \nothing;
  @*/
static void build_error_reply(Fcall *r, ushort tag, char *ename) {
  memset(r, 0, sizeof(*r));
  r->type = Rerror;
  r->tag = tag;
  r->ename = ename;
}

static int p9_build_reply_batch(Proc *caller, BatchHeader *batch,
                                u16int batch_num_messages,
                                u16int batch_used_bytes, u64int batch_seqno) {
  uchar *resp_page;
  BatchHeader *resp;
  int read_offset;
  int write_offset;
  int i;

  resp_page = smalloc(BY2PG);
  if (resp_page == nil)
    return -1;
  memset(resp_page, 0, BY2PG);

  resp = (BatchHeader *)resp_page;
  resp->magic = BATCH_PAGE_MAGIC;
  resp->nonce = batch_seqno;
  resp->num_messages = 0;
  resp->used_bytes = BATCH_DATA_START;

  read_offset = BATCH_DATA_START;
  write_offset = BATCH_DATA_START;
  for (i = 0; i < batch_num_messages; i++) {
    if (read_offset + BIT16SZ > batch_used_bytes)
      break;

    u16int msg_len = GBIT16((u8int *)batch + read_offset);
    read_offset += BIT16SZ;
    if (msg_len == 0 || msg_len > P9_MSG_SIZE)
      break;
    if (read_offset + msg_len > batch_used_bytes)
      break;

    Fcall t, r;
    t = (Fcall){0};
    if (convM2S((u8int *)batch + read_offset, msg_len, &t) == 0) {
      build_error_reply(&r, NOTAG, "bad 9p message");
    } else {
      r = (Fcall){0};
      if (waserror()) {
        build_error_reply(&r, t.tag, up->errstr);
      } else {
        if (p9_dispatch(caller, &t, &r) < 0 && r.type != Rerror)
          build_error_reply(&r, t.tag, "dispatch failed");
        poperror();
      }
    }

    if (write_offset + BIT16SZ >= BY2PG)
      break;
    u16int avail = BY2PG - (write_offset + BIT16SZ);
    u16int rep_size = convS2M(&r, resp_page + write_offset + BIT16SZ, avail);
    if (rep_size == 0)
      break;

    PBIT16(resp_page + write_offset, rep_size);
    write_offset += BIT16SZ + rep_size;
    resp->num_messages++;
    resp->used_bytes = write_offset;

    read_offset += msg_len;
  }

  memmove(batch, resp_page, BY2PG);
  free(resp_page);
  return 0;
}

/* Channel Management Helpers */

static void channel_release_pages(ExchangeChannel *ch) {
  if (ch == nil)
    return;

  if (ch->ring_kaddr != nil) {
    xfree(ch->ring_kaddr);
    ch->ring_kaddr = nil;
  }
  if (ch->ipc_kaddr != nil) {
    xfree(ch->ipc_kaddr);
    ch->ipc_kaddr = nil;
  }
}

static void channel_unregister_physsegs(ExchangeChannel *ch) {
  char segname[32];

  if (ch == nil || ch->chan_id < 0)
    return;

  snprint(segname, sizeof(segname), "#X/%d/ring", ch->chan_id);
  delphysseg(segname);
  snprint(segname, sizeof(segname), "#X/%d/ipcring", ch->chan_id);
  delphysseg(segname);
}

static ExchangeChannel *channel_alloc(Proc *owner) {
  ExchangeChannel *ch;
  int i;
  Physseg ps;
  char segname[32];

  ch = smalloc(sizeof(ExchangeChannel));
  if (ch == nil)
    return nil;

  memset(ch, 0, sizeof(ExchangeChannel));
  ch->owner = owner;
  ch->mode = CHAN_KERNEL;
  ch->pool_size = DEFAULT_POOL_SIZE;
  ch->ref.ref = 1;

  /* Allocate ring buffer control page */
  void *ring_kva;
  void *ipc_kva;
  uintptr ring_pa;
  uintptr ipc_pa;
  u8int vault_secret[32];
  BlindLedgerError err;

  ring_kva = xspanalloc(4096, BY2PG, 0);
  if (ring_kva == nil) {
    free(ch);
    return nil;
  }
  ring_pa = paddr(ring_kva);

  /* Generate vault secret and mint capability */
  ledger_generate_secret(vault_secret);
  err = ledger_mint(&ch->ring_cap, (uintptr)ring_pa, 4096, owner,
                    CAP_PERM_READ | CAP_PERM_WRITE, vault_secret);
  if (err != BLIND_LEDGER_OK) {
    xfree(ring_kva);
    free(ch);
    return nil;
  }

  ch->ring_kaddr = (RingControl *)ring_kva;
  memset(ch->ring_kaddr, 0, 4096);
  ch->ring_kaddr->magic = 0x52494E47;
  ch->ring_kaddr->version = 1;
  ch->ring_kaddr->submission.mask = 119; /* 120 - 1 */
  ch->ring_kaddr->completion.mask = 119;

  /* Generate session UUID */
  uuid_new_v8(&ch->ring_kaddr->session_uuid);
  uuid_copy(&ch->session_uuid, &ch->ring_kaddr->session_uuid);

  /* Allocate IPC ring buffer control page */
  ipc_kva = xspanalloc(4096, BY2PG, 0);
  if (ipc_kva == nil) {
    xfree(ring_kva);
    free(ch);
    return nil;
  }
  ipc_pa = paddr(ipc_kva);

  /* Generate vault secret and mint capability */
  ledger_generate_secret(vault_secret);
  err = ledger_mint(&ch->ipc_cap, (uintptr)ipc_pa, 4096, owner,
                    CAP_PERM_READ | CAP_PERM_WRITE, vault_secret);
  if (err != BLIND_LEDGER_OK) {
    xfree(ipc_kva);
    xfree(ring_kva);
    free(ch);
    return nil;
  }

  ch->ipc_kaddr = (IpcChannel *)ipc_kva;
  memset(ch->ipc_kaddr, 0, 4096);
  ch->ipc_kaddr->magic = 0x52494E47;
  ch->ipc_kaddr->submission.mask = RING_MASK;
  ch->ipc_kaddr->completion.mask = RING_MASK;
  ch->ipc_last_seqno = 0;

  /* Allocate pool capability array */
  ch->pool_caps = smalloc(ch->pool_size * sizeof(UserCapability));
  if (ch->pool_caps == nil) {
    channel_release_pages(ch);
    free(ch);
    return nil;
  }
  ch->pool_head = 0;
  ch->pool_tail = 0;

  /* Find free channel slot */
  lock(&channels_lock);
  for (i = 0; i < MAX_EXCHANGE_CHANNELS; i++) {
    if (channels[i] == nil) {
      channels[i] = ch;
      ch->chan_id = i;

      /*
       * Register channel ring pages as attachable physical segments so
       * segattach("#X/N/{ring,ipcring}", ...) can map them.
       */
      memset(&ps, 0, sizeof(ps));
      ps.attr = SG_PHYSICAL | SG_CACHED;
      ps.size = BY2PG;

      snprint(segname, sizeof(segname), "#X/%d/ring", ch->chan_id);
      ps.name = segname;
      ps.pa = paddr(ch->ring_kaddr);
      if (addphysseg(&ps) == nil)
        print("exchange: failed to register physseg %s\n", segname);

      snprint(segname, sizeof(segname), "#X/%d/ipcring", ch->chan_id);
      ps.name = segname;
      ps.pa = paddr(ch->ipc_kaddr);
      if (addphysseg(&ps) == nil)
        print("exchange: failed to register physseg %s\n", segname);

      unlock(&channels_lock);
      return ch;
    }
  }
  unlock(&channels_lock);

  /* No free slots */
  free(ch->pool_caps);
  channel_release_pages(ch);
  free(ch);
  return nil;
}

/*@
  @ requires ch == \null || \valid(ch);
  @ assigns \nothing;
  @*/
static void channel_free(ExchangeChannel *ch) {
  if (ch == nil)
    return;

  lock(&channels_lock);
  if (ch->chan_id >= 0 && ch->chan_id < MAX_EXCHANGE_CHANNELS)
    channels[ch->chan_id] = nil;
  unlock(&channels_lock);

  channel_unregister_physsegs(ch);
  channel_release_pages(ch);
  if (ch->pool_caps)
    free(ch->pool_caps);
  free(ch);
}

static ExchangeChannel *channel_get(int chan_id) {
  ExchangeChannel *ch;

  if (chan_id < 0 || chan_id >= MAX_EXCHANGE_CHANNELS)
    return nil;

  lock(&channels_lock);
  ch = channels[chan_id];
  if (ch != nil)
    incref(&ch->ref);
  unlock(&channels_lock);

  return ch;
}

/*@
  @ requires ch == \null || \valid(ch);
  @ assigns \nothing;
  @*/
static void channel_put(ExchangeChannel *ch) {
  if (ch == nil)
    return;

  if (decref(&ch->ref) == 0)
    channel_free(ch);
}

static int pool_size_valid(uint size) {
  if (size < 16 || size > 1024)
    return 0;
  return (size & (size - 1)) == 0;
}

static uint pool_count_locked(ExchangeChannel *ch) {
  return (ch->pool_tail - ch->pool_head) & (ch->pool_size - 1);
}

static int pool_resize(ExchangeChannel *ch, uint new_size) {
  UserCapability *new_caps, *old_caps;
  uint old_size, count, i;

  if (ch == nil || !pool_size_valid(new_size))
    return -1;

  lock(&ch->pool_lock);
  old_size = ch->pool_size;
  if (new_size == old_size) {
    unlock(&ch->pool_lock);
    return 0;
  }
  unlock(&ch->pool_lock);

  new_caps = smalloc(new_size * sizeof(UserCapability));
  if (new_caps == nil)
    return -1;

  lock(&ch->pool_lock);
  old_size = ch->pool_size;
  if (new_size == old_size) {
    unlock(&ch->pool_lock);
    free(new_caps);
    return 0;
  }

  count = pool_count_locked(ch);
  if (count >= new_size) {
    unlock(&ch->pool_lock);
    free(new_caps);
    return -1;
  }

  for (i = 0; i < count; i++)
    new_caps[i] = ch->pool_caps[(ch->pool_head + i) & (old_size - 1)];

  old_caps = ch->pool_caps;
  ch->pool_caps = new_caps;
  ch->pool_size = new_size;
  ch->pool_head = 0;
  ch->pool_tail = count;
  unlock(&ch->pool_lock);

  free(old_caps);
  return 0;
}

static int parse_channel_endpoint(const char *endpoint, int *chan_id) {
  const char *p, *suffix;
  long id;
  char *endp;

  if (endpoint == nil || chan_id == nil)
    return -1;

  p = endpoint;
  while (*p == ' ' || *p == '\t')
    p++;

  if (p[0] != '#' || p[1] != 'X' || p[2] != '/')
    return -1;

  p += 3;
  id = strtol(p, &endp, 10);
  if (endp == p || id < 0 || id >= MAX_EXCHANGE_CHANNELS)
    return -1;

  if (*endp != 0) {
    if (*endp != '/')
      return -1;
    suffix = endp + 1;
    if (strcmp(suffix, "data") != 0 && strcmp(suffix, "ctl") != 0 &&
        strcmp(suffix, "ring") != 0 && strcmp(suffix, "ipcring") != 0 &&
        strcmp(suffix, "pool") != 0 && strcmp(suffix, "stats") != 0 &&
        strcmp(suffix, "peers") != 0)
      return -1;
  }

  *chan_id = (int)id;
  return 0;
}

static int read_endpoint_path(const char *path, char *buf, int buflen) {
  int fd, i;
  long nread;

  if (path == nil || buf == nil || buflen <= 1)
    return -1;

  fd = -1;
  if (waserror()) {
    if (fd >= 0)
      fdclose(fd, 0);
    return -1;
  }

  fd = kopen((char *)path, OREAD);
  nread = kread(fd, buf, buflen - 1);
  fdclose(fd, 0);
  fd = -1;
  poperror();

  if (nread <= 0)
    return -1;

  buf[nread] = 0;
  for (i = 0; buf[i] != 0; i++) {
    if (buf[i] == '\n' || buf[i] == '\r') {
      buf[i] = 0;
      break;
    }
  }

  return 0;
}

static ExchangeChannel *resolve_peer_channel(const char *target) {
  ExchangeChannel *peer;
  char endpoint[256];
  char srv_endpoint[256];
  int chan_id;
  int n;

  if (target == nil || target[0] == 0)
    return nil;

  if (parse_channel_endpoint(target, &chan_id) == 0)
    return channel_get(chan_id);

  if (read_endpoint_path(target, endpoint, sizeof(endpoint)) == 0 &&
      parse_channel_endpoint(endpoint, &chan_id) == 0)
    return channel_get(chan_id);

  if (strncmp(target, "/srv/", 5) == 0) {
    n = snprint(srv_endpoint, sizeof(srv_endpoint), "%s.endpoint", target);
    if (n > 0 && n < sizeof(srv_endpoint)) {
      if (read_endpoint_path(srv_endpoint, endpoint, sizeof(endpoint)) == 0 &&
          parse_channel_endpoint(endpoint, &chan_id) == 0)
        return channel_get(chan_id);
    }
  }

  peer = nil;
  return peer;
}

/* Pool Management */
/*@
  @ requires ch == \null || \valid(ch);
  @ requires out_cap == \null || \valid(out_cap);
  @ assigns \nothing;
  @*/
static int pool_alloc_page(ExchangeChannel *ch, UserCapability *out_cap) {
  void *kva;
  uintptr pa;
  BlindLedgerError err;
  u8int vault_secret[32];

  if (ch == nil || out_cap == nil)
    return -1;

  lock(&ch->pool_lock);

  /* Check if pool is full */
  if (((ch->pool_tail + 1) & (ch->pool_size - 1)) == ch->pool_head) {
    unlock(&ch->pool_lock);
    return -1; /* Pool full */
  }

  /* Allocate page directly (Phase 1 - simple allocation) */
  kva = xspanalloc(BY2PG, BY2PG, 0);
  if (kva == nil) {
    unlock(&ch->pool_lock);
    return -1;
  }
  pa = paddr(kva);

  /* Generate vault secret */
  ledger_generate_secret(vault_secret);

  /* Mint capability for the page */
  err = ledger_mint(out_cap, pa, BY2PG, ch->owner,
                    CAP_PERM_READ | CAP_PERM_WRITE, vault_secret);
  if (err != BLIND_LEDGER_OK) {
    unlock(&ch->pool_lock);
    xfree(kva);
    return -1;
  }

  /* Add to pool */
  ch->pool_caps[ch->pool_tail] = *out_cap;
  ch->pool_tail = (ch->pool_tail + 1) & (ch->pool_size - 1);
  ch->pool_allocs++;

  unlock(&ch->pool_lock);
  return 0;
}

/*@
  @ requires ch == \null || \valid(ch);
  @ requires out_cap == \null || \valid(out_cap);
  @ assigns \nothing;
  @*/
static int pool_get_page(ExchangeChannel *ch, UserCapability *out_cap) {
  if (ch == nil || out_cap == nil)
    return -1;

  lock(&ch->pool_lock);

  /* Check if pool is empty */
  if (ch->pool_head == ch->pool_tail) {
    unlock(&ch->pool_lock);
    return -1; /* Pool empty */
  }

  /* Get capability from pool */
  *out_cap = ch->pool_caps[ch->pool_head];
  ch->pool_head = (ch->pool_head + 1) & (ch->pool_size - 1);

  unlock(&ch->pool_lock);
  return 0;
}

/*@
  @ requires ch == \null || \valid(ch);
  @ requires cap == \null || \valid(cap);
  @ assigns \nothing;
  @*/
static int pool_return_page(ExchangeChannel *ch, const UserCapability *cap) {
  if (ch == nil || cap == nil)
    return -1;

  lock(&ch->pool_lock);

  /* Check if pool is full */
  if (((ch->pool_tail + 1) & (ch->pool_size - 1)) == ch->pool_head) {
    unlock(&ch->pool_lock);
    return -1; /* Pool full */
  }

  /* Return to pool */
  ch->pool_caps[ch->pool_tail] = *cap;
  ch->pool_tail = (ch->pool_tail + 1) & (ch->pool_size - 1);
  ch->pool_frees++;
  unlock(&ch->pool_lock);
  return 0;
}

static void scrub_exchange_page(uchar *msg_buf, P9Control *ctl, uint rep_size,
                                int ring_mode) {
  if (msg_buf == nil)
    return;

  if (ctl == nil) {
    /* Full scrub for fresh allocations or error states */
    memset(msg_buf, 0, P9_CONTROL_OFFSET);
    return;
  }

  if (ring_mode) {
    u32int i;
    u32int head = ctl->rep_head;
    u32int tail = ctl->rep_tail;

    for (i = 0; i < P9_RING_SLOTS; i++) {
      int active = 0;
      /* Check if slot i is in the active [head, tail) range (handling wrap) */
      if (head <= tail) {
        if (i >= head && i < tail)
          active = 1;
      } else {
        if (i >= head || i < tail)
          active = 1;
      }

      if (!active) {
        memset(msg_buf + (i * P9_RING_SLOT_SIZE), 0, P9_RING_SLOT_SIZE);
      } else {
        /* Slot is active, but we should still zero the tail of the reply */
        uchar *slot = msg_buf + (i * P9_RING_SLOT_SIZE);
        u32int slot_rep_size = GBIT32(slot + 4);
        if (slot_rep_size < P9_RING_DATA_SIZE) {
          memset(slot + P9_RING_HEADER_SIZE + slot_rep_size, 0,
                 P9_RING_DATA_SIZE - slot_rep_size);
        }
      }
    }
  } else {
    /* Large message mode: zero everything after the reply */
    if (rep_size < P9_CONTROL_OFFSET) {
      memset(msg_buf + rep_size, 0, P9_CONTROL_OFFSET - rep_size);
    }
  }
}

static int data_has_replies(void *a) {
  ExchangeChannel *ch = a;
  return ch->ring_kaddr->submission.head != ch->ring_kaddr->submission.tail;
}

static long service_write_request(ExchangeChannel *ch, void *vp, long n) {
  UserCapability cap;
  BatchHeader *batch;
  void *kva;
  u32int tail;

  if (ch->dead)
    error(Ehungup);

  if (n > P9_MSG_SIZE)
    error(Etoobig);

  /* 1. Get a page from the pool */
  if (pool_get_page(ch, &cap) < 0) {
    if (pool_alloc_page(ch, &cap) < 0)
      error("service: no pool pages");
  }

  /* 2. Lookup PA to get KVA */
  BlindLedgerEntry entry;
  if (ledger_verify(&cap, &entry) != BLIND_LEDGER_OK)
    error("service: pool page verification failed");

  kva = (void *)hhdm_virt(entry.physical_address);

  batch = (BatchHeader *)kva;
  memset(batch, 0, sizeof(BatchHeader));
  batch->magic = BATCH_PAGE_MAGIC;
  batch->num_messages = 1;
  batch->used_bytes = BATCH_DATA_START + n;

  /* INJECT PID2 */
  uuid_copy(&batch->uuids[0], &up->pid2);

  memmove((uchar *)kva + BATCH_DATA_START, vp, n);

  /* 3. Put in submission ring */
  lock(&ch->lock);
  tail = ch->ipc_kaddr->submission.tail;
  ch->ipc_kaddr->submission.pages[tail & RING_MASK] =
      (u64int)entry.physical_address;
  ch->ipc_kaddr->submission.tail++;
  unlock(&ch->lock);

  /* 4. Kick service (if owner is sleeping) */
  wakeup(&ch->data_wait);

  return n;
}

static int completion_has_data(void *a) {
  ExchangeChannel *ch = a;
  return ch->ipc_kaddr->completion.head != ch->ipc_kaddr->completion.tail;
}

static long service_read_reply(ExchangeChannel *ch, void *buf, long n) {
  u32int head;
  u64int page_handle;
  BatchHeader *batch;
  void *kva;
  long len;

  /* Hybrid Polling */
  int spins = 0;
  while (!completion_has_data(ch)) {
    if (ch->dead)
      error(Ehungup);

    if (spins < 100000) {
      spins++;
      /* Yield every now and then? For now busy spin */
      continue;
    }
    sleep(&ch->data_wait, completion_has_data, ch);
  }

  if (ch->dead)
    error(Ehungup);

  lock(&ch->lock);
  head = ch->ipc_kaddr->completion.head;
  page_handle = ch->ipc_kaddr->completion.pages[head & RING_MASK];

  kva = (void *)hhdm_virt(page_handle);
  batch = (BatchHeader *)kva;

  if (batch->magic != BATCH_PAGE_MAGIC || batch->num_messages == 0) {
    ch->ipc_kaddr->completion.head++;
    unlock(&ch->lock);
    error("service: bad reply batch");
  }

  /* Extract first message (simple model) */
  len = batch->used_bytes - BATCH_DATA_START;
  if (len > n)
    len = n;
  memmove(buf, (uchar *)kva + BATCH_DATA_START, len);

  ch->ipc_kaddr->completion.head++;
  unlock(&ch->lock);

  /* Return page to pool */
  UserCapability cap;
  BlindLedgerEntry entry;
  if (ledger_lookup_by_pa_and_owner(page_handle, ch->owner, &cap, &entry) ==
      BLIND_LEDGER_OK) {
    pool_return_page(ch, &cap);
  }

  return len;
}

/*@
  @ requires ch == \null || \valid(ch);
  @ assigns \nothing;
  @*/
static int exchange_ipc_process(ExchangeChannel *ch) {
  struct IpcPageRing *submission;
  struct IpcPageRing *completion;
  u32int head, tail;
  u64int page_handle;
  uintptr page_phys, user_vaddr;
  P9Control *ctl;
  uchar *msg_buf;
  u64int *pte;

  if (ch == nil || ch->ipc_kaddr == nil)
    return -1;
  if (ch->owner != up)
    return -1;

  submission = &ch->ipc_kaddr->submission;
  completion = &ch->ipc_kaddr->completion;
  head = submission->head;
  tail = submission->tail;

  while (head != tail) {
    page_handle = submission->pages[head & RING_MASK];
    user_vaddr = (uintptr)page_handle;

    if ((user_vaddr & (BY2PG - 1)) != 0) {
      print("exchange: ipc invalid page alignment: %#p\n", user_vaddr);
      goto skip_page;
    }

    pte = mmuwalk(m->pml4, user_vaddr, 0, 0);
    if (pte == nil || (*pte & PTEVALID) == 0) {
      print("exchange: ipc invalid page mapping: %#p\n", user_vaddr);
      goto skip_page;
    }

    page_phys = PADDR(*pte);
    if (!pageown_is_owned(page_phys)) {
      print("exchange: ipc page not owned: pa=%#p\n", page_phys);
      goto skip_page;
    }

    if (pageown_get_owner(page_phys) != ch->owner) {
      print("exchange: ipc page owned by different process: pa=%#p\n",
            page_phys);
      goto skip_page;
    }

    // Borrow page from process
    if (borrow_transfer(ch->owner, up, (uintptr)kaddr(page_phys)) != BORROW_OK) {
      print("exchange: ipc borrow failed: pa=%#p\n", page_phys);
      goto skip_page;
    }

    // Temporarily unmap from user to prevent TOCTOU
    u64int saved_pte = *pte;
    *pte = 0;
    putcr3(getcr3());

    // Access page via HHDM
    ctl = (P9Control *)((uintptr)hhdm_virt(page_phys) + P9_CONTROL_OFFSET);
    msg_buf = (uchar *)hhdm_virt(page_phys);

    u32int rep_size = 0;

    // Hybrid Detection: Check if it's a RING buffer
    if (ctl->req_tail != ctl->req_head) {
      // Memory barrier
      __asm__ volatile("mfence" ::: "memory");
      if (p9_handle_ring(up, ctl, msg_buf) < 0) {
        print("exchange: ipc ring processing failed: pa=%#p\n", page_phys);
        atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
      } else {
        atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
      }
    } else {
      // Single message mode (LARGE)
      Fcall t, r;
      u32int msg_size = GBIT32(msg_buf);
      if (msg_size >= 7 && msg_size <= P9_MSG_SIZE) {
        __asm__ volatile("mfence" ::: "memory");
        if (convM2S(msg_buf, msg_size, &t) != 0) {
          if (p9_dispatch(up, &t, &r) < 0)
            build_error_reply(&r, t.tag, "dispatch failed");

          rep_size = convS2M(&r, msg_buf, P9_MSG_SIZE);
          if (rep_size > 0) {
            atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
          } else {
            atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
          }
        }
      }
    }

    // Scrub for security: preserve replies
    int is_ring = (ctl->req_tail != ctl->req_head);
    scrub_exchange_page(msg_buf, ctl, rep_size, is_ring);

    // Restore page to process
    *pte = saved_pte;
    putcr3(getcr3());
    borrow_transfer(up, ch->owner, (uintptr)kaddr(page_phys));

  skip_page:
    u32int c_tail = completion->tail;
    completion->pages[c_tail & RING_MASK] = page_handle;
    completion->tail++;

    head++;
  }

  submission->head = head;
  return 0;
}

/*@
  @ assigns \nothing;
  @*/
static void exchinit(void) {
  int i;

  /* Initialize channel table */
  for (i = 0; i < MAX_EXCHANGE_CHANNELS; i++)
    channels[i] = nil;

  /* Initialize legacy exchange control structure */
  exchctl.nprepared = 0;

  print("exchange: 9P device initialized (pooled channels enabled)\n");
}

static Chan *exchattach(char *spec) {
  Chan *c;

  c = devattach(L'X', spec);
  mkqid(&c->qid, Qtopdir, 0, QTDIR);
  c->dev = 0;
  return c;
}

static int exchanchanqid(vlong path) {
  switch (SUBFILE(path)) {
  case Qchandir:
  case Qctl:
  case Qring:
  case Qipcring:
  case Qpool:
  case Qstats:
  case Qpeers:
  case Qdata:
    return 1;
  default:
    return 0;
  }
}

static int exchlookupstatic(Chan *c, char *name, Dirtab *tab, int ntab, Dir *dp,
                            int chan_id) {
  int i;
  Qid qid;

  for (i = 1; i < ntab; i++) {
    if (strcmp(name, tab[i].name) != 0)
      continue;
    qid = tab[i].qid;
    if (chan_id >= 0)
      qid.path = CHANQID(chan_id, SUBFILE(qid.path));
    devdir(c, qid, tab[i].name, tab[i].length, eve, tab[i].perm, dp);
    return 1;
  }
  return -1;
}

static int exchgen(Chan *c, char *name, Dirtab *tab, int ntab, int s, Dir *dp) {
  Qid qid;
  int chan_id;
  int i;
  char buf[32];
  ExchangeChannel *ch;

  USED(tab, ntab);

  if (s == DEVDOTDOT) {
    mkqid(&qid, Qtopdir, 0, QTDIR);
    devdir(c, qid, "#X", 0, eve, DMDIR | 0555, dp);
    return 1;
  }

  if (!exchanchanqid(c->qid.path)) {
    if (name != nil) {
      if (exchlookupstatic(c, name, exchdir, nelem(exchdir), dp, -1) > 0)
        return 1;

      chan_id = atoi(name);
      if (name[0] < '0' || name[0] > '9')
        return -1;
      ch = channel_get(chan_id);
      if (ch == nil)
        return -1;
      channel_put(ch);
      mkqid(&qid, CHANQID(chan_id, Qchandir), 0, QTDIR);
      devdir(c, qid, name, 0, eve, DMDIR | 0555, dp);
      return 1;
    }

    if (s < nelem(exchdir) - 1) {
      i = s + 1;
      devdir(c, exchdir[i].qid, exchdir[i].name, exchdir[i].length, eve,
             exchdir[i].perm, dp);
      return 1;
    }

    s -= nelem(exchdir) - 1;
    lock(&channels_lock);
    for (i = 0; i < MAX_EXCHANGE_CHANNELS; i++) {
      if (channels[i] == nil)
        continue;
      if (s-- > 0)
        continue;
      unlock(&channels_lock);
      snprint(buf, sizeof(buf), "%d", i);
      mkqid(&qid, CHANQID(i, Qchandir), 0, QTDIR);
      devdir(c, qid, buf, 0, eve, DMDIR | 0555, dp);
      return 1;
    }
    unlock(&channels_lock);
    return -1;
  }

  chan_id = CHANID(c->qid.path);
  ch = channel_get(chan_id);
  if (ch == nil)
    return -1;
  channel_put(ch);

  if (name != nil)
    return exchlookupstatic(c, name, chandir, nelem(chandir), dp, chan_id);

  if (s >= nelem(chandir) - 1)
    return -1;
  i = s + 1;
  qid = chandir[i].qid;
  qid.path = CHANQID(chan_id, SUBFILE(qid.path));
  devdir(c, qid, chandir[i].name, chandir[i].length, eve, chandir[i].perm, dp);
  return 1;
}

static Walkqid *exchwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, nil, 0, exchgen);
}

/*@
  @ requires c == \null || \valid(c);
  @ requires dp == \null || \valid(dp);
  @ assigns \nothing;
  @*/
static int exchstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, nil, 0, exchgen);
}

static Chan *exchopen(Chan *c, int omode) {
  int subfile;
  ExchangeChannel *ch;

  omode &= 3; /* mask off exec bit */
  subfile = SUBFILE(c->qid.path);

  switch (subfile) {
  case Qtopdir:
  case Qchandir:
    if (omode != OREAD)
      error(Eperm);
    break;

  case Qclone:
    /* Clone: allocate new channel */
    ch = channel_alloc(up);
    if (ch == nil)
      error(Enomem);
    /*
     * Keep one reference in the global channel table and one on the clone fd.
     * Closing #X/clone must not destroy the channel before #X/N/{ctl,data}.
     */
    incref(&ch->ref);
    /* Store channel in c->aux for read */
    c->aux = ch;
    break;

  case Qring:
  case Qipcring:
  case Qctl:
  case Qpool:
  case Qglobalctl:
  case Qstats:
  case Qpeers:
  case Qdata:
    /* Lookup channel and increment ref */
    /* Only for channel-specific files */
    if (subfile != Qglobalctl) {
      int chan_id = CHANID(c->qid.path);
      ch = channel_get(chan_id);
      if (ch == nil)
        error(Enochannel); // "channel does not exist"
      c->aux = ch;
    }

    /* Access checks */
    if (subfile == Qstats || subfile == Qpeers || subfile == Qstat) {
      if (omode != OREAD) {
        if (c->aux) {
          channel_put((ExchangeChannel *)c->aux);
          c->aux = nil;
        }
        error(Eperm);
      }
    }
    break;

  case Qexchange:
    /* Legacy: allow read/write */
    break;
  }

  /* Use appropriate directory table */
  c = devopen(c, omode, nil, 0, exchgen);

  return c;
}

/*@
  @ requires c == \null || \valid(c);
  @ requires name == \null || \valid(name);
  @ assigns \nothing;
  @*/
static Chan *exchcreate(Chan *c, char *name, int omode, ulong perm) {
  (void)c;
  (void)name;
  (void)omode;
  (void)perm;
  error(Eperm);
  return c;
}

/*@
  @ requires c == \null || \valid(c);
  @ assigns \nothing;
  @*/
static void exchclose(Chan *c) {
  ExchangeChannel *ch;

  /* If clone was opened, release the channel */
  /* Release channel reference if held in aux */
  if (c->aux != nil) {
    ch = c->aux;

    /* Detect service death: if owner is closing connection */
    if (ch->mode == CHAN_SERVICE && ch->owner == up) {
      /* Service is disconnecting/dying */
      lock(&ch->lock);
      if (!ch->dead) {
        ch->dead = 1;
        /* Wake up any kernel waiters (devmnt) */
        wakeup(&ch->data_wait);
      }
      unlock(&ch->lock);
    }

    channel_put(ch);
    c->aux = nil;
  }
}

/*@
  @ requires c == \null || \valid(c);
  @ requires buf == \null || \valid(buf);
  @ assigns \nothing;
  @*/
static long exchread(Chan *c, void *buf, long n, vlong off) {
  char *p, *e;
  int i, chan_id;
  ExchangeChannel *ch;
  UserCapability cap;

  switch (SUBFILE(c->qid.path)) {
  case Qtopdir:
    return devdirread(c, buf, n, nil, 0, exchgen);

  case Qclone:
    /* Return channel ID allocated during open */
    ch = c->aux;
    if (ch == nil)
      error("no channel allocated");
    p = smalloc(32);
    if (p == nil)
      error(Enomem);
    snprint(p, 32, "%d", ch->chan_id);
    n = readstr(off, buf, n, p);
    free(p);
    return n;

  case Qpool:
    /* Allocate page from pool and return UserCapability */
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil)
      error("invalid channel");

    if (n < sizeof(UserCapability)) {
      channel_put(ch);
      error("buffer too small");
    }

    if (pool_get_page(ch, &cap) < 0) {
      /* Pool empty, allocate new page */
      if (pool_alloc_page(ch, &cap) < 0) {
        channel_put(ch);
        error("pool allocation failed");
      }
    }

    memmove(buf, &cap, sizeof(UserCapability));
    channel_put(ch);
    return sizeof(UserCapability);

  case Qctl:
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil)
      error("invalid channel");

    p = smalloc(1024);
    if (p == nil) {
      channel_put(ch);
      error(Enomem);
    }

    lock(&ch->lock);
    seprint(p, p + 1024, "mode %s\n",
            ch->mode == CHAN_SERVICE
                ? "service"
                : (ch->mode == CHAN_PEER ? "peer" : "kernel"));
    seprint(p + strlen(p), p + 1024, "poolsize %ud\n", ch->pool_size);
    if (ch->peer != nil)
      seprint(p + strlen(p), p + 1024, "peer %d\n", ch->peer->chan_id);
    unlock(&ch->lock);

    n = readstr(off, buf, n, p);
    free(p);
    channel_put(ch);
    return n;

  case Qstats:
    /* Return channel statistics */
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil)
      error("invalid channel");

    p = smalloc(2048);
    if (p == nil) {
      channel_put(ch);
      error(Enomem);
    }
    e = p + 2048;

    lock(&ch->lock);
    seprint(p, e, "Channel %d Statistics\n", ch->chan_id);
    seprint(p + strlen(p), e, "Owner PID: %d\n",
            ch->owner ? ch->owner->pid : -1);
    seprint(p + strlen(p), e, "Mode: %s\n",
            ch->mode == CHAN_KERNEL ? "kernel" : "peer");
    seprint(p + strlen(p), e, "Pool size: %ud\n", ch->pool_size);
    seprint(p + strlen(p), e, "Pool used: %ud\n",
            (ch->pool_tail - ch->pool_head) & (ch->pool_size - 1));
    seprint(p + strlen(p), e, "Messages sent: %llud\n", ch->messages_sent);
    seprint(p + strlen(p), e, "Messages recv: %llud\n", ch->messages_recv);
    seprint(p + strlen(p), e, "Bytes sent: %llud\n", ch->bytes_sent);
    seprint(p + strlen(p), e, "Bytes recv: %llud\n", ch->bytes_recv);
    seprint(p + strlen(p), e, "Pool allocs: %llud\n", ch->pool_allocs);
    seprint(p + strlen(p), e, "Pool frees: %llud\n", ch->pool_frees);
    unlock(&ch->lock);

    n = readstr(off, buf, n, p);
    free(p);
    channel_put(ch);
    return n;

  case Qdata:
    ch = c->aux;
    if (ch == nil) {
      chan_id = CHANID(c->qid.path);
      ch = channel_get(chan_id);
    }
    if (ch == nil)
      error("invalid channel");
    if (ch->mode != CHAN_SERVICE) {
      if (ch != c->aux)
        channel_put(ch);
      error("not a service channel");
    }
    /* Read from completion ring (replies from service) */
    if (ch != c->aux && waserror()) {
      channel_put(ch);
      nexterror();
    }
    n = service_read_reply(ch, buf, n);
    if (ch != c->aux) {
      poperror();
      channel_put(ch);
    }
    return n;

  case Qring:
    /* Return ring buffer capability for mapping */
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil)
      error("invalid channel");

    if (n < sizeof(UserCapability)) {
      channel_put(ch);
      error("buffer too small");
    }
    memmove(buf, &ch->ring_cap, sizeof(UserCapability));
    channel_put(ch);
    return sizeof(UserCapability);
  case Qipcring:
    /* Return IPC ring capability for mapping */
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil)
      error("invalid channel");

    if (n < sizeof(UserCapability)) {
      channel_put(ch);
      error("buffer too small");
    }
    memmove(buf, &ch->ipc_cap, sizeof(UserCapability));
    channel_put(ch);
    return sizeof(UserCapability);

  case Qpeers:
    /* Return peer list (if any) */
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil)
      error("invalid channel");

    p = smalloc(1024);
    if (p == nil) {
      channel_put(ch);
      error(Enomem);
    }

    lock(&ch->lock);
    if (ch->peer != nil)
      snprint(p, 1024, "peer: channel %d\n", ch->peer->chan_id);
    else
      snprint(p, 1024, "no peers\n");
    unlock(&ch->lock);

    n = readstr(off, buf, n, p);
    free(p);
    channel_put(ch);
    return n;

  case Qexchange:
    /* Return information about prepared exchanges (legacy) */
    p = smalloc(4096);
    if (p == nil)
      error(Enomem);
    e = p + 4096;
    seprint(p, e, "Page Exchange System\n");
    seprint(p + strlen(p), e, "Prepared pages: %d\n", exchctl.nprepared);
    seprint(p + strlen(p), e, "Index  Owner PID   Original VAddr\n");
    seprint(p + strlen(p), e, "-----  ----------  ---------------\n");
    qlock(&exchctl.qlock);
    for (i = 0; i < exchctl.nprepared; i++) {
      seprint(p + strlen(p), e, "%-5d  %-10d  0x%016llux\n", i,
              exchctl.prepared[i].owner ? exchctl.prepared[i].owner->pid : -1,
              (uvlong)exchctl.prepared[i].original_vaddr);
    }
    qunlock(&exchctl.qlock);
    n = readstr(off, buf, n, p);
    free(p);
    return n;

  case Qstat:
    /* Return statistics (legacy) */
    p = smalloc(1024);
    if (p == nil)
      error(Enomem);
    pageown_stats();
    seprint(p, p + 1024, "Exchange device statistics\n");
    seprint(p + strlen(p), p + 1024, "Total prepared: %d\n", exchctl.nprepared);
    n = readstr(off, buf, n, p);
    free(p);
    return n;

  default:
    /* Channel directory listing */
    if (ISCHANDIR(c->qid.path)) {
      chan_id = CHANID(c->qid.path);
      ch = channel_get(chan_id);
      if (ch == nil)
        error("invalid channel");
      n = devdirread(c, buf, n, nil, 0, exchgen);
      channel_put(ch);
      return n;
    }
  }

  return 0;
}

/*@
  @ requires c == \null || \valid(c);
  @ requires vp == \null || \valid(vp);
  @ assigns \nothing;
  @*/
static long exchwrite(Chan *c, void *vp, long n, vlong off) {
  char *buf;
  char *fields[4];
  int nf, chan_id;
  uintptr vaddr;
  ExchangeHandle handle;
  int result;
  ExchangeChannel *ch;
  UserCapability cap;

  (void)off;

  buf = smalloc(n + 1);
  if (buf == nil)
    error(Enomem);
  memmove(buf, vp, n);
  buf[n] = 0;

  switch (SUBFILE(c->qid.path)) {
  case Qctl:
    /* Handle channel control commands */
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil) {
      free(buf);
      error("invalid channel");
    }

    nf = tokenize(buf, fields, nelem(fields));
    if (nf < 1) {
      channel_put(ch);
      free(buf);
      error("usage: command [args]");
    }

    if (strcmp(fields[0], "mode") == 0) {
      if (nf < 2) {
        channel_put(ch);
        free(buf);
        error("usage: mode kernel|peer|service");
      }
      lock(&ch->lock);
      if (strcmp(fields[1], "service") == 0)
        ch->mode = CHAN_SERVICE;
      else if (strcmp(fields[1], "peer") == 0)
        ch->mode = CHAN_PEER;
      else if (strcmp(fields[1], "kernel") == 0)
        ch->mode = CHAN_KERNEL;
      else {
        unlock(&ch->lock);
        channel_put(ch);
        free(buf);
        error("mode must be kernel|peer|service");
      }
      unlock(&ch->lock);
    } else if (strcmp(fields[0], "attach") == 0) {
      ExchangeChannel *peer, *old_peer;

      if (nf < 2) {
        channel_put(ch);
        free(buf);
        error("usage: attach kernel|/srv/name");
      }

      peer = nil;
      if (strcmp(fields[1], "kernel") != 0) {
        peer = resolve_peer_channel(fields[1]);
        if (peer == nil) {
          channel_put(ch);
          free(buf);
          error("attach: invalid peer endpoint");
        }
        if (peer == ch) {
          channel_put(peer);
          channel_put(ch);
          free(buf);
          error("attach: cannot attach channel to itself");
        }
      }

      lock(&ch->lock);
      old_peer = ch->peer;
      if (strcmp(fields[1], "kernel") == 0) {
        ch->mode = CHAN_KERNEL;
        ch->peer = nil;
      } else {
        ch->mode = CHAN_PEER;
        ch->peer = peer;
      }
      unlock(&ch->lock);
      if (old_peer != nil)
        channel_put(old_peer);
    } else if (strcmp(fields[0], "poolsize") == 0) {
      if (nf < 2) {
        channel_put(ch);
        free(buf);
        error("usage: poolsize N");
      }
      uint new_size = strtoul(fields[1], nil, 0);
      if (!pool_size_valid(new_size)) {
        channel_put(ch);
        free(buf);
        error("poolsize must be a power of two between 16 and 1024");
      }
      if (pool_resize(ch, new_size) < 0) {
        channel_put(ch);
        free(buf);
        error("poolsize resize failed");
      }
    } else if (strcmp(fields[0], "mapring") == 0) {
      /* Map ring buffer into process address space
       * For Phase 2, we store the channel in the process
       * and userspace will use segattach() to map it.
       * This command just prepares the channel for mapping.
       */
      lock(&ch->lock);
      /* Mark channel as ready for mapping */
      ch->owner = up;
      unlock(&ch->lock);

      /* Store channel pointer in process for later segattach */
      /* In full implementation, would use up->exch_channel or similar */
      /* For now, userspace can use segattach(SG_PHYSICAL, "#X/N/ring", ...) */
    } else if (strcmp(fields[0], "kickipc") == 0) {
      if (exchange_ipc_process(ch) < 0) {
        channel_put(ch);
        free(buf);
        error("ipc ring processing failed");
      }
    } else if (strcmp(fields[0], "kickreply") == 0) {
      /* Wake up readers waiting for reply (e.g. devmnt) */
      wakeup(&ch->data_wait);
    } else {
      channel_put(ch);
      free(buf);
      error("unknown control command");
    }

    channel_put(ch);
    free(buf);
    return n;

  case Qpool:
    /* Return page to pool */
    if (n < sizeof(UserCapability)) {
      free(buf);
      error("invalid capability size");
    }

    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil) {
      free(buf);
      error("invalid channel");
    }

    memmove(&cap, vp, sizeof(UserCapability));
    if (pool_return_page(ch, &cap) < 0) {
      channel_put(ch);
      free(buf);
      error("pool full");
    }

    channel_put(ch);
    free(buf);
    return n;

  case Qexchange:
    /* Parse command: prepare <vaddr> or accept <handle> <dest_vaddr> <prot> or
     * cancel <handle> */
    if (strncmp(buf, "prepare ", 8) == 0) {
      /* prepare <vaddr> */
      vaddr = strtoul(buf + 8, nil, 0);
      if (vaddr == 0 || (vaddr & (BY2PG - 1)) != 0) {
        free(buf);
        error("invalid virtual address");
      }

      /* Prepare the page for exchange */
      if (exchange_prepare(vaddr, &handle) != BLIND_LEDGER_OK) {
        free(buf);
        error("exchange_prepare failed");
      }

      /* Track the prepared page */
      qlock(&exchctl.qlock);
      if (exchctl.nprepared < nelem(exchctl.prepared)) {
        exchctl.prepared[exchctl.nprepared].handle = handle;
        exchctl.prepared[exchctl.nprepared].original_vaddr = vaddr;
        exchctl.prepared[exchctl.nprepared].owner = up;
        exchctl.prepared[exchctl.nprepared].time = m->ticks;
        exchctl.nprepared++;
      }
      qunlock(&exchctl.qlock);

      free(buf);
      return n;
    } else if (strncmp(buf, "accept ", 7) == 0) {
      /* accept <handle> <dest_vaddr> <prot> */
      /* Parse "accept <cap_index> <dest_vaddr> <prot>" */
      char *p = buf + 7;
      int cap_idx = strtol(p, &p, 0);
      while (*p == ' ')
        p++;
      uintptr dest_vaddr = strtoul(p, &p, 0);
      while (*p == ' ')
        p++;
      int prot = strtol(p, nil, 0);

      qlock(&exchctl.qlock);
      if (cap_idx < 0 || cap_idx >= exchctl.nprepared ||
          (dest_vaddr & (BY2PG - 1)) != 0) {
        qunlock(&exchctl.qlock);
        free(buf);
        error("invalid parameters");
      }

      handle = exchctl.prepared[cap_idx].handle;
      result = exchange_accept(&handle, dest_vaddr, prot);
      if (result != EXCHANGE_OK) {
        qunlock(&exchctl.qlock);
        free(buf);
        error("exchange_accept failed");
      }

      /* Remove from prepared tracking */
      /*@ loop invariant 0 <= j <= exchctl.nprepared - 1;
  @ loop assigns j;
  @ loop variant exchctl.nprepared - 1 - j;
  @*/
      for (int j = cap_idx; j < exchctl.nprepared - 1; j++) {
        exchctl.prepared[j] = exchctl.prepared[j + 1];
      }
      exchctl.nprepared--;
      qunlock(&exchctl.qlock);

      free(buf);
      return n;
    } else if (strncmp(buf, "cancel ", 7) == 0) {
      /* cancel <cap_index> */
      int cap_idx = strtol(buf + 7, nil, 0);

      qlock(&exchctl.qlock);
      if (cap_idx < 0 || cap_idx >= exchctl.nprepared) {
        qunlock(&exchctl.qlock);
        free(buf);
        error("invalid capability index");
      }

      handle = exchctl.prepared[cap_idx].handle;
      result = exchange_cancel(&handle);
      if (result != EXCHANGE_OK) {
        qunlock(&exchctl.qlock);
        free(buf);
        error("exchange_cancel failed");
      }

      /* Remove from prepared tracking */
      /*@ loop invariant 0 <= j <= exchctl.nprepared - 1;
  @ loop assigns j;
  @ loop variant exchctl.nprepared - 1 - j;
  @*/
      for (int j = cap_idx; j < exchctl.nprepared - 1; j++) {
        exchctl.prepared[j] = exchctl.prepared[j + 1];
      }
      exchctl.nprepared--;
      qunlock(&exchctl.qlock);

      free(buf);
      return n;
    } else {
      free(buf);
      error("unknown command");
    }
    break;

  case Qdata:
    chan_id = CHANID(c->qid.path);
    ch = channel_get(chan_id);
    if (ch == nil) {
      free(buf);
      error("invalid channel");
    }
    if (ch->mode != CHAN_SERVICE) {
      channel_put(ch);
      free(buf);
      error("not a service channel");
    }
    /* Write to submission ring (requests to service) */
    result = service_write_request(ch, vp, n);
    channel_put(ch);
    free(buf);
    return result;

  default:
    free(buf);
    error(Eperm);
  }

  free(buf);
  return 0;
}

/* Phase 3 will implement capability-based mapping
 * For now, Phase 2 exposes capabilities via read(#X/N/ring)
 * and userspace will use those capabilities to map memory
 */

/*@
  @ requires c == \null || \valid(c);
  @ assigns \nothing;
  @*/
static void exchremove(Chan *c) {
  (void)c;
  error(Eperm);
}

/*@
  @ requires c == \null || \valid(c);
  @ requires dp == \null || \valid(dp);
  @ assigns \nothing;
  @*/
static int exchwstat(Chan *c, uchar *dp, int n) {
  (void)c;
  (void)dp;
  (void)n;
  error(Eperm);
  return 0;
}

static void exchreset(void);

Dev exchdevtab = {
    .dc = L'X',
    .name = "exchange",
    .reset = exchreset,
    .init = exchinit,
    .shutdown = nil,
    .attach = exchattach,
    .walk = exchwalk,
    .stat = exchstat,
    .open = exchopen,
    .create = exchcreate,
    .close = exchclose,
    .read = exchread,
    .write = exchwrite,
    .remove = exchremove,
    .wstat = exchwstat,
    .power = nil,
    .config = nil,
};

/*@
  @ assigns \nothing;
  @*/
static void exchreset(void) {
  /* Nothing to prime yet; hook exists to satisfy chandevreset(). */
}

/* Kernel boot integration - setup exchange infrastructure for init process
 * Called from proc0() during kernel boot to proactively set up 9P exchange.
 * This eliminates the chicken-egg problem of init needing syscalls to open #X.
 *
 * Returns: ExchangeChannel on success, nil on failure
 */
void *kernel_setup_init_exchange(Proc *p) {
  ExchangeChannel *ch;
  UserCapability cap;
  uintptr pool_pa;

  if (p == nil)
    return nil;

  print("BOOT[kernel_setup_init_exchange]: allocating exchange channel for PID "
        "%ld\n",
        p->pid);

  /* Allocate exchange channel using same infrastructure as #X/clone */
  ch = channel_alloc(p);
  if (ch == nil) {
    print("BOOT[kernel_setup_init_exchange]: channel_alloc failed\n");
    return nil;
  }

  print("BOOT[kernel_setup_init_exchange]: channel %d allocated, ring at %#p\n",
        ch->chan_id, ch->ring_kaddr);

  /* Allocate initial pool pages (2 pages for request/reply) */
  if (pool_alloc_page(ch, &cap) < 0) {
    channel_put(ch);
    print("BOOT[kernel_setup_init_exchange]: pool_alloc_page failed\n");
    return nil;
  }

  /* Get physical addresses from capabilities for mapping */
  /* The capability contains the kernel physical address in the hash
   * For now, we'll allocate dedicated pages and map them directly */

  /* Allocate 2 exchange pages (request + reply) */
  void *exch_pages = mallocalign(BY2PG * 2, BY2PG, 0, 0);
  if (exch_pages == nil) {
    channel_put(ch);
    print("BOOT[kernel_setup_init_exchange]: failed to allocate exchange "
          "pages\n");
    return nil;
  }
  memset(exch_pages, 0, BY2PG * 2);

  /* Store exchange pages in p->p9page for doorbell handler */
  p->p9page = exch_pages;

  /* Get physical addresses for borrowchecker tracking */
  uintptr req_pa = PADDR(exch_pages);
  uintptr rep_pa = PADDR(exch_pages) + BY2PG;

  /*
   * CRITICAL: Userspace claims pages FIRST (per user requirement)
   * "During init, once the page is setup, USERSPACE (init) claims the page
   * FIRST. This is logical: the kernel creates the way to communicate with it,
   *  the init (an application running on top of the kernel) utilizes the method
   * provided."
   *
   * Acquire ownership for init process via borrowchecker.
   * This ensures exclusive access - only userspace can read/write until syscall
   * doorbell.
   */
  enum BorrowError berr = borrow_acquire(p, (uintptr)kaddr(req_pa));
  if (berr != BORROW_OK) {
    print("BOOT[kernel_setup_init_exchange]: borrow_acquire(req_page) failed: "
          "%d\n",
          berr);
    channel_put(ch);
    return nil;
  }

  berr = borrow_acquire(p, (uintptr)kaddr(rep_pa));
  if (berr != BORROW_OK) {
    print("BOOT[kernel_setup_init_exchange]: borrow_acquire(rep_page) failed: "
          "%d\n",
          berr);
    borrow_release(p, (uintptr)kaddr(req_pa)); /* Rollback first page */
    channel_put(ch);
    return nil;
  }

  /* Map exchange pages to userspace at per-process VA
   * Page 1: Request buffer (base)
   * Page 2: Reply buffer (base + BY2PG)
   *
   * Pages are now owned exclusively by userspace (init process).
   * Kernel CANNOT access until ownership is transferred via syscall doorbell.
   */
  /* Create a physical segment for the exchange pages (2 pages) */
  /* We MUST create a segment so that rfork/dupseg treats it as SG_PHYSICAL
   * (shared/phys) instead of SG_DATA (COW), which would cause the parent to
   * lose connection to the kernel-mapped page after fork. */
  if (p->p9uaddr == 0)
    p->p9uaddr = p9_pick_uaddr(p, nil);
  if (p->p9uaddr == 0)
    p->p9uaddr = EXCHANGE_PAGE_ADDR;
  uintptr ubase = p9_user_base(p);
  Segment *s = newseg(SG_PHYSICAL, ubase, 2);
  if (s == nil) {
    print("BOOT[kernel_setup_init_exchange]: newseg failed\n");
    borrow_release(p, (uintptr)kaddr(req_pa));
    borrow_release(p, (uintptr)kaddr(rep_pa));
    channel_put(ch);
    return nil;
  }

  /* Allocate Physseg tracker */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("BOOT[kernel_setup_init_exchange]: malloc failed for pseg\n");
    putseg(s);
    borrow_release(p, (uintptr)kaddr(req_pa));
    borrow_release(p, (uintptr)kaddr(rep_pa));
    channel_put(ch);
    return nil;
  }

  /* Configure physical segment backing */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "9pexchange_init";
  s->pseg->pa = req_pa;
  s->pseg->size = BY2PG * 2;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Assign segment to process at P9SEG slot */
  p->seg[P9SEG] = s;

  /* Map using userpmap to ensure PTEs are present immediately */
  userpmap(ubase, req_pa, PTEVALID | PTEUSER | PTEWRITE);
  userpmap(ubase + BY2PG, rep_pa, PTEVALID | PTEUSER | PTEWRITE);

  print("BOOT[kernel_setup_init_exchange]: userspace claimed exchange pages at "
        "%#p (PA req=%#p rep=%#p)\n",
        ubase, req_pa, rep_pa);

  /* Map ring buffer control page to userspace (for future use)
   * The ring buffer provides batched message submission/completion */
  /* ring_pa = PADDR(ch->ring_kaddr); */
  /* Note: Ring mapping will be done when init calls segattach() on #X/N/ring */

  print("BOOT[kernel_setup_init_exchange]: exchange channel ready (id=%d)\n",
        ch->chan_id);

  return ch;
}
