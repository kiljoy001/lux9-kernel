/* wasm_arena.c - WASM Linear Memory Arena Implementation
 *
 * Manages allocation of 8GB slots for WASM linear memories.
 * Uses a bitmap allocator to track used slots.
 */

#include "../include/wasm_arena.h"
#include "../include/fns.h"
#include "../include/mem.h"
#include "../include/portlib.h"

static Lock arena_lock;
static u32int slot_bitmap[WASM_ARENA_MAX_SLOTS / 32];

/* Helper to find free bit */
static int find_free_bit(void) {
  for (int i = 0; i < WASM_ARENA_MAX_SLOTS / 32; i++) {
    if (slot_bitmap[i] != 0xFFFFFFFF) {
      for (int j = 0; j < 32; j++) {
        if (!((slot_bitmap[i] >> j) & 1)) {
          return i * 32 + j;
        }
      }
    }
  }
  return -1;
}

void wasm_arena_init(void) { memset(slot_bitmap, 0, sizeof(slot_bitmap)); }

uintptr wasm_arena_alloc_slot(Proc *p) {
  lock(&arena_lock);
  int slot = find_free_bit();
  if (slot < 0) {
    unlock(&arena_lock);
    return 0;
  }

  /* Mark used */
  slot_bitmap[slot / 32] |= (1 << (slot % 32));
  unlock(&arena_lock);

  /* Calculate base address */
  /* Use the same logic as wasm_runtime.c relative to USTKTOP initially */
  /* WASM region ends at USTKTOP - USTKSIZE */
  uintptr region_size = (uintptr)WASM_ARENA_MAX_SLOTS * WASM_ARENA_SLOT_SIZE;
  uintptr base = USTKTOP - USTKSIZE - region_size;

  /* Add slot offset + guard */
  uintptr slot_addr = base + (uintptr)slot * WASM_ARENA_SLOT_SIZE +
                      4 * BY2PG; // WASM_LINEAR_GUARD

  return slot_addr;
}

void wasm_arena_free_slot(uintptr base) {
  if (base == 0)
    return;

  uintptr region_size = (uintptr)WASM_ARENA_MAX_SLOTS * WASM_ARENA_SLOT_SIZE;
  uintptr arena_start = USTKTOP - USTKSIZE - region_size;

  if (base < arena_start)
    return;

  /* Reverse calc slot */
  /* Subtract guard page offset */
  uintptr relative = base - arena_start - 4 * BY2PG;
  int slot = (int)(relative / WASM_ARENA_SLOT_SIZE);

  if (slot < 0 || slot >= WASM_ARENA_MAX_SLOTS)
    return;

  lock(&arena_lock);
  slot_bitmap[slot / 32] &= ~(1 << (slot % 32));
  unlock(&arena_lock);
}
