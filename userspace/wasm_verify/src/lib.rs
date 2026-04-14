#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn run_test() -> u32 {
    // Try to grow by 20 pages (1.25MB)
    // 20 * 64KB = 1,310,720 bytes
    // Kernel limit is 1MB = 1,048,576 bytes
    // This should FAIL (return -1 / usize::MAX)
    
    let res = core::arch::wasm32::memory_grow(0, 20);
    
    if res == usize::MAX {
        return 42; // SUCCESS: Limit enforced
    }
    
    return 0; // FAILURE: Allowed to exceed limit
}
