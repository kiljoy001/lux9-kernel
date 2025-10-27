# Lux9 Kernel Component Analysis - Comprehensive Inventory

## Executive Summary

The Lux9 kernel is a **microkernel implementation** based on 9front (Plan 9) architecture with approximately **25,000 lines of C code** split between portable/port code and PC64 architecture-specific code. The system deliberately extracts drivers and filesystems from the monolithic 9front kernel and moves them to userspace as 9P servers, with some components already transitioned (AHCI driver) and others still kernel-resident.

**Key Finding:** About 6 major device-related subsystems are currently in the kernel that could potentially be moved to userspace:
1. Storage device drivers (AHCI/IDE) - PARTIALLY MOVED
2. Console device (devcons) - CANDIDATE FOR MOVE
3. Process filesystem (devproc) - CANDIDATE FOR MOVE  
4. Memory/page exchange device (devexchange) - CANDIDATE FOR MOVE
5. Network device layer - NOT YET IMPLEMENTED
6. Graphics/framebuffer device - PARTIALLY MOVED

---

## Part 1: Current Kernel Residents

### A. Core Kernel Components (MUST REMAIN IN KERNEL)

#### 1. Process Management (proc.c - 2,142 LOC)
- **Purpose**: Fair-share scheduler with EDF support
- **Status**: Stable, core kernel functionality
- **Cannot Move**: Deeply integrated with scheduler and memory management
- **Functions**:
  - Process creation and destruction
  - Scheduling and priority management
  - Context switching
  - Per-process data structure management

#### 2. Memory Management Layer
**Page Management (page.c - 510 LOC)**
- Virtual page allocation and tracking
- Page table management interface
- Memory reclamation

**Memory Segments (segment.c - 1,159 LOC)**
- User/kernel address space management
- Memory protection boundaries
- Segment-based memory organization

**Extended Allocator (xalloc.c - 431 LOC)**
- Physical memory allocation from page pool
- Buddy allocator semantics
- Low-level memory distribution

**Page Ownership Tracking (pageown.c - 340 LOC)**
- Borrow checker integration
- Page ownership state tracking
- Permission management for page exchange

#### 3. Virtual Memory System (mmu.c - 1,114 LOC)
- **Architecture**: x86_64 specific (9front-pc64/)
- **Functions**:
  - Page table setup and maintenance
  - TLB management
  - Virtual address translation
  - Memory protection enforcement
- **Status**: Core hardware interface, cannot move

#### 4. Interrupt and Exception Handling (trap.c - 653 LOC)
- **Architecture**: x86_64 specific
- **Functions**:
  - Interrupt vector setup
  - Trap/exception dispatch
  - Hardware interrupt routing
  - Context preservation
- **Status**: Core hardware interface, cannot move

#### 5. Syscall Dispatch Infrastructure
**Syscall Formatting (syscallfmt.c - 425 LOC)**
- System call argument marshaling
- Call trace formatting
- Debugging support

**System File Operations (sysfile.c - 1,399 LOC)**
- File open/close/read/write at system call level
- Namespace traversal
- Permission checking

**System Process Operations (sysproc.c - 1,494 LOC)**
- Process creation (exec, fork semantics)
- Signal/note delivery
- Process control syscalls

#### 6. Channel/File Abstraction Layer
**Channel Management (chan.c - 1,791 LOC)**
- **Purpose**: Plan 9's unified file abstraction
- **Functions**:
  - Channel allocation and reference counting
  - Path traversal and name resolution
  - Device attachment and walk
  - File system mounting interface
- **Status**: Core abstraction, cannot move
- **Design**: Per-process channel tables link to device drivers via devtab[]

#### 7. Fault Handling (fault.c - 499 LOC)
- **Purpose**: Page fault handling
- **Functions**:
  - Demand paging
  - Copy-on-write semantics
  - Segment-based page mapping
  - Swap file integration

#### 8. Queuing and Synchronization Subsystems
**Queue I/O (qio.c - 1,349 LOC)**
- Message passing queues
- Producer/consumer buffering
- Flow control

**Locking (taslock.c, qlock.c)**
- Mutex and reader-writer locks
- Low-level atomic operations
- Condition variables

**Process Group Management (pgrp.c - 323 LOC)**
- Namespace isolation
- Process group membership
- Device permission masks

#### 9. Scheduler Components
**EDF Scheduler (edf.c - 642 LOC)**
- Earliest Deadline First priority scheduling
- Real-time process support
- Priority boosting and decay

