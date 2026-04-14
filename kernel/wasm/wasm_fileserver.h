/* wasm_fileserver.h - Simple WASM File Server Interface
 *
 * WASM file servers use exchange page pools + msgord + pebble waves:
 *   - Pool of exchange pages for parallel message submission
 *   - MSGORD provides lock-free total ordering
 *   - WASM processes messages in consensus order
 *   - Zero-copy: messages passed by reference via capabilities
 *   - Pebble wave bits (bottom 3 bits) for OOB signaling
 *
 * Architecture:
 *   Physical Layer:
 *     Client 1 → Page 0 ┐
 *     Client 2 → Page 1 ├→ MSGORD → Ordered Queue → WASM Server
 *     Client 3 → Page 2 ┘
 *
 *   OOB Layer (Pebble Waves):
 *     Every pointer has 3 holographic bits (8 channels)
 *     PEBBLE_WAVE_7 = panic/urgent
 *     PEBBLE_WAVE_0 = admin/root
 *     Other waves for priority, hints, etc.
 *
 * Example:
 *   ptr = PEBBLE_PROJECT(page_addr, PEBBLE_WAVE_7);  // Tag as panic
 *   if (PEBBLE_TUNED(ptr, PEBBLE_WAVE_7)) {          // Check wave
 *     handle_panic();
 *   }
 *
 * WASM exports:
 *   - fs_handle_message(msg_offset, msg_size) -> response_size
 *
 * Keep it simple - just files.
 */

#ifndef WASM_FILESERVER_H
#define WASM_FILESERVER_H

#include "../include/u.h"
#include "../include/exchange.h"
#include "../include/msgord.h"

/* Forward declarations */
typedef struct Fcall Fcall;
typedef struct Proc Proc;

/* WASM3 opaque types (actual definitions in wasm_fileserver.c) */
typedef struct M3Runtime* IM3Runtime;
typedef struct M3Module* IM3Module;

/* Default page pool size for WASM servers */
#define WASM_DEFAULT_PAGE_POOL 16

/* Auto-scaling configuration */
typedef struct {
  u32int enabled;        /* Auto-scaling enabled? */
  u32int min_pages;      /* Minimum pool size (never shrink below) */
  u32int max_pages;      /* Maximum pool size (never grow above) */
  u32int target_util;    /* Target utilization % (e.g., 75) */
  u32int high_threshold; /* Grow when util > this (e.g., 85) */
  u32int low_threshold;  /* Shrink when util < this (e.g., 50) */
  u32int check_interval; /* How often to check (milliseconds) */

  /* Algorithm state (internal) */
  u32int smoothed_util;  /* EMA-smoothed utilization */
  u32int ema_alpha;      /* EMA smoothing factor (0-100, e.g., 30 = 0.3) */
  u64int last_check;     /* Last check timestamp (fastticks) */
  u32int stable_count;   /* How many intervals at stable size */
} WasmAutoScaleConfig;

/* WASM file server instance */
typedef struct wasm_fileserver {
  IM3Runtime runtime;     /* WASM3 runtime */
  IM3Module module;       /* Loaded WASM module */
  void *memory;           /* Linear memory pointer (maps to exchange pages) */
  u32int memory_size;     /* Memory size in bytes */
  u8int *module_bytes;    /* Raw module bytes (must outlive module) */
  u32int module_size;     /* Size of module bytes */

  /* Exchange page pool for parallel message submission */
  ExchangeHandle *pages;  /* Pool of exchange pages */
  u32int num_pages;       /* Size of page pool */
  u32int next_page;       /* Next available page (round-robin) */

  /* MSGORD for message ordering */
  MsgOrd *msgord;         /* Message ordering DAG */

  /* Auto-scaling state */
  WasmAutoScaleConfig autoscale;
} wasm_fileserver_t;

/* Load WASM module as file server with page pool */
wasm_fileserver_t *wasm_fileserver_load(const char *wasm_path, u32int num_pages);

/* Destroy file server */
void wasm_fileserver_destroy(wasm_fileserver_t *server);

