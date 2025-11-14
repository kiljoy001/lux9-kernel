# Serious bugs identified - UPDATED STATUS

This report lists critical defects discovered in the repository. **NEW DIRECTION**: Focus on crypto-first architecture instead of traditional hardware drivers.

## Strategic Shift: Crypto-First Architecture

**Rationale**: Instead of implementing hardware drivers first (AHCI, USB, etc.), we're pivoting to a revolutionary crypto-backed filesystem approach where:
- Crypto operations are file I/O (write→read pattern)
- All system data encrypted in RAM via borrow checking + pebble budgeting  
- Drive storage acts as mass cache (backwards from traditional OS)
- Hardware drivers will come later via NetBSD rump servers

## FIXED - Crypto Server Foundation

1. **Go 9P Server Protocol Issues** - RESOLVED AS DEPRECATED
   - **Previous Issue**: Messages exceed `msize`, malformed strings crash parser, allocation failures
   - **Reason**: Switching to C-based crypto server using standard 9P
   - **New Status**: Use proven NetBSD rump server for hardware when needed, C server for crypto
   - **Files**: `userspace/go-servers/p9/` - Will be refactored to C
   - **Priority**: NO LONGER CRITICAL

2. **MemFS Walk/Read Issues** - RESOLVED AS DEPRECATED  
   - **Previous Issue**: Walk ignores paths, Read disregards offsets
   - **Reason**: Building custom crypto filesystem, not using Go-based services
   - **New Status**: Focus on write→read crypto file operations in C
   - **Files**: `userspace/go-servers/memfs/` - Not needed for crypto-first approach
   - **Priority**: NO LONGER CRITICAL

## NEW PRIORITY - Crypto Server Implementation

3. **Supercop Integration Critical**
   - **Issue**: Need battle-tested crypto foundation before building crypto server
   - **Location**: `crypto-standards/supercop/` 
   - **Action Required**: 
     - [ ] Integrate core algorithms (SHA256, AES-GCM, Ed25519, randombytes)
     - [ ] Create crypto wrapper library for unified interface
     - [ ] Performance testing and memory safety audit
   - **Priority**: Phase 1 critical
   - **Dependencies**: None

