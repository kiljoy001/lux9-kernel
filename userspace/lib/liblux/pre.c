# 0 "src/syscalls.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "src/syscalls.c"
# 1 "src/lux_internal.h" 1



# 1 "src/../inc/lux.h" 1




# 1 "../../../kernel/include/fcall.h" 1




#pragma src "/sys/src/libc/9sys"
#pragma lib "libc.a"






typedef struct Fcall {
  uchar type;
  u32int fid;
  ushort tag;
  union {
    struct {
      u32int msize;
      char *version;
    };
    struct {
      ushort oldtag;
    };
    struct {
      char *ename;
    };
    struct {
      Qid qid;
      u32int iounit;
    };
    struct {
      Qid aqid;
    };
    struct {
      u32int afid;
      char *uname;
      char *aname;
    };
    struct {
      u32int perm;
      char *name;
      uchar mode;
    };
    struct {
      u32int newfid;
      ushort nwname;
      char *wname[16];
    };
    struct {
      ushort nwqid;
      Qid wqid[16];
    };
    struct {
      vlong offset;
      u32int count;
      char *data;
    };
    struct {
      ushort nstat;
      uchar *stat;
    };
    struct {
      u32int scallnr;
      u32int sflags;
      uchar *sdata;
      u32int scount;
      u64int retval;
    };

    struct {
      u32int flags;
      u32int pid;
    };
    struct {
      char **argv;
      u32int argc;
    };
    struct {
      u64int addr;
    };
    struct {
      char *oldpath;
      u32int fd;
    };
    struct {
      u32int fid0;
      u32int fid1;
    };
    struct {
      int whence;
    };
    struct {
      u64int handler;
    };
  };
} Fcall;
# 155 "../../../kernel/include/fcall.h"
enum {
  Tversion = 100,
  Rversion,
  Tauth = 102,
  Rauth,
  Tattach = 104,
  Rattach,
  Terror = 106,
  Rerror,
  Tflush = 108,
  Rflush,
  Twalk = 110,
  Rwalk,
  Topen = 112,
  Ropen,
  Tcreate = 114,
  Rcreate,
  Tread = 116,
  Rread,
  Twrite = 118,
  Rwrite,
  Tclunk = 120,
  Rclunk,
  Tremove = 122,
  Rremove,
  Tstat = 124,
  Rstat,
  Twstat = 126,
  Rwstat,
  Tmax,


  Texec = 128,
  Rexec,




  Tsyscall = 130,
  Rsyscall,



  Tsysopen = 132,
  Rsysopen,
  Tsyscreate = 134,
  Rsyscreate,
  Tsysread = 136,
  Rsysread,
  Tsyswrite = 138,
  Rsyswrite,
  Tsysclose = 140,
  Rsysclose,
  Tsyspread = 142,
  Rsyspread,
  Tsyspwrite = 144,
  Rsyspwrite,
  Tsysremove = 146,
  Rsysremove,


  Tsysstat = 148,
  Rsysstat,
  Tsysfstat = 150,
  Rsysfstat,
  Tsyswstat = 152,
  Rsyswstat,
  Tsysfwstat = 154,
  Rsysfwstat,


  Tsysfork = 160,
  Rsysfork,
  Tsysexec = 162,
  Rsysexec,
  Tsysexit = 164,
  Rsysexit,
  Tsyswait = 166,
  Rsyswait,
  Tsysbrk = 168,
  Rsysbrk,
  Tsyssleep = 170,
  Rsyssleep,


  Tsysbind = 180,
  Rsysbind,
  Tsysmount = 182,
  Rsysmount,
  Tsysunmount = 184,
  Rsysunmount,
  Tsyschdir = 186,
  Rsyschdir,


  Tsysdup = 190,
  Rsysdup,
  Tsyspipe = 192,
  Rsyspipe,
  Tsysfd2path = 194,
  Rsysfd2path,


  Tsysseek = 200,
  Rsysseek,
  Tsysnotify = 202,
  Rsysnotify,
  Tsysalarm = 204,
  Rsysalarm,

  Tsysmax,
};

uint convM2S(uchar *, uint, Fcall *);
uint convS2M(Fcall *, uchar *, uint);
uint sizeS2M(Fcall *);

int statcheck(uchar *abuf, uint nbuf);
uint convM2D(uchar *, uint, Dir *, char *);
uint convD2M(Dir *, uchar *, uint);
uint sizeD2M(Dir *);

int fcallfmt(Fmt *);
int dirfmt(Fmt *);
int dirmodefmt(Fmt *);

int read9pmsg(int, void *, uint);


#pragma varargck type "F" Fcall *
#pragma varargck type "M" ulong
#pragma varargck type "D" Dir *



