#include <stdio.h>
#include <stdlib.h> // For malloc, free
#include "kernel_mock.h"

// Global variable definitions (declared extern in kernel_mock.h)
Proc _up;
Mach _m;
uintptr limine_kernel_phys_base = 0x200000ULL; // Mock value, assuming kernel loaded at 2MB physical
uintptr saved_limine_hhdm_offset = VMAP; // Mock value
uintptr hhdm_base = VMAP; // Mock value for kaddr/paddr logic
uintptr mock_pml4_table[512] __attribute__((aligned(BY2PG))); /* legacy; unused after root alloc */

static void
init_proc(Proc *p, const char *text, uint32_t pid)
{
    uintptr pa = rampage();
    void *host = lookup_mock_host_va(pa);
    if(host == nil) panic("init_proc: host nil");

    p->pml4 = (uintptr*)host;
    p->pml4_pa = pa;
    memset(host, 0, PTSZ);
    p->mmuhead = p->mmutail = nil;
    p->mmucount = 0;
    p->kp = 0;
    p->pid = pid;
    p->text = text;
}

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
    m->mmufree = nil;
    m->mmucount = 0;
    m->saved_limine_hhdm_offset = VMAP; // Initialize for kaddr/paddr
    m->havenx = 1; // Assume NX support

    init_proc(up, "init", 1);
    m->pml4 = up->pml4;
    m->pml4_pa = up->pml4_pa;
    /* Second proc to test mmuswitch */
    Proc other = {0};
    init_proc(&other, "other", 2);

    // --- Test Case: Simulate newseg for user stack, page allocation, and mapping ---
    printf("\n--- Test Case: Simulate newseg for user stack ---\n");
    uintptr stack_base = 0x7ffffeffe000ULL; // match the VA we map below
    ulong stack_size_pages = 4096 / BY2PG; // 4KB, which is 1 page
    
    Segment *stack_seg = newseg(SG_STACK | SG_NOEXEC, stack_base, stack_size_pages);
    if (stack_seg == nil) {
        panic("Test: newseg for stack failed");
    }
    printf("newseg for stack created: base=%#lx top=%#lx\n", (ulong)stack_seg->base, (ulong)stack_seg->top);

    // Simulate page allocation and mapping into the segment (this is where fixfault calls newpage and segpage)
    printf("--- Simulating segpage for stack page at VA %#lx ---\n", (ulong)0x7ffffeffe000ULL);
    Page *p = newpage(0x7ffffeffe000ULL, nil); // va=0x7ffffeffe000 from log
    if (p == nil) {
        panic("Test: newpage for stack page failed");
    }
    printf("newpage allocated PA=%#lx VA=%#lx\n", (ulong)p->pa, (ulong)p->va);
    
    // Call segpage, which internally calls pmap
    segpage(stack_seg, p);
    printf("segpage completed for stack page.\n");

    // Verify mapping by trying to walk the page table
    uartprintf("main: Before mmuwalk, mock_pml4_table[%d] = %#llx\n",
        PTLX(0x7ffffeffe000ULL, PML4E), (uvlong)mock_pml4_table[PTLX(0x7ffffeffe000ULL, PML4E)]);

    uintptr *pte_ptr = mmuwalk(m->pml4, 0x7ffffeffe000ULL, PTE_LEVEL, 0);
    if (pte_ptr == nil || (*pte_ptr & PTEVALID) == 0) {
        printf("Error: Stack page not correctly mapped!\n");
        // For valgrind, this indicates a problem, but it might not be a direct memory error.
        // We can add an assert here later if needed.
    } else {
        printf("Success: Stack page mapped correctly. PTE: %#lx\n", (ulong)*pte_ptr);
    }

    /* Demand-fault another user VA to exercise fault() and CR2 */
    uintptr fault_va = 0x7ffffefff000ULL;
    if(fault(fault_va, 0, 1) == 0){
        uintptr *pte_fault = mmuwalk(m->pml4, fault_va, PTE_LEVEL, 0);
        if(pte_fault && (*pte_fault & PTEVALID))
            printf("fault() mapped VA=%#lx, PTE=%#lx, CR2=%#lx\n",
                   (ulong)fault_va, (ulong)*pte_fault, (ulong)getcr2());
        else
            printf("fault() failed to install mapping for VA=%#lx\n", (ulong)fault_va);
    }else{
        printf("fault() returned error for VA=%#lx\n", (ulong)fault_va);
    }

    // Simulate cleanup (important for leak detection with Valgrind)
    // In a full test, we'd also unmap and free pages
    putseg(stack_seg);
    free_page_mock(p); // free mock Page struct
    free_mock_phys(up->pml4_pa);
    free_mock_phys(other.pml4_pa);

    /* Simulate a user entry/exit and CR3 changes across procs */
    mock_reset_tlb_flushes();
    enter_user(up, 0, 0);
    enter_kernel();
    enter_user(&other, 0, 0);
    enter_kernel();
    printf("TLB flushes after switches: %d\n", mock_get_tlb_flushes());
    printf("Final mode (0=kernel,1=user): %d\n", mock_get_in_user());

    /* Hammer test: map/unmap a bunch of pages */
    uintptr hammer_base = 0x7fffef000000ULL;
    for(int i = 0; i < 32; i++){
        uintptr va = hammer_base + i*BY2PG;
        Page *hp = newpage(va, nil);
        pmap(hp->pa | PTEWRITE | PTEUSER, va, BY2PG);
        punmap(va, BY2PG);
        free_page_mock(hp);
    }

    printf("\nUserspace MMU test harness finished.\n");
    return 0;
}
