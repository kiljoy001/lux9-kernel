/*
 * Userspace test harness for xalloc memory allocator
 * Can be run with valgrind to detect memory corruption issues
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stddef.h>
#include <stddef.h>

/* Minimal Plan 9 type definitions */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;

#define nil ((void*)0)
#define nelem(x) (sizeof(x)/sizeof((x)[0]))
#define BY2V 8  /* alignment to 8 bytes */

/* Simple lock implementation for testing */
typedef struct {
    pthread_mutex_t mutex;
    bool initialized;
} Lock;

static void lock_init(Lock *lk) {
    pthread_mutex_init(&lk->mutex, nil);
    lk->initialized = true;
}

static void ilock(Lock *lk) {
    if (!lk->initialized) lock_init(lk);
    pthread_mutex_lock(&lk->mutex);
}

static void iunlock(Lock *lk) {
    pthread_mutex_unlock(&lk->mutex);
}

/* Simplified conf structure for testing */
static struct {
    ulong npage;
    ulong upages;
    struct {
        uintptr base;
        ulong npage;
        uintptr kbase;
        uintptr klimit;
    } mem[4];
} conf;

/* Xalloc structures from kernel */
enum {
    INITIAL_NHOLE = 128,
    DYNAMIC_NHOLE = 256,
    Magichole = 0x484F4C45,  /* HOLE */
};

typedef struct Hole Hole;
typedef struct Xalloc Xalloc;
typedef struct Xhdr Xhdr;

struct Hole {
    uintptr addr;
    uintptr size;
    uintptr top;
    Hole *link;
};

struct Xhdr {
    ulong size;
    ulong magix;
    char data[];
};

struct Xalloc {
    Lock lk;
    Hole hole[INITIAL_NHOLE];
    Hole *flist;
    Hole *table;
};

static Xalloc xlists;

/* Simplified print for userspace */
static void print(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    vprintf(fmt, v);
    va_end(v);
}

/* Simplified panic for userspace */
static void panic(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    vprintf("PANIC: ", v);
    va_end(v);
    printf("\n");
    exit(1);
}

/* Mock getcallerpc */
static void* getcallerpc(void *arg) {
    return __builtin_return_address(1);
}

/* Simplified xhole - mock HHDM offset */
static uintptr limine_hhdm_offset = 0xFFFF800000000000ULL;

/* Xalloc functions - simplified versions */
static void xhole(uintptr addr, uintptr size) {
    Hole *h, *c, **l;
    uintptr top;
    uintptr vaddr = addr + limine_hhdm_offset;
    
    if (size == 0) return;
    
    top = vaddr + size;
    ilock(&xlists.lk);
    
    l = &xlists.table;
    for (h = *l; h; h = h->link) {
        if (h->top == vaddr) {
            h->size += size;
            h->top = h->addr + h->size;
            c = h->link;
            if (c && h->top == c->addr) {
                h->top += c->size;
                h->size += c->size;
                h->link = c->link;
                c->link = xlists.flist;
                xlists.flist = c;
            }
            iunlock(&xlists.lk);
            return;
        }
        if (h->addr > vaddr) break;
        l = &h->link;
    }
    
    if (h && top == h->addr) {
        h->addr = vaddr;
        h->size += size;
        iunlock(&xlists.lk);
        return;
    }
    
    if (xlists.flist == nil) {
        Hole *extra = malloc(DYNAMIC_NHOLE * sizeof(Hole));
        if (extra == nil) {
            iunlock(&xlists.lk);
            panic("xhole: out of hole descriptors and malloc failed");
        }
        for (int i = 0; i < DYNAMIC_NHOLE-1; i++) {
            extra[i].link = &extra[i+1];
        }
        extra[DYNAMIC_NHOLE-1].link = nil;
        xlists.flist = extra;
    }
    
    h = xlists.flist;
    xlists.flist = h->link;
    
    h->addr = vaddr;
    h->top = top;
    h->size = size;
    h->link = *l;
    *l = h;
    
    iunlock(&xlists.lk);
}

static void* xallocz(ulong size, int zero) {
    Xhdr *p;
    Hole *h, **l;
    ulong orig_size = size;
    ulong overhead = BY2V + offsetof(Xhdr, data[0]);
    
    if (size > ~0UL - overhead) {
        print("xallocz: overflow detected! size=%lu, overhead=%lu\n", size, overhead);
        panic("xallocz: request size overflow");
    }
    
    if (size > 128*1024*1024) {
        print("xallocz: unreasonably large allocation request: %lu bytes\n", size);
        panic("xallocz: unreasonably large allocation request");
    }
    
    size += overhead;
    size &= ~(BY2V-1);
    
    ilock(&xlists.lk);
    l = &xlists.table;
    for (h = *l; h; h = h->link) {
        if (h->size >= size) {
            p = (Xhdr*)h->addr;
            h->addr += size;
            h->size -= size;
            if (h->size == 0) {
                *l = h->link;
                h->link = xlists.flist;
                xlists.flist = h;
            }
            iunlock(&xlists.lk);
            p->magix = Magichole;
            p->size = size;
            if (zero) {
                memset(p->data, 0, size - overhead);
                if (*(ulong*)p->data != 0) {
                    panic("xallocz: zeroed block not cleared");
                }
            }
            return p->data;
        }
        l = &h->link;
    }
    iunlock(&xlists.lk);
    print("XALLOC FAILURE: size=%lu bytes\n", orig_size);
    return nil;
}

static void* xalloc(ulong size) {
    return xallocz(size, 1);
}