enum {
  SYS_OPEN = 1,
  SYS_CLOSE,
  SYS_READ,
  SYS_WRITE,
  SYS_PREAD,
  SYS_PWRITE,
  SYS_CREATE,
  SYS_EXIT,
  SYS_FORK,
  SYS_STAT,
  SYS_WSTAT,
  SYS_RFORK = 19,
  SYS_PIPE = 21,
  SYS_SEEK = 39,
  SYS_MOUNT = 46,
  SYS_NSEC = 53,
  SYS_BRK = 55,
  SYS_WAIT = 166,
  SYS_GETPID2 = 66
};
# 6 "src/../inc/lux.h" 2
# 1 "./inc/libc.h" 1



# 1 "../../../kernel/include/u.h" 1
# 16 "../../../kernel/include/u.h"
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;

typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;


typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;

typedef u32int Rune;
# 51 "../../../kernel/include/u.h"
_Static_assert(sizeof(ulong) == sizeof(void *), "ulong must match pointer size");
_Static_assert(sizeof(uintptr) == sizeof(void *),
              "uintptr must match pointer size");
_Static_assert(sizeof(usize) == sizeof(void *), "usize must match pointer size");
_Static_assert(sizeof(ssize) == sizeof(void *), "ssize must match pointer size");
# 5 "./inc/libc.h" 2



typedef struct Qid Qid;
typedef struct Dir Dir;

struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};

struct Dir {
  ushort type;
  uint dev;
  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
};

typedef struct Fmt Fmt;
struct Fmt {
  unsigned char runes;
  void *start;
  void *to;
  void *stop;
  int (*flush)(Fmt *);
  void *farg;
  int nfmt;
  void *args;
  int r;
  int width;
  int prec;
  unsigned long flags;
};


void *memmove(void *dst, const void *src, ulong n);
void *memset(void *dst, int c, ulong n);
int snprint(char *buf, int n, char *fmt, ...);
int atoi(const char *s);
unsigned long long strtoull(const char *s, char **endptr, int base);
ulong strlen(const char *s);
# 7 "src/../inc/lux.h" 2
# 1 "../../../kernel/include/pebble.h" 1
       

# 1 "../../../kernel/include/types_fwd.h" 1
# 14 "../../../kernel/include/types_fwd.h"
typedef struct Proc Proc;





typedef struct Mach Mach;





typedef struct Lock Lock;





typedef struct Chan Chan;





typedef struct Fcall Fcall;





typedef struct Ureg Ureg;





typedef struct Rendez Rendez;
# 4 "../../../kernel/include/pebble.h" 2
# 30 "../../../kernel/include/pebble.h"
enum PebbleColor {
  PEBBLE_COLOR_COLORLESS = 0,
  PEBBLE_COLOR_WHITE = 1,
  PEBBLE_COLOR_BLACK = 2,
  PEBBLE_COLOR_RED = 3,
  PEBBLE_COLOR_BLUE = 4,
};
# 50 "../../../kernel/include/pebble.h"
extern int pebble_enabled;
extern int pebble_debug;







extern Lock pebble_bank_lock;
extern ulong pebble_global_colorless_bank;
extern ulong pebble_total_system_tokens;
# 110 "../../../kernel/include/pebble.h"
# 1 "../../../kernel/include/blind_ledger.h" 1
# 24 "../../../kernel/include/blind_ledger.h"
# 1 "../../../kernel/include/../include/rbtree.h" 1
# 26 "../../../kernel/include/../include/rbtree.h"
typedef unsigned long uintptr;
# 36 "../../../kernel/include/../include/rbtree.h"
struct rb_node {
  uintptr __rb_parent_color;
  struct rb_node *rb_right;
  struct rb_node *rb_left;
};




struct rb_root {
  struct rb_node *rb_node;
};
# 74 "../../../kernel/include/../include/rbtree.h"
void rb_insert_color(struct rb_node *node, struct rb_root *root);
void rb_erase(struct rb_node *node, struct rb_root *root);


static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link) {
  node->__rb_parent_color = (uintptr)parent;
  node->rb_left = node->rb_right = ((void *)0);

  *rb_link = node;
}


static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p,
                                       int color) {
  rb->__rb_parent_color = (uintptr)p | (uintptr)color;
}

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p) {
  rb->__rb_parent_color = (((rb)->__rb_parent_color) & 1) | (uintptr)p;
}

static inline void rb_set_black(struct rb_node *rb) {
  rb->__rb_parent_color |= 1;
}

static inline void rb_set_red(struct rb_node *rb) {
  rb->__rb_parent_color &= ~1UL;
}