/* Submit 9P message for processing (non-blocking)
 *
 * @param server: WASM file server instance
 * @param caller: Calling process
 * @param request: 9P request (Fcall)
 * @param path: Target path
 * @returns: Message ID for tracking, or 0 on error
 *
 * Message is submitted to msgord and will be processed in consensus order.
 * Allocates an exchange page from the pool, submits to msgord DAG.
 */
uint wasm_fs_submit(wasm_fileserver_t *server, Proc *caller, Fcall *request, char *path);

/* Process next ordered message from msgord queue
 *
 * @param server: WASM file server instance
 * @returns: 0 on success, -1 if no messages ready
 *
 * Gets next message from msgord in consensus order,
 * calls WASM fs_handle_message(), returns response to caller.
 * Should be called from scheduler or server main loop.
 */
int wasm_fs_process_next(wasm_fileserver_t *server);

/* Process all ready messages
 *
 * @param server: WASM file server instance
 * @returns: Number of messages processed
 *
 * Processes all messages that msgord has ordered and marked ready.
 * Useful for batch processing.
 */
int wasm_fs_process_all(wasm_fileserver_t *server);

/* Handle a single 9P request synchronously via fs_handle_message */
int wasm_fs_handle_fcall(wasm_fileserver_t *server, Fcall *request,
                         Fcall *response);

/* Get an available exchange page from pool
 *
 * @param server: WASM file server instance
 * @returns: Index of available page, or -1 if pool full
 *
 * Round-robin allocation from page pool.
 */
int wasm_fs_get_page(wasm_fileserver_t *server);

/* Resize exchange page pool at runtime
 *
 * @param server: WASM file server instance
 * @param new_size: New pool size (number of pages)
 * @returns: 0 on success, -1 on error
 *
 * Dynamically grows or shrinks the exchange page pool.
 * Allows adapting to changing load without restarting the server.
 * Handles copying existing pages and freeing old pool.
 */
int wasm_fs_resize_pool(wasm_fileserver_t *server, u32int new_size);

/* Get current pool size
 *
 * @param server: WASM file server instance
 * @returns: Current number of pages in pool
 */
u32int wasm_fs_get_pool_size(wasm_fileserver_t *server);

/* Get pool utilization percentage
 *
 * @param server: WASM file server instance
 * @returns: Utilization 0-100%
 *
 * Estimates how many pages are currently in use based on
 * pending messages in MSGORD queue.
 */
u32int wasm_fs_get_pool_utilization(wasm_fileserver_t *server);

/* Enable auto-scaling with specified configuration
 *
 * @param server: WASM file server instance
 * @param cfg: Auto-scaling configuration (or nil for defaults)
 * @returns: 0 on success, -1 on error
 *
 * Enables automatic pool resizing based on utilization.
 * Uses hybrid AIMD + EMA algorithm with hysteresis.
 *
 * Default config (if cfg == nil):
 *   min_pages: current size / 2
 *   max_pages: current size * 8
 *   target_util: 75%
 *   high_threshold: 85%
 *   low_threshold: 50%
 *   check_interval: 1000ms
 *   ema_alpha: 30 (0.3)
 */
int wasm_fs_enable_autoscale(wasm_fileserver_t *server, WasmAutoScaleConfig *cfg);

/* Disable auto-scaling
 *
 * @param server: WASM file server instance
 *
 * Stops automatic pool resizing. Current pool size is preserved.
 */
void wasm_fs_disable_autoscale(wasm_fileserver_t *server);

/* Run one iteration of auto-scaling algorithm
 *
 * @param server: WASM file server instance
 * @returns: 1 if pool was resized, 0 if no change, -1 on error
 *
 * Called periodically (e.g., from timer interrupt or scheduler).
 * Measures utilization, updates EMA, and resizes pool if needed.
 *
 * Algorithm: Hybrid AIMD + EMA with Hysteresis
 *   1. Measure current utilization
 *   2. Update EMA: smoothed = α×current + (1-α)×smoothed
 *   3. If smoothed > high_threshold: grow by 50%
 *   4. Elif smoothed < low_threshold: shrink by 25%
 *   5. Else: no change (deadband)
 */
int wasm_fs_autoscale_tick(wasm_fileserver_t *server);

#endif /* WASM_FILESERVER_H */
