#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "security.h"

// Forward declarations
typedef struct RingBuf RingBuf;

/*
 * Core ring buffer structure
 */
struct RingBuf {
    void **slots;              // Ring buffer slots
    uint32_t size;             // Power of 2 size
    uint32_t mask;             // Size - 1 for fast modulo
    volatile uint32_t head;    // Producer index
    volatile uint32_t tail;    // Consumer index
    
    // Security layers
    HMACKey hmac_key;          // HMAC authentication
    BorrowTracker borrow;      // Memory ownership tracking
    PebbleChain pebble;        // Pebble chain verification
    
    // Performance counters
    uint64_t produced;
    uint64_t consumed;
    uint64_t dropped;
    uint64_t security_violations;
    uint64_t total_operations;
    
    // Metadata
    char name[32];
    bool initialized;
};

/*
 * Ring buffer operations
 */
int ringbuf_create(RingBuf *buf, uint32_t size, const char *name);
void ringbuf_destroy(RingBuf *buf);
int ringbuf_enqueue(RingBuf *buf, void *item);
int ringbuf_enqueue_secure(RingBuf *buf, void *item, uint64_t owner_id, 
                          uint64_t pebble_token, SecurityContext *ctx);
void* ringbuf_dequeue(RingBuf *buf);
void* ringbuf_dequeue_secure(RingBuf *buf, SecurityContext *ctx, uint64_t *auth_tag_out);
bool ringbuf_is_empty(const RingBuf *buf);
bool ringbuf_is_full(const RingBuf *buf);
uint32_t ringbuf_available(const RingBuf *buf);

/*
 * Statistics and debugging
 */
void ringbuf_print_stats(const RingBuf *buf);
void ringbuf_reset_stats(RingBuf *buf);

/*
 * Utility functions
 */
uint64_t ringbuf_get_performance_ns(const RingBuf *buf);

#endif /* RINGBUF_H */