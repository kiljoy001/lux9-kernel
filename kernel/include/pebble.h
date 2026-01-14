#pragma once

#include "types_fwd.h"

/*
 * Pebble Primitives - Core Kernel Feature
 *
 * Three capability-style resource types:
 * - Black-only: kernel-managed, non-clonable resources
 * - Black-White: user white token validated to black handle
 * - Red-Blue: copy-on-write shadow (red = safe, blue = speculative)
 */

/* Compile-time configuration */
#define PEBBLE_DEFAULT_BUDGET 0 /* per-process starts at 0 (caller-managed) */
#define PEBBLE_BOOT_BUDGET (256 * 1024 * 1024) /* 256 MiB for boot kernel */
#define PEBBLE_INIT_BUDGET                                                     \
  (64 * 1024 * 1024) /* 64 MiB for init/proc0 bootstrap */
#define PEBBLE_MAX_TOKENS 4096
#define PEBBLE_DEBUG 1 /* Enable debug output for arena testing */

/* Token economics: 1 token = 8 bytes of memory authorization */
#define PEBBLE_BYTES_PER_TOKEN 8

/*
 * Pebble Token Color States (5 mutually exclusive)
 * Used for page-level tracking in fork/COW mechanism.
 * Maps to borrowchecker states: BLACK=EXCLUSIVE, RED=SHARED_OWNED
 */
enum PebbleColor {
  PEBBLE_COLOR_COLORLESS = 0, /* Free pool, not in use */
  PEBBLE_COLOR_WHITE = 1,     /* Unverified/uninitialized (future) */
  PEBBLE_COLOR_BLACK = 2,     /* Exclusive access (one writer) */
  PEBBLE_COLOR_RED = 3,       /* Shared/read-only (multiple readers) */
  PEBBLE_COLOR_BLUE = 4,      /* I/O buffer (future) */
};

/* Syscall costs (in tokens) - Security enforcement */
#define PEBBLE_PROC_COST                                                       \
  (1024 * 1024 / PEBBLE_BYTES_PER_TOKEN) /* 1MB for fork */
#define PEBBLE_PIPE_COST                                                       \
  (64 * 1024 / PEBBLE_BYTES_PER_TOKEN) /* 64KB for pipe */
#define PEBBLE_MOUNT_COST                                                      \
  (4 * 1024 / PEBBLE_BYTES_PER_TOKEN)                      /* 4KB for mount */
#define PEBBLE_FD_COST (1 * 1024 / PEBBLE_BYTES_PER_TOKEN) /* 1KB per FD */

/*
 * Pebble runtime toggles.
 */
extern int pebble_enabled;
extern int pebble_debug;

/*
 * Global colorless bank - single pool for entire system.
 * Total tokens = system RAM / PEBBLE_BYTES_PER_TOKEN.
 * Processes pull tokens from this pool via PoW.
 * PoW difficulty increases as pool shrinks (scarcity mechanism).
 */
extern Lock pebble_bank_lock;
extern ulong pebble_global_colorless_bank; /* tokens available globally */
extern ulong pebble_total_system_tokens;   /* RAM/8, constant after init */

/* Error handling */
#define PEBBLE_E_PERM "permission denied"
#define PEBBLE_E_AGAIN "resource temporarily unavailable"
#define PEBBLE_E_NOMEM "out of memory"
#define PEBBLE_E_BADARG "bad argument"
#define PEBBLE_E_BUSY "resource busy"

/* Capability flags - Universal CBS model */
#define PEBBLE_CAP_BLACK (1 << 0)
#define PEBBLE_CAP_ACTIVE (1 << 1)
#define PEBBLE_CAP_DEVICE (1 << 2)
#define PEBBLE_CAP_IOPORT (1 << 3)
#define PEBBLE_CAP_NET (1 << 4)
#define PEBBLE_CAP_IRQ (1 << 5)
#define PEBBLE_CAP_DMA (1 << 6)
#define PEBBLE_CAP_PCI (1 << 7)
#define PEBBLE_CAP_FS (1 << 8)
#define PEBBLE_CAP_ADMIN                                                       \
  (1 << 9) /* Administrative: can modify other processes' capabilities */

/* Helper macro for capability checking */
/* Helper macro for capability checking */
#define has_capability(p, cap) ((p)->capabilities & (cap))

/*
 * 3-Bit Tagged Capabilities (Pointer-Carried Authority)
 * Leveraging the 8-byte alignment gap (Peg).
 */