struct rb_node *rb_first(const struct rb_root *root);
struct rb_node *rb_last(const struct rb_root *root);
struct rb_node *rb_next(const struct rb_node *node);
struct rb_node *rb_prev(const struct rb_node *node);


void rb_replace_node(struct rb_node *victim, struct rb_node *new_node,
                     struct rb_root *root);


typedef void (*rb_augment_f)(struct rb_node *node, void *data);

void rb_insert_augmented(struct rb_node *node, struct rb_root *root,
                         rb_augment_f augment_rotate, void *data);
void rb_erase_augmented(struct rb_node *node, struct rb_root *root,
                        rb_augment_f augment_rotate, void *data);
# 25 "../../../kernel/include/blind_ledger.h" 2

# 1 "../../../kernel/include/u.h" 1
# 27 "../../../kernel/include/blind_ledger.h" 2
# 1 "../../../kernel/include/uuid.h" 1



typedef struct {
  unsigned char data[16];
} uuid_t;


void uuid_clear(uuid_t *u);
int uuid_compare(const uuid_t *a, const uuid_t *b);
void uuid_copy(uuid_t *dst, const uuid_t *src);
int uuid_parse(const char *in, uuid_t *uu);
void uuid_unparse(const uuid_t *uu, char *out);
int uuid_is_null(const uuid_t *uu);


void uuid_new_v8(uuid_t *u);

void uuid_pack_v8(uuid_t *u, unsigned long long data_a, unsigned short data_b,
                  unsigned long long data_c);


typedef struct {
  unsigned int token;
  unsigned int generation;
  unsigned short index;
} pebble_uuid_data_t;

void uuid_pack_pebble(uuid_t *u, unsigned int token, unsigned int generation,
                      unsigned short index);
int uuid_unpack_pebble(const uuid_t *u, unsigned int *token,
                       unsigned int *generation, unsigned short *index);
# 45 "../../../kernel/include/uuid.h"
void uuid_pack_capability(uuid_t *u, const unsigned char *pa_hash,
                          unsigned short epoch, unsigned char type,
                          unsigned char perms);

int uuid_unpack_capability(const uuid_t *u, unsigned short *epoch,
                           unsigned char *type, unsigned char *perms);


void uuid_get_pa_hash_bits(const uuid_t *u, unsigned char *pa_hash_out);
# 28 "../../../kernel/include/blind_ledger.h" 2
# 45 "../../../kernel/include/blind_ledger.h"
typedef struct UserCapability {
  uuid_t uuid;
  u8int
      hash[32];
  u64int size;
  u32int type;
  u32int perms;
} UserCapability;


enum {
  CAP_TYPE_MEMORY = 1,
  CAP_TYPE_CHANNEL = 2,
  CAP_TYPE_DEVICE = 3,
  CAP_TYPE_IPC = 4,
  CAP_TYPE_SPAWN = 5,
};


enum {
  CAP_PERM_READ = 1 << 0,
  CAP_PERM_WRITE = 1 << 1,
  CAP_PERM_EXEC = 1 << 2,
  CAP_PERM_TRANSFER = 1 << 3,
  CAP_PERM_GRANT = 1 << 4,
};


typedef enum BlindLedgerState {
  BLIND_LEDGER_STATE_INACTIVE = 0,
  BLIND_LEDGER_STATE_ACTIVE = 1,
  BLIND_LEDGER_STATE_BURNED = 2,
  BLIND_LEDGER_STATE_COW_RED = 3,
  BLIND_LEDGER_STATE_COW_BLUE = 4,
} BlindLedgerState;


typedef u8int BlindLedgerHash[32];







typedef struct BlindLedgerEntry {
  UserCapability capability;
  uintptr physical_address;
  Proc *owner;
  u8int secret[32];

  u64int epoch;
  u64int span_len;
  u32int permissions;
  BlindLedgerState state;
  BlindLedgerHash leaf_hash;
  BlindLedgerHash process_hash;



  BlindLedgerHash parent_hash;
  BlindLedgerHash derivation_sig;
} BlindLedgerEntry;


typedef enum BlindLedgerError {
  BLIND_LEDGER_OK = 0,
  BLIND_LEDGER_EINVAL = 1,
  BLIND_LEDGER_ENOMEM = 2,
  BLIND_LEDGER_EPERM = 3,
  BLIND_LEDGER_ENOTFOUND = 4,
  BLIND_LEDGER_EEXPIRED = 5,
  BLIND_LEDGER_EFAULT = 6,
  BLIND_LEDGER_EBUSY = 7,
} BlindLedgerError;


