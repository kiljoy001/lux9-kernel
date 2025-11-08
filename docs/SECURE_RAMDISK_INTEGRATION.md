# Secure Ramdisk Integration with Stub Functions

## Overview

This document describes the relationship between the basic stub function implementations and the advanced secure ramdisk feature planned for integration with the pebbeling system.

## Current State

### Basic Ramdisk Implementation
The current `ramdiskinit()` stub in `kernel/9front-pc64/globals.c:314` provides minimal functionality:
```c
void ramdiskinit(void) {}
```

The full implementation will be ported from `9/port/sdram.c:125-165` and will provide:
- Block device interface (`/dev/ramdisk0`, `/dev/ramdisk1`, etc.)
- Kernel parameter configuration (`ramdisk0=size`)
- Mountable storage for filesystems
- Integration with Plan 9's device system

## Secure Ramdisk Evolution Path

### Phase 1: Basic Ramdisk (Current Focus)
```c
// After porting from 9front
void ramdiskinit(void) {
    // Parse kernel parameters
    // Create block devices
    // Register with device system
    // Provide standard ramdisk functionality
}
```

### Phase 2: Secure Extensions (Future Work)
```c
// Enhanced implementation
void ramdiskinit(void) {
    // Standard ramdisk functionality
    secure_ramdiskinit();  // NEW: Secure storage extensions
}

void secure_ramdiskinit(void) {
    // Create isolated secure areas
    // Initialize encryption if enabled
    // Set up access control mechanisms
    // Integrate with pebble budget system
    // Configure auto-wipe functionality
}
```

## Relationship to Other Stub Functions

### Supporting Infrastructure

#### i8042reset() → Security Hardware Initialization
- Provides keyboard/mouse controller reset
- May be needed for secure input handling
- Supports secure boot processes

#### bootscreeninit() → Secure Display
- Initializes boot display
- Could be extended for secure visual feedback
- May support secure graphics operations

#### vmxshutdown() → Secure VM Management  
- Handles VMX shutdown sequences
- Important for secure virtualization
- Cleans up secure contexts properly

#### links() → System Integration
- Critical for boot sequence completion
- Must work before secure features can initialize
- Foundation for all advanced functionality

## Integration Points with Pebbling System

### Resource Management
```c
// Current pebble system
struct PebbleBudget {
    usize total_budget;
    usize used_budget;
    // ... other fields
};

// Enhanced for secure storage
struct PebbleBudget {
    usize total_budget;
    usize used_budget;
    usize secure_budget;        // NEW: Secure storage budget
    SecureRamdisk *secure_rd;   // NEW: Associated secure area
    // ... existing fields
};
```

### Allocation Interface Evolution

#### Standard Pebble Allocation
```c
// Existing functionality
void* pebble_alloc(Proc *p, usize size);
void pebble_free(Proc *p, void *ptr);
```

#### Secure Pebble Allocation (Future)
```c
// Extended functionality
void* pebble_alloc(Proc *p, usize size);                    // Standard
void* pebble_secure_alloc(Proc *p, usize size, int flags);  // NEW: Secure
int pebble_secure_share(Proc *src, Proc *dst, void *ptr);   // NEW: Sharing
void pebble_secure_free(Proc *p, void *ptr);                // NEW: Secure free
```

## Boot Sequence Integration

### Current Boot Flow
```
main_after_cr3()
├── mmuinit()
├── device initialization
├── chandevreset()
├── ramdiskinit()           # Basic ramdisk
├── environment setup
└── userinit()
```

### Enhanced Boot Flow (Future)
```
main_after_cr3()
├── mmuinit()
├── device initialization
├── chandevreset()
├── ramdiskinit()           # Standard + secure ramdisk
├── secure_system_init()    # NEW: Security subsystem
├── environment setup
└── userinit()
```

## Configuration Evolution

### Current Simple Configuration
```
# Kernel parameters
ramdisk0=128M               # Basic ramdisk size
```

### Enhanced Secure Configuration (Future)
```
# Extended parameters
ramdisk0=128M               # Standard ramdisk
secure.ramdisk.size=64M     # Secure area size
secure.ramdisk.encrypt=1    # Enable encryption
secure.ramdisk.audit=1      # Enable audit logging
pebble.secure.budget=32M    # Default secure budget per process
```

## Development Sequence

### Step 1: Get Basic Functions Working
1. Port `ramdiskinit()` from 9front
2. Ensure standard block device creation works
3. Verify basic filesystem mounting
4. Test with existing initrd integration

### Step 2: Verify Integration Points
1. Test with init0 process initialization  
2. Ensure chandevinit() includes ramdisk devices
3. Verify devroot integration
4. Confirm initrd_register() still works

### Step 3: Add Secure Extensions (Future)
1. Extend ramdiskinit() with secure functionality
2. Create SecureRamdisk structures
3. Integrate with pebble budget system
4. Add process lifecycle hooks
5. Implement access control mechanisms

## API Compatibility

### Legacy Compatibility
```c
// Existing code continues to work unchanged
ramdiskinit();  // Creates standard ramdisk devices
```

### Enhanced API (Future)
```c
// New functionality available when needed
ramdiskinit();              // Standard + secure capability
pebble_secure_alloc();      // Secure allocation API
secure_rd_mount();          // Secure filesystem mounting
```

## Security Considerations

### Memory Safety Requirements
- Secure ramdisk memory must never be swapped
- All allocations must support secure wipe
- Access controls must prevent unauthorized sharing
- Process termination must trigger secure cleanup

### Integration Safety
- Basic ramdisk functionality must work independently
- Secure extensions should be optional enhancements
- Error handling must degrade gracefully
- Security features should not break existing functionality

## Testing Approach

### Phase 1 Testing (Current)
- Basic ramdisk device creation
- Block device read/write operations
- Filesystem mounting and access
- Integration with existing initrd

### Phase 2 Testing (Future)  
- Secure allocation/deallocation
- Access control verification
- Process lifecycle integration
- Security audit functionality
- Performance impact measurement

## Conclusion

The basic stub functions provide the essential foundation for system functionality, while the secure ramdisk represents an advanced enhancement that builds upon this stable base. The evolution from basic to secure functionality maintains backward compatibility while providing enhanced security features for sensitive operations.

This staged approach ensures system stability while enabling the development of sophisticated security features that leverage Plan 9's unique architectural advantages.