## AHCI Driver - Userspace Block Device Driver

A userspace AHCI (Advanced Host Controller Interface) driver implemented as a Software Isolated Process (SIP) server. Exposes SATA/AHCI block devices via 9P protocol at `/dev/sd`.

## Architecture

```
┌──────────────────────────────────────┐
│  Applications                        │
│  dd if=/dev/sd/sdC0/raw of=backup    │
└─────────────┬────────────────────────┘
              │ 9P operations
┌─────────────▼────────────────────────┐
│  AHCI Driver (this program)          │
│  ┌────────────────────────────────┐  │
│  │  Block9PServer                 │  │
│  │  - Exports /dev/sd via 9P      │  │
│  │  - Handles read/write requests │  │
│  └──────────┬─────────────────────┘  │
│  ┌──────────▼─────────────────────┐  │
│  │  AHCIController                │  │
│  │  - Manages AHCI ports          │  │
│  │  - Issues commands to hardware │  │
│  └──────────┬─────────────────────┘  │
└─────────────┼────────────────────────┘
              │ /dev/mem + /dev/irq
┌─────────────▼────────────────────────┐
│  Kernel                              │
│  - devmem: MMIO access               │
│  - devirq: Interrupt delivery        │
└─────────────┬────────────────────────┘
              │ Hardware
┌─────────────▼────────────────────────┐
│  AHCI SATA Controller                │
└──────────────────────────────────────┘
```

## Filesystem Structure

When running, exposes the following 9P filesystem:

```
/dev/sd/
├── sdC0/              # First SATA device
│   ├── ctl           # Control file (read status, write commands)
│   ├── raw           # Raw device access
│   ├── data          # Alias for raw
│   ├── geometry      # Disk geometry info
│   └── part/         # Partitions
│       ├── 0        # Whole disk
│       ├── 1        # First partition
│       └── 2        # Second partition
├── sdC1/              # Second SATA device
│   └── ...
└── ...
```

## Usage Examples

### Read from raw device
```bash
# Read first sector
dd if=/dev/sd/sdC0/raw of=sector0.bin bs=512 count=1

# Read from offset
dd if=/dev/sd/sdC0/raw of=data.bin bs=4096 skip=1024 count=10
```

### Write to device
```bash
echo "test data" > /dev/sd/sdC0/raw
```

### Read partition
```bash
# Mount partition 2 via ext4 filesystem
ext4fs -device /dev/sd/sdC0/part/2 -mount /root
```

### Control operations
```bash
# Read device status
cat /dev/sd/sdC0/ctl
# Output: device sdC0
#         model QEMU HARDDISK
#         serial QM00001
#         status ready

# Read geometry
cat /dev/sd/sdC0/geometry
# Output: sectors 20971520
#         secsize 512
#         cylinders 1305
#         heads 255
#         sectors/track 63

# Flush cache
echo flush > /dev/sd/sdC0/ctl

# Rescan partitions
echo rescan > /dev/sd/sdC0/ctl
```

## Implementation Details

### Hardware Access

The driver accesses AHCI hardware through kernel-provided files:

**MMIO (Memory-Mapped I/O):**
```go
// Open /dev/mem with CapDeviceAccess capability
memfd, _ := os.OpenFile("/dev/mem", os.O_RDWR, 0)

// Seek to AHCI BAR address
memfd.Seek(0xFEDC0000, 0)

// Read/write registers
binary.Read(memfd, binary.LittleEndian, &register)
```

**Interrupts:**
```go
// Register for IRQ 11 (AHCI)
ctlfd, _ := os.OpenFile("/dev/irq/ctl", os.O_WRONLY, 0)
fmt.Fprintf(ctlfd, "register 11 ahci-driver")

// Read blocks until interrupt
irqfd, _ := os.OpenFile("/dev/irq/11", os.O_RDONLY, 0)
irqfd.Read(buf) // Blocks until IRQ fires
```

**DMA Buffers:**
```go
// Allocate DMA-capable memory
dmafd, _ := os.OpenFile("/dev/dma/alloc", os.O_RDWR, 0)
fmt.Fprintf(dmafd, "size 4096 align 4096")

// Get physical address for hardware
var paddr uintptr
fmt.Fscanf(dmafd, "paddr:0x%x", &paddr)
```

### AHCI Command Flow

1. Application reads `/dev/sd/sdC0/raw` via 9P
2. Block9PServer receives Tread message
3. Converts offset to LBA (Logical Block Address)
4. Calls `AHCIPort.Read(lba, count)`
5. Driver builds AHCI FIS (Frame Information Structure)
6. Writes command to AHCI command list
7. Sets bit in Port Command Issue register
8. Hardware performs DMA transfer
9. Hardware raises interrupt
10. Kernel writes to `/dev/irq/11`
11. Driver's IRQ goroutine wakes up
12. Checks completion status
13. Returns data to Block9PServer
14. Block9PServer sends Rread response

### Partition Detection

At device attachment:
1. Read sector 0 (MBR)
2. Check for GPT signature at LBA 1
3. Parse partition table (GPT or MBR)
4. Create `/dev/sd/sdC0/part/N` files for each partition
5. Offset translation: partition reads are relative to partition start

### Capabilities Required

The driver requires these SIP capabilities:
- `CapDeviceAccess`: Access to `/dev/mem` for MMIO
- `CapInterrupt`: Access to `/dev/irq/*` for interrupts
- `CapDMA`: Ability to allocate DMA-capable memory

These are checked at startup and enforced by the kernel.

## Building

```bash
cd userspace/go-servers/ahci-driver
go build
```

## Running

As a SIP server (started by init):
```bash
# In init script
ahci-driver &
```

Or manually:
```bash
./ahci-driver
```

## Configuration

Configuration is handled via SIP ServerConfig:
```go
config := &sip.ServerConfig{
    Name:         "ahci-driver",
    Capabilities: sip.CapDeviceAccess | sip.CapInterrupt | sip.CapDMA,
    MountPoint:   "/dev/sd",
    Priority:     10,
}
```

## Status

- [x] Basic AHCI driver structure
- [x] 9P filesystem design
- [x] Hardware access via /dev/mem
- [x] Interrupt handling via /dev/irq
- [x] MBR partition table parsing
- [ ] GPT partition table parsing
- [ ] DMA buffer allocation
- [ ] Actual AHCI FIS command building
- [ ] NCQ (Native Command Queuing) support
- [ ] TRIM/DISCARD support
- [ ] Hot-plug detection
- [ ] Error recovery
- [ ] Integration with actual 9P library

## Benefits of Userspace Driver

1. **Isolation**: Driver crashes don't kernel panic
2. **Debuggability**: Use normal userspace tools (gdb, printf)
3. **Development**: Faster iteration, no kernel rebuilds
4. **Safety**: Go memory safety prevents buffer overflows
5. **Hot Reload**: Restart driver without rebooting
6. **Network**: Can serve block devices over network via 9P

## Future Enhancements

- Support for ATAPI (CD/DVD drives)
- Port multiplier support
- Power management (spin down idle drives)
- SMART health monitoring
- Drive encryption integration
- Performance statistics via /dev/sd/stats
