/* wasm_arena.h - WASM Linear Memory Arena Manager
 *
 * Manages the virtual address space reservation for WASM linear memories.
 * Each WASM process gets a dedicated slot in the WASM memory region.
 * This provides hardware isolation and simplifies pointer compression/checking.
 */

#ifndef WASM_ARENA_H
#define WASM_ARENA_H

#include "../include/dat.h"

/*
 * Arena Parameters:
 * - 4TB total reserved user space for WASM (0x0000_1000_0000_0000 start?)
 *   Actually, lets stick to the existing layout in wasm_runtime.c for now but
 * managed better.
 * - Slot size: 8GB
 * - Max slots: 1024 (8TB total)
 */

#define WASM_ARENA_SLOT_SIZE (8ULL * 1024 * 1024 * 1024)
#define WASM_ARENA_MAX_SLOTS 1024

/* Initialize the arena allocator */
void wasm_arena_init(void);

/* Allocate a linear memory slot for a process */
/* Returns base address or 0 on failure */
uintptr wasm_arena_alloc_slot(Proc *p);

/* Free a linear memory slot */
void wasm_arena_free_slot(uintptr base);

#endif /* WASM_ARENA_H */
