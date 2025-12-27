/* wasm_fileserver.c - WASM File Server Implementation
 *
 * WASM file servers with exchange page pools + msgord.
 * High-bandwidth, lock-free, totally ordered message processing.
 */

#include "wasm_fileserver.h"
#include "../include/u.h"
#include "wasm_runtime/wasm3/wasm3.h"
#include <string.h>

#ifndef nil
#define nil ((void*)0)
#endif

/* Global WASM environment (shared across all servers) */
static IM3Environment wasm_env = nil;

/* ========== File Server Management ========== */

wasm_fileserver_t *wasm_fileserver_load(const char *wasm_path, u32int num_pages) {
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
  server->module = nil;  /* Will be set when we have actual WASM bytes */

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

  print("wasm_fileserver: created server with %u page pool (WASM runtime ready)\n", num_pages);
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

uint wasm_fs_submit(wasm_fileserver_t *server, Proc *caller, Fcall *request, char *path) {
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

    result = m3_Call(func, msg_offset, msg_size);
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
