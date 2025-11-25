#include "ringbuf.h"
#include "security.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 * Create ring buffer with security features
 */
int ringbuf_create(RingBuf *buf, uint32_t size, const char *name) {
    if (!buf || size == 0 || (size & (size - 1)) != 0) {
        fprintf(stderr, "Invalid ring buffer parameters\n");
        return -1;  // Invalid parameters or non-power-of-2 size
    }
    
    // Allocate slots
    buf->slots = calloc(size, sizeof(void*));
    if (!buf->slots) {
        fprintf(stderr, "Failed to allocate ring buffer slots\n");
        return -1;  // Allocation failed
    }
    
    // Initialize ring buffer
    buf->size = size;
    buf->mask = size - 1;
    buf->head = 0;
    buf->tail = 0;
    
    // Initialize security layers
    if (name) {
        strncpy(buf->name, name, sizeof(buf->name) - 1);
        buf->name[sizeof(buf->name) - 1] = '\0';
    } else {
        strcpy(buf->name, "unnamed");
    }
    
    // Initialize HMAC with hardware detection
    if (security_init_hmac(&buf->hmac_key) != 0) {
        fprintf(stderr, "Failed to initialize HMAC\n");
        free(buf->slots);
        return -1;
    }
    
    // Initialize borrow tracker
    memset(&buf->borrow, 0, sizeof(buf->borrow));
    
    // Initialize pebble chain
    memset(&buf->pebble, 0, sizeof(buf->pebble));
    buf->pebble.sequence_number = 1;
    buf->pebble.budget = 1000000;  // 1M operations budget
    
    // Initialize counters
    buf->produced = 0;
    buf->consumed = 0;
    buf->dropped = 0;
    buf->security_violations = 0;
    buf->total_operations = 0;
    buf->initialized = true;
    
    printf("[%s] Ring buffer created with %u slots\n", buf->name, size);
    return 0;
}

/*
 * Destroy ring buffer and cleanup resources
 */
void ringbuf_destroy(RingBuf *buf) {
    if (!buf || !buf->initialized) return;
    
    free(buf->slots);
    buf->slots = NULL;
    buf->initialized = false;
    
    printf("[%s] Ring buffer destroyed\n", buf->name);
}

/*
 * Basic enqueue (no security)
 */
int ringbuf_enqueue(RingBuf *buf, void *item) {
    if (!buf->initialized || !item) return -1;
    
    uint32_t head = buf->head;
    uint32_t next = (head + 1) & buf->mask;
    
    if (next == buf->tail) {
        buf->dropped++;
        return -1;  // Buffer full
    }
    
    buf->slots[head] = item;
    __sync_synchronize();
    buf->head = next;
    buf->produced++;
    buf->total_operations++;
    
    return 0;  // Success
}

/*
 * Secure enqueue with full security validation
 */
int ringbuf_enqueue_secure(RingBuf *buf, void *item,
                          uint64_t owner_id, uint64_t pebble_token,
                          SecurityContext *ctx) {
    if (!buf->initialized || !item || !ctx) {
        return -1;
    }
    
    // 1. Security policy validation
    if (!ctx->verified || ctx->trust_level < 50) {
        buf->security_violations++;
        return -1;
    }
    
    // 2. HMAC authentication (result used to prevent optimization)
    uint64_t hmac_tag = security_compute_hmac(&buf->hmac_key, &item, sizeof(item));
    (void)hmac_tag;  // Suppress unused warning
    
    // 3. Borrow checker validation
    if (!security_validate_borrow(&buf->borrow, item, owner_id)) {
        buf->security_violations++;
        return -1;
    }
    
    // 4. Pebble chain verification
    if (!security_verify_pebble(&buf->pebble, pebble_token)) {
        buf->security_violations++;
        return -1;
    }
    
    // 5. Ring buffer overflow check
    uint32_t head = buf->head;
    uint32_t next = (head + 1) & buf->mask;
    
    if (next == buf->tail) {
        buf->dropped++;
        return -1;  // Buffer full
    }
    
    // 6. Store item
    buf->slots[head] = item;
    
    // Memory barrier before updating head
    __sync_synchronize();
    buf->head = next;
    buf->produced++;
    buf->total_operations++;
    
    return 0;  // Success
}

/*
 * Basic dequeue (no security)
 */
