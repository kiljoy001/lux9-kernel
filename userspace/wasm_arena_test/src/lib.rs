#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

// Test 1: Progressive allocation (should succeed up to ~1MB)
#[no_mangle]
pub extern "C" fn test_progressive_alloc() -> u32 {
    // Allocate in 64KB chunks (WASM page size)
    // Budget is 1MB = 16 pages
    // Should succeed for first 16 allocations

    let mut pages_allocated = 0;

    for _i in 0..20 {
        let result = unsafe { core::arch::wasm32::memory_grow(0, 1) };

        if result == usize::MAX {
            // Allocation failed - expected after ~16 pages
            return pages_allocated; // Return how many succeeded
        }

        pages_allocated += 1;
    }

    return pages_allocated;
}

// Test 2: Single large allocation (should fail if > 1MB)
#[no_mangle]
pub extern "C" fn test_large_alloc() -> u32 {
    // Try to allocate 20 pages (1.25MB) at once
    // Should FAIL (budget is only 1MB = 16 pages)

    let result = unsafe { core::arch::wasm32::memory_grow(0, 20) };

    if result == usize::MAX {
        return 1; // SUCCESS: Limit enforced
    }

    return 0; // FAILURE: Allowed to exceed limit
}

// Test 3: Exact budget allocation (should succeed)
#[no_mangle]
pub extern "C" fn test_exact_budget() -> u32 {
    // Allocate exactly 16 pages (1MB)
    // Should succeed (matches initial budget)

    let result = unsafe { core::arch::wasm32::memory_grow(0, 16) };

    if result == usize::MAX {
        return 0; // FAILURE: Rejected exact budget
    }

    return 1; // SUCCESS: Accepted exact budget
}

// Test 4: Refill trigger test
#[no_mangle]
pub extern "C" fn test_refill() -> u32 {
    // Allocate 13 pages (812.5KB)
    // This should drop below low water mark (256KB)
    // Kernel should auto-refill from process bank

    // First allocation: 13 pages
    let result1 = unsafe { core::arch::wasm32::memory_grow(0, 13) };
    if result1 == usize::MAX {
        return 10; // ERROR: First allocation failed
    }

    // Now at ~200KB remaining (below low_water = 256KB)
    // Next allocation should trigger refill

    // Second allocation: 4 pages (should succeed after refill)
    let result2 = unsafe { core::arch::wasm32::memory_grow(0, 4) };
    if result2 == usize::MAX {
        return 20; // ERROR: Refill didn't work
    }

    return 1; // SUCCESS: Refill triggered and worked
}

// Test 5: Token conservation check
// This would require a kernel syscall to read arena state
// For now, we rely on kernel debug output
#[no_mangle]
pub extern "C" fn test_conservation() -> u32 {
    // Allocate then deallocate
    // Kernel should log token counts

    // Allocate 8 pages
    let result1 = unsafe { core::arch::wasm32::memory_grow(0, 8) };
    if result1 == usize::MAX {
        return 0;
    }

    // TODO: Add syscall to deallocate WASM pages
    // For now, this tests allocation path only

    return 1;
}
