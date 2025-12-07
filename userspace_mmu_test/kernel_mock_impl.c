#include "kernel_mock.h"

// --- Mock Physical to Host Virtual Address Mapping ---
#define MAX_MOCK_PHYS_MAP_ENTRIES 1024 // Max number of mock physical pages

typedef struct {
    uintptr mock_pa;
    void *host_va;
    void *host_base; /* original malloc pointer for freeing */
} MockPhysMapEntry;

static MockPhysMapEntry mock_phys_to_host_map[MAX_MOCK_PHYS_MAP_ENTRIES];
static int mock_phys_map_count = 0;

/* Mock CPU state */
static uintptr mock_cr2;
static uintptr mock_cr3;
static uintptr mock_fs_base;
static uintptr mock_gs_base;
static uintptr mock_kernel_gs_base;
static int mock_in_user;
static int mock_tlb_flushes;

// Function to add a mapping
void add_mock_phys_map(uintptr mock_pa, void *host_va, void *host_base) {
    if (mock_phys_map_count >= MAX_MOCK_PHYS_MAP_ENTRIES) {
        panic("Mock physical map exhausted!");
    }
    mock_phys_to_host_map[mock_phys_map_count].mock_pa = mock_pa;
    mock_phys_to_host_map[mock_phys_map_count].host_va = host_va;
    mock_phys_to_host_map[mock_phys_map_count].host_base = host_base;
    mock_phys_map_count++;
}

// Function to lookup host_va from mock_pa
void* lookup_mock_host_va(uintptr mock_pa) {
    for (int i = 0; i < mock_phys_map_count; i++) {
        if (mock_phys_to_host_map[i].mock_pa == mock_pa) {
            return mock_phys_to_host_map[i].host_va;
        }
    }
    return nil; // Not found
}

void free_mock_phys(uintptr mock_pa) {
    for (int i = 0; i < mock_phys_map_count; i++) {
        if (mock_phys_to_host_map[i].mock_pa == mock_pa) {
            if (mock_phys_to_host_map[i].host_base)
                free(mock_phys_to_host_map[i].host_base);
            /* compact array */
            mock_phys_to_host_map[i] = mock_phys_to_host_map[mock_phys_map_count-1];
            mock_phys_map_count--;
            return;
        }
    }
}


// --- Mocked Kernel Functions Implementations (Moved from kernel_mock.h) ---

// Placeholder for error handling
void __attribute__((noreturn)) panic(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "KERNEL PANIC: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

// Console output
void uartputs(char *s, int len) {
    fwrite(s, 1, len, stdout);
}

void uartprintf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// Memory allocation mocks
// Simplistic page allocator for userspace. Replaces rampage.
static uintptr mock_physical_memory_base = 0x100000000ULL; // Start physical at 4GB
static uintptr next_mock_physical_page = 0x100000000ULL;
static uintptr mock_page_pool_end = 0x100000000ULL + (1024ULL * MiB); // 1GB of mock physical RAM

uintptr rampage(void) {
    if (next_mock_physical_page + BY2PG > mock_page_pool_end) {
        panic("rampage: Mock physical memory exhausted!");
    }
    uintptr mock_pa = next_mock_physical_page;
    next_mock_physical_page += BY2PG;

    /* Allocate slightly larger so we can return a page-aligned slice */
    void *host_base = mallocz(BY2PG + BY2PG, 1);
    if (host_base == nil) {
        panic("rampage: mallocz returned nil for host_va");
    }
    void *host_va = (void*)ROUND((uintptr)host_base, BY2PG);
    add_mock_phys_map(mock_pa, host_va, host_base); // Store the mapping

    return mock_pa; // Return the unique mock physical address
}

void *mallocz(size_t size, int zero) {
    void *ptr = malloc(size);
    if (zero && ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

// MMU related mocks (for kaddr/paddr)
// Simplified HHDM: directly subtract HHDM base
uintptr hhdm_virt(uintptr pa) {
    if (pa == 0) return 0;
    return pa + hhdm_base; // hhdm_base is 0xFFFF800000000000ULL
}

void* get_host_va_from_mock_pa(uintptr mock_pa) {
    void *host_va = lookup_mock_host_va(mock_pa);
    if (host_va == nil) {
        panic("get_host_va_from_mock_pa: No host_va mapping found for mock_pa %#llx", (uvlong)mock_pa);
    }
    return host_va;
}

uintptr hhdm_phys(uintptr va) {
    // This function is trickier as it needs to reverse lookup host_va -> mock_pa.
    // For now, only handle direct HHDM range for kernel images / known physical addresses.
    // This is primarily used by paddr in mmu_mock.c
    if (va >= hhdm_base && va < hhdm_base + (256ULL*GiB)) { // Max 256GB HHDM mapping in mock
        return va - hhdm_base;
    }
    // For allocated mock pages, we need to find their original mock_pa
    for (int i = 0; i < mock_phys_map_count; i++) {
        if ((uintptr)mock_phys_to_host_map[i].host_va == va) {
            return mock_phys_to_host_map[i].mock_pa;
        }
    }

    panic("hhdm_phys: VA %#llx not in HHDM range, nor in mock map (no mock_pa mapping)", (uvlong)va);
    return 0; // Should not reach here
}

// Placeholder for other kernel functions
#define ROUND(x, y) (((x) + (y) - 1) & ~((y) - 1)) // Generic round up

/* Mock MSR/CR helpers */
uintptr getcr3(void){ return mock_cr3; }
void putcr3(uintptr val){ mock_cr3 = val; mock_tlb_flushes++; }
uintptr getcr2(void){ return mock_cr2; }
void setcr2(uintptr val){ mock_cr2 = val; }

void wrmsr(uintptr msr, uintptr val){
    /* Minimal MSR handling for FS/GS */
    if(msr == 0xC0000100){ /* IA32_FS_BASE */
        mock_fs_base = val;
    }else if(msr == 0xC0000101){ /* IA32_GS_BASE */
        mock_gs_base = val;
    }else if(msr == 0xC0000102){ /* IA32_KERNEL_GS_BASE */
        mock_kernel_gs_base = val;
    }
}

void swapgs(void){
    uintptr tmp = mock_gs_base;
    mock_gs_base = mock_kernel_gs_base;
    mock_kernel_gs_base = tmp;
}

int mock_get_tlb_flushes(void){ return mock_tlb_flushes; }
void mock_reset_tlb_flushes(void){ mock_tlb_flushes = 0; }
int mock_get_in_user(void){ return mock_in_user; }
void mock_set_mode_user(int user){ mock_in_user = user; }

void enter_user(Proc *proc, uintptr pc, uintptr sp){
    if(proc == nil) panic("enter_user: nil proc");
    mmuswitch(proc);
    putcr3(proc->pml4_pa);
    mock_in_user = 1;
    swapgs(); /* emulate swapgs before iret to user */
    (void)pc; (void)sp;
}

void enter_kernel(void){
    if(mock_in_user){
        swapgs();
        mock_in_user = 0;
    }
}
