// Benchmark secure wipe performance: 7 vs 35 passes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MB (1024*1024)
#define TEST_SIZE (100*MB)  // 100MB test

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void wipe_7pass(uint8_t *addr, size_t size) {
    // DoD 5220.22-M ECE standard (7 passes)
    // Pass 1: 0x00
    memset(addr, 0x00, size);
    // Pass 2: 0xFF
    memset(addr, 0xFF, size);
    // Pass 3: random
    for(size_t i = 0; i < size; i++)
        addr[i] = rand() & 0xFF;
    // Pass 4: 0x00
    memset(addr, 0x00, size);
    // Pass 5: 0xFF
    memset(addr, 0xFF, size);
    // Pass 6: random
    for(size_t i = 0; i < size; i++)
        addr[i] = rand() & 0xFF;
    // Pass 7: 0x00
    memset(addr, 0x00, size);

    // Memory barrier
    __asm__ __volatile__ ("mfence" ::: "memory");
}

void wipe_35pass(uint8_t *addr, size_t size) {
    // Gutmann method (35 passes)
    static const uint8_t patterns[] = {
        0x55, 0xAA, 0x92, 0x49, 0x24, 0x6D, 0xB6, 0xDB,
        0x49, 0x92, 0x24, 0x6D, 0xB6, 0xDB, 0x92, 0x49,
        0x24, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
        0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    // Random passes
    for(int pass = 0; pass < 4; pass++) {
        for(size_t i = 0; i < size; i++)
            addr[i] = rand() & 0xFF;
    }

    // Pattern passes
    for(int pass = 0; pass < 27; pass++) {
        memset(addr, patterns[pass % (sizeof(patterns))], size);
    }

    // Final random passes
    for(int pass = 0; pass < 4; pass++) {
        for(size_t i = 0; i < size; i++)
            addr[i] = rand() & 0xFF;
    }

    // Memory barrier
    __asm__ __volatile__ ("mfence" ::: "memory");
}

int main(void) {
    uint8_t *test_area = malloc(TEST_SIZE);
    if(!test_area) {
        printf("Failed to allocate %d MB\n", TEST_SIZE/MB);
        return 1;
    }

    srand(time(NULL));

    printf("Benchmarking secure wipe on %d MB\n\n", TEST_SIZE/MB);

    // Benchmark 7-pass wipe
    printf("7-Pass Wipe (DoD 5220.22-M):\n");
    memset(test_area, 0xAA, TEST_SIZE); // Initialize with pattern

    uint64_t start = rdtsc();
    wipe_7pass(test_area, TEST_SIZE);
    uint64_t end = rdtsc();

    uint64_t cycles_7 = end - start;
    double sec_7 = (double)cycles_7 / 2.4e9; // Assume 2.4 GHz CPU
    printf("  Cycles: %lu\n", cycles_7);
    printf("  Time: %.3f seconds\n", sec_7);
    printf("  Throughput: %.1f MB/s\n\n", TEST_SIZE/(sec_7*MB));

    // Benchmark 35-pass wipe
    printf("35-Pass Wipe (Gutmann):\n");
    memset(test_area, 0xAA, TEST_SIZE); // Initialize with pattern

    start = rdtsc();
    wipe_35pass(test_area, TEST_SIZE);
    end = rdtsc();

    uint64_t cycles_35 = end - start;
    double sec_35 = (double)cycles_35 / 2.4e9;
    printf("  Cycles: %lu\n", cycles_35);
    printf("  Time: %.3f seconds\n", sec_35);
    printf("  Throughput: %.1f MB/s\n\n", TEST_SIZE/(sec_35*MB));

    // Comparison
    printf("Performance comparison:\n");
    printf("  35-pass is %.1fx slower than 7-pass\n", (double)cycles_35/cycles_7);
    printf("  7-pass overhead: %.3f sec per 100MB\n", sec_7);
    printf("  35-pass overhead: %.3f sec per 100MB\n", sec_35);
    printf("  Additional time for 35-pass: %.3f sec\n", sec_35 - sec_7);

    free(test_area);
    return 0;
}
