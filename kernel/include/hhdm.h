/* Higher-Half Direct Map utilities */
#pragma once

#include "u.h"

/* Global HHDM base offset - set during early boot */
extern uintptr hhdm_base;

/* Convert physical address to HHDM virtual address */
static inline void* hhdm_virt(uintptr pa) {
    return (void*)(hhdm_base + pa);
}

/* Convert HHDM virtual address to physical address */
static inline uintptr hhdm_phys(void* va) {
    return (uintptr)va - hhdm_base;
}

/* Check if address is in HHDM range */
static inline int is_hhdm_virt(void* va) {
    return (uintptr)va >= hhdm_base;
}