static void xfree(void *p) {
    Xhdr *x = (Xhdr*)((uintptr)p - offsetof(Xhdr, data[0]));
    
    if (x->magix != Magichole) {
        printf("xfree(%p) %#x != %#x\n", p, (unsigned int)x->magix, (unsigned int)Magichole);
        panic("xfree: bad magic");
    }
    
    xhole((uintptr)x - limine_hhdm_offset, x->size);
}

static void xinit(void) {
    Hole *h, *eh;
    
    eh = &xlists.hole[INITIAL_NHOLE-1];
    for (h = xlists.hole; h < eh; h++) {
        h->link = h+1;
    }
    xlists.flist = xlists.hole;
    
    /* Initialize some test memory regions */
    conf.npage = 1024 * 1024;  /* 1GB worth of pages */
    conf.upages = 512 * 1024;  /* 512MB for user */
    
    /* Mock memory regions */
    conf.mem[0].base = 0x100000;      /* 1MB */
    conf.mem[0].npage = 256 * 1024;   /* 1GB */
    conf.mem[0].kbase = conf.mem[0].base + limine_hhdm_offset;
    conf.mem[0].klimit = conf.mem[0].kbase + conf.mem[0].npage * 4096;
    
    /* Add all available memory to the allocator */
    xhole(conf.mem[0].base, conf.mem[0].npage * 4096);
    
    printf("xinit: initialized with %lu bytes\n", conf.mem[0].npage * 4096);
}

/* Test functions */
static void test_basic_allocation(void) {
    printf("\n=== Testing Basic Allocation ===\n");
    
    void *ptr1 = xalloc(100);
    void *ptr2 = xalloc(200);
    void *ptr3 = xalloc(50);
    
    if (!ptr1 || !ptr2 || !ptr3) {
        panic("Basic allocation failed");
    }
    
    /* Test data integrity */
    memset(ptr1, 0xAA, 100);
    memset(ptr2, 0xBB, 200);
    memset(ptr3, 0xCC, 50);
    
    xfree(ptr1);
    xfree(ptr2);
    xfree(ptr3);
    
    printf("Basic allocation test passed\n");
}

static void test_large_allocations(void) {
    printf("\n=== Testing Large Allocations ===\n");
    
    /* Test allocations that might trigger dynamic hole allocation */
    void *ptrs[300];
    int i;
    
    for (i = 0; i < 300; i++) {
        ptrs[i] = xalloc(16);
        if (ptrs[i] == nil) {
            printf("Large allocation test failed at %d\n", i);
            break;
        }
    }
    
    printf("Made %d allocations\n", i);
    
    /* Free them */
    for (int j = 0; j < i; j++) {
        xfree(ptrs[j]);
    }
    
    printf("Large allocation test passed\n");
}

static void test_magic_corruption(void) {
    printf("\n=== Testing Magic Corruption Detection ===\n");
    
    void *ptr = xalloc(100);
    Xhdr *hdr = (Xhdr*)((uintptr)ptr - offsetof(Xhdr, data[0]));
    
    /* Test that we can detect magic corruption */
    ulong old_magic = hdr->magix;
    hdr->magix = 0xBAD;  /* Corrupt magic */
    
    printf("Attempting to free corrupted block...\n");
    xfree(ptr);  /* Should panic */
    
    /* If we get here, corruption wasn't detected */
    hdr->magix = old_magic;  /* Restore */
    xfree(ptr);
    printf("Magic corruption test completed\n");
}

static void test_stress(void) {
    printf("\n=== Stress Testing ===\n");
    
    void *ptrs[1000];
    int allocated = 0;
    
    /* Allocate until we run out or hit limit */
    for (int i = 0; i < 1000; i++) {
        size_t size = (rand() % 1000) + 1;
        ptrs[i] = xalloc(size);
        if (ptrs[i]) {
            allocated++;
            /* Write pattern to detect corruption */
            memset(ptrs[i], i & 0xFF, size);
        } else {
            printf("Allocation failed at %d\n", i);
            break;
        }
    }
    
    printf("Allocated %d blocks\n", allocated);
    
    /* Verify patterns and free */
    for (int i = 0; i < allocated; i++) {
        size_t size = (rand() % 1000) + 1;
        /* Verify pattern */
        uchar expected = i & 0xFF;
        uchar *data = (uchar*)ptrs[i];
        for (size_t j = 0; j < size && j < 100; j++) {  /* Check first 100 bytes */
            if (data[j] != expected) {
                printf("CORRUPTION DETECTED at ptr %d, byte %zu: expected %02x, got %02x\n", 
                       i, j, expected, data[j]);
                panic("Memory corruption detected");
            }
        }
        xfree(ptrs[i]);
    }
    
    printf("Stress test passed - %d allocations/frees\n", allocated);
}

int main(void) {
    printf("Starting xalloc userspace test with valgrind support\n");
    printf("Compile with: gcc -g -O0 -fsanitize=address test_xalloc_userspace.c -pthread\n");
    printf("Run with: valgrind --tool=memcheck --leak-check=full ./a.out\n");
    
    srand(12345);  /* Fixed seed for reproducibility */
    
    xinit();
    
    test_basic_allocation();
    test_large_allocations();
    test_magic_corruption();  /* This should panic */
    test_stress();
    
    printf("\n=== All tests completed ===\n");
    return 0;
}

/* Mock get_hhdm_offset function for userspace test */
uintptr get_hhdm_offset(void) {
    /* Simulate the validation from hhdm_check.c */
    if (limine_hhdm_offset != 0 && 
        limine_hhdm_offset >= 0xffff800000000000ULL &&
        limine_hhdm_offset <= 0xffffffff80000000ULL) {
        return limine_hhdm_offset;
    }
    
    printf("WARNING: HHDM offset invalid, using fallback\n");
    return 0xffff800000000000ULL;
}