void blind_ledger_init(void);
BlindLedgerError ledger_mint(UserCapability *out_cap, uintptr pa, ulong len,
                             Proc *owner, u32int permissions,
                             const u8int *vault_secret);
BlindLedgerError ledger_verify(const UserCapability *cap,
                               BlindLedgerEntry *out_entry);
BlindLedgerError ledger_verify_by_uuid(const uuid_t *uuid,
                                       BlindLedgerEntry *out_entry);
BlindLedgerError ledger_transfer(const UserCapability *cap, Proc *from_owner,
                                 Proc *to_owner);
BlindLedgerError ledger_burn(const UserCapability *cap, Proc *owner);
BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
void blind_ledger_update_merkle_root(void);
const u8int *blind_ledger_get_merkle_root(void);


u64int ledger_get_current_epoch(void);
void ledger_advance_epoch(void);


BlindLedgerError ledger_generate_secret(u8int *secret_out);
BlindLedgerError ledger_destroy_secret(const u8int *secret);


typedef struct LedgerRollbackToken {
  UserCapability
      original_capability;
  Proc *original_owner;
  BlindLedgerHash
      original_process_hash;
  u64int is_valid;
} LedgerRollbackToken;



BlindLedgerError
ledger_transfer_reversible(const UserCapability *cap, Proc *from_owner,
                           Proc *to_owner, LedgerRollbackToken *rollback_token);

typedef struct BlindLedgerStats {
  u64int active_entries;
  u64int burned_entries;
  u64int total_memory_tracked;
  u64int tree_depth;
  u64int epoch;
} BlindLedgerStats;

BlindLedgerError blind_ledger_get_stats(BlindLedgerStats *stats);


BlindLedgerError blind_ledger_attest_root(u8int *out_signature,
                                          u32int *out_len);
# 186 "../../../kernel/include/blind_ledger.h"
typedef struct DerivationStep {
  BlindLedgerHash parent_hash;
  BlindLedgerHash derivation_sig;
  u32int constraints;
} DerivationStep;







typedef struct DerivationProof {
  BlindLedgerHash target_hash;
  u32int chain_length;
  DerivationStep chain[16];
} DerivationProof;







BlindLedgerError ledger_derive(const UserCapability *parent_cap, Proc *owner,
                               u32int child_constraints,
                               UserCapability *out_child_cap);






BlindLedgerError ledger_get_derivation_proof(const UserCapability *cap,
                                             Proc *owner,
                                             DerivationProof *out_proof);







BlindLedgerError ledger_verify_derivation_proof(const DerivationProof *proof);





BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
# 111 "../../../kernel/include/pebble.h" 2
# 1 "../../../kernel/include/borrowchecker.h" 1





       


# 1 "../../../kernel/include/lock.h" 1






struct Lock {
  ulong key;
  ulong sr;
  uintptr pc;
  Proc *p;
  Mach *m;
  ushort isilock;
  long lockcycles;
} __attribute__((aligned(64)));
# 10 "../../../kernel/include/borrowchecker.h" 2




enum BorrowState {
  BORROW_FREE = 0,
  BORROW_EXCLUSIVE,
  BORROW_SHARED_OWNED,
  BORROW_MUT_LENT,
};


struct IdentKey {
  u64int gen;
  u64int nonce;
};


enum BorrowSystemOwner {
  OWNER_BOOTLOADER = 0,
  OWNER_KERNEL,
  OWNER_TRAMPOLINE,
};


enum AllocSource {
  ALLOC_BOOTSTRAP,
  ALLOC_XALLOC,
};


struct MemoryRange {
  uintptr start;
  uintptr end;
  enum BorrowSystemOwner owner;
  struct MemoryRange *next;
};


struct BorrowOwner {

  uintptr key;
  struct IdentKey key_cap;


  Proc *owner;
  enum BorrowState state;
  enum BorrowSystemOwner system_owner;
  int is_system_owned;


  int shared_count;
  Proc *mut_borrower;
  struct SharedBorrower *shared_list;


  uvlong acquired_ns;
  uvlong borrow_deadline_ns;


  ulong borrow_count;


  enum AllocSource alloc_source;
  struct BorrowOwner *next;
};


struct SharedBorrower {
  Proc *proc;
  enum AllocSource alloc_source;
  struct SharedBorrower *next;
};


struct BorrowBucket {
  struct BorrowOwner *head;
};


struct BorrowPool {
  Lock lock;
  struct BorrowBucket *owners;
  ulong nbuckets;
  ulong nowners;
  ulong nshared;
  ulong nmut;
  u8int *bloom;
  ulong bloom_bits;
  u32int bloom_hashes;
};


