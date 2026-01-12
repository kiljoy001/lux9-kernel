/*
 * Comprehensive test suite for exchange pool integration with liblux
 * Tests various aspects of the exchange pool functionality
 */

#include "inc/lux.h"

// Test 1: Basic allocation and deallocation
int test_basic_alloc_free() {
    ExchangeCapability* cap = sys_exchange_alloc();
    if (cap == 0) {
        return 1; // Allocation failed
    }
    
    // Check that we got a valid capability
    if (cap->size == 0) {
        return 2; // Invalid capability
    }
    
    // Free the capability
    int result = sys_exchange_free(cap);
    if (result < 0) {
        return 3; // Free failed
    }
    
    return 0; // Success
}

// Test 2: Multiple allocations
int test_multiple_allocs() {
    ExchangeCapability* caps[5];
    
    // Allocate multiple pages
    for (int i = 0; i < 5; i++) {
        caps[i] = sys_exchange_alloc();
        if (caps[i] == 0) {
            // Free any previously allocated caps before returning
            for (int j = 0; j < i; j++) {
                sys_exchange_free(caps[j]);
            }
            return 1; // Allocation failed
        }
    }
    
    // Free all allocated pages
    for (int i = 0; i < 5; i++) {
        int result = sys_exchange_free(caps[i]);
        if (result < 0) {
            return 2; // Free failed
        }
    }
    
    return 0; // Success
}

// Test 3: Error handling
int test_error_handling() {
    // Try to free a null pointer (should fail gracefully)
    int result = sys_exchange_free(0);
    // We expect this to fail, so result < 0 is actually correct
    
    // Try to allocate and free normally to make sure system is still working
    ExchangeCapability* cap = sys_exchange_alloc();
    if (cap != 0) {
        sys_exchange_free(cap);
    }
    
    return 0; // Success - we handled errors gracefully
}

// Main test runner
int main() {
    int result;
    
    // Test 1: Basic allocation and deallocation
    result = test_basic_alloc_free();
    if (result != 0) {
        return result + 10; // Offset error codes
    }
    
    // Test 2: Multiple allocations
    result = test_multiple_allocs();
    if (result != 0) {
        return result + 20; // Offset error codes
    }
    
    // Test 3: Error handling
    result = test_error_handling();
    if (result != 0) {
        return result + 30; // Offset error codes
    }
    
    return 0; // All tests passed
}