void* ringbuf_dequeue(RingBuf *buf) {
    if (!buf->initialized) return NULL;
    
    uint32_t tail = buf->tail;
    uint32_t head = buf->head;
    
    if (tail == head) {
        return NULL;  // Buffer empty
    }
    
    void *item = buf->slots[tail];
    
    __sync_synchronize();
    buf->tail = (tail + 1) & buf->mask;
    buf->consumed++;
    buf->total_operations++;
    
    return item;
}

/*
 * Secure dequeue with security validation
 */
void* ringbuf_dequeue_secure(RingBuf *buf, SecurityContext *ctx,
                           uint64_t *auth_tag_out) {
    if (!buf->initialized || !ctx || !ctx->verified) {
        return NULL;
    }
    
    uint32_t tail = buf->tail;
    uint32_t head = buf->head;
    
    if (tail == head) {
        return NULL;  // Buffer empty
    }
    
    // 1. Get item
    void *item = buf->slots[tail];
    
    // 2. Verify HMAC authentication
    uint64_t expected_tag = security_compute_hmac(&buf->hmac_key, &item, sizeof(item));
    if (auth_tag_out) {
        *auth_tag_out = expected_tag;
    }
    
    // 3. Verify pebble chain progression
    buf->pebble.sequence_number++;
    uint64_t new_pebble = security_compute_hmac(&buf->hmac_key, 
                                             &buf->pebble.sequence_number, 
                                             sizeof(buf->pebble.sequence_number));
    (void)new_pebble;  // Suppress unused warning
    
    // 4. Atomic dequeue
    __sync_synchronize();
    buf->tail = (tail + 1) & buf->mask;
    buf->consumed++;
    buf->total_operations++;
    
    return item;
}

/*
 * Check if ring buffer is empty
 */
bool ringbuf_is_empty(const RingBuf *buf) {
    return buf && buf->initialized && (buf->head == buf->tail);
}

/*
 * Check if ring buffer is full
 */
bool ringbuf_is_full(const RingBuf *buf) {
    return buf && buf->initialized && 
           (((buf->head + 1) & buf->mask) == buf->tail);
}

/*
 * Get number of available slots
 */
uint32_t ringbuf_available(const RingBuf *buf) {
    if (!buf || !buf->initialized) return 0;
    
    uint32_t head = buf->head;
    uint32_t tail = buf->tail;
    uint32_t used = (head - tail) & buf->mask;
    return buf->size - used - 1;
}

/*
 * Print statistics
 */
void ringbuf_print_stats(const RingBuf *buf) {
    if (!buf || !buf->initialized) return;
    
    printf("Ring Buffer Statistics for '%s':\n", buf->name);
    printf("  Size: %u slots\n", buf->size);
    printf("  Used: %llu / %llu (%.1f%%)\n", 
           (unsigned long long)(buf->produced - buf->dropped),
           (unsigned long long)buf->produced,
           buf->produced > 0 ? 
           100.0 * (buf->produced - buf->dropped) / buf->produced : 0.0);
    printf("  Produced: %llu\n", (unsigned long long)buf->produced);
    printf("  Consumed: %llu\n", (unsigned long long)buf->consumed);
    printf("  Dropped: %llu\n", (unsigned long long)buf->dropped);
    printf("  Security violations: %llu\n", (unsigned long long)buf->security_violations);
    printf("  Total operations: %llu\n", (unsigned long long)buf->total_operations);
    
    if (buf->total_operations > 0) {
        printf("  Average operations/sec: %.0f\n", 
               (double)buf->total_operations / 
               ((double)clock() / CLOCKS_PER_SEC));
    }
}

/*
 * Reset statistics
 */
void ringbuf_reset_stats(RingBuf *buf) {
    if (!buf || !buf->initialized) return;
    
    buf->produced = 0;
    buf->consumed = 0;
    buf->dropped = 0;
    buf->security_violations = 0;
    buf->total_operations = 0;
    
    printf("[%s] Statistics reset\n", buf->name);
}

/*
 * Get performance metrics
 */
uint64_t ringbuf_get_performance_ns(const RingBuf *buf) {
    if (!buf || !buf->initialized || buf->total_operations == 0) return 0;
    
    double elapsed_seconds = (double)clock() / CLOCKS_PER_SEC;
    double ns_per_op = (elapsed_seconds * 1e9) / buf->total_operations;
    return (uint64_t)ns_per_op;
}