/* wasm_fileserver.c - WASM File Server Implementation
 *
 * WASM file servers with exchange page pools + msgord.
 * High-bandwidth, lock-free, totally ordered message processing.
 */

#include "../include/9p_router.h"
#include "../include/dat.h"
#include "../include/fcall.h"
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
  memset(server, 0, sizeof(*server));

  /* Create WASM3 runtime (64KB stack) */
  server->runtime = m3_NewRuntime(wasm_env, 64 * 1024, nil);
  if (!server->runtime) {
    print("wasm_fileserver: failed to create WASM runtime\n");
    free(server);
    return nil;
  }

  /* Load WASM module from file */
  Chan *c = namec(wasm_path, Aopen, OREAD, 0);
  if (c == nil) {
    print("wasm_fileserver: failed to open %s\n", wasm_path);
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  if (waserror()) {
    cclose(c);
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  Dir d;
  devtab[c->type]->stat(c, (uchar *)&d, sizeof(Dir));
  if (d.length <= 0) {
    print("wasm_fileserver: empty module %s\n", wasm_path);
    cclose(c);
    poperror();
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  server->module_size = (u32int)d.length;
  server->module_bytes = mallocz(server->module_size, 0);
  if (!server->module_bytes) {
    cclose(c);
    poperror();
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  long nread =
      devtab[c->type]->read(c, server->module_bytes, server->module_size, 0);
  cclose(c);
  poperror();
  if (nread != (long)server->module_size) {
    print("wasm_fileserver: short read %s (%ld/%ud)\n", wasm_path, nread,
          server->module_size);
    free(server->module_bytes);
    server->module_bytes = nil;
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  result = m3_ParseModule(wasm_env, &server->module, server->module_bytes,
                          server->module_size);
  if (result) {
    print("wasm_fileserver: module parse failed: %s\n", result);
    free(server->module_bytes);
    server->module_bytes = nil;
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

  result = m3_LoadModule(server->runtime, server->module);
  if (result) {
    print("wasm_fileserver: module load failed: %s\n", result);
    free(server->module_bytes);
    server->module_bytes = nil;
    m3_FreeRuntime(server->runtime);
    free(server);
    return nil;
  }

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
  if (server->module_bytes)
    free(server->module_bytes);

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

  uint msg_id = msgord_submit(server->msgord, caller, request, path, chacha20_csprng_u64());
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
  if (!server || !server->msgord)
    return -1;

  /* Get next ordered message from msgord */
  OrdMsg *msg = msgord_next(server->msgord);
  if (!msg)
    return -1; /* No messages ready */

  print("wasm_fs: processing msg %u\n", msg->gm_id);

  Fcall reply;
  memset(&reply, 0, sizeof(reply));
  reply.tag = msg->gm_payload.fcall ? msg->gm_payload.fcall->tag : 0;

  if (server->module && server->runtime && msg->gm_payload.fcall) {
    if (wasm_fs_handle_fcall(server, msg->gm_payload.fcall, &reply) < 0) {
      reply.type = Rerror;
      reply.ename = "wasm fs_handle_message failed";
    }
  } else {
    reply.type = Rerror;
    reply.ename = "wasm server not initialized";
  }

  if (msg->gm_caller && msg->gm_caller->p9page) {
    P9Control *ctl =
        (P9Control *)((uintptr)msg->gm_caller->p9page + P9_CONTROL_OFFSET);
    uchar *rep_buf = (uchar *)msg->gm_caller->p9page + P9_REPLY_OFFSET;
    uint rep_size = convS2M(&reply, rep_buf, P9_REPLY_SIZE);
    ctl->rep_head = 0;
    ctl->rep_tail = rep_size;
    ctl->rep_seq++;
    atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
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

int wasm_fs_handle_fcall(wasm_fileserver_t *server, Fcall *request,
                         Fcall *response) {
  if (!server || !server->runtime || !server->module || !request || !response)
    return -1;

  server->memory = m3_GetMemory(server->runtime, &server->memory_size, 0);
  if (!server->memory || server->memory_size == 0) {
    print("wasm_fs: no linear memory\n");
    return -1;
  }

  IM3Function func;
  M3Result result = m3_FindFunction(&func, server->runtime, "fs_handle_message");
  if (result) {
    print("wasm_fs: fs_handle_message not found: %s\n", result);
    return -1;
  }

  u32int req_size = sizeS2M(request);
  if (req_size == 0 || req_size > server->memory_size) {
    print("wasm_fs: invalid request size %ud\n", req_size);
    return -1;
  }

  u32int req_off = 0;
  u32int resp_off = (req_size + 7) & ~7U;
  if (resp_off >= server->memory_size) {
    print("wasm_fs: no space for response buffer\n");
    return -1;
  }

  u32int resp_cap = server->memory_size - resp_off;
  if (resp_cap < BIT32SZ + BIT8SZ + BIT16SZ) {
    print("wasm_fs: response buffer too small\n");
    return -1;
  }

  u8int *mem = (u8int *)server->memory;
  if (convS2M(request, mem + req_off, req_size) == 0) {
    print("wasm_fs: request marshal failed\n");
    return -1;
  }

  result = m3_CallV(func, req_off, req_size, resp_off, resp_cap);
  if (result) {
    print("wasm_fs: WASM call failed: %s\n", result);
    return -1;
  }

  uint32_t resp_size = 0;
  result = m3_GetResultsV(func, &resp_size);
  if (result) {
    print("wasm_fs: failed to read response size: %s\n", result);
    return -1;
  }

  if (resp_size == 0 || resp_size > resp_cap) {
    print("wasm_fs: invalid response size %u (cap=%ud)\n", resp_size,
          resp_cap);
    return -1;
  }

  if (convM2S(mem + resp_off, resp_size, response) == 0) {
    print("wasm_fs: response parse failed\n");
    return -1;
  }

  return 0;
}
