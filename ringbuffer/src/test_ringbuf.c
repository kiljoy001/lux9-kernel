#include "ringbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>  // For usleep

#define TEST_ITERATIONS 10000
#define TEST_BUFFER_SIZE 256
#define NUM_THREADS 4

/*
 * Test data structure
 */
typedef struct TestItem {
    uint64_t id;
    uint64_t timestamp;
    char data[64];
} TestItem;

/*
 * Global test data
 */
static TestItem *test_items;
static RingBuf test_buf;
static pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t items_processed = 0;

/*
 * Create test items
 */
void create_test_items(void) {
    test_items = malloc(TEST_ITERATIONS * sizeof(TestItem));
    assert(test_items);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        test_items[i].id = i;
        test_items[i].timestamp = time(NULL) + i;
        snprintf(test_items[i].data, sizeof(test_items[i].data), "Test item %d", i);
    }
}

/*
 * Performance benchmark
 */
void benchmark_ringbuf(void) {
    printf("=== Performance Benchmark ===\n");
    
    SecurityContext ctx = {
        .process_id = 1,
        .thread_id = 1,
        .trust_level = 100,
        .capabilities = 0xFFFFFFFF,
        .verified = true
    };
    
    // Register test items with borrow checker for secure testing
    printf("Registering test items with borrow checker...\n");
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        security_register_borrow(&test_buf.borrow, &test_items[i], 1);
    }
    
    // Test 1: Basic operations (no security)
    printf("\n1. Basic ring buffer operations:\n");
    ringbuf_reset_stats(&test_buf);
    
    clock_t start = clock();
    int enqueued = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        if (ringbuf_enqueue(&test_buf, &test_items[i]) == 0) {
            enqueued++;
        }
    }
    clock_t enqueue_end = clock();
    
    int dequeued = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        void *item = ringbuf_dequeue(&test_buf);
        if (item) dequeued++;
    }
    clock_t dequeue_end = clock();
    
    double enqueue_time = (double)(enqueue_end - start) / CLOCKS_PER_SEC;
    double dequeue_time = (double)(dequeue_end - enqueue_end) / CLOCKS_PER_SEC;
    double total_time = (double)(dequeue_end - start) / CLOCKS_PER_SEC;
    
    printf("  Enqueued: %d / %d items in %.6f seconds (%.0f ops/sec)\n", 
           enqueued, TEST_ITERATIONS, enqueue_time, enqueued / enqueue_time);
    printf("  Dequeued: %d items in %.6f seconds (%.0f ops/sec)\n", 
           dequeued, dequeue_time, dequeued / dequeue_time);
    printf("  Total time: %.6f seconds\n", total_time);
    printf("  Nanoseconds per operation: %.2f\n", (total_time * 1e9) / (enqueued + dequeued));
    
    // Test 2: Secure operations
    printf("\n2. Secure ring buffer operations:\n");
    ringbuf_reset_stats(&test_buf);
    
    start = clock();
    enqueued = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        if (ringbuf_enqueue_secure(&test_buf, &test_items[i], 1, 1, &ctx) == 0) {
            enqueued++;
        }
    }
    enqueue_end = clock();
    
    dequeued = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        void *item = ringbuf_dequeue_secure(&test_buf, &ctx, NULL);
        if (item) dequeued++;
    }
    dequeue_end = clock();
    
    enqueue_time = (double)(enqueue_end - start) / CLOCKS_PER_SEC;
    dequeue_time = (double)(dequeue_end - enqueue_end) / CLOCKS_PER_SEC;
    total_time = (double)(dequeue_end - start) / CLOCKS_PER_SEC;
    
    printf("  Enqueued: %d / %d items in %.6f seconds (%.0f ops/sec)\n", 
           enqueued, TEST_ITERATIONS, enqueue_time, enqueued / enqueue_time);
    printf("  Dequeued: %d items in %.6f seconds (%.0f ops/sec)\n", 
           dequeued, dequeue_time, dequeued / dequeue_time);
    printf("  Total time: %.6f seconds\n", total_time);
    printf("  Security violations: %llu\n", (unsigned long long)test_buf.security_violations);
    printf("  Nanoseconds per operation: %.2f\n", (total_time * 1e9) / (enqueued + dequeued));
    
    // Performance report
    printf("\n3. HMAC Performance:\n");
    security_perf_report(&test_buf.hmac_key);
}

