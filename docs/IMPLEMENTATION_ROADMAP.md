# Implementation Roadmap: Secure Ramdisk with Pebbling Integration

## Current Status

### Phase 1: Critical Build-Fixing Issues
- [x] Fix devenv.c include paths (Build blockers)
- [x] Fix newegrp memory initialization
- [x] Environment device initialization working
- [x] Kernel reaches userinit() and scheduler

### Phase 2: Missing Stub Function Porting (IN PROGRESS)
- [x] Identify all missing stub functions
- [ ] Port vmxshutdown() from 9/pc/devvmx.c
- [ ] Port i8042reset() from 9/pc/devkbd.c
- [ ] Port bootscreeninit() from 9/pc/screen.c
- [ ] Port ramdiskinit() from 9/port/sdram.c
- [ ] Locate/implement missing links() function

## Phased Implementation Plan

### Phase 1: Critical Stability (Completed)
**Duration**: 2-3 days
**Priority**: CRITICAL
**Deliverables**:
- Fixed include paths to enable builds
- Memory safety fixes for critical bugs
- Working kernel boot through userinit

### Phase 2: System Integration (Current Focus)
**Duration**: 2-3 weeks
**Priority**: HIGH
**Deliverables**:
- All stub functions ported and working
- Memory alignment and safety fixes
- Portability improvements

### Phase 3: Advanced Features (Future Work)
**Duration**: 3-4 weeks
**Priority**: MEDIUM
**Deliverables**:
- Secure ramdisk implementation
- Pebbling system integration
- Process-level security features

## Detailed Task Breakdown

### Stub Function Porting Tasks

#### Task: vmxshutdown() Implementation
**File**: `kernel/9front-pc64/globals.c:287`
**Source**: `9/pc/devvmx.c:2047`
**Complexity**: Medium
**Dependencies**: None
**Estimated Time**: 3-4 hours

#### Task: i8042reset() Implementation  
**File**: `kernel/9front-pc64/globals.c:300`
**Source**: `9/pc/devkbd.c:97-125`
**Complexity**: Medium
**Dependencies**: None
**Estimated Time**: 2-3 hours

#### Task: bootscreeninit() Implementation
**File**: `kernel/9front-pc64/globals.c:306`
**Source**: `9/pc/screen.c:714-783`
**Complexity**: High (VGA/console integration)
**Dependencies**: Display device drivers
**Estimated Time**: 4-5 hours

#### Task: ramdiskinit() Implementation
**File**: `kernel/9front-pc64/globals.c:314`
**Source**: `9/port/sdram.c:125-165`
**Complexity**: Medium-High
**Dependencies**: Block device infrastructure
**Estimated Time**: 3-4 hours

#### Task: links() Investigation & Implementation
**File**: `kernel/9front-pc64/globals.c:309`
**Status**: No implementation found in 9front
**Complexity**: Unknown
**Dependencies**: Bootloader integration?
**Estimated Time**: 2-3 hours research + implementation

### Memory Safety Fixes

#### Task: namec Memory Leak Fix
**File**: `kernel/9front-port/chan.c:1330-1341`
**Complexity**: Low
**Estimated Time**: 2-3 hours

#### Task: Bootstrap Memory Corruption Fix
**File**: `kernel/borrowchecker.c`
**Complexity**: Medium
**Estimated Time**: 4-5 hours

#### Task: Memory Alignment Validation
**File**: `kernel/9front-pc64/memory_9front.c:691-699`
**Complexity**: Medium
**Estimated Time**: 2-3 hours

### Portability Improvements

#### Task: GDB Script Path Hardcoding Fix
**Files**: `gdb_scripts/corrected.gdb`, `gdb_scripts/debug_borrow.gdb`
**Complexity**: Low
**Estimated Time**: 2 hours

#### Task: HHDM Page Table Exhaustion Fix
**File**: `kernel/9front-pc64/mmu.c:468-475`
**Complexity**: High
**Estimated Time**: 6-8 hours

### API Consistency Fixes

#### Task: Userspace API Header Mismatches
**Files**: `userspace/include/syscall.h`, `userspace/lib/syscall.c`
**Complexity**: Low-Medium
**Estimated Time**: 3-4 hours

### Security Feature Implementation (Future)

#### Task: Secure Ramdisk Core
**Files**: New files in `kernel/9front-port/`
**Complexity**: High
**Dependencies**: Working ramdiskinit()
**Estimated Time**: 2-3 weeks

#### Task: Pebbling Integration
**Files**: `kernel/9front-port/pebble.c`, enhancements to existing pebble system
**Complexity**: High
**Dependencies**: Secure ramdisk core
**Estimated Time**: 2-3 weeks

## Milestones

### Milestone 1: Stable Boot (COMPLETED)
**Target**: Kernel reaches userinit() without hanging
**Indicators**:
- Environment setup completes successfully
- No crashes during device initialization
- Process system initialized

### Milestone 2: Complete Stub Functions (IN PROGRESS)
**Target**: All 5 stub functions properly implemented
**Indicators**:
- vmxshutdown() callable without crashing
- i8042reset() handles keyboard controller properly
- bootscreeninit() can initialize display (if available)
- ramdiskinit() creates functional ramdisk devices
- links() function properly implemented

### Milestone 3: Memory Safety Verified (IN PROGRESS)
**Target**: No memory leaks or corruption in critical paths
**Indicators**:
- No duplicate allocations
- Proper error handling balances
- Bootstrap memory handled correctly

### Milestone 4: Secure Ramdisk MVP (FUTURE)
**Target**: Basic secure ramdisk functionality with pebbeling integration
**Indicators**:
- Secure allocation/deallocation working
- Integration with existing pebble budget system
- Process lifecycle integration

## Risk Management

### High-Risk Items
1. **links() function implementation** - No source found, potential bootloader dependency
2. **HHDM page table exhaustion** - Complex memory management fix
3. **Display/boot screen integration** - Hardware-dependent complexity

### Mitigation Strategies
1. **links() function**: Create minimal working implementation, research bootloader integration
2. **Memory management**: Incremental testing, memory debugging tools
3. **Display integration**: Start with minimal VGA support, expand gradually

## Quality Assurance

### Testing Approach
1. **Unit Testing**: Individual function validation
2. **Integration Testing**: Cross-component compatibility
3. **Memory Testing**: Valgrind/AddressSanitizer for leak detection
4. **Boot Testing**: Full kernel boot cycle verification

### Success Metrics
- **Build Success**: Clean compilation with no warnings
- **Boot Success**: Kernel reaches user space consistently
- **Memory Safety**: Zero memory leaks in critical paths
- **Functionality**: All ported functions execute without crashing

## Resource Planning

### Development Resources
- **Developer Time**: 45-55 hours total
- **Testing Infrastructure**: QEMU for consistent testing
- **Debugging Tools**: GDB, memory analysis tools

### Timeline
- **Week 1**: Build fixes + core stub porting
- **Week 2**: Memory safety + remaining stubs
- **Week 3**: Portability improvements + QA
- **Week 4+**: Secure features (future phase)

## Dependencies and Blocking Issues

### Current Blockers
- None - kernel now boots through userinit()

### Potential Blockers
- **links() function**: May require bootloader research
- **Hardware dependencies**: Display functions require hardware presence

### Resolution Path
1. Address known blockers first
2. Document unknowns for future investigation
3. Create minimal implementations for critical blockers
4. Implement full features incrementally

This roadmap provides a structured approach to implementing the secure ramdisk with pebbeling integration while ensuring stability and maintainability throughout the development process.