enum MemoryCoordinationState {
  MEMORY_BOOTLOADER = 0,
  MEMORY_COORDINATED,
  MEMORY_KERNEL_ACTIVE,
};


struct MemoryCoordination {
  enum MemoryCoordinationState state;
  enum BorrowSystemOwner current_owner;
  int coordination_enabled;
};


enum BorrowError {
  BORROW_OK = 0,
  BORROW_EALREADY,
  BORROW_ENOTOWNER,
  BORROW_EBORROWED,
  BORROW_EMUTBORROW,
  BORROW_ESHAREDBORROW,
  BORROW_ENOTBORROWED,
  BORROW_EINVAL,
  BORROW_ENOMEM,
  BORROW_ENOTFOUND,
};


extern struct BorrowPool borrowpool;


void borrowinit(void);
enum BorrowError borrow_acquire(Proc *p, uintptr key);
enum BorrowError borrow_release(Proc *p, uintptr key);
enum BorrowError borrow_transfer(Proc *from, Proc *to, uintptr key);


enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_return_shared(Proc *borrower, uintptr key);
enum BorrowError borrow_return_mut(Proc *borrower, uintptr key);


enum BorrowError borrow_acquire_system(uintptr key,
                                       enum BorrowSystemOwner owner);
enum BorrowError borrow_release_system(uintptr key,
                                       enum BorrowSystemOwner owner);
enum BorrowError borrow_transfer_system(enum BorrowSystemOwner from,
                                        enum BorrowSystemOwner to, uintptr key);
enum BorrowSystemOwner borrow_get_system_owner(uintptr key);
int borrow_is_owned_by_system(uintptr key, enum BorrowSystemOwner owner);


enum BorrowError borrow_acquire_range_phys(uintptr start_pa, usize size,
                                           enum BorrowSystemOwner owner);
int borrow_range_owned_by_system(uintptr start_pa, usize size,
                                 enum BorrowSystemOwner owner);
int borrow_can_access_range_phys(uintptr start_pa, usize size,
                                 enum BorrowSystemOwner requester);


int borrow_is_owned(uintptr key);
Proc *borrow_get_owner(uintptr key);
int borrow_get_owner_snapshot(uintptr key, struct BorrowOwner *out);
enum BorrowState borrow_get_state(uintptr key);
int borrow_can_borrow_shared(uintptr key);
int borrow_can_borrow_mut(uintptr key);


void borrow_cleanup_process(Proc *p);


void memory_range_init(void);
void memory_range_add(uintptr start, uintptr end, enum BorrowSystemOwner owner);
void memory_range_add_discovered(uintptr start, uintptr end,
                                 enum BorrowSystemOwner owner);
void memory_range_remove(uintptr start, uintptr end);
void memory_range_dump(void);
int memory_range_capacity(void);
enum BorrowSystemOwner memory_range_get_owner(uintptr addr);
int memory_range_check_access(uintptr addr, enum BorrowSystemOwner requester);


void boot_memory_coordination_init(void);
void transfer_bootloader_to_kernel(void);
void establish_memory_ownership_zones(void);
void establish_memory_ownership_zones_dynamic(void);
int validate_memory_coordination_ready(void);
int memory_system_ready_before_cr3(void);
int post_cr3_memory_system_operational(void);


void borrow_stats(void);
void borrow_dump_resource(uintptr key);


ulong borrow_hash(uintptr key);
# 112 "../../../kernel/include/pebble.h" 2



typedef struct PebbleWhite {
  u32int token;
  u32int generation;
  void *data_ptr;
  ulong size;
} PebbleWhite;


typedef struct PebbleBlue {
  void *blue_data;
  ulong blue_size;
  ulong flags;
  struct PebbleBlue *next;
} PebbleBlue;


typedef struct PebbleRed {
  void *red_data;
  ulong red_size;
  ulong flags;
  struct PebbleRed *next;
} PebbleRed;

typedef struct PebbleBlack {
  UserCapability capability;
  void *
      physical_addr;
  ulong size;
  ulong flags;
  struct PebbleBlack *next;
} PebbleBlack;


typedef struct PebbleState {
  ulong colorless_bank;
  ulong black_inuse;
  ulong blue_inuse;
  ulong red_inuse;
  ulong white_verified;
  ulong white_pending;
  ulong red_count;
  ulong blue_count;
  ulong total_allocs;
  ulong total_frees;


  PebbleBlack *black_list;
  PebbleBlue *blue_list;
  PebbleRed *red_list;


  int in_syscall;
  ulong drop_budget;


  PebbleWhite whites[4096];
  uchar whites_active[4096];
  ulong white_generation;
  int white_head;
} PebbleState;
# 188 "../../../kernel/include/pebble.h"
typedef struct arena_branch {
  Lock lock;
  ulong local_colorless;
  ulong borrowed_from_proc;
  ulong max_tokens;
  ulong low_water;
  ulong high_water;
  ulong total_allocated;
  ulong total_freed;
  PebbleState *owner_ps;
} arena_branch_t;


