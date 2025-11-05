# Lux9 Microkernel Operating System

Revolutionary microkernel OS combining Plan 9 architecture with capability-based security and zero-copy IPC.

## Overview

Lux9 is a microkernel operating system featuring innovative **pebble memory management** with Rust-style borrowing semantics, **Plan 9 heritage** with modern security enhancements, and **zero-copy page exchange** for high-performance IPC.

**Key innovations:**
- **Pebble System**: Capability-based resource management with Red-Blue copy-on-write
- **Exchange Pages**: Zero-copy memory transfer between processes (Singularity-inspired)
- **Borrow Checker**: Rust-style ownership and borrowing for kernel primitives
- **9P Enhancement**: Revolutionary capability-safe distributed filesystem architecture

## Current Status

**Kernel Core**: ✅ **Functional** (Plan 9 heritage, advanced memory management)  
**Pebble Infrastructure**: ✅ **Operational** (borrow checker, page ownership, exchange system)  
**9P Protocol**: 🚧 **Security Issues** (protocol violations, buffer overflows)  
**System Init**: ❌ **Blocked** (syscall stubs prevent userspace boot)  
**Hardware**: ⚠️ **Limited** (no interrupts/PCI, limits testing scenarios)  

## Revolutionary Features

### Pebble Memory Management
- **Capability-based security** via white tokens
- **Red-Blue copy-on-write** for crash-safe operations
- **Resource quotas** prevent DoS attacks
- **Speculative execution** prevents partial write corruption

### Zero-Copy Exchange System
- **Process-to-process page transfer** without copying
- **Singularity-style exchange heap** semantics
- **Memory ownership tracking** with borrow checker integration

### Enhanced 9P Protocol
- **Capability-scoped FIDs** instead of integer handles
- **Budget-based client limiting** per connection
- **Atomic operations** via Red-Blue system
- **Zero-copy large file transfers** via exchange handles

## Quick Start

```bash
# Build kernel (currently bootable to graphics initialization)
make kernel

# Testing blocked by system init issues
# see docs/serious-bugs.md for current blockers

# Development
cd shell_scripts && ./debug.sh  # Debug utilities
```

## Architecture

```
Lux9 Kernel (C)
├── Pebble System (capability-based security)
├── Exchange Pages (zero-copy IPC)  
├── Borrow Checker (Rust-style memory safety)
├── 9P Protocol (enhanced with pebble integration)
└── Plan 9 Heritage (channels, mount, devmnt)

Userspace Servers
├── Go 9P servers (protocol implementation)
├── ext4fs (filesystem server)
└── Test utilities
```

## Current Development Focus

**Immediate Priority**: System initialization and 9P protocol security
- Fix syscall stubs to enable userspace boot
- Resolve 9P message corruption and buffer overflow vulnerabilities
- Implement interrupt/PCI initialization for hardware access

**Next Phase**: Pebble-9P Integration
- Deploy capability-based FIDs (P9Cap structures)
- Implement budget guards per 9P connection
- Add zero-copy file transfer via exchange system

**Long-term Vision**: World's first capability-safe distributed filesystem

## Documentation

Comprehensive documentation in `docs/`:
- `docs/serious-bugs.md` - Detailed bug analysis and status
- `docs/PEBBLE_9P_INTEGRATION_PLAN.md` - Revolutionary 9P enhancement roadmap
- `docs/BOOT_MEMORY_COORDINATION_PLAN.md` - Memory management improvements
- `docs/SYSTEM_VISION.md` - Project vision and architecture
- `docs/PROJECT_STRUCTURE.md` - Directory organization and conventions

## Recent Major Improvements

✅ **Infrastructure Stabilization**
- Borrow checker data structures completely rebuilt and operational
- Page ownership system functional (pebble foundation ready)
- Memory management safety and alignment issues resolved
- Time system functional with real hardware tick support

✅ **Innovation Foundation**
- Pebble system architecture ready for 9P integration
- Exchange page system operational for zero-copy IPC
- Red-Blue copy-on-write for crash-safe operations

🚧 **Next Development Sprint**
- System init syscall implementation
- 9P protocol security hardening
- Hardware foundation (interrupts, PCI, console)

## Innovation Status

Lux9 represents a potential breakthrough in operating system security:

**Traditional OS**: Memory safety issues, resource exhaustion vulnerabilities, unreliable IPC
**Lux9 Vision**: Capability-based security, resource quotas, zero-copy performance, atomic operations

The pebble system creates possibilities that **no other operating system has achieved**, combining Plan 9's proven IPC model with modern capability security and zero-copy performance.

## Scripts

Development utilities in `shell_scripts/`:
- Debugging and analysis tools
- 9P protocol testing frameworks  
- QEMU development environments

## License

MIT-style license. Individual components retain their original licenses where applicable.