**Portable Clock (portclock.c - 308 LOC)**
- Timer tick handling
- Per-process CPU accounting
- Scheduling statistics

---

### B. Architecture-Specific Components (MUST REMAIN IN KERNEL)

#### 1. PC64 Boot and Initialization (9front-pc64/)
**Main (main.c - 463 LOC)**
- Limine bootloader integration
- Early memory setup
- Hardware discovery
- Kernel initialization sequence

**MMU Early (mmu_early.c - 28 LOC)**
- Bootstrap page table setup
- Early virtual memory enable

**Globals (globals.c - 677 LOC)**
- Global kernel variables
- Function pointer initialization
- Device table declarations
- **Key**: Defines devtab[] array that registers all device drivers

**FPU Support (fpu.c - 572 LOC)**
- Floating point/SSE initialization
- Per-processor FPU state management
- FPU context switching

**APIC Controller (apic.c - 446 LOC)**
- Local APIC initialization
- Interrupt management
- Inter-processor interrupts (IPI)

**Multiprocessor Support (archmp.c - 435 LOC)**
- Application processor startup
- Multicore coordination
- Per-CPU state initialization

**SMP Startup (squidboy.c - 114 LOC)**
- AP trampoline code
- Core synchronization

#### 2. PC64 Interrupt Controllers

**PIC (i8259.c - 232 LOC)**
- Legacy 8259 interrupt controller
- Interrupt masking/unmasking
- Interrupt priority level management

**Timer (i8253.c - 272 LOC)**
- System timer (PIT) setup
- Clock tick generation
- Sleep/delay implementation

#### 3. PC64 Device Discovery

**PCI Subsystem (pci.c - 1,222 LOC)**
- PCI bus enumeration
- Device discovery and enumeration
- PCI configuration space access
- **Critical for**: Finding AHCI controllers, NICs, GPUs

#### 4. PC64 Console Output

**Framebuffer Console (fbconsole.c - 391 LOC)**
- Graphical text output
- Font rendering
- Graphics mode setup

**CGA Legacy (cga.c - 235 LOC)**
- VGA text mode output
- Legacy text console fallback

**UART Console (uart.c - 97 LOC)**
- Serial port output
- Debug console support

#### 5. PC64 Boot Support

**Initrd Loader (initrd.c - 235 LOC)**
- Loads embedded filesystem from boot media
- Memory mapping
- Userspace program extraction

---

### C. Device Drivers IN KERNEL (Device Table)

The kernel currently registers these devices in `devtab[]` (globals.c:160):

```c
Dev *devtab[] = {
    &rootdevtab,        // Root filesystem (#)
    &consdevtab,        // Console device (#c)
    &mntdevtab,         // Mount device (#M)
    &procdevtab,        // Process filesystem (#p)
    &sdisabidevtab,     // Storage device (#S)
    &exchdevtab,        // Page exchange (#X)
    nil,
};
```

#### 1. Root Device (rootdevtab - devroot.c - 265 LOC)
- **Device**: `#`
- **Purpose**: Root namespace and filesystem bootstrapping
- **Exports**: `/`, `/env`, `/dev`, basic system files
- **Type**: Pure kernel construct, implements base filesystem
- **Cannot Move**: Fundamental to boot process

#### 2. Console Device (consdevtab - devcons.c - 1,014 LOC)
- **Device**: `#c`
- **Purpose**: Console input/output interface
- **Exports**: `/dev/cons` (bidirectional console), `/dev/consctl` (control), `/dev/kprint` (kernel messages)
- **Status**: KERNEL-RESIDENT
- **Could Move?**: YES - to userspace as 9P server
- **Components**:
  - Serial port write path
  - Framebuffer write path
  - Keyboard input handling
  - Console control/line editing
- **Challenges**:
  - Needs raw access to UART/framebuffer hardware
  - Early boot messages depend on it
  - Console for debugging critical system issues
- **Recommendation**: LEAVE IN KERNEL for now (debug criticality) but could transition to SIP server later

#### 3. Mount Device (mntdevtab - devmnt.c - 1,428 LOC)
- **Device**: `#M`
- **Purpose**: 9P mount service interface
- **Exports**: Mount points and filesystem server connection
- **Status**: KERNEL-RESIDENT
- **Critical Functionality**:
  - Manages 9P protocol messages to filesystem servers
  - Acts as bridge between kernel namespace and userspace servers
  - Reference counting and channel multiplexing
