/*
 * SIP Protocol Model for Helgrind/Valgrind Verification
 *
 * This tool simulates the Lux9 Secure Interface Paging (SIP) protocol
 * in userspace using standard pthreads and C11 atomics.
 *
 * Purpose:
 * Verify that the proposed state machine and synchronization logic
 * correctly enforce exclusive access to the Exchange Page buffers,
 * ensuring no data races exist if the protocol is followed.
 *
 * Compile:
 * gcc -g -O0 -pthread -o tools/sip_model tools/sip_model.c
 *
 * Run with Helgrind:
 * valgrind --tool=helgrind ./tools/sip_model
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <assert.h>

/* --- Mimic kernel/include/atomic.h --- */

#define ORDER_RELAXED memory_order_relaxed
#define ORDER_ACQUIRE memory_order_acquire
#define ORDER_RELEASE memory_order_release
#define ORDER_SEQ_CST memory_order_seq_cst

#define atomic_load(ptr, order) atomic_load_explicit(ptr, order)
#define atomic_store(ptr, val, order) atomic_store_explicit(ptr, val, order)

/* --- Definitions from kernel/include/9p_router.h --- */

#define P9_PAGE_SIZE 8192
#define P9_REQUEST_OFFSET 0x000
#define P9_REQUEST_SIZE 0xF00
#define P9_REPLY_OFFSET 0x1000
#define P9_REPLY_SIZE 0x1000
#define P9_CONTROL_OFFSET 0xF00

/* Status codes */
#define P9_STATUS_IDLE 0
#define P9_STATUS_PENDING 1
#define P9_STATUS_COMPLETE 2
#define P9_STATUS_ERROR 3

/* 
 * The Control Block
 */
typedef struct P9Control {
    atomic_uint doorbell;
    atomic_uint status;
    
    /* These are not atomic because they are protected by the protocol */
    uint32_t req_head;
    uint32_t req_tail;
    uint32_t rep_head;
    uint32_t rep_tail;
    uint32_t req_seq;
    uint32_t rep_seq;
    
    uint8_t session_pebble[32];
    uint8_t reserved[192];
} P9Control;

/* The Full Exchange Page */
typedef struct ExchangePage {
    uint8_t req_buf[P9_REQUEST_SIZE];
    P9Control control; /* Fits in gap between Request and Reply */
    uint8_t padding[P9_REPLY_OFFSET - (P9_REQUEST_SIZE + sizeof(P9Control))];
    uint8_t rep_buf[P9_REPLY_SIZE];
} ExchangePage;

/* --- Global Shared State --- */

ExchangePage *g_page;
int g_running = 1;

/* --- Helpers --- */

void random_delay() {
    /* Random tiny delay to provoke races */
    if (rand() % 10 == 0) usleep(1);
}

/* --- Userspace Client Simulation --- */

void* client_thread(void* arg) {
    int iterations = 100;
    
    printf("[Client] Starting...\n");

    for (int i = 0; i < iterations; i++) {
        /* 
         * STATE: IDLE
         * Client has ownership of Request Buffer and Control Write fields 
         */
        
        /* 1. Wait for IDLE state (sanity check) */
        while (atomic_load(&g_page->control.status, ORDER_RELAXED) != P9_STATUS_IDLE) {
            random_delay(); /* Spin */
        }

        /* 2. Prepare Request */
        snprintf((char*)g_page->req_buf, 64, "Request #%d payload data", i);
        g_page->control.req_tail = 64; // Update length
        g_page->control.req_seq = i;
        
        /* 3. Ring Doorbell (Signal Kernel) */
        /* Release semantics: Ensures all previous writes (req_buf) are visible before doorbell=1 */
        atomic_store(&g_page->control.doorbell, 1, ORDER_RELEASE);
        
        // printf("[Client] Sent request #%d\n", i);

        /* 
         * STATE: USER_REQUEST_PENDING -> KERNEL_PROCESSING 
         * Client yields ownership. Must NOT touch buffers.
         */

        /* 4. Wait for Reply (Spin on Status) */
        /* Acquire semantics: Ensures we see writes made by Kernel before status=COMPLETE */
        while (atomic_load(&g_page->control.status, ORDER_ACQUIRE) != P9_STATUS_COMPLETE) {
            random_delay(); 
        }

        /* 
         * STATE: KERNEL_REPLY_READY
         * Client regains read ownership of Reply Buffer
         */

        /* 5. Process Reply */
        char* reply_data = (char*)g_page->rep_buf;
        int reply_val = atoi(reply_data + 7); /* "Reply #<n>" */
        
        if (reply_val != i) {
            printf("FATAL: Data corruption! Expected %d got %d\n", i, reply_val);
            exit(1);
        }

        /* 6. Acknowledge / Reset to IDLE */
        atomic_store(&g_page->control.status, P9_STATUS_IDLE, ORDER_RELAXED);
    }

    printf("[Client] Finished.\n");
    g_running = 0;
    return NULL;
}

/* --- Kernel Server Simulation --- */

void* kernel_thread(void* arg) {
    printf("[Kernel] Starting...\n");

    while (g_running) {
        /* 
         * STATE: IDLE / Wait for Doorbell
         */
        
        /* 1. Poll Doorbell */
        /* Acquire semantics: Syncs with Client's release of doorbell */
        if (atomic_load(&g_page->control.doorbell, ORDER_ACQUIRE) == 1) {
            
            /* 
             * STATE: KERNEL_PROCESSING_REQUEST
             * Kernel gains ownership. 
             */
            
            /* 2. Update Status to PENDING */
            atomic_store(&g_page->control.doorbell, 0, ORDER_RELAXED);
            atomic_store(&g_page->control.status, P9_STATUS_PENDING, ORDER_RELAXED);
            
            /* 3. Read Request */
            char* req_data = (char*)g_page->req_buf;
            // printf("[Kernel] Processing: '%s'\n", req_data);
            
            /* Simulate work */
            random_delay();

            /* 4. Write Reply */
            int seq = g_page->control.req_seq;
            snprintf((char*)g_page->rep_buf, 64, "Reply #%d", seq);
            
            /* 6. Set Status COMPLETE (Signal Client) */
            /* Release semantics: Ensures reply data visible before status=COMPLETE */
            atomic_store(&g_page->control.status, P9_STATUS_COMPLETE, ORDER_RELEASE);
        } else {
            usleep(10); /* Prevent tight loop burning CPU */
        }
    }
    printf("[Kernel] Stopped.\n");
    return NULL;
}

int main() {
    pthread_t t_cli, t_kern;

    /* Allocate Page (Zeroed) */
    g_page = calloc(1, sizeof(ExchangePage));
    if (!g_page) return 1;

    printf("SIP Model Checker\n");
    printf("=================\n");
    printf("This program simulates the SIP protocol state machine.\n");
    printf("Run with: valgrind --tool=helgrind ./tools/sip_model\n\n");

    /* Create Threads */
    pthread_create(&t_kern, NULL, kernel_thread, NULL);
    pthread_create(&t_cli, NULL, client_thread, NULL);

    /* Wait for completion */
    pthread_join(t_cli, NULL);
    
    /* Tell kernel to stop */
    g_running = 0;
    pthread_join(t_kern, NULL);

    free(g_page);
    return 0;
}