void arena_branch_init(arena_branch_t *branch, PebbleState *ps,
                       ulong initial_budget);
int arena_branch_alloc(arena_branch_t *branch,
                       ulong size);
void arena_branch_free(arena_branch_t *branch,
                       ulong size);
int arena_branch_refill(arena_branch_t *branch);
void arena_branch_drain(arena_branch_t *branch);


extern Lock pebble_global_lock;


int pebble_black_alloc(PebbleWhite *white, void *buf, ulong size,
                       UserCapability *out_cap);
void *pebble_get_black_addr(const UserCapability *cap);
int pebble_black_free(const UserCapability *cap);
int pebble_white_verify(PebbleWhite *white_cap, void **black_cap);
int pebble_create_token_uuid(PebbleWhite *white, uuid_t *out_uuid);
int pebble_alloc_with_white(ulong size, UserCapability *out_cap,
                            void **out_addr);


PebbleBlue *pebble_blue_alloc(ulong size);
int pebble_blue_free(PebbleBlue *blue);
PebbleRed *pebble_red_alloc(ulong size);
int pebble_red_free(PebbleRed *red);
int pebble_red_snapshot(PebbleBlue *blue,
                        PebbleRed **out_red);


int pebble_red_copy(PebbleBlue *blue_obj,
                    PebbleRed **red_copy);
int pebble_blue_discard(PebbleBlue *blue_obj);


PebbleState *pebble_state(void);
int pebble_set_budget(ulong budget);
ulong pebble_get_budget(void);
int pebble_increase_budget(ulong size, u64int nonce);
void pebble_auto_verify(Proc *p, Ureg *ureg);

void pebble_red_blue_exit(void);
int pebble_valid_white_token(PebbleState *ps, PebbleWhite *white);
PebbleWhite *pebble_issue_white(PebbleState *ps, void *data, ulong size);
void pebble_return_white(PebbleState *ps, PebbleWhite *white);
PebbleBlack *pebble_lookup_black(PebbleState *ps, void *handle);
int pebble_blue_exists(PebbleState *ps, PebbleBlue *blue);
int pebble_has_matching_red(PebbleState *ps, PebbleBlue *blue);
PebbleRed *pebble_duplicate_blue(PebbleState *ps, PebbleBlue *blue);
void pebble_mark_red(PebbleState *ps, PebbleBlue *blue, PebbleRed *red);
void pebble_ensure_red_snapshots(PebbleState *ps);

void pebble_cleanup(struct Proc *p);
void pebble_selftest(void);
void pebble_sip_issue_test(void);
# 266 "../../../kernel/include/pebble.h"
void pebbleinit(void);
void pebbleprocinit(Proc *p);
void *pebble_meta_alloc(ulong size);
void pebble_meta_free(void *v);
# 289 "../../../kernel/include/pebble.h"
int pow_calculate_difficulty(int op_class, ulong magnitude);
int pow_verify(u64int nonce, u64int context, int required_diff);
void pow_gate_init(void);
# 8 "src/../inc/lux.h" 2



int sys_open(char *path, int mode);
int sys_close(int fd);
long sys_read(int fd, void *buf, long n);
long sys_write(int fd, void *buf, long n);
long sys_pwrite(int fd, void *buf, long n, long offset);
long sys_pread(int fd, void *buf, long n, long offset);
void sys_exit(char *msg);
int sys_create(char *path, int mode, uint perm);
int sys_rfork(int flags);
void sys_exec(char *path);
int sys_pipe(int *fds);
long sys_seek(int fd, long offset, int whence);
int sys_wait(void);
uvlong sys_nsec(void);
int sys_stat(char *path, uchar *buf, int nbuf);
int sys_wstat(char *path, uchar *buf, int nbuf);
int sys_mount(int fd, int afd, char *old, int flags, char *aname);
int sys_sleep(long ms);







int msgord_submit(char *path, Fcall *t);
int pow_solve(u64int context, int difficulty, u64int *nonce_out);


int pebble_alloc(ulong size, void **addr);
int pebble_free(void *addr);
# 5 "src/lux_internal.h" 2
# 13 "src/lux_internal.h"
void _syscall(void);
# 2 "src/syscalls.c" 2

extern uint convS2M(Fcall *f, uchar *ap, uint n);
extern uint convM2S(uchar *ap, uint n, Fcall *f);
extern void *memmove(void *dst, const void *src, ulong n);
extern void *memset(void *dst, int c, ulong n);