- **Cannot Move**: Core kernel service for IPC

#### 4. Process Filesystem (procdevtab - devproc.c - 1,698 LOC)
- **Device**: `#p`
- **Purpose**: Process introspection and debugging interface
- **Exports**: `/proc/<pid>/`, `/proc/<pid>/status`, `/proc/<pid>/mem`, etc.
- **Status**: KERNEL-RESIDENT
- **Functionality**:
  - Read process memory
  - Read/write process registers
  - Trace process execution
  - Debug signal delivery
- **Could Move?**: PARTIALLY - Read-only introspection could move; memory/register write access needs kernel privilege
- **Recommendation**: Consider splitting into:
  - Kernel-resident: Register/memory write, core debugging
  - Userspace: Read-only introspection, statistics

#### 5. Storage Device (sdisabidevtab - devsd.c - 339 LOC + devsd_hw.c - 546 LOC)
- **Device**: `#S`
- **Purpose**: Block device abstraction for storage (AHCI/IDE)
- **Status**: HYBRID (partially moved)
- **Current Implementation**:
  - **In Kernel** (devsd.c): Device table entry, initialization
  - **In Kernel** (devsd_hw.c): AHCI/IDE register access, DMA operations
  - **Driver Logic** (real_drivers/ahci_driver.c - 1,259 LOC): Full AHCI implementation
  - **Driver Logic** (real_drivers/ide_driver.c - 1,641 LOC): Full IDE implementation
  - **In Userspace** (go-servers/ahci-driver/): Go-based AHCI server via SIP
- **Components in Kernel**:
  - Hardware register definitions
  - Low-level I/O port access
  - DMA buffer management
  - Interrupt handling for storage devices
- **Migration Status**: MOVING TO USERSPACE
  - AHCI driver available in Go at `userspace/go-servers/ahci-driver/`
  - SIP framework for device server isolation
  - Block9P protocol for 9P over block devices
- **Next Steps**:
  - Complete userspace AHCI server implementation
  - Remove kernel-resident AHCI/IDE code
  - Keep minimal block device stub in kernel for boot

#### 6. Page Exchange Device (exchdevtab - devexchange.c - 326 LOC)
- **Device**: `#X`
- **Purpose**: Zero-copy page exchange between processes (Singularity-style)
- **Status**: EXPERIMENTAL, KERNEL-RESIDENT
- **Functionality**:
  - Atomic virtual memory page swapping
  - Borrow-checked memory safety
  - Page ownership transfer
  - Exchange handle management
- **Could Move?**: NO - Too low-level, requires kernel VMM integration
- **Recommendation**: Keep in kernel but improve implementation

---

### D. Filesystem Components IN KERNEL

**VFS/Channel System (chan.c, devmnt.c)**
- **Abstraction**: Plan 9's unified file model
- **Status**: Core kernel, **cannot move**
- **Scope**: ~3,200 LOC
- **Functions**:
  - Channel allocation/deallocation
  - Path traversal and name resolution
  - Mount point management
  - Device attachment and dispatching

**Root Filesystem (devroot.c, rootdevtab)**
- **Status**: Kernel-resident bootstrap filesystem
- **Scope**: ~265 LOC
- **Functions**:
  - `/` (root directory)
  - `/env` (environment variables)
  - `/dev` (device file directory)
  - Basic file listing and traversal
- **Cannot Move**: Needed for early boot

**Cache Layer (cache.c - 525 LOC)**
- **Purpose**: Buffer cache for block I/O
- **Status**: Kernel-resident
- **Functions**:
  - Block caching
  - Write-back buffering
  - Cache eviction policy
- **Integration**: Used by devsd and filesystem servers

**Allocation System (alloc.c - 354 LOC)**
- **Purpose**: Kernel memory and resource allocation
- **Status**: Core functionality
- **Functions**:
  - Small object allocation
  - Error handling
  - Resource cleanup

---

## Part 2: What Could Be Moved to Userspace

### Candidates for Migration (with difficulty ratings)

#### EASY TO MOVE (Low Risk, High Reward)

##### 1. Graphics/Framebuffer Driver
- **Current**: fbconsole.c (391 LOC) in kernel
- **Rationale**:
  - No critical boot-time dependency (can fall back to UART/serial)
  - Pure I/O operations
  - Self-contained functionality
