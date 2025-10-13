#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

extern uintptr saved_limine_hhdm_offset;

void mmu_map(uintptr *pml4, uintptr va, uintptr pa, uintptr size, uintptr flags)
{
    uintptr *pte;
    uintptr end;

    pml4 = (uintptr*)((uintptr)pml4 + saved_limine_hhdm_offset);

    va = PPN(va);
    pa = PPN(pa);
    end = va + size;

    while (va < end) {
        pte = mmuwalk(pml4, va, 0, 1);
        if (pte == nil)
            panic("mmu_map: out of memory");
        *pte = pa | flags | PTEVALID;
        va += BY2PG;
        pa += BY2PG;
    }
}