4. **9P Crypto Server Development**
   - **Issue**: Need file-based crypto interface using write→read pattern
   - **Location**: NEW - Create `kernel/crypto/` directory
   - **Action Required**:
     - [ ] Create basic 9P server with crypto file operations
     - [ ] Implement file types: hash, encrypt, decrypt, random, sign
     - [ ] Process isolation for crypto operations
     - [ ] Example: `echo "data" > /secure/crypto/hash/sha256` then `cat /secure/crypto/hash/sha256`
   - **Priority**: Phase 1 critical
   - **Dependencies**: Requires supercop integration (#3)

5. **Borrow Lock + Pebble Integration**
   - **Issue**: Need process isolation and resource accounting for crypto operations
   - **Location**: `kernel/9front-port/lock_borrow.c`, `kernel/pebble.c`
   - **Action Required**:
     - [ ] File access control via borrow checker for crypto files
     - [ ] Exclusive/shared access permissions for crypto operations
     - [ ] Resource costing and budget checking for crypto usage
     - [ ] Security model validation and audit trail
   - **Priority**: Phase 1 high
   - **Dependencies**: Requires working crypto server (#4)

## REMAINING CRITICAL - Kernel Stability Issues

6. **Random Number Generator Corruption** - STILL CRITICAL
   - **Issue**: `randomseed` scribbles over own lock, corrupting synchronization primitives
   - **Location**: `kernel/9front-port/random.c:71-108`
   - **Impact**: Race conditions, system instability, potential deadlocks
   - **Fix**: `randomseed()` seeds `rs->chacha` directly and leaves surrounding `QLock` intact
   - **Priority**: Kernel stability - must fix before crypto server
   - **Dependencies**: None

7. **Page-Table Pool Race Conditions** - STILL CRITICAL  
   - **Issue**: Unsynchronized PT allocation can overflow on SMP systems, memory corruption
   - **Location**: `kernel/9front-pc64/mmu.c:165-185`
   - **Impact**: Memory corruption, system crashes on multiprocessor systems
   - **Fix**: Guard pool with `allocptlock` and tighten exhaustion test to trip at 512 tables
   - **Priority**: Kernel stability - must fix before crypto server
   - **Dependencies**: None

8. **Memory Management Issues** - STILL CRITICAL
   - **Issues**: Pool allocation bugs, formatting problems, memory leaks
   - **Location**: Multiple files in `kernel/9front-pc64/globals.c`
   - **Impact**: Memory corruption, resource leaks, system instability
   - **Priority**: Kernel stability - must fix before crypto server
   - **Dependencies**: None

## DEFERRED - Hardware Driver Bugs (Lower Priority)

**Strategic Decision**: Hardware driver bugs deferred to Phase 4+ after crypto foundation complete.

**Reason**: Crypto-first approach prioritizes secure filesystem foundation over hardware drivers.

**Future Approach**: Use NetBSD rump servers for hardware when needed - proven, POSIX-compatible drivers.

**Deferred Issues Include**:
- PCI configuration space unreachable
- Device reset hooks are inert stubs  
- Interrupt controllers never initialized
- Architecture-specific devices cannot be registered

## Implementation Strategy

### New Priority Order:
1. **Phase 1 (Weeks 1-8)**: Crypto server foundation
   - Fix kernel stability issues (#6, #7, #8)
   - Implement supercop integration (#3)  
   - Build basic 9P crypto server (#4)
   - Add borrow lock + pebble integration (#5)

2. **Phase 2 (Weeks 9-14)**: Security enhancement
   - RAM encryption for all crypto data
   - Advanced security features (cold boot protection, audit trails)
   - Complete process isolation

3. **Phase 3 (Weeks 15-20)**: Filesystem integration
   - RAM-primary filesystem with `/secure/` mount
   - Hybrid storage model with drive as cache
   - RC shell integration

4. **Phase 4 (Weeks 21-26)**: Advanced features
   - Synthetic files and translator chains
   - System integration and optimization
   - Hardware drivers via NetBSD rump servers

### Success Criteria for Phase 1:
- [ ] Kernel stability issues resolved (#6, #7, #8)
- [ ] Working crypto operations: `echo "data" > /secure/crypto/hash/sha256` → `cat /secure/crypto/hash/sha256`
- [ ] Borrow checking prevents unauthorized crypto access
- [ ] Pebble budgets limit and account for crypto resource usage
- [ ] System stable under load testing

## Rationale for Strategic Pivot

**Why Crypto-First Over Hardware-First**:

1. **Unique Value Proposition**: First crypto-backed filesystem with Plan 9 philosophy
2. **Faster Implementation**: No hardware complexity, focus on software innovation
3. **Research Impact**: Novel architecture with academic publication potential
4. **Security Foundation**: Everything can build on encrypted, isolated base
5. **Market Differentiation**: No competitor has crypto-backed file operations
6. **Hardware Later**: NetBSD rump servers provide proven drivers when ready

**This strategic pivot creates a unique competitive advantage while leveraging proven cryptographic libraries (Supercop + NIST standards) and established Plan 9 principles.**

## Files Updated by This Change

- **NEW**: `docs/IMPLEMENTATION_PLAN_CRYPTO_FILESYSTEM.md` - Complete implementation roadmap
- **UPDATED**: `docs/serious-bugs.md` - This document with new priorities
- **FUTURE**: `kernel/crypto/` - Will contain crypto server implementation
- **FUTURE**: `userspace/go-servers/p9/` - Will be refactored or deprecated

The crypto-first direction represents a fundamental shift toward software innovation over hardware compatibility, positioning Lux9 as a leader in secure operating system research.