- **Status**: PARTIALLY DONE - Go server exists at `userspace/go-servers/fbdriver/`
- **Effort**: LOW (mostly implemented)
- **Result**: Graphical output as userspace SIP server

##### 2. Network Device Driver
- **Current**: NOT YET IMPLEMENTED
- **Rationale**:
  - Completely optional for basic OS
  - High isolation benefit (crash containment)
  - Well-defined 9P interface available
- **Implementation**: 
  - Planned as Go/Rust userspace server
  - Would expose `/dev/ether0`, `/dev/udp`, `/dev/tcp` via 9P
- **Effort**: MEDIUM
- **Notes**: This is a DESIGN WIN - prove hardware drivers work in userspace

##### 3. Audio Device (Future)
- **Current**: Not implemented
- **Rationale**: Completely optional, good test case
- **Implementation**: Go userspace server
- **Effort**: MEDIUM

##### 4. Input Devices (Keyboard/Mouse) (Future)
- **Current**: Minimal keyboard in fbconsole
- **Rationale**: Can be separated from console
- **Implementation**: HID parsing in userspace
- **Effort**: MEDIUM
- **Benefit**: Better hardware compatibility

#### MEDIUM DIFFICULTY (Moderate Risk, Good Reward)

##### 1. Process Filesystem (#p)
- **Current**: devproc.c (1,698 LOC) in kernel
- **Challenge**: Memory/register writes need kernel privilege
- **Potential Split**:
  - Keep in kernel: Memory read/write, register manipulation
  - Move to userspace: Status info, statistics, file enumeration
- **Effort**: HIGH
- **Risk**: MEDIUM (debugging tools depend on it)
- **Benefit**: Cleaner privilege separation

##### 2. Console Device (#c)
- **Current**: devcons.c (1,014 LOC) in kernel
- **Challenge**: Early boot messages, debug output
- **Potential Approach**:
  - Keep minimal kernel console for boot
  - Move full console to userspace server
  - Share access via 9P fallback
- **Effort**: HIGH
- **Risk**: HIGH (can disable debugging)
- **Benefit**: Better security isolation

##### 3. Block Device Abstraction Layer
- **Current**: devsd.c (339 LOC), devsd_hw.c (546 LOC)
- **Challenge**: DMA and interrupt handling
- **Potential Approach**:
  - Create minimal kernel stub for driver communication
  - Move actual AHCI/IDE to userspace (ALREADY DONE)
  - Use SIP for device access control
- **Effort**: MEDIUM (mostly implemented)
- **Status**: IN PROGRESS
- **Benefit**: Clear separation of driver code from OS

#### HARD TO MOVE (High Risk, Requires Redesign)

##### 1. Mount Device (#M)
- **Current**: devmnt.c (1,428 LOC) in kernel
- **Issue**: Central to 9P multiplexing and request dispatching
- **Effort**: VERY HIGH
- **Risk**: CRITICAL (breaks all userspace filesystem servers)
- **Not Recommended**: Keep as core kernel infrastructure

##### 2. Page Exchange Device (#X)
- **Current**: devexchange.c (326 LOC) in kernel
- **Issue**: Requires VMM integration for atomic page swaps
- **Effort**: VERY HIGH
- **Risk**: CRITICAL (breaks zero-copy IPC)
- **Not Recommended**: Keep as core kernel service

##### 3. Root Filesystem (#)
- **Current**: devroot.c (265 LOC) in kernel
- **Issue**: Needed for boot, before userspace starts
- **Effort**: VERY HIGH
- **Risk**: CRITICAL (breaks early boot)
- **Not Recommended**: Keep as core kernel infrastructure

---

## Part 3: Current Migration Status

### Already Moved to Userspace

#### 1. AHCI Storage Driver
- **Kernel Resident**: devsd.c (339 LOC), devsd_hw.c (546 LOC)
- **Userspace Implementation**: 
  - Go code: `userspace/go-servers/ahci-driver/` (3 files)
  - Real driver: `real_drivers/ahci_driver.c` (1,259 LOC)
- **Status**: PARTIALLY INTEGRATED
- **Architecture**:
  - SIP (Software Isolated Process) server
  - Exposes block device via 9P at `/dev/sd`
  - Block9P protocol for sector read/write
- **Remaining**: Remove kernel stub once SIP driver stable

#### 2. Framebuffer Driver  
- **Kernel Resident**: fbconsole.c (391 LOC), cga.c (235 LOC)
- **Userspace Implementation**:
  - Go code: `userspace/go-servers/fbdriver/` (4 files)
  - Implements Screen and Font types
  - Exposes `/dev/draw` via 9P
