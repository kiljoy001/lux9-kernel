# Lux9 Architecture Diagrams

This directory contains SVG diagrams illustrating the Lux9 system architecture and security model.

## Diagrams

### system-architecture.svg
Complete system architecture showing:
- Hardware layer (CPU, MMU, PCI devices, APIC, TPM)
- Kernel subsystems (Core, Security, IPC, Devices)
- Userspace servers (cryptosrv, ext4fs, drivers)
- System call interface
- Communication flows

**Used in:** Root README.md Architecture section

### ipc-architecture.svg
Inter-process communication architecture:
- Ring buffer for batched small messages (<4KB)
- Page exchange for large messages
- Security layer integration (page ownership, TOCTOU, replay protection, bounds checking)
- Message flow details with 5-step process

**Used in:** Root README.md IPC Architecture section

### security-model.svg
Multi-layer security defense mechanisms:
- **Layer 1:** Page ownership (Rust-style borrow checker)
- **Layer 2:** TOCTOU prevention (page unmapping + value copying)
- **Layer 3:** Replay protection (sequence numbers + session IDs)
- **Layer 4:** Multi-layer bounds checking

Each layer shows attack scenarios and how they're blocked.

**Used in:** Root README.md Security Model section

## Viewing

These SVG files can be viewed:
- **In GitHub:** Rendered directly in markdown files
- **In browser:** Open the .svg file directly
- **In editor:** VSCode, Inkscape, or any SVG-compatible editor

## Technical Details

All diagrams are pure SVG 1.1 with embedded CSS styles. No external dependencies required.

**Color scheme:**
- Blue (#e3f2fd): Kernel space
- Purple (#f3e5f5): Userspace
- Green (#e8f5e9): Hardware layer
- Orange (#fff3e0): Security layer
- Red (#ffcdd2): Unsafe/attack scenarios
- Green (#c8e6c9): Safe/protected operations

## Regenerating

These diagrams are hand-crafted SVG. To modify:
1. Open in an SVG editor (Inkscape recommended)
2. Or edit XML directly (all diagrams use clear structure)
3. Maintain consistent styling with existing diagrams

## License

Same as parent project (9front MIT-style license).
