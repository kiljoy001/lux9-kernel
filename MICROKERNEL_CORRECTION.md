# Microkernel Architecture Correction

## The Mistake
I implemented block device drivers as kernel modules when they should be userspace processes in a 9front/Plan 9 microkernel architecture.

## Correct 9front Microkernel Architecture

### Kernel Responsibilities (Minimal)
- Memory management
- Process scheduling  
- Basic IPC/message passing
- Device file interface (/dev/sd0, etc.)

### Userspace Driver Responsibilities
- Hardware device communication
- Block device operations (read/write sectors)
- 9P protocol server for device access
- Run as dedicated driver processes

### Communication Model
```
Userspace Process ↔ 9P Protocol ↔ Kernel Device Interface ↔ Hardware
```

### What Should Be Implemented Instead

1. **Kernel Device Interface**
   - Create device files in `/dev/` 
   - Provide 9P mount points for devices
   - Minimal kernel support for device enumeration

2. **Userspace Driver Processes**
   - `sdiahci` - Userspace AHCI driver process
   - `sdide` - Userspace IDE driver process
   - Communicate with kernel via 9P
   - Handle actual hardware I/O

3. **Service Architecture**
   - Drivers start as system services
   - Bind to kernel device interfaces
   - Provide block device access to other processes

## Files to Create

### In Kernel
- Minimal device interface (already exists in devsd.c or similar)
- Device file creation support

### In Userspace  
- `/sys/src/cmd/sdiahci/` - AHCI driver process
- `/sys/src/cmd/sdide/` - IDE driver process
- 9P server implementation for each driver

## Correct Implementation Approach

1. Study existing 9front block drivers (like sd(3) in documentation)
2. Implement userspace drivers that bind to kernel device files
3. Use 9P protocol for communication
4. Provide standard block device interface to applications

This is the fundamental difference between monolithic kernels (drivers in kernel) and microkernels (drivers as services).