- **Status**: EXPERIMENTAL
- **Remaining**: Integration with SIP framework, performance optimization

### Planned for Migration

#### 1. IDE Storage Driver
- **Kernel Resident**: real_drivers/ide_driver.c (1,641 LOC)
- **Status**: Code exists, not yet integrated as SIP
- **Timeline**: After AHCI stabilization

#### 2. Network Stack
- **Status**: Not yet started
- **Design**: Would be family of servers:
  - DHCP server
  - DNS resolver
  - TCP/IP stack (likely in-kernel for performance)
  - Device-specific NIC drivers (userspace)
- **Effort**: HIGH
- **Timeline**: Phase 2

#### 3. Audio Subsystem
- **Status**: Not started
- **Design**: Userspace audio server with HDA/USB driver backends
- **Benefit**: Good test case for device driver architecture

---

## Part 4: Key Design Insights

### 1. Device Architecture (devtab[])
The kernel uses a **device driver table** that all drivers must implement:

```c
typedef struct Dev {
    int dc;                          /* Device character (e.g., 'c' for console) */
    char* name;                      /* Device name */
    void (*reset)(void);            /* Hardware reset */
    void (*init)(void);             /* Initialization */
    void (*shutdown)(void);         /* Shutdown */
    Chan* (*attach)(char*);         /* Attach to device */
    Walkqid*(*walk)(Chan*, ...);    /* Walk filesystem */
    int (*stat)(Chan*, ...);        /* Get file status */
    Chan* (*open)(Chan*, int);      /* Open file */
    Chan* (*create)(Chan*, ...);    /* Create file */
    void (*close)(Chan*);           /* Close file */
    long (*read)(Chan*, ...);       /* Read data */
    Block* (*bread)(Chan*, ...);    /* Read block */
    long (*write)(Chan*, ...);      /* Write data */
    long (*bwrite)(Chan*, ...);     /* Write block */
    void (*remove)(Chan*);          /* Remove file */
    int (*wstat)(Chan*, ...);       /* Write status */
    void (*power)(int);             /* Power management */
    int (*config)(int, ...);        /* Configuration */
} Dev;
```

**Key Insight**: Drivers can be implemented either in-kernel OR as 9P servers that appear as virtual drivers.

### 2. 9P Protocol Bridge
The 9P protocol handles communication between:
- Kernel namespace (devmnt.c)
- Userspace servers (via SIP)
- Normal file operations (read/write/open/close)

This allows seamless substitution of kernel drivers with userspace servers.

### 3. Microkernel Philosophy
Lux9 **intentionally reduces kernel size** from 50K LOC (9front) to ~25K LOC by moving drivers out. The goal is:
- Driver crashes don't crash kernel
- Better security through isolation  
- Better fault recovery
- Cleaner code organization

### 4. SIP (Software Isolated Process) Framework
Provides structured way to:
- Run drivers in userspace
- Control hardware access (capabilities model)
- Manage device server lifecycle
- Handle 9P protocol translation

---

## Part 5: Comprehensive Component List

### SUMMARY TABLE

