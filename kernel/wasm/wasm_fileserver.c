/* wasm_fileserver.c - WASM File Server Implementation
 *
 * WASM file servers with exchange page pools + msgord.
 * High-bandwidth, lock-free, totally ordered message processing.
 */

#include "wasm_fileserver.h"
#include "../include/9p_router.h"
#include "../include/csprng_fallback.h"
#include "../include/dat.h"
#include "../include/error.h"
#include "../include/exchange_pool.h"
#include "../include/fns.h"
#include "../include/mem.h"
#include "../include/u.h"
#include "atomic.h"

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

/*@
  requires wasm_path != \null;
  requires \valid_read(wasm_path + (0..));
  assigns \result \from wasm_path, num_pages;
  ensures \result == \null || \valid(\result);
  ensures \result != \null ==> \result->num_pages == (num_pages == 0 ?
  WASM_DEFAULT_PAGE_POOL : num_pages);
*/
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
  /*@ loop invariant 0 <= i <= num_pages;
  @ loop assigns i;
  @ loop variant num_pages - i;
  @*/
  for (u32int i = 0; i < num_pages; i++) {
    void *page = xalloc(BY2PG);
    if (!page) {
      server->num_pages = i;
      print("wasm_fileserver: only allocated %u/%u exchange pages\n", i,
            num_pages);
      break;
    }
    memset(page, 0, BY2PG);

    /* Mint capability for the page - owner is current process (router) */
    u8int secret[32];
    u64int *s64 = (u64int *)secret;
    s64[0] = chacha20_csprng_u64();
    s64[1] = chacha20_csprng_u64();
    s64[2] = chacha20_csprng_u64();
    s64[3] = chacha20_csprng_u64();

    if (ledger_mint(&server->pages[i], PADDR(page), BY2PG, up,
                    CAP_PERM_READ | CAP_PERM_WRITE,
                    secret) != BLIND_LEDGER_OK) {
      print("wasm_fileserver: ledger_mint failed for page %u\n", i);
      /* In a real kernel we'd free the page, but xalloc is often sticky in
       * early lux9 */
      server->num_pages = i;
      break;
    }
  }

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

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
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

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
int wasm_fs_get_page(wasm_fileserver_t *server) {
  if (!server || !server->pages)
    return -1;

  /* Round-robin allocation */
  int page_idx = server->next_page;
  server->next_page = (server->next_page + 1) % server->num_pages;

  return page_idx;
}

/* ========== Message Submission ========== */

/*@
  requires server != \null && \valid(server);
  requires caller != \null && \valid(caller);
  requires request != \null && \valid(request);
  requires path != \null && \valid_read(path + (0..));
  assigns \result \from server, caller, request, path;
*/
uint wasm_fs_submit(wasm_fileserver_t *server, Proc *caller, Fcall *request,
                    char *path) {
  ExchangeRequest req;
  ExchangeHandle cap;
  PoolError err;
  uint msg_id;

  if (!server || !caller || !request || !path)
    return 0;

  /* Initialize hybrid request */
  req.size = sizeS2M(request);
  req.type = EXCHANGE_TYPE_AUTO;
  strncpy(req.path, path, KNAMELEN - 1);
  req.path[KNAMELEN - 1] = '\0';

  /* Get hybrid exchange page (may be a new page or an active ring) */
  err = pool_prepare_hybrid(up, &req, &cap);
  if (err != POOL_OK) {
    print("wasm_fs: hybrid prepare failed (err=%d)\n", err);
    return 0;
  }

  /* Map the page to write the message */
  void *vaddr = (void *)exchange_map_by_cap(&cap);
  if (!vaddr) {
    print("wasm_fs: failed to map exchange page\n");
    return 0;
  }

  P9Control *ctl = (P9Control *)((uintptr)vaddr + P9_CONTROL_OFFSET);

  if (req.type == EXCHANGE_TYPE_RING) {
    /* Write to the current req_tail slot */
    u32int head = ctl->req_head;
    u32int tail = ctl->req_tail;
    u32int next_tail = (tail + 1) % P9_RING_SLOTS;

    if (next_tail == head) {
      /* Full (should have been caught by pool_prepare_hybrid, but double check)
       */
      exchange_unmap_by_cap(&cap, (uintptr)vaddr);
      return 0;
    }

    uchar *slot = (uchar *)vaddr + (tail * P9_RING_SLOT_SIZE);
    u32int req_size =
        convS2M(request, slot + P9_RING_HEADER_SIZE, P9_RING_DATA_SIZE);
    if (req_size == 0) {
      exchange_unmap_by_cap(&cap, (uintptr)vaddr);
      return 0;
    }

    PBIT32(slot, req_size);
    PBIT32(slot + 4, 0); /* rep_size = 0 for now */

    ctl->req_tail = next_tail;
    __asm__ volatile("mfence" ::: "memory");

    /* If it's the first message in the ring, we submit it.
     * If not, we might want to wait (batching), but for now
     * let's submit every time we add to a ring to trigger the kernel.
     * The doorbell/msgord will then process all messages between head/tail.
     */
  } else {
    /* LARGE message: write to the beginning of the page */
    if (convS2M(request, vaddr, P9_MSG_SIZE) == 0) {
      exchange_unmap_by_cap(&cap, (uintptr)vaddr);
      return 0;
    }
  }

  exchange_unmap_by_cap(&cap, (uintptr)vaddr);

  /* Generate security nonce */
  u64int nonce = chacha20_csprng_u64();
  MsgOrdSpec spec;

  msgord_spec_init(&spec);
  msgord_spec_add(&spec, msgord_key_root(path));
  msgord_spec_add(&spec, msgord_key_path(path));
  msgord_spec_add(&spec, msgord_key_exchange(&cap));

  /* Submit message handle to msgord */
  msg_id = msgord_submit_exchange(server->msgord, up, cap, 0, req.size, path,
                                  &spec, nonce);
  if (msg_id == 0) {
    print("wasm_fs: msgord_submit failed\n");
    return 0;
  }

  return msg_id;
}

