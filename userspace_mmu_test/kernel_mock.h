#ifndef KERNEL_MOCK_H
#define KERNEL_MOCK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memset, memcpy
#include <stdarg.h> // For va_list in uartprintf

// --- Mock Constants ---
#define BY2PG 0x1000 // 4KB page size
#define PTSZ BY2PG  // Page table size (one page)
#define MiB 0x100000ULL // 1MB
#define GiB (1024ULL * MiB) // 1GB

// x86-64 Paging definitions - using 0-indexed levels
#define PTLX(v, l) (((v) >> (12 + (l) * 9)) & 0x1FF) // Page Table Level X index
#define PML4E 3 // PML4 is at level 3
#define PDPE 2  // PDPT is at level 2
#define PDE 1   // PD is at level 1
#define PTE_LEVEL 0 // PT is at level 0

// PTE Flags - simplified for userspace context
#define PTEVALID (1ULL << 0)
#define PTEWRITE (1ULL << 1)
#define PTEUSER (1ULL << 2)
#define PTESIZE (1ULL << 7) // 1 for 2MB/1GB pages, 0 for 4KB pages
#define PTEGLOBAL (1ULL << 8)
#define PTEACCESSED (1ULL << 5)
#define PTEDIRTY (1ULL << 6)
#define PTENOEXEC (1ULL << 63) // Simplified: Actual bit depends on EFER.NXE

#define KZERO 0xFFFFFFFF80000000ULL // Start of kernel virtual address space
#define USTKTOP 0x0000800000000000ULL // Top of user stack (e.g., 128TB into VA space)
#define VMAP 0xFFFF800000000000ULL // Higher-half direct map start, simplified

// Segment types (simplified)
#define SG_STACK 0x400
#define SG_TEXT 0x20
#define SG_DATA 0x1
#define SG_NOEXEC 0x2
#define SG_RONLY 0x4

#define PPN(x)          ((x) & ~(BY2PG-1)) // Physical Page Number (removes page offset)
#define nil NULL // Define nil for consistency

// Forward declare structs before functions that use them
typedef struct Proc Proc;
typedef struct Segment Segment;
typedef struct Image Image;
typedef struct MMU MMU;
typedef struct Mach Mach;
typedef struct Page Page; 

// --- Mock Types ---
typedef uint64_t ulong;
typedef uint64_t u64int; // Added definition for u64int
typedef long long vlong;   // Added definition for vlong
typedef uint64_t uvlong;
typedef uint64_t uintptr;
typedef uint8_t uchar;
typedef ulong Pte; // Page Table Entry type, simplify to ulong for now

// Now define the structs in dependency order
// MMU must be defined before Proc and Mach
struct MMU {
    MMU *next;
    void *alloc; // Pointer to allocated memory for the page table
    uintptr *page; // Aligned pointer to the page table (points into alloc)
    int index;
    int level;
};

struct Proc {
    uintptr *pml4; // Mock PML4 for this process
    MMU *mmuhead; // Head of MMU structures
    MMU *mmutail; // Tail of MMU structures
    int mmucount;
    int kp; // is kernel process (0 for user)
    const char *text; // for panic msgs
    uint32_t pid;     // for panic msgs
};

struct Segment {
    uintptr base; // Virtual base address
    ulong size;   // in pages
    ulong ref;
    uintptr top;  // Virtual top address
    void *map;    // Placeholder for PTE array
    int mapsize;
    int type;     // Segment type (SG_STACK, SG_TEXT, SG_DATA)
    struct Image *image;  // Pointer to Image struct
};

struct Image {
    ulong ref;
    uintptr qid_path; // Simplified identifier
    struct Segment *s; // Pointer to text Segment
    void *c; // Pointer to Chan (mock)
    void *hash; // For hash table (mock)
    int nattach;
    int notext;
    void *link; // For idle list (mock)
    void *next;
    ulong pgref;
};

struct Mach {
    int machno;
    uintptr *pml4; // Current PML4 being used
    MMU *mmufree; // Free list for MMU structures
    int mmucount;
    int havenx;
    uintptr saved_limine_hhdm_offset; // Limine HHDM offset
};

struct Page {
    uintptr pa; // Physical address
    uintptr *va; // Virtual address (HHDM for kernel)
};


// --- Mock Global Variables (analogous to kernel globals) ---
// --- Mock Global Variables (analogous to kernel globals) ---
extern Proc _up; // Mock 'up' global variable
extern Mach _m;  // Mock 'm' global variable
#define up (&_up)
#define m (&_m)

extern uintptr limine_kernel_phys_base; // Mock value, assuming kernel loaded at 2MB physical
extern uintptr saved_limine_hhdm_offset; // Mock value
extern uintptr hhdm_base; // Mock value for kaddr/paddr logic

extern uintptr mock_pml4_table[512]; // Declare this in header


// --- Mocked Kernel Functions ---

// Placeholder for error handling
void __attribute__((noreturn)) panic(const char *fmt, ...);

// Console output
void uartputs(char *s, int len);
void uartprintf(char *fmt, ...);

// Memory allocation mocks
// Simplistic page allocator for userspace. Replaces rampage.
extern uintptr rampage(void); // Mock physical page allocation

extern struct Page* newpage(uintptr va, struct Image *image); // Extern for newpage
extern void free_page_mock(struct Page *p); // Extern for free_page_mock

void *mallocz(size_t size, int zero);

// MMU related mocks (for kaddr/paddr)
extern void* kaddr(uintptr pa); // Extern because it's in mmu_mock.c
extern uintptr paddr(void *v);   // Extern because it's in mmu_mock.c
extern uintptr hhdm_virt(uintptr pa); // Extern because it's in mmu_mock.c
extern uintptr hhdm_phys(uintptr va); // Extern because it's in mmu_mock.c

// mmuwalk, mmucreate, getpte, pmap, punmap are implemented in mmu_mock.c
extern uintptr* mmuwalk(uintptr *table, uintptr va, int level, int create);
extern uintptr* mmucreate(uintptr *table, uintptr va, int level, int index);
extern uintptr* getpte(uintptr va);
extern void pmap(uintptr pa, uintptr va, vlong size);
extern void punmap(uintptr va, vlong size);

// Segment mocks (from segment_mock.c)
extern Segment* newseg(int type, uintptr base, ulong size);
extern void putseg(Segment *s);
extern void segpage(Segment *s, Page *p);


// Lock mocks (no-op for single-threaded Valgrind test)
#define lock(x) ((void)(x))
#define unlock(x) ((void)(x))
#define qlock(x) ((void)(x))
#define qunlock(x) ((void)(x))
// Corrected incref/decref to take a pointer to ulong
#define incref(x) ((*x)++)
#define decref(x) ((*x)--)

// Placeholder for other kernel functions
#define ROUND(x, y) (((x) + (y) - 1) & ~((y) - 1)) // Generic round up

#endif // KERNEL_MOCK_H