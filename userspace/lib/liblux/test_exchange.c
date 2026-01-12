/*
 * Simple test program for exchange pool integration with liblux
 * This demonstrates basic usage of the exchange pool syscalls
 */

#include "inc/lux.h"

// Minimal test without stdio dependencies
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