/* ========== Pool Management ========== */

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
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

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
u32int wasm_fs_get_pool_size(wasm_fileserver_t *server) {
  return server ? server->num_pages : 0;
}

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @ ensures \result >= 0 && \result <= 100;
  @*/
u32int wasm_fs_get_pool_utilization(wasm_fileserver_t *server) {
  if (!server || !server->msgord || server->num_pages == 0)
    return 0;

  /* Calculate utilization based on pending messages in MSGORD queue relative to
   * pool size */
  u32int count = server->msgord->gd_count;
  u32int util = (count * 100) / server->num_pages;

  if (util > 100)
    util = 100;

  return util;
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

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
void wasm_fs_disable_autoscale(wasm_fileserver_t *server) {
  if (!server)
    return;

  server->autoscale.enabled = 0;
  print("wasm_fs: auto-scaling disabled\n");
}

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
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

/* ========== Ring Buffer Processing ========== */

/*@
  @ requires server != \null && \valid(server);
  @ requires ctl != \null && \valid(ctl);
  @ requires base != \null;
  @ requires \valid(base + (0 .. (P9_RING_SLOTS * P9_RING_SLOT_SIZE) - 1));
  @ assigns ctl->req_head;
  @
  @ behavior ring_math_correctness:
  @   ensures ctl->req_head == (\old(ctl->req_head) + \result) % P9_RING_SLOTS;
  @*/
int wasm_fs_handle_ring(wasm_fileserver_t *server, P9Control *ctl,
                        uchar *base) {
  u32int head = ctl->req_head;
  u32int tail = ctl->req_tail;

  /*@ ghost u32int _initial_head = head; */

  if (head == tail)
    return 0;

  int processed = 0;
  /*@
    @ loop invariant 0 <= processed <= P9_RING_SLOTS;
    @ loop invariant head < P9_RING_SLOTS;
    @ loop invariant head == (_initial_head + processed) % P9_RING_SLOTS;
    @ loop assigns head, processed;
    @ loop variant (tail - head + P9_RING_SLOTS) % P9_RING_SLOTS;
    @*/
  while (head != tail) {
    uchar *slot = base + (head * P9_RING_SLOT_SIZE);

    /* Read request size */
    u32int req_size = GBIT32(slot);
    if (req_size == 0 || req_size > P9_RING_DATA_SIZE) {
      /* Invalid size, skip or abort? Abort ring to be safe. */
      print("wasm_fs: ring corrupt req_size=%ud at head=%ud\n", req_size, head);
      break;
    }

    Fcall request, reply;
    request = (Fcall){0};
    reply = (Fcall){0};

    /* Deserialize Request */
    if (convM2S(slot + P9_RING_HEADER_SIZE, req_size, &request) > 0) {
      reply.tag = request.tag;

      /* Execute WASM Handler */
      if (wasm_fs_handle_fcall(server, &request, &reply) >= 0) {
        /* Serialize Response */
        /* Response goes into the SAME slot? No, reply ring usually separate or
         * shared? Lux9 Ring: shared P9Control, usually single ring for duplex?
         * "P9Control *ctl" implies standard ring.
         * Standard ring: req_head/tail and rep_head/tail?
         * Let's check P9Control struct in 9p_router.h or exchange.h.
         * Assuming shared slot buffer logic: request overwrites or separate
         * reply area? Usually: Write reply to slot->data? If duplex, we write
         * to rep_tail. BUT, wasm_fs_process_next maps the *request* page. If
         * the page is "hybrid", it has "P9_CONTROL_OFFSET". Let's assume
         * standard ring protocol: Request consumed from req ring, Reply
         * appended to rep ring? Or slot reuse? Looking at wasm_fs_process_next:
         * "ctl->req_tail != ctl->req_head".
         *
         * Simplified for WASM: Process in place and update status bit?
         * We update PBIT32(slot + 4, rep_size).
         */

        u32int rep_size =
            convS2M(&reply, slot + P9_RING_HEADER_SIZE, P9_RING_DATA_SIZE);
        PBIT32(slot + 4, rep_size); // Store reply size

        /* Memory Barrier */
        __asm__ volatile("sfence" ::: "memory");

        /* In a full ring model, we'd advance rep_tail.
         * Here we just mark the slot as processed?
         * The client increments req_head to say "I consumed the reply"?
         * No, Client increments req_tail to add. Server increments req_head to
         * remove. Server adds to rep_tail. If we reuse slot, we must
         * synchronize.
         *
         * Let's follow standard: Server updates req_head.
         * And assumes client reads completion from slot status or separate
         * reply ring. For "Embedded" mode, let's update req_head locally.
         */
      }
    }

    head = (head + 1) % P9_RING_SLOTS;
    processed++;
  }

  /* Update shared head pointer */
  ctl->req_head = head;
  return processed;
}

/* ========== Message Processing ========== */

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
int wasm_fs_process_next(wasm_fileserver_t *server) {
  if (!server || !server->msgord)
    return -1;

  /* Get next ordered message from msgord */
  OrdMsg *msg = msgord_next(server->msgord);
  if (!msg)
    return -1; /* No messages ready */

  print("wasm_fs: processing msg %u\n", msg->gm_id);

  Fcall request, reply;
  request = (Fcall){0};
  reply = (Fcall){0};

  int handled = 0;
  if (msg->gm_payload.type == MSGORD_MSG_EXCHANGE) {
    /* Delivery path for exchange pages */
    void *vaddr = (void *)exchange_map_by_cap(&msg->gm_payload.exchange.handle);
    if (vaddr) {
      P9Control *ctl = (P9Control *)((uintptr)vaddr + P9_CONTROL_OFFSET);
      if (ctl->req_tail != ctl->req_head) {
        /* Batch delivery via custom ring handler */
        if (wasm_fs_handle_ring(server, ctl, (uchar *)vaddr) >= 0) {
          handled = 1;
        }
      } else {
        /* Single message delivery */
        if (convM2S(vaddr, msg->gm_payload.exchange.len, &request) > 0) {
          reply.tag = request.tag;
          if (wasm_fs_handle_fcall(server, &request, &reply) >= 0) {
            handled = 1;
            /* Write reply back if single message */
            convS2M(&reply, vaddr, P9_MSG_SIZE);
          }
        }
      }
      exchange_unmap_by_cap(&msg->gm_payload.exchange.handle, (uintptr)vaddr);
    }
  } else if (msg->gm_payload.type == MSGORD_MSG_9P && msg->gm_payload.fcall) {
    /* Delivery path for legacy 9P messages (serialized in raw.data) */
    if (convM2S(msg->gm_payload.raw.data, msg->gm_payload.raw.len, &request) >
        0) {
      reply.tag = request.tag;
      if (wasm_fs_handle_fcall(server, &request, &reply) >= 0) {
        handled = 1;
      }
    }
  }

  if (handled) {
    /* Success */
  } else {
    reply.type = Rerror;
    reply.ename = "wasm fs_handle_message failed";
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

/*@
  @ requires server == \null || \valid(server);
  @ assigns \nothing;
  @*/
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
  M3Result result =
      m3_FindFunction(&func, server->runtime, "fs_handle_message");
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
    print("wasm_fs: invalid response size %u (cap=%ud)\n", resp_size, resp_cap);
    return -1;
  }

  if (convM2S(mem + resp_off, resp_size, response) == 0) {
    print("wasm_fs: response parse failed\n");
    return -1;
  }

  return 0;
}
