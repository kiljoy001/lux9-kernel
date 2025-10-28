# SIP 9P Interface Design

## Philosophy

In Lux9, SIP servers are controlled entirely through 9P filesystem operations. This follows Plan 9's "everything is a file" principle where:
- Server lifecycle is managed by reading/writing control files
- Server status is read from status files
- Device access is granted through file descriptors
- Interrupts are received by reading interrupt files

## Filesystem Layout

```
/dev/sip/                    # SIP control filesystem
├── clone                    # Create new server instance
├── servers/                 # Running servers directory
│   ├── ahci-driver/
│   │   ├── ctl             # Control file (write commands)
│   │   ├── status          # Status file (read state)
│   │   ├── health          # Health metrics
│   │   ├── devices/        # Detected devices
│   │   │   ├── sd0
│   │   │   └── sd1
│   │   └── irq             # Read to block until interrupt
│   ├── ext4fs/
│   │   ├── ctl
│   │   ├── status
│   │   └── mount           # 9P mount point info
│   └── ...
└── types                    # Available server types

/dev/mem                     # Physical memory access (restricted)
/dev/io                      # I/O port access (restricted)
/dev/irq/                    # IRQ control
    ├── ctl                  # Register IRQ handlers
    └── N                    # IRQ N events (read blocks)
```

## Server Lifecycle via 9P

### 1. Creating a Server

```bash
# Read clone file to get new server ID
echo "example-driver" > /dev/sip/clone
# Returns: "0" (server ID)

# Configure via ctl file
echo "name ahci-driver" > /dev/sip/servers/0/ctl
echo "capability device" > /dev/sip/servers/0/ctl
echo "capability interrupt" > /dev/sip/servers/0/ctl
echo "mount /dev/sd" > /dev/sip/servers/0/ctl

# Start the server
echo "start" > /dev/sip/servers/0/ctl
```

### 2. Monitoring Status

```bash
# Read status
cat /dev/sip/servers/ahci-driver/status
# Output:
# name: ahci-driver
# type: example-driver
# status: healthy
# uptime: 3600
# requests: 15234
# errors: 2

# Read health
cat /dev/sip/servers/ahci-driver/health
# Output:
# healthy
# Last error: timeout on sd1 at 2025-10-26 14:30:15
```

### 3. Stopping a Server

```bash
echo "stop" > /dev/sip/servers/ahci-driver/ctl
```

## Device Access via 9P

### Memory-Mapped I/O (MMIO)

SIP servers use `/dev/mem` with capability checks:

```go
// Server opens /dev/mem with proper capabilities
memfd, err := os.OpenFile("/dev/mem", os.O_RDWR, 0)
if err != nil {
    // Kernel checks CapDeviceAccess capability
    return err
}

// Seek to physical address
memfd.Seek(0xFEDC0000, io.SeekStart)

// Read/write device registers
var status uint32
binary.Read(memfd, binary.LittleEndian, &status)
```

Kernel checks:
- Process has `CapDeviceAccess` capability
- Address is in allowed MMIO range
- Not accessing kernel memory

### I/O Ports

```go
// Open I/O port access
iofd, err := os.OpenFile("/dev/io", os.O_RDWR, 0)

// Format: "outb 0x3F8 0x41" or "inb 0x3F8"
fmt.Fprintf(iofd, "outb 0x%x 0x%x", port, value)
```

### DMA Buffers

```go
// Allocate DMA-capable buffer via special file
dmafd, err := os.OpenFile("/dev/dma/alloc", os.O_RDWR, 0)

// Write allocation request
fmt.Fprintf(dmafd, "size 4096 align 4096")

// Read returns: "vaddr:0x... paddr:0x... handle:123"
var vaddr, paddr, handle uintptr
fmt.Fscanf(dmafd, "vaddr:0x%x paddr:0x%x handle:%d", &vaddr, &paddr, &handle)

// Use physical address for device
// Access memory at virtual address
```

## Interrupt Handling via 9P

### Registering for Interrupts

```go
// Open IRQ control file
ctlfd, err := os.OpenFile("/dev/irq/ctl", os.O_WRONLY, 0)

// Register for IRQ 11 (AHCI)
fmt.Fprintf(ctlfd, "register 11 %s", serverName)

// Open IRQ event file (blocks until interrupt)
irqfd, err := os.OpenFile("/dev/irq/11", os.O_RDONLY, 0)

// In goroutine: read blocks until interrupt
go func() {
    buf := make([]byte, 32)
    for {
        n, _ := irqfd.Read(buf)
        if n > 0 {
            // Interrupt occurred
            handleInterrupt()
        }
    }
}()
```

Kernel behavior:
- IRQ handler writes to `/dev/irq/N` file
- All readers wake up
- Server processes interrupt

## 9P Message Flow

### Server Registration

```
Client                          Kernel (9P Server)
  |                                    |
  | Topen "clone"                      |
  |------------------------------------->
  |                                    | Allocate server slot
  | Ropen (fid for server/0/ctl)       |
  |<-------------------------------------|
  |                                    |
  | Twrite "name ahci-driver"          |
  |------------------------------------->
  |                                    | Store config
  | Rwrite                             |
  |<-------------------------------------|
  |                                    |
  | Twrite "start"                     |
  |------------------------------------->
  |                                    | Fork process
  |                                    | Set capabilities
  |                                    | Execute server
  | Rwrite                             |
  |<-------------------------------------|
```

### Device I/O

```
Server                          Kernel (9P Server)
  |                                    |
  | Topen "/dev/mem"                   |
  |------------------------------------->
  |                                    | Check CapDeviceAccess
  | Ropen                              |
  |<-------------------------------------|
  |                                    |
  | Tseek 0xFEDC0000                   |
  |------------------------------------->
  | Rseek                              |
  |<-------------------------------------|
  |                                    |
  | Tread 4                            |
  |------------------------------------->
  |                                    | Map physical memory
  |                                    | Read register
  | Rread [data]                       |
  |<-------------------------------------|
```

## Capability Enforcement

Capabilities are checked at 9P operation level:

```go
// In kernel 9P server
func (s *DevMemServer) Read(fid *Fid, offset uint64, count uint32) ([]byte, error) {
    proc := fid.process

    // Check capability
    if proc.capabilities & CapDeviceAccess == 0 {
        return nil, errors.New("permission denied: CapDeviceAccess required")
    }

    // Validate physical address range
    if !isValidMMIO(offset) {
        return nil, errors.New("invalid MMIO address")
    }

    // Perform read
    return readPhysicalMemory(offset, count), nil
}
```

## Benefits of 9P Interface

1. **Uniform Interface**: Everything uses read/write, no custom syscalls
2. **Namespace Integration**: Servers appear in filesystem hierarchy
3. **Remote Capable**: Can manage servers over network via 9P
4. **Tool Friendly**: Use cat/echo to control servers
5. **Language Agnostic**: Any language with file I/O can use SIP
6. **Debugging**: Easily inspect state with standard tools

## Implementation Strategy

1. **Kernel Side**: Create devmem, devio, devirq, devsip 9P servers in kernel
2. **Userspace Side**: Go SIP framework becomes a convenience wrapper around 9P operations
3. **Protocol**: Use existing 9P implementation, no new IPC needed
4. **Security**: Capabilities checked per-process during 9P operations

This approach is pure Plan 9 philosophy - the kernel provides files, userspace reads/writes them.