/*
 * Security violation test
 */
void test_security_violations(void) {
    printf("\n=== Security Violation Tests ===\n");
    
    TestItem bad_item;
    bad_item.id = 999;
    bad_item.timestamp = time(NULL);
    strcpy(bad_item.data, "Security test item");
    
    // Test 1: Unverified context
    printf("1. Testing unverified context:\n");
    SecurityContext unverified_ctx = {
        .process_id = 999,
        .thread_id = 999,
        .trust_level = 10,
        .capabilities = 0x0,
        .verified = false
    };
    
    int result = ringbuf_enqueue_secure(&test_buf, &bad_item, 1, 1, &unverified_ctx);
    assert(result == -1);
    printf("   ✓ Unverified context rejected\n");
    
    // Test 2: Low trust level
    printf("2. Testing low trust level:\n");
    SecurityContext low_trust_ctx = {
        .process_id = 999,
        .thread_id = 999,
        .trust_level = 20,  // Below threshold of 50
        .capabilities = 0x0,
        .verified = true
    };
    
    result = ringbuf_enqueue_secure(&test_buf, &bad_item, 1, 1, &low_trust_ctx);
    assert(result == -1);
    printf("   ✓ Low trust level rejected\n");
    
    // Test 3: Invalid owner (should succeed with trusted process)
    printf("3. Testing invalid owner (should be allowed for trusted process):\n");
    SecurityContext trusted_ctx = {
        .process_id = 1,  // Trusted process
        .thread_id = 1,
        .trust_level = 100,
        .capabilities = 0xFFFFFFFF,
        .verified = true
    };
    
    result = ringbuf_enqueue_secure(&test_buf, &bad_item, 999, 1, &trusted_ctx);
    printf("   Result: %s (trusted process allowed with any owner)\n", 
           result == 0 ? "✓ Accepted" : "✗ Rejected");
    
    printf("\nAll security violation tests completed!\n");
    printf("Total security violations: %llu\n", (unsigned long long)test_buf.security_violations);
}

/*
 * Thread worker function
 */
void* thread_worker(void *arg) {
    uint64_t thread_id = *(uint64_t*)arg;
    SecurityContext ctx = {
        .process_id = thread_id,
        .thread_id = thread_id,
        .trust_level = 100,
        .capabilities = 0xFFFFFFFF,
        .verified = true
    };
    
    uint64_t local_processed = 0;
    
    for (int i = 0; i < TEST_ITERATIONS / NUM_THREADS; i++) {
        TestItem *item = &test_items[thread_id * (TEST_ITERATIONS / NUM_THREADS) + i];
        
        // Ensure item is registered with borrow checker for this thread
        security_register_borrow(&test_buf.borrow, item, thread_id);
        
        // Enqueue with slight randomization
        int attempts = 0;
        while (ringbuf_enqueue_secure(&test_buf, item, thread_id, thread_id, &ctx) == -1 && attempts < 10) {
            attempts++;
            usleep(10);  // Brief pause if buffer full
        }
        
        // Dequeue
        void *dequeued = ringbuf_dequeue_secure(&test_buf, &ctx, NULL);
        if (dequeued) {
            local_processed++;
        }
    }
    
    pthread_mutex_lock(&test_mutex);
    items_processed += local_processed;
    pthread_mutex_unlock(&test_mutex);
    
    return NULL;
}

/*
 * Parallel processing test
 */
