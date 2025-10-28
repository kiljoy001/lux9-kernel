# SIP Integration Test Plan

## Objective

Test the complete SIP kernel infrastructure with a real AHCI controller to verify that all 4 devices (`/dev/mem`, `/dev/irq`, `/dev/dma`, `/dev/pci`) work correctly for userspace device drivers.

## Test Program

**File:** `userspace/bin/test_ahci.c` (249 lines)

### Test Sequence

1. **PCI Enumeration Test**
   - Open `/dev/pci/bus`
   - Parse for AHCI controller (class 01.06.01)
   - Extract device name and BAR5 address
   - **Expected:** Find AHCI at `0000:00:1f.2` (QEMU) with BAR5 address

2. **MMIO Access Test**
   - Open `/dev/mem`
   - Seek to BAR5 + register offset
   - Read AHCI Capabilities Register (CAP)
   - Read AHCI Version Register (VS)
   - Read Ports Implemented Register (PI)
   - **Expected:** Valid register values, version 1.x, ports bitmap

3. **DMA Allocation Test**
   - Open `/dev/dma/alloc`
   - Write allocation request: "size 4096 align 1024"
   - Read back virtual and physical addresses
   - **Expected:** Non-zero addresses, proper alignment

4. **IRQ Registration Test**
   - Read IRQ from PCI device info
   - Open `/dev/irq/ctl`
   - Write registration: "register <irq> test_ahci"
   - **Expected:** Successful registration, no errors

### Success Criteria

✅ All 4 tests pass
✅ No kernel panics
✅ Devices accessible from userspace
✅ Register values are sensible

### Failure Modes

❌ Device not found → Check PCI enumeration
❌ Cannot open device → Check devtab registration
❌ Invalid register values → Check MMIO address validation
❌ DMA allocation fails → Check xalloc availability
❌ IRQ registration fails → Check intrenable integration

## Test Execution

### Environment

- **Platform:** QEMU q35 (includes AHCI controller)
- **Memory:** 2GB
- **Storage:** ISO boot (no actual disk I/O test yet)
- **Serial:** Output to qemu.log

### Command

```bash
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -cdrom lux9.iso \
    -boot d \
    -serial file:qemu.log \
    -no-reboot \
    -display none
```

### Expected Boot Sequence

```
Lux9
initrd: limine reports module
BOOT: printinit complete
xinit: initialization complete
pageown: using borrowchecker
exchange: initialized
fbconsoleinit: starting
[... framebuffer init ...]
init: starting
sh: starting shell
# /bin/test_ahci
```

## Known Issues

### Issue #1: Framebuffer Hang
**Status:** Boot may hang at fbconsoleinit
**Workaround:** Boot usually completes after delay, or use serial console
**Impact:** May prevent reaching userspace

### Issue #2: No Init Process
**Status:** Userspace init may not be configured
**Workaround:** Manual test execution if shell available
**Impact:** Cannot run tests automatically

### Issue #3: QEMU AHCI Configuration
**Status:** Q35 includes AHCI, but may need specific configuration
**Workaround:** Use `-drive` flag to attach disk if needed
**Impact:** AHCI controller may not appear

## Alternative Test Approaches

### Approach 1: Direct Kernel Boot Test

Add device initialization prints in kernel:

```c
// In devmem.c::memreset()
print("devmem: /dev/mem initialized\n");

// In devirq.c::irqreset()
print("devirq: /dev/irq initialized\n");

// In devdma.c::dmareset()
print("devdma: /dev/dma initialized\n");

// In devpci.c::devpcireset()
print("devpci: /dev/pci initialized\n");
```

**Benefit:** Verify devices initialize even if userspace unreachable

### Approach 2: Minimal Test in Init

Create tiny init that just tests device existence:

```c
void
main(void)
{
    int fd;

    fd = open("/dev/mem", OREAD);
    if(fd >= 0) print("✓ /dev/mem exists\n");
    close(fd);

    fd = open("/dev/irq/ctl", OWRITE);
    if(fd >= 0) print("✓ /dev/irq exists\n");
    close(fd);

    // etc...
}
```

**Benefit:** Minimal dependencies, quick verification

### Approach 3: Build Integration Test

Run `nm` on kernel to verify symbols:

```bash
$ nm lux9.elf | grep devtab
# Should show memdevtab, irqdevtab, dmadevtab, pcidevtab
```

**Benefit:** No runtime testing needed, verifies linking

## Test Results Template

### Build Status
- [ ] Kernel compiles without errors
- [ ] Test program compiles
- [ ] ISO image created successfully
- [ ] All device symbols present in binary

### Boot Status
- [ ] Kernel boots with Limine
- [ ] Reaches userspace init
- [ ] Shell accessible
- [ ] Test program can execute

### Device Tests
- [ ] /dev/pci enumeration works
- [ ] AHCI controller found
- [ ] BAR addresses readable
- [ ] /dev/mem MMIO access works
- [ ] AHCI registers readable
- [ ] Register values sensible
- [ ] /dev/dma allocation works
- [ ] Physical addresses returned
- [ ] /dev/irq registration works
- [ ] IRQ number correct

### Integration Status
- [ ] All 4 tests passed
- [ ] No kernel panics
- [ ] No errors in serial log
- [ ] AHCI controller fully accessible

## Next Steps After Success

1. **Full AHCI Driver**
   - Implement command issuing
   - Implement interrupt handling
   - Test actual disk I/O

2. **9P Block Device**
   - Export `/dev/sd` via 9P
   - Allow filesystem access
   - Mount ext4 from AHCI disk

3. **Production Hardening**
   - Enable capability enforcement
   - Add resource limits
   - Implement audit logging

4. **Additional Drivers**
   - Port IDE to userspace
   - Create NVMe driver
   - Network drivers

## Conclusion

This test plan provides a comprehensive approach to validating the SIP kernel infrastructure. Success means userspace drivers can fully access hardware through pure 9P operations, achieving the microkernel goal.

**Current Status:** Test program created, ISO built, ready to boot and test.