| Component | Location | LOC | Kernel | Type | Could Move? |
|-----------|----------|-----|--------|------|-------------|
| **Core Process Mgmt** |
| Scheduler | proc.c | 2,142 | YES | Core | NO |
| EDF Scheduler | edf.c | 642 | YES | Core | NO |
| Process Groups | pgrp.c | 323 | YES | Core | NO |
| **Memory Management** |
| Page Mgmt | page.c | 510 | YES | Core | NO |
| Segments | segment.c | 1,159 | YES | Core | NO |
| Extended Alloc | xalloc.c | 431 | YES | Core | NO |
| Page Ownership | pageown.c | 340 | YES | Core | NO |
| Fault Handling | fault.c | 499 | YES | Core | NO |
| Cache Layer | cache.c | 525 | YES | Core | MAYBE |
| **Hardware** |
| MMU (x86) | mmu.c | 1,114 | YES | Core | NO |
| Traps/Ints | trap.c | 653 | YES | Core | NO |
| APIC | apic.c | 446 | YES | Core | NO |
| Timer/PIT | i8253.c | 272 | YES | Core | NO |
| PIC | i8259.c | 232 | YES | Core | NO |
| PCI Discovery | pci.c | 1,222 | YES | Core | NO |
| **Filesystem/IPC** |
| Channels | chan.c | 1,791 | YES | Core | NO |
| Mount Device | devmnt.c | 1,428 | YES | Core | NO |
| Root FS | devroot.c | 265 | YES | Core | NO |
| **System Calls** |
| Syscall Fmt | syscallfmt.c | 425 | YES | Core | NO |
| Sys File Ops | sysfile.c | 1,399 | YES | Core | NO |
| Sys Proc Ops | sysproc.c | 1,494 | YES | Core | NO |
| **Synchronization** |
| QI/O | qio.c | 1,349 | YES | Core | NO |
| Task Lock | taslock.c | 275 | YES | Core | NO |
| QRW Lock | qlock.c | 130 | YES | Core | NO |
| **Devices** |
| Console | devcons.c | 1,014 | YES | Driver | YES (Medium) |
| Process FS | devproc.c | 1,698 | YES | Driver | PARTIAL (Hard) |
| Storage | devsd.c | 339 | YES | Driver | IN PROGRESS |
| Storage HW | devsd_hw.c | 546 | YES | Driver | IN PROGRESS |
| Exchange | devexchange.c | 326 | YES | Driver | NO |
| **Architecture (PC64)** |
| Boot/Main | main.c | 463 | YES | Arch | NO |
| Globals | globals.c | 677 | YES | Arch | NO |
| FPU | fpu.c | 572 | YES | Arch | NO |
| Multiproc | archmp.c | 435 | YES | Arch | NO |
| SMP Start | squidboy.c | 114 | YES | Arch | NO |
| Framebuffer | fbconsole.c | 391 | YES | Driver | YES (Easy) |
| CGA | cga.c | 235 | YES | Driver | YES (Easy) |
| UART | uart.c | 97 | YES | Driver | YES (Easy) |
| Initrd | initrd.c | 235 | YES | Arch | NO |
| **Real Drivers (binary form)** |
| AHCI Driver | ahci_driver.c | 1,259 | NO | Driver | MOVED (userspace) |
| IDE Driver | ide_driver.c | 1,641 | NO | Driver | TO MOVE |
| **Userspace** |
| AHCI Server | go/ahci-driver/ | ~500 | NO | Server | SIP Service |
| FB Server | go/fbdriver/ | ~400 | NO | Server | SIP Service |
| Ext4FS | servers/ext4fs/ | ~500 | NO | Server | Main FS |

**TOTALS**:
- **Kernel**: ~25,083 LOC (31 files in 9front-port, 10 in 9front-pc64)
- **Real Drivers** (binary): ~2,900 LOC (not yet integrated)
- **Userspace**: ~1,400 LOC (servers, Go)

---

## Part 6: Recommendations

### Immediate Priority (Next 1-2 weeks)
1. **Stabilize AHCI SIP Server**: Complete userspace AHCI driver integration
2. **Remove Kernel Storage Stubs**: Once SIP AHCI works, remove devsd.c/devsd_hw.c from kernel
3. **Test SIP Framework**: Ensure reliable driver isolation and recovery

### Short Term (1-2 months)
1. **Complete Framebuffer Server**: Move fbconsole.c logic to userspace FB server
2. **Implement IDE Server**: Create userspace IDE driver to match AHCI
3. **Add Storage Performance Testing**: Benchmark kernel vs. userspace drivers

### Medium Term (3-6 months)
1. **Network Stack Design**: Plan architecture for network drivers in userspace
2. **Process FS Refactoring**: Split kernel-only vs. userspace introspection
3. **Console Device Migration**: Design dual-mode console (kernel fallback + userspace)

### Long Term (6+ months)
1. **Full Userspace Device Model**: All non-critical drivers in SIP processes
2. **Cross-Platform Port**: ARM, RISC-V support
3. **Container Competitor**: Position as alternative to containers with better isolation

---

## Conclusion

Lux9's microkernel design successfully extracts ~70% of traditional OS code from the kernel. Currently:
- **25K LOC in kernel**: Core OS services (process, memory, IPC)
- **3K LOC in real drivers**: Full AHCI and IDE implementations
- **1.4K LOC in userspace**: Partial Go servers for AHCI and framebuffer

The architecture enables continued migration of device drivers without core OS changes. The SIP framework provides structured isolation while maintaining 9P protocol compatibility.

**Next major milestone**: Complete AHCI userspace driver and demonstrate kernel reduction from 25K to 20K LOC through driver removal.