extern int pebble_alloc(ulong size, void **addr);
extern int pebble_free(void *addr);


static void pack8(uchar *p, int v) { p[0] = v; }
static void pack16(uchar *p, int v) {
  p[0] = v;
  p[1] = v >> 8;
}
static void pack32(uchar *p, int v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
}
static void pack64(uchar *p, uvlong v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
  p[4] = v >> 32;
  p[5] = v >> 40;
  p[6] = v >> 48;
  p[7] = v >> 56;
}
static int packstr(uchar *p, char *s) {
  int n = 0;
  while (s[n])
    n++;
  pack16(p, n);
  memmove(p + 2, s, n);
  return 2 + n;
}

int lux_call(Fcall *tx, Fcall *rx) {
  uchar *page = (uchar *)0x7FFFFEEFF000ULL;


  int n = convS2M(tx, page + 0x000, 0xF00);
  if (n <= 0)
    return -1;


  _syscall();



  if ((u64int)rx > 0x7FFFFFFFFFFF) {
    return -2;
  }

  memset(rx, 0, sizeof(Fcall));


  uint ret = convM2S(page + 0x000, 0xF00, rx);

  if ((int)ret <= 0)
    return (int)ret;

  if (rx->type == Rerror)
    return -1;
  return 0;
}


static int do_syscall(int scallnr, uchar *sdata, int scount, u64int *retval) {
  Fcall tx, rx;


  memset(&tx, 0, sizeof(Fcall));


  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = scallnr;
  tx.sflags = 0;
  tx.sdata = sdata;
  tx.scount = scount;

  int err = lux_call(&tx, &rx);
  if (err < 0)
    return err;

  if (retval)
    *retval = rx.retval;
  return 0;
}

int sys_open(char *path, int mode) {
  uchar buf[1024];
  uchar *p = buf;


  p += packstr(p, path);
  pack8(p, mode);
  p += 1;

  u64int ret;
  if (do_syscall(SYS_OPEN, buf, p - buf, &ret) < 0)
    return -1;
  return (int)ret;
}

int sys_close(int fd) {
  uchar buf[16];
  uchar *p = buf;


  pack32(p, fd);
  p += 4;

  return do_syscall(SYS_CLOSE, buf, p - buf, ((void *)0));
}

long sys_read(int fd, void *buf, long n) {
  uchar sbuf[32];
  uchar *p = sbuf;
  Fcall tx, rx;


  pack32(p, fd);
  p += 4;
  pack64(p, 0);
  p += 8;
  pack32(p, n);
  p += 4;

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_READ;
  tx.sdata = sbuf;
  tx.scount = p - sbuf;

  if (lux_call(&tx, &rx) < 0)
    return -1;


  if (rx.count > n)
    rx.count = n;
  if (rx.count > 0 && rx.sdata) {
    memmove(buf, rx.sdata, rx.count);
  }
  return rx.count;
}

long sys_write(int fd, void *buf, long n) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = ((void *)0);
  uchar *data_start;

  int hdr_len = 4 + 8 + 4;
  if (hdr_len + n <= sizeof(sbuf)) {
    p = sbuf;
  } else {
    if (pebble_alloc(hdr_len + n, (void **)&allocbuf) < 0)
      return -1;
    p = allocbuf;
  }

  data_start = p;

  pack32(p, fd);
  p += 4;
  pack64(p, 0);
  p += 8;
  pack32(p, n);
  p += 4;
  memmove(p, buf, n);
  p += n;

  u64int ret;
  int res = do_syscall(SYS_WRITE, data_start, p - data_start, &ret);

  if (allocbuf)
    pebble_free(allocbuf);

  if (res < 0)
    return -1;
  return (long)ret;
}

long sys_pwrite(int fd, void *buf, long n, long offset) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = ((void *)0);
  uchar *data_start;

  int hdr_len = 4 + 8 + 4;
  if (hdr_len + n <= sizeof(sbuf)) {
    p = sbuf;
  } else {
    if (pebble_alloc(hdr_len + n, (void **)&allocbuf) < 0)
      return -1;
    p = allocbuf;
  }

  data_start = p;

  pack32(p, fd);
  p += 4;
  pack64(p, offset);
  p += 8;
  pack32(p, n);
  p += 4;
  memmove(p, buf, n);
  p += n;

  u64int ret;
  int res = do_syscall(SYS_PWRITE, data_start, p - data_start, &ret);

  if (allocbuf)
    pebble_free(allocbuf);

  if (res < 0)
    return -1;
  return (long)ret;
}