#define PEBBLE_WAVE_MASK 0x7ULL
#define PEBBLE_PTR_ADDR(p) ((void *)((uintptr)(p) & ~PEBBLE_WAVE_MASK))
#define PEBBLE_PTR_WAVE(p) ((int)((uintptr)(p) & PEBBLE_WAVE_MASK))

/* The 8 Holographic Wavelengths (Channels) */
#define PEBBLE_WAVE_0 0 /* The "White" Channel (Root/Admin) */
#define PEBBLE_WAVE_1 1 /* Channel 1 */
#define PEBBLE_WAVE_2 2 /* Channel 2 */
#define PEBBLE_WAVE_3 3 /* Channel 3 */
#define PEBBLE_WAVE_4 4 /* Channel 4 */
#define PEBBLE_WAVE_5 5 /* Channel 5 */
#define PEBBLE_WAVE_6 6 /* Channel 6 */
#define PEBBLE_WAVE_7 7 /* Channel 7 */

/* Holographic Projection */
#define PEBBLE_PROJECT(p, wave)                                                \
  ((void *)((uintptr)PEBBLE_PTR_ADDR(p) | ((wave) & PEBBLE_WAVE_MASK)))
#define PEBBLE_TUNED(p, wave) (PEBBLE_PTR_WAVE(p) == (wave))

#include "blind_ledger.h"
#include "borrowchecker.h"
#include "uuid.h"

/* White token structure - opaque to user */
typedef struct PebbleWhite {
  u32int token;
  u32int generation;
  void *data_ptr;
  ulong size;
} PebbleWhite;

/* Blue object structure - independent colored token for block I/O */
typedef struct PebbleBlue {
  void *blue_data;         /* Physical memory (separate allocation) */
  ulong blue_size;         /* Size of allocation */
  ulong flags;             /* State flags */
  struct PebbleBlue *next; /* List linkage */
} PebbleBlue;

/* Red copy structure - independent colored token for snapshots */
typedef struct PebbleRed {
  void *red_data;         /* Physical memory (separate allocation) */
  ulong red_size;         /* Size of allocation */
  ulong flags;            /* State flags */
  struct PebbleRed *next; /* List linkage */
} PebbleRed;

typedef struct PebbleBlack {
  UserCapability capability; // The UserCapability provided by Blind Ledger
  void *
      physical_addr; // The actual physical memory address managed by this token
  uintptr user_vaddr; // The user-space virtual address mapping (if any)
  ulong size;         // Size of the allocation
  ulong flags;
  struct PebbleBlack *next;
} PebbleBlack;

/* Per-process Pebble state */
typedef struct PebbleState {
  ulong colorless_bank; /* remaining bytes for this process (COLORLESS pool) */
  ulong black_inuse;    /* bytes in BLACK state */
  ulong blue_inuse;     /* bytes in BLUE state */
  ulong red_inuse;      /* bytes in RED state */
  ulong white_verified; /* count of active white→black conversions */
  ulong white_pending;  /* bytes authorized by white tokens (WHITE state) */
  ulong red_count;      /* number of live red tokens */
  ulong blue_count;     /* number of live blue tokens */
  ulong total_allocs;   /* total allocations made */
  ulong total_frees;    /* total frees performed */

  uintptr vbase; /* next available user virtual address for Pebble mapping */

  /* Lists for tracking objects */
  PebbleBlack *black_list;
  PebbleBlue *blue_list;
  PebbleRed *red_list;

  /* State tracking */
  int in_syscall;    /* set when in Pebble syscalls */
  ulong drop_budget; /* budget that will drop on exit */

  /* White token bookkeeping */
  PebbleWhite whites[PEBBLE_MAX_TOKENS];
  uchar whites_active[PEBBLE_MAX_TOKENS];
  ulong white_generation;
  int white_head;
} PebbleState;

/*
 * Arena Branch Bank - Per-Container Resource Management
 *
 * Each WASM container (or other arena) gets its own branch bank
 * for lock-free local allocations. Branches periodically reconcile
 * with the process colorless bank.
 *
 * Token flow: Process colorless_bank → branch local_colorless → BLACK
 * All transitions remain 1:1 (token conservation).
 *
 * See docs/WASM_ARENA_BRANCH_BANKS.md for design details.
 */
