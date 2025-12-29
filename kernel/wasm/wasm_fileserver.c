/* wasm_fileserver.c - WASM File Server Implementation
 *
 * WASM file servers with exchange page pools + msgord.
 * High-bandwidth, lock-free, totally ordered message processing.
 */

#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/mem.h"
#include "../include/portlib.h"
#include "../include/u.h"

/* Provide C99 types for wasm3 (kernel uses u8int, u32int, u64int) */
typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s8int int8_t;
typedef s16int int16_t;
typedef s32int int32_t;
typedef s64int int64_t;
typedef usize size_t;

#include "wasm_fileserver.h"
#include "wasm_runtime/wasm3/wasm3.h"

#ifndef nil
#define nil ((void *)0)
#endif

/* Global WASM environment (shared across all servers) */
static IM3Environment wasm_env = nil;

/* ========== File Server Management ========== */

wasm_fileserver_t *wasm_fileserver_load(const char *wasm_path,
                                        u32int num_pages) {
  M3Result result;

  if (!wasm_path)
    return nil;

  if (num_pages == 0)
    num_pages = WASM_DEFAULT_PAGE_POOL;

  /* Initialize WASM3 environment (once) */
  if (!wasm_env) {
    wasm_env = m3_NewEnvironment();
    if (!wasm_env) {
      print("wasm_fileserver: failed to create WASM environment\n");
      return nil;
    }
  }

  wasm_fileserver_t *server = malloc(sizeof(wasm_fileserver_t));
  if (!server)
    return nil;

  /* Create WASM3 runtime (64KB stack) */
  server->runtime = m3_NewRuntime(wasm_env, 64 * 1024, nil);
  if (!server->runtime) {
    print("wasm_fileserver: failed to create WASM runtime\n");
    free(server);
    return nil;
  }

  /* TODO: Load WASM module from file
   * For now, assume wasm_path points to WASM bytes in memory
   * In real implementation, use file I/O to load the .wasm file
   */

  /* TODO: Parse and load module */
  server->module = nil; /* Will be set when we have actual WASM bytes */

  /* Get linear memory pointer */
  server->memory = m3_GetMemory(server->runtime, &server->memory_size, 0);
  if (!server->memory) {
    server->memory_size = 0;
    print("wasm_fileserver: warning: no WASM linear memory yet\n");
  }

  /* Allocate exchange page pool */
  server->pages = malloc(num_pages * sizeof(ExchangeHandle));
  if (!server->pages) {
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  server->num_pages = num_pages;
  server->next_page = 0;

  /* Initialize exchange pages */
  /* TODO: Actually prepare exchange pages */
  memset(server->pages, 0, num_pages * sizeof(ExchangeHandle));

  /* Create msgord instance for this server */
  server->msgord = msgord_create_instance(MSGORD_K_PARAMETER);
  if (!server->msgord) {
    free(server->pages);
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  print("wasm_fileserver: created server with %u page pool (WASM runtime "
        "ready)\n",
        num_pages);
  return server;
}

void wasm_fileserver_destroy(wasm_fileserver_t *server) {
  if (!server)
    return;

  /* Destroy msgord instance */
  if (server->msgord)
    msgord_destroy_instance(server->msgord);

  /* Release exchange pages */
  if (server->pages)
    free(server->pages);

  /* Cleanup WASM3 runtime (frees module too) */
  if (server->runtime)
    m3_FreeRuntime(server->runtime);

  print("wasm_fileserver: destroyed\n");
  free(server);
}

/* ========== Page Pool Management ========== */

int wasm_fs_get_page(wasm_fileserver_t *server) {
  if (!server || !server->pages)
    return -1;

  /* Round-robin allocation */
  int page_idx = server->next_page;
  server->next_page = (server->next_page + 1) % server->num_pages;

  return page_idx;
}

/* ========== Message Submission ========== */

uint wasm_fs_submit(wasm_fileserver_t *server, Proc *caller, Fcall *request,
                    char *path) {
  if (!server || !caller || !request || !path)
    return 0;

  /* TODO: Get available exchange page from pool
   * TODO: Prepare exchange page with request Fcall
   * TODO: Submit to msgord
   *
   * For now, just submit directly to msgord without exchange page
   */

  uint msg_id = msgord_submit(server->msgord, caller, request, path);
  if (msg_id == 0) {
    print("wasm_fs: msgord_submit failed\n");
    return 0;
  }

  print("wasm_fs: submitted msg %u to msgord\n", msg_id);
  return msg_id;
}

/* ========== Pool Management ========== */

int wasm_fs_resize_pool(wasm_fileserver_t *server, u32int new_size) {
  if (!server || new_size == 0)
    return -1;

  if (new_size == server->num_pages)
    return 0; /* No change */

  print("wasm_fs: resizing pool from %u to %u pages\n", server->num_pages,
        new_size);

  /* Allocate new pool */
  ExchangeHandle *new_pages = malloc(new_size * sizeof(ExchangeHandle));
  if (!new_pages) {
    print("wasm_fs: resize failed - out of memory\n");
    return -1;
  }

  /* Copy existing pages (up to min of old/new size) */
  u32int copy_count =
      (new_size < server->num_pages) ? new_size : server->num_pages;
  if (copy_count > 0) {
    memmove(new_pages, server->pages, copy_count * sizeof(ExchangeHandle));
  }

  /* Initialize new pages if growing */
  if (new_size > server->num_pages) {
    memset(new_pages + server->num_pages, 0,
           (new_size - server->num_pages) * sizeof(ExchangeHandle));
  }

  /* Swap pools atomically
   * Note: In production, this needs proper synchronization
   * with concurrent wasm_fs_get_page() calls. For now, assume
   * single-threaded or external synchronization. */
  ExchangeHandle *old_pages = server->pages;
  server->pages = new_pages;
  server->num_pages = new_size;

  /* Adjust round-robin pointer if it's now out of bounds */
  if (server->next_page >= new_size)
    server->next_page = 0;

  /* Free old pool */
  free(old_pages);

  print("wasm_fs: resize complete, now %u pages\n", new_size);
  return 0;
}

u32int wasm_fs_get_pool_size(wasm_fileserver_t *server) {
  return server ? server->num_pages : 0;
}

u32int wasm_fs_get_pool_utilization(wasm_fileserver_t *server) {
  if (!server || !server->msgord)
    return 0;

  /* TODO: Track per-page utilization properly
   * For now, return a simple estimate based on round-robin position
   * In production, maintain a bitmap of busy pages */

  /* Placeholder: assume moderate utilization */
  return 50; /* 50% - replace with actual tracking */
}

/* ========== Auto-Scaling ========== */

int wasm_fs_enable_autoscale(wasm_fileserver_t *server,
                             WasmAutoScaleConfig *cfg) {
  if (!server)
    return -1;

  if (cfg) {
    /* Use provided configuration */
    server->autoscale = *cfg;
  } else {
    /* Use default configuration */
    u32int current = server->num_pages;
    server->autoscale.min_pages = current / 2;
    if (server->autoscale.min_pages < 4)
      server->autoscale.min_pages = 4;

    server->autoscale.max_pages = current * 8;
    if (server->autoscale.max_pages > 1024)
      server->autoscale.max_pages = 1024;

    server->autoscale.target_util = 75;
    server->autoscale.high_threshold = 85;
    server->autoscale.low_threshold = 50;
    server->autoscale.check_interval = 1000; /* 1 second */
    server->autoscale.ema_alpha = 30;        /* 0.3 */
  }

  /* Initialize algorithm state */
  server->autoscale.enabled = 1;
  server->autoscale.smoothed_util = server->autoscale.target_util;
  server->autoscale.last_check = fastticks(nil);
  server->autoscale.stable_count = 0;

  print("wasm_fs: auto-scaling enabled (min=%u, max=%u, target=%u%%)\n",
        server->autoscale.min_pages, server->autoscale.max_pages,
        server->autoscale.target_util);

  return 0;
}

void wasm_fs_disable_autoscale(wasm_fileserver_t *server) {
  if (!server)
    return;

  server->autoscale.enabled = 0;
  print("wasm_fs: auto-scaling disabled\n");
}

int wasm_fs_autoscale_tick(wasm_fileserver_t *server) {
  if (!server || !server->autoscale.enabled)
    return -1;

  WasmAutoScaleConfig *cfg = &server->autoscale;

  /* Check if enough time has passed since last check */
  u64int now = fastticks(nil);
  u64int elapsed_ms = (now - cfg->last_check) / 1000000; /* Convert to ms */

  if (elapsed_ms < cfg->check_interval)
    return 0; /* Not time yet */

  cfg->last_check = now;

  /* Step 1: Measure current utilization */
  u32int current_util = wasm_fs_get_pool_utilization(server);

  /* Step 2: Update EMA (Exponential Moving Average)
   * smoothed = α × current + (1-α) × smoothed
   * α is stored as integer 0-100, so divide by 100 */
  u32int alpha = cfg->ema_alpha;
  cfg->smoothed_util =
      (alpha * current_util + (100 - alpha) * cfg->smoothed_util) / 100;

  u32int current_size = server->num_pages;
  u32int new_size = current_size;

  /* Step 3: Decide action based on smoothed utilization with hysteresis */
  if (cfg->smoothed_util > cfg->high_threshold) {
    /* OVERLOADED: Grow pool aggressively (multiplicative increase)
     * Grow by 50% to handle burst traffic quickly */
    new_size = (current_size * 3) / 2;

    if (new_size > cfg->max_pages)
      new_size = cfg->max_pages;

    if (new_size != current_size) {
      print("wasm_fs: HIGH LOAD (%u%% > %u%%), growing %u → %u pages\n",
            cfg->smoothed_util, cfg->high_threshold, current_size, new_size);
      cfg->stable_count = 0;
    }
  } else if (cfg->smoothed_util < cfg->low_threshold) {
    /* UNDERUTILIZED: Shrink pool slowly (multiplicative decrease)
     * Shrink by 25% to avoid thrashing
     * Only shrink if stable for at least 3 intervals (avoid oscillation) */

    if (cfg->stable_count >= 3) {
      new_size = (current_size * 3) / 4;

      if (new_size < cfg->min_pages)
        new_size = cfg->min_pages;

      if (new_size != current_size) {
        print("wasm_fs: LOW LOAD (%u%% < %u%%), shrinking %u → %u pages\n",
              cfg->smoothed_util, cfg->low_threshold, current_size, new_size);
        cfg->stable_count = 0;
      }
    } else {
      cfg->stable_count++;
    }
  } else {
    /* IN DEADBAND: No action, increment stability counter */
    cfg->stable_count++;
    if (cfg->stable_count > 10)
      cfg->stable_count = 10; /* Cap to prevent overflow */
  }

  /* Step 4: Resize if needed */
  if (new_size != current_size) {
    int ret = wasm_fs_resize_pool(server, new_size);
    return (ret == 0) ? 1 : -1; /* 1 = resized, -1 = error */
  }

  return 0; /* No change */
}

/* ========== Message Processing ========== */

int wasm_fs_process_next(wasm_fileserver_t *server) {
  M3Result result;
  IM3Function func;

  if (!server || !server->msgord)
    return -1;

  /* Get next ordered message from msgord */
  OrdMsg *msg = msgord_next(server->msgord);
  if (!msg)
    return -1; /* No messages ready */

  print("wasm_fs: processing msg %u\n", msg->gm_id);

  /* If module is loaded, call WASM function */
  if (server->module && server->runtime) {
    /* Find the fs_handle_message export */
    result = m3_FindFunction(&func, server->runtime, "fs_handle_message");
    if (result) {
      print("wasm_fs: fs_handle_message not found: %s\n", result);
      goto complete;
    }

    /* TODO: Marshal 9P message into WASM linear memory
     * 1. Write Fcall to linear memory at offset 0
     * 2. Call fs_handle_message(offset=0, size=msg_size)
     * 3. Read response from linear memory
     *
     * For now, just call with dummy arguments
     */

    const void *ret_ptr;
    u32int msg_offset = 0;
    u32int msg_size = 1024; /* TODO: actual message size */

    result = m3_CallV(func, msg_offset, msg_size);
    if (result) {
      print("wasm_fs: WASM call failed: %s\n", result);
      goto complete;
    }

    /* Get return value (response size) */
    uint32_t response_size = 0;
    result = m3_GetResultsV(func, &response_size);
    if (!result) {
      print("wasm_fs: WASM returned response_size=%u\n", response_size);
    }

    /* TODO: Read response from WASM linear memory */
    /* TODO: Send response back to caller */
  } else {
    print("wasm_fs: no WASM module loaded, skipping execution\n");
  }

complete:
  /* Complete and remove from msgord */
  msgord_complete(server->msgord, msg);

  return 0;
}

int wasm_fs_process_all(wasm_fileserver_t *server) {
  if (!server || !server->msgord)
    return -1;

  int count = 0;
  while (wasm_fs_process_next(server) == 0)
    count++;

  return count;
}