void test_parallel_processing(void) {
    printf("\n=== Parallel Processing Test ===\n");
    
    pthread_t threads[NUM_THREADS];
    uint64_t thread_ids[NUM_THREADS];
    
    printf("Testing with %d threads...\n", NUM_THREADS);
    
    // Reset statistics
    ringbuf_reset_stats(&test_buf);
    items_processed = 0;
    
    clock_t start = clock();
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_worker, &thread_ids[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Results:\n");
    printf("  Items processed: %llu\n", (unsigned long long)items_processed);
    printf("  Elapsed time: %.6f seconds\n", elapsed);
    printf("  Throughput: %.0f operations/sec\n", items_processed / elapsed);
    printf("  Security violations: %llu\n", (unsigned long long)test_buf.security_violations);
}

/*
 * Memory safety test
 */
void test_memory_safety(void) {
    printf("\n=== Memory Safety Test ===\n");
    printf("This test should be run with valgrind:\n");
    printf("  valgrind --leak-check=full --track-origins=yes ./test_ringbuf memory_test\n\n");
    
    // Allocate and free many items
    printf("Testing allocation/deallocation patterns...\n");
    for (int i = 0; i < 1000; i++) {
        TestItem *item = malloc(sizeof(TestItem));
        assert(item);
        
        item->id = i;
        item->timestamp = time(NULL);
        snprintf(item->data, sizeof(item->data), "Memory test %d", i);
        
        // Register with borrow checker
        security_register_borrow(&test_buf.borrow, item, 1);
        
        SecurityContext ctx = {
            .process_id = 1,
            .thread_id = 1,
            .trust_level = 100,
            .capabilities = 0xFFFFFFFF,
            .verified = true
        };
        
        int result = ringbuf_enqueue_secure(&test_buf, item, 1, 1, &ctx);
        if (result == 0) {
            void *dequeued = ringbuf_dequeue_secure(&test_buf, &ctx, NULL);
            if (dequeued == item) {
                free(item);  // Clean up
            } else {
                printf("Warning: Dequeued item doesn't match original\n");
                free(item);
            }
        } else {
            free(item);  // Clean up if enqueue failed
        }
    }
    
    printf("Memory safety test completed\n");
    printf("Final statistics:\n");
    ringbuf_print_stats(&test_buf);
}

/*
 * Usage information
 */
void print_usage(const char *program_name) {
    printf("Usage: %s [test_type]\n", program_name);
    printf("Test types:\n");
    printf("  (no args)     - Run all tests\n");
    printf("  benchmark     - Performance benchmarking only\n");
    printf("  security      - Security violation tests only\n");
    printf("  parallel      - Parallel processing test only\n");
    printf("  memory_test   - Memory safety test only\n");
    printf("  stats         - Show ring buffer statistics only\n");
}

/*
 * Main function
 */
int main(int argc, char *argv[]) {
    printf("Lux9 Ring Buffer Security Test Suite\n");
    printf("=====================================\n");
    
    // Check for help
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Create test items and register with borrow checker
    printf("Creating %d test items...\n", TEST_ITERATIONS);
    create_test_items();
    
    // Register all test items with borrow checker
    printf("Registering test items with borrow checker...\n");
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        security_register_borrow(&test_buf.borrow, &test_items[i], 1);
    }
    
    // Initialize ring buffer
    printf("Initializing ring buffer...\n");
    if (ringbuf_create(&test_buf, TEST_BUFFER_SIZE, "test_buf") != 0) {
        fprintf(stderr, "Failed to create ring buffer\n");
        free(test_items);
        return 1;
    }
    
    printf("\nHardware detection:\n");
    printf("  SHA-NI available: %s\n", security_has_sha_ni() ? "Yes" : "No");
    
    // Run tests based on arguments
    if (argc > 1) {
        if (strcmp(argv[1], "benchmark") == 0) {
            benchmark_ringbuf();
        } else if (strcmp(argv[1], "security") == 0) {
            test_security_violations();
        } else if (strcmp(argv[1], "parallel") == 0) {
            test_parallel_processing();
        } else if (strcmp(argv[1], "memory_test") == 0) {
            test_memory_safety();
        } else if (strcmp(argv[1], "stats") == 0) {
            ringbuf_print_stats(&test_buf);
            security_perf_report(&test_buf.hmac_key);
        } else {
            printf("Unknown test type: %s\n", argv[1]);
            print_usage(argv[0]);
        }
    } else {
        // Run all tests
        benchmark_ringbuf();
        test_security_violations();
        test_parallel_processing();
        printf("\nFor memory safety testing, run:\n");
        printf("  valgrind --leak-check=full --track-origins=yes ./test_ringbuf memory_test\n");
    }
    
    // Final report
    printf("\n=== Final Statistics ===\n");
    ringbuf_print_stats(&test_buf);
    security_perf_report(&test_buf.hmac_key);
    
    // Cleanup
    ringbuf_destroy(&test_buf);
    free(test_items);
    
    printf("\nTest suite completed successfully!\n");
    return 0;
}