typedef struct arena_branch {
  Lock lock;                /* Per-branch lock (no global contention) */
  ulong local_colorless;    /* Tokens available locally in this branch */
  ulong borrowed_from_proc; /* Tokens borrowed from process bank */
  ulong max_tokens;         /* Hard cap on branch tokens */
  ulong low_water;          /* Request refill when below this threshold */
  ulong high_water;         /* Return excess when above this threshold */
  ulong total_allocated;    /* Statistics: total bytes allocated from branch */
  ulong total_freed;        /* Statistics: total bytes freed to branch */
  PebbleState *owner_ps;    /* Back-pointer to owning process PebbleState */
} arena_branch_t;

/* Arena Branch API */
void arena_branch_init(arena_branch_t *branch, PebbleState *ps,
                       ulong initial_budget);
int arena_branch_alloc(arena_branch_t *branch,
                       ulong size); /* Branch tokens → allocation */
void arena_branch_free(arena_branch_t *branch,
                       ulong size); /* Allocation → branch tokens */
int arena_branch_refill(arena_branch_t *branch); /* Process bank → branch */
void arena_branch_drain(arena_branch_t *branch); /* Branch → process bank */

/* Global Pebble lock - one system-wide lock for now */
extern Lock pebble_global_lock;

/* Core API functions */
int pebble_black_alloc(PebbleWhite *white, void *buf, ulong size,
                       UserCapability *out_cap);
void *pebble_get_black_addr(const UserCapability *cap);
int pebble_black_free(const UserCapability *cap);
int pebble_white_verify(PebbleWhite *white_cap, void **black_cap);
int pebble_create_token_uuid(PebbleWhite *white, uuid_t *out_uuid);
int pebble_alloc_with_white(ulong size, UserCapability *out_cap,
                            void **out_addr);

/* Blue/Red API - Independent colored tokens for block I/O transactions */
PebbleBlue *pebble_blue_alloc(ulong size); /* COLORLESS → BLUE */
int pebble_blue_free(PebbleBlue *blue);    /* BLUE → COLORLESS */
PebbleRed *pebble_red_alloc(ulong size);   /* COLORLESS → RED */
int pebble_red_free(PebbleRed *red);       /* RED → COLORLESS */
int pebble_red_snapshot(PebbleBlue *blue,
                        PebbleRed **out_red); /* Copy Blue → Red */

/* Legacy API - DEPRECATED, will be removed */
int pebble_red_copy(PebbleBlue *blue_obj,
                    PebbleRed **red_copy);     /* Use pebble_red_snapshot */
int pebble_blue_discard(PebbleBlue *blue_obj); /* Use pebble_blue_free */

/* Internal helper functions */
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

/* Debug support */
#if PEBBLE_DEBUG
#define pebble_dprint(fmt, ...) print("PEBBLE: " fmt "\n", ##__VA_ARGS__)
#else
#define pebble_dprint(fmt, ...)
#endif

/* Initialization */
void pebbleinit(void);
void pebbleprocinit(Proc *p);
void *pebble_meta_alloc(ulong size);
void pebble_meta_free(void *v);

/* Constants for validation */
#define PEBBLE_TOKEN_MAGIC 0x50454242 /* "PEBB" */
#define PEBBLE_MEM_PER_TOKEN 8        /* 8 bytes per Token Unit */
#define PEBBLE_MIN_ALLOC 8            /* Minimum allocation is 1 Token */
#define PEBBLE_MAX_ALLOC (1024 * 1024 * 1024) /* 1 GiB max single alloc */

/*
 * B.E.V.I.S. (Byzantine Energy Verification & Isolation Subsystem)
 * B.U.T.T.H.E.A.D. (Bandwidth-Utilizing Thermodynamic Token Hardened Economic
 * Allocation Dispatcher)
 */
#define POW_OP_ALLOC 1
#define POW_OP_SPAWN 2
#define POW_OP_NET_BIND 3
#define POW_OP_REALTIME 4
#define POW_OP_STACK_ALLOC 5 /* CIL localloc - cheaper than heap */
#define POW_OP_MSGORD 6      /* MsgOrd consensus admission */

int pow_calculate_difficulty(int op_class, ulong magnitude);
int pow_verify(u64int nonce, u64int context, int required_diff);
void pow_gate_init(void);

#ifndef ROUNDUP
#define ROUNDUP(n, sz) (((n) + ((sz) - 1)) & ~((sz) - 1))
#endif
