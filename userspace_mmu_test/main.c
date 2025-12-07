#include <stdio.h>
#include <stdlib.h> // For malloc, free
#include "kernel_mock.h"

// Global variable definitions (declared extern in kernel_mock.h)
Proc _up;
Mach _m;
uintptr limine_kernel_phys_base = 0x200000ULL; // Mock value, assuming kernel loaded at 2MB physical
uintptr saved_limine_hhdm_offset = VMAP; // Mock value
uintptr hhdm_base = VMAP; // Mock value for kaddr/paddr logic
uintptr mock_pml4_table[512] __attribute__((aligned(BY2PG)));

// Extern declarations for functions defined in other mock .c files
extern uintptr* mmuwalk(uintptr *table, uintptr va, int level, int create);
extern void pmap(uintptr pa, uintptr va, vlong size);
extern void punmap(uintptr pa, vlong size);
extern uintptr* getpte(uintptr va);
extern Segment* newseg(int type, uintptr base, ulong size);
extern void putseg(Segment *s);
extern Page* newpage(uintptr va, Image *image);
extern void free_page_mock(Page *p);

int main() {
    printf("Userspace MMU test harness running.\n");

    // Initialize mock Mach and Proc structures
    m->machno = 0;
    m->pml4 = mock_pml4_table; // Assign mock PML4 to global 'm'
    m->mmufree = nil;
    m->mmucount = 0;
    m->saved_limine_hhdm_offset = VMAP; // Initialize for kaddr/paddr
    m->havenx = 1; // Assume NX support

    up->pml4 = mock_pml4_table; // Assign mock PML4 to global 'up'
    up->mmuhead = nil;
    up->mmutail = nil;
    up->mmucount = 0;
    up->kp = 0; // Not a kernel process
    up->pid = 1;
    up->text = "init";

    // Clear the mock PML4 table
    memset(mock_pml4_table, 0, PTSZ);

    // --- Test Case: Simulate newseg for user stack, page allocation, and mapping ---
    printf("\n--- Test Case: Simulate newseg for user stack ---\n");
    uintptr stack_base = 0x7ffffdfff000ULL;
    ulong stack_size_pages = 4096 / BY2PG; // 4KB, which is 1 page
    
    Segment *stack_seg = newseg(SG_STACK | SG_NOEXEC, stack_base, stack_size_pages);
    if (stack_seg == nil) {
        panic("Test: newseg for stack failed");
    }
    printf("newseg for stack created: base=%#llx top=%#llx\n", (uvlong)stack_seg->base, (uvlong)stack_seg->top);

    // Simulate page allocation and mapping into the segment (this is where fixfault calls newpage and segpage)
    printf("--- Simulating segpage for stack page at VA %#llx ---\n", (uvlong)0x7ffffeffe000ULL);
    Page *p = newpage(0x7ffffeffe000ULL, nil); // va=0x7ffffeffe000 from log
    if (p == nil) {
        panic("Test: newpage for stack page failed");
    }
    printf("newpage allocated PA=%#llx VA=%#llx\n", (uvlong)p->pa, (uvlong)p->va);
    
    // Call segpage, which internally calls pmap
    segpage(stack_seg, p);
    printf("segpage completed for stack page.\n");

    // Verify mapping by trying to walk the page table
    uintptr *pte_ptr = mmuwalk(m->pml4, 0x7ffffeffe000ULL, PTE_LEVEL, 0);
    if (pte_ptr == nil || (*pte_ptr & PTEVALID) == 0) {
        printf("Error: Stack page not correctly mapped!\n");
        // For valgrind, this indicates a problem, but it might not be a direct memory error.
        // We can add an assert here later if needed.
    } else {
        printf("Success: Stack page mapped correctly. PTE: %#llx\n", (uvlong)*pte_ptr);
    }

    // Simulate cleanup (important for leak detection with Valgrind)
    // In a full test, we'd also unmap and free pages
    putseg(stack_seg);
    free_page_mock(p); // free mock Page struct

    printf("\nUserspace MMU test harness finished.\n");
    return 0;
}
