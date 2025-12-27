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
typedef struct IM3Runtime* IM3Runtime;
typedef struct IM3Module* IM3Module;

/* Default page pool size for WASM servers */
#define WASM_DEFAULT_PAGE_POOL 16

/* WASM file server instance */
typedef struct wasm_fileserver {
  IM3Runtime runtime;     /* WASM3 runtime */
  IM3Module module;       /* Loaded WASM module */
  void *memory;           /* Linear memory pointer (maps to exchange pages) */
  u32int memory_size;     /* Memory size in bytes */

  /* Exchange page pool for parallel message submission */
  ExchangeHandle *pages;  /* Pool of exchange pages */
  u32int num_pages;       /* Size of page pool */
  u32int next_page;       /* Next available page (round-robin) */

  /* MSGORD for message ordering */
  MsgOrd *msgord;         /* Message ordering DAG */
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

/* Get an available exchange page from pool
 *
 * @param server: WASM file server instance
 * @returns: Index of available page, or -1 if pool full
 *
 * Round-robin allocation from page pool.
 */
int wasm_fs_get_page(wasm_fileserver_t *server);

#endif /* WASM_FILESERVER_H */
