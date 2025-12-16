#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <fcall.h>
#include "sip.h"

#define EXCHANGE_PAGE_ADDR 0x7FFFFFFF0000ULL
#define P9_REQUEST_OFFSET 0x000
#define P9_REQUEST_SIZE 0xF00
#define P9_REPLY_OFFSET 0x1000
#define P9_REPLY_SIZE 0x1000
#define P9_CONTROL_OFFSET 0xF00

#define P9_STATUS_IDLE 0
#define P9_STATUS_PENDING 1
#define P9_STATUS_COMPLETE 2
#define P9_STATUS_ERROR 3

/* Atomic Control Block */
typedef struct P9Control {
    atomic_uint doorbell;
    atomic_uint status;
    uint32_t req_head;
    uint32_t req_tail;
    uint32_t rep_head;
    uint32_t rep_tail;
    uint32_t req_seq;
    uint32_t rep_seq;
    uint8_t session_pebble[32];
    uint8_t reserved[192];
} P9Control;

static P9Control *ctl = (P9Control *)(uintptr_t)(EXCHANGE_PAGE_ADDR + P9_CONTROL_OFFSET);
static uint8_t *req_buf = (uint8_t *)(uintptr_t)(EXCHANGE_PAGE_ADDR + P9_REQUEST_OFFSET);
static uint8_t *rep_buf = (uint8_t *)(uintptr_t)(EXCHANGE_PAGE_ADDR + P9_REPLY_OFFSET);

static int initialized = 0;

int sip_init(void) {
    /* 
     * In Phase 6, the kernel maps the exchange page at creation.
     * We just verify we can access it.
     * A real implementation might check a magic number or use a syscall 
     * to map it if missing (but we are eliminating syscalls!).
     */
    /* Simple check: access status */
    volatile uint32_t s = atomic_load(&ctl->status);
    (void)s;
    initialized = 1;
    return 0;
}

int sip_transact(Fcall *tx, Fcall *rx) {
    if (!initialized) sip_init();

    /* 1. Wait for IDLE */
    while (atomic_load_explicit(&ctl->status, memory_order_relaxed) != P9_STATUS_IDLE) {
        __builtin_ia32_pause(); 
    }

    /* 2. Serialize Request */
    uint32_t n = convS2M(tx, req_buf, P9_REQUEST_SIZE);
    if (n == 0) return -1;
    
    ctl->req_tail = n;
    ctl->req_head = 0;
    
    /* 3. Ring Doorbell (Release) */
    atomic_store_explicit(&ctl->doorbell, 1, memory_order_release);

    /* 4. Wait for Reply (Acquire) */
    int status;
    while ((status = atomic_load_explicit(&ctl->status, memory_order_acquire)) != P9_STATUS_COMPLETE) {
        if (status == P9_STATUS_ERROR) {
            atomic_store_explicit(&ctl->status, P9_STATUS_IDLE, memory_order_relaxed);
            return -1;
        }
        __builtin_ia32_pause();
    }

    /* 5. Parse Reply */
    /* n = ctl->rep_tail; We assume size is in the packet or we trust convM2S to handle buffer limit */
    /* convM2S returns bytes consumed/produced? */
    if (convM2S(rep_buf, P9_REPLY_SIZE, rx) == 0) {
        atomic_store_explicit(&ctl->status, P9_STATUS_IDLE, memory_order_relaxed);
        return -1;
    }

    /* 6. Reset to IDLE */
    atomic_store_explicit(&ctl->status, P9_STATUS_IDLE, memory_order_relaxed);
    
    return 0;
}