void sys_exit(char *msg) {
  uchar buf[256];
  uchar *p = buf;


  p += packstr(p, msg ? msg : "");

  do_syscall(SYS_EXIT, buf, p - buf, ((void *)0));
  while (1)
    ;
}

int sys_create(char *path, int mode, uint perm) {
  uchar buf[1024];
  uchar *p = buf;


  p += packstr(p, path);
  pack32(p, mode);
  p += 4;
  pack32(p, perm);
  p += 4;

  u64int ret;
  if (do_syscall(SYS_CREATE, buf, p - buf, &ret) < 0)
    return -1;
  return (int)ret;
}

int sys_rfork(int flags) {
  uchar buf[16];
  uchar *p = buf;



  pack32(p, flags);
  p += 4;

  u64int ret;
  int err = do_syscall(SYS_RFORK, buf, p - buf, &ret);
  if (err < 0)
    return (int)((*(u32int *)0x7FFFFEEFF000ULL));
  return (int)ret;
}

void sys_exec(char *path) {

  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Texec;
  tx.tag = 1;
  tx.count = 0;
  tx.name = path;

  lux_call(&tx, &rx);

}

int sys_pipe(int *fds) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));

  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_PIPE;
  tx.sflags = 0;
  tx.sdata = 0;
  tx.scount = 0;

  if (lux_call(&tx, &rx) < 0)
    return -1;


  if (rx.scount >= 8 && rx.sdata) {
    uchar *p = rx.sdata;
    fds[0] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    p += 4;
    fds[1] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
  } else {
    return -1;
  }
  return 0;
}

long sys_seek(int fd, long offset, int whence) {
  uchar buf[32];
  uchar *p = buf;


  pack32(p, fd);
  p += 4;
  pack64(p, offset);
  p += 8;
  pack32(p, whence);
  p += 4;

  u64int ret;
  if (do_syscall(SYS_SEEK, buf, p - buf, &ret) < 0)
    return -1;
  return (long)ret;
}

int sys_wait(void) {
  u64int ret;
  if (do_syscall(SYS_WAIT, 0, 0, &ret) < 0)
    return -1;
  return (int)ret;
}

long sys_pread(int fd, void *buf, long n, long offset) {
  uchar sbuf[32];
  uchar *p = sbuf;
  Fcall tx, rx;


  pack32(p, fd);
  p += 4;
  pack64(p, offset);
  p += 8;
  pack32(p, n);
  p += 4;

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_PREAD;
  tx.sdata = sbuf;
  tx.scount = p - sbuf;

  if (lux_call(&tx, &rx) < 0)
    return -1;

  if (rx.count > n)
    rx.count = n;
  if (rx.count > 0 && rx.sdata) {
    memmove(buf, rx.sdata, rx.count);
  }
  return rx.count;
}

uvlong sys_nsec(void) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_NSEC;
  tx.scount = 0;
  tx.sdata = 0;

  if (lux_call(&tx, &rx) < 0)
    return 0;
  return rx.retval;
}

int sys_stat(char *path, uchar *buf, int nbuf) {
  uchar sbuf[1024];
  uchar *p = sbuf;


  p += packstr(p, path);

  u64int ret;
  if (do_syscall(SYS_STAT, sbuf, p - sbuf, &ret) < 0)
    return -1;

  return 0;
}

int sys_wstat(char *path, uchar *buf, int nbuf) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = ((void *)0);
  uchar *data_start;

  int pathlen = 0;
  while (path[pathlen])
    pathlen++;

  int hdr_len = 2 + pathlen + 2;
  if (hdr_len + nbuf <= sizeof(sbuf)) {
    p = sbuf;
  } else {
    if (pebble_alloc(hdr_len + nbuf, (void **)&allocbuf) < 0)
      return -1;
    p = allocbuf;
  }

  data_start = p;

  p += packstr(p, path);
  pack16(p, nbuf);
  p += 2;
  memmove(p, buf, nbuf);
  p += nbuf;

  u64int ret;
  int res = do_syscall(SYS_WSTAT, data_start, p - data_start, &ret);

  if (allocbuf)
    pebble_free(allocbuf);

  if (res < 0)
    return -1;
  return 0;
}

int sys_mount(int fd, int afd, char *old, int flags, char *aname) {
  uchar buf[1024];
  uchar *p = buf;


  pack32(p, fd);
  p += 4;
  pack32(p, afd);
  p += 4;
  p += packstr(p, old);
  pack32(p, flags);
  p += 4;
  p += packstr(p, aname);

  u64int ret;
  if (do_syscall(SYS_MOUNT, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_sleep(long ms) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyssleep;
  tx.tag = 1;
  tx.count = (u32int)ms;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return 0;
}
