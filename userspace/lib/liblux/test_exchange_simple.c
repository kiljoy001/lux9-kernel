/*
 * Simple test program for exchange pool integration with liblux
 * This demonstrates basic usage of the exchange pool syscalls
 */

#include "inc/lux.h"

// Simple printf replacement for testing
static void simple_print(char *msg) {
    // This would normally use a syscall to print
    // For now, we'll just rely on return codes to indicate success
}

int main() {
    // Try to allocate a page from the exchange pool
    ExchangeCapability* cap = sys_exchange_alloc();
    if (cap == 0) {
        return 1; // Indicates failure
    }
    
    // Successfully allocated - now try to free
    int result = sys_exchange_free(cap);
    if (result < 0) {
        return 2; // Indicates free failure
    }
    
    return 0; // Success
}