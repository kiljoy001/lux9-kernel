/*
 * Userspace Test for Page-Flipping Ring Architecture (v3.0)
 * Compile with: gcc -I userspace/include -g userspace/test_ring_flip.c -o test_ring_flip
 * Run with: valgrind ./test_ring_flip
 */

#define _GNU_SOURCE
#include <stdint.h>

/* Hack to satisfy glibc headers if __uint32_t is missing */
typedef uint32_t __uint32_t;

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* 
 * Shim for Plan 9 types to avoid u.h conflicts with Linux headers.
 * We define _U_H_ to prevent ipc_ring.h from including the real u.h.
 */
#define _U_H_ 
typedef uint8_t  u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef int8_t   s8int;
typedef int16_t  s16int;
typedef int32_t  s32int;
typedef int64_t  s64int;
typedef uintptr_t uintptr;

/* ipc_ring.h includes "u.h" but we blocked it */
#include "ipc_ring.h"

#define PAGE_SIZE 4096

/* Mock 9P Message Type */
#define TWRITE 118
#define RWRITE 119

struct NineP_Header {
    u32int size;
    u8int  type;
    u16int tag;
    /* payload follows */
} __attribute__((packed));

/* Shared Memory State */
struct IpcChannel *channel;
void *page_pool[16]; /* Simulate physical pages */

/* 
 * Initialize the ring and pages
 */
void setup() {
    /* Allocate Control Page */
    channel = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!channel) { perror("aligned_alloc"); exit(1); }
    memset(channel, 0, PAGE_SIZE);
    channel->magic = 0x52494E47;
    channel->submission.mask = RING_MASK;
    channel->completion.mask = RING_MASK;

    /* Allocate Batch Pages */
    for (int i = 0; i < 16; i++) {
        page_pool[i] = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!page_pool[i]) { perror("aligned_alloc page"); exit(1); }
        memset(page_pool[i], 0, PAGE_SIZE);
    }
    
    printf("Setup complete: Control Page at %p\n", channel);
}

/*
 * Producer (User)
 * Fills a page with messages and pushes to Submission Ring.
 */
void user_send_batch(int page_idx, int count) {
    struct BatchHeader *batch = page_pool[page_idx];
    u8int *cursor;
    
    /* Reset Batch Header */
    memset(batch, 0, sizeof(BatchHeader));
    batch->magic = BATCH_PAGE_MAGIC;
    batch->num_messages = 0;
    batch->used_bytes = BATCH_DATA_START;
    
    cursor = (u8int*)batch + BATCH_DATA_START;
    
    for (int i = 0; i < count; i++) {
        char payload[64];
        sprintf(payload, "Message %d content for page %d", i, page_idx);
        int payload_len = strlen(payload) + 1;
        
        u32int msg_size = sizeof(struct NineP_Header) + payload_len;
        u16int slot_len = msg_size;
        
        /* Check capacity */
        if (batch->used_bytes + 2 + slot_len > PAGE_SIZE) {
            printf("User: Page full at msg %d\n", i);
            break;
        }
        
        /* Write Framing Length */
        *(u16int*)cursor = slot_len;
        cursor += 2;
        
        /* Write 9P Header */
        struct NineP_Header *h = (struct NineP_Header*)cursor;
        h->size = msg_size;
        h->type = TWRITE;
        h->tag = i;
        
        /* Write Payload */
        memcpy(cursor + sizeof(struct NineP_Header), payload, payload_len);
        
        cursor += slot_len;
        batch->used_bytes += 2 + slot_len;
        batch->num_messages++;
    }
    
    /* Push to Ring */
    u32int tail = channel->submission.tail;
    channel->submission.pages[tail & RING_MASK] = (u64int)(uintptr_t)batch; /* Virtual Address */
    
    /* Memory Barrier would go here */
    channel->submission.tail++;
    
    // printf("User: Pushed page %d with %d messages (bytes: %d)\n", page_idx, batch->num_messages, batch->used_bytes);
}

/*
 * Consumer (Kernel)
 * Reads from Submission, validates, processes, pushes to Completion.
 */
void kernel_process() {
    u32int head = channel->submission.head;
    u32int tail = channel->submission.tail;
    
    while (head != tail) {
        u64int page_addr = channel->submission.pages[head & RING_MASK];
        struct BatchHeader *batch = (struct BatchHeader*)(uintptr_t)page_addr;
        
        /* 1. Validate Page */
        assert(batch != NULL);
        if (batch->magic != BATCH_PAGE_MAGIC) {
            printf("Kernel: Bad Magic! %x\n", batch->magic);
            /* Drop or Error */
            head++;
            continue;
        }
        
        // printf("Kernel: Processing batch. %d messages.\n", batch->num_messages);
        
        /* 2. Iterate Messages */
        u8int *cursor = (u8int*)batch + BATCH_DATA_START;
        int safe_offset = BATCH_DATA_START;
        
        for (int i = 0; i < batch->num_messages; i++) {
            /* Bounds Check 1 */
            if (safe_offset + 2 > PAGE_SIZE) {
                printf("Kernel: Overflow check 1 hit\n");
                break;
            }
            
            u16int msg_len = *(u16int*)cursor;
            cursor += 2;
            safe_offset += 2;
            
            /* Bounds Check 2 */
            if (safe_offset + msg_len > PAGE_SIZE) {
                printf("Kernel: Overflow check 2 hit\n");
                break;
            }
            
            struct NineP_Header *h = (struct NineP_Header*)cursor;
            /* Validate 9P Consistency */
            if (h->size != msg_len) {
                printf("Kernel: Size mismatch fram=%d 9p=%d\n", msg_len, h->size);
            }
            
            cursor += msg_len;
            safe_offset += msg_len;
        }
        
        /* 3. Return to Completion */
        u32int c_tail = channel->completion.tail;
        channel->completion.pages[c_tail & RING_MASK] = page_addr;
        channel->completion.tail++;
        
        head++;
    }
    channel->submission.head = head;
}

/*
 * User (Recycle)
 * Checks Completion Ring.
 */
void user_check_completion() {
    u32int head = channel->completion.head;
    u32int tail = channel->completion.tail;
    
    while (head != tail) {
        /* u64int page_addr = */ channel->completion.pages[head & RING_MASK];
        /* Mark page as free in our pool logic (not implemented here) */
        head++;
    }
    channel->completion.head = head;
}

int main() {
    printf("=== Ring Buffer Page-Flip Test ===\n");
    
    setup();
    
    /* Test 1: Fill and Send */
    user_send_batch(0, 50); /* 50 messages in page 0 */
    
    /* Test 2: Kernel Process */
    kernel_process();
    
    /* Test 3: User Recycle */
    user_check_completion();
    
    /* Test 4: Full Ring - Wrap Around */
    printf("\n--- Stress Test (1000000 batches) ---\n");
    for (int i = 0; i < 1000000; i++) {
        user_send_batch(i % 16, 10);
        kernel_process();
        user_check_completion();
        if (i % 100000 == 0) printf(".");
        fflush(stdout);
    }
    printf("\n");
    
    /* Cleanup */
    free(channel);
    for (int i = 0; i < 16; i++) free(page_pool[i]);
    
    printf("\n=== Test Complete ===\n");
    return 0;
}
