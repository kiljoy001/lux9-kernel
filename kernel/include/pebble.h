#pragma once

struct Proc;
struct Ureg;

/*
 * Pebble Primitives - Core Kernel Feature
 *
 * Three capability-style resource types:
 * - Black-only: kernel-managed, non-clonable resources
 * - Black-White: user white token validated to black handle
 * - Red-Blue: copy-on-write shadow (red = safe, blue = speculative)
 */

/* Compile-time configuration */
#define PEBBLE_DEFAULT_BUDGET    (256*1024*1024)  /* 256 MiB per process */
#define PEBBLE_MAX_TOKENS    128
#define PEBBLE_DEBUG      0

/*
 * Pebble runtime toggles.
 */
extern int pebble_enabled;
extern int pebble_debug;

/* Error handling */
#define PEBBLE_E_PERM      "permission denied"
#define PEBBLE_E_AGAIN      "resource temporarily unavailable"
#define PEBBLE_E_NOMEM      "out of memory"
#define PEBBLE_E_BADARG    "bad argument"
#define PEBBLE_E_BUSY      "resource busy"

/* Capability flags */
#define PEBBLE_CAP_BLACK   (1<<0)
#define PEBBLE_CAP_ACTIVE  (1<<1)

/* White token structure - opaque to user */
typedef struct PebbleWhite {
  u32int  token;
  u32int  generation;
  void  *data_ptr;
  ulong  size;
} PebbleWhite;

/* Blue object structure - speculative */
typedef struct PebbleBlue {
  void  *owner;      /* originating black handle */
  void  *blue_data;
  ulong  blue_size;
  void  *matching_red;  /* corresponding red copy when it exists */
  struct PebbleBlue *next;
} PebbleBlue;

/* Red copy structure - safe snapshot */
typedef struct PebbleRed {
  void  *red_data;
  ulong  red_size;
  struct PebbleRed *next;
} PebbleRed;

typedef struct PebbleBlack {
  void    *addr;
  ulong    size;
  ulong    flags;
  PebbleBlue  *blue;
  PebbleRed  *red;
  struct PebbleBlack *next;
} PebbleBlack;

/* Per-process Pebble state */
typedef struct PebbleState {
  ulong  black_budget;    /* remaining bytes for this process */
  ulong  black_inuse;    /* current allocation */
  ulong  white_verified;    /* count of active white→black conversions */
  ulong  white_pending;    /* bytes authorized by white tokens */
  ulong  red_count;    /* number of live red shadows */
  ulong  blue_count;    /* live blue objects */
  ulong  total_allocs;    /* total allocations made */
  ulong  total_frees;    /* total frees performed */

  /* Lists for tracking objects */
  PebbleBlack *black_list;
  PebbleBlue  *blue_list;
  PebbleRed  *red_list;

  /* State tracking */
  int    in_syscall;    /* set when in Pebble syscalls */
  ulong  drop_budget;    /* budget that will drop on exit */

  /* White token bookkeeping */
  PebbleWhite  whites[PEBBLE_MAX_TOKENS];
  uchar    whites_active[PEBBLE_MAX_TOKENS];
  ulong    white_generation;
  int    white_head;
} PebbleState;

/* Global Pebble lock - one system-wide lock for now */
extern Lock pebble_global_lock;

/* Core API functions */
int  pebble_black_alloc(uintptr size, void **handle);
int  pebble_black_free(void *handle);
int  pebble_white_verify(PebbleWhite *white_cap, void **black_cap);
int  pebble_red_copy(PebbleBlue *blue_obj, PebbleRed **red_copy);
int  pebble_blue_discard(PebbleBlue *blue_obj);

/* Internal helper functions */
PebbleState*  pebble_state(void);
int    pebble_set_budget(ulong budget);
ulong    pebble_get_budget(void);
void    pebble_auto_verify(Proc *p, Ureg *ureg);
void    pebble_red_blue_exit(void);
int    pebble_valid_white_token(PebbleState *ps, PebbleWhite *white);
PebbleWhite*  pebble_issue_white(PebbleState *ps, void *data, ulong size);
PebbleBlack*  pebble_lookup_black(PebbleState *ps, void *handle);
int    pebble_blue_exists(PebbleState *ps, PebbleBlue *blue);
int    pebble_has_matching_red(PebbleState *ps, PebbleBlue *blue);
PebbleRed*  pebble_duplicate_blue(PebbleState *ps, PebbleBlue *blue);
void    pebble_mark_red(PebbleState *ps, PebbleBlue *blue, PebbleRed *red);
void    pebble_ensure_red_snapshots(PebbleState *ps);

void    pebble_cleanup(struct Proc *p);
void    pebble_selftest(void);
void    pebble_sip_issue_test(void);

/* Debug support */
#if PEBBLE_DEBUG
#define pebble_dprint(fmt, ...)  print("PEBBLE: " fmt "\n", ##__VA_ARGS__)
#else
#define pebble_dprint(fmt, ...)
#endif

/* Initialization */
void  pebbleinit(void);
void  pebbleprocinit(Proc *p);

/* Constants for validation */
#define PEBBLE_TOKEN_MAGIC    0x50454242  /* "PEBB" */
#define PEBBLE_MIN_ALLOC    64
#define PEBBLE_MAX_ALLOC    (1024*1024*1024)  /* 1 GiB max single alloc */
