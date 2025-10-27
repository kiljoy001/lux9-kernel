# Block Device 9P Interface Design

## Philosophy

Block devices in Lux9 are exposed as 9P filesystems, following Plan 9's sd(3) device model. Instead of syscalls like `read()` on block devices, everything goes through 9P file operations.

## Filesystem Layout

```
/dev/sd/                     # Block device root
├── sdctl                    # Global control
├── sdC0/                    # Controller 0 (AHCI port 0)
│   ├── ctl                  # Controller control
│   ├── raw                  # Raw device access (whole disk)
│   ├── data                 # Alias for raw
│   ├── part/                # Partitions
│   │   ├── 0               # Partition 0 (entire disk)
│   │   ├── 1               # Partition 1 (e.g., EFI System)
│   │   ├── 2               # Partition 2 (e.g., root)
│   │   └── ...
│   └── geometry             # Disk geometry info
├── sdC1/                    # Controller 1 (AHCI port 1)
│   └── ...
└── sdD0/                    # IDE device 0
    └── ...
```

## 9P Operations on Block Devices

### Reading from Disk

```go
// Open raw device
fd, err := os.Open("/dev/sd/sdC0/raw")
if err != nil {
    return err
}

// Seek to LBA 1024
offset := int64(1024 * 512)  // 512 byte sectors
fd.Seek(offset, io.SeekStart)

// Read 4KB
buf := make([]byte, 4096)
n, err := fd.Read(buf)
```

9P message flow:
```
Client → Kernel: Topen tag=1 fid=1 name="sdC0/raw"
Kernel → Driver: (via channel/queue)
Driver → Kernel: Device ready
Kernel → Client: Ropen tag=1 qid=...

Client → Kernel: Tread tag=2 fid=1 offset=524288 count=4096
Kernel → Driver: Read request (LBA=1024, count=8 sectors)
Driver → HW: Issue AHCI read command
HW → Driver: Interrupt (transfer complete)
Driver → Kernel: Data ready
Kernel → Client: Rread tag=2 data=[4096 bytes]
```

### Writing to Disk

```go
fd, err := os.OpenFile("/dev/sd/sdC0/raw", os.O_RDWR, 0)

// Write at specific offset
fd.Seek(offset, io.SeekStart)
n, err := fd.Write(data)
```

### Partition Access

```go
// Read partition 2 (e.g., root partition)
fd, err := os.Open("/dev/sd/sdC0/part/2")

// Read from partition start (offset 0 = partition start)
buf := make([]byte, 512)
fd.Read(buf)  // Reads first sector of partition 2
```

### Control Operations

```go
// Open control file
ctl, err := os.OpenFile("/dev/sd/sdC0/ctl", os.O_WRONLY, 0)

// Flush cache
fmt.Fprintf(ctl, "flush")

// Read geometry
geom, err := os.ReadFile("/dev/sd/sdC0/geometry")
// Output:
// sectors 976773168
// secsize 512
// cylinders 60801
// heads 255
// sectors 63
```

## AHCI Driver as 9P Server

The AHCI driver runs as a userspace SIP server that exports 9P:

```go
package main

import (
    "lux9/sip"
    "lux9/9p"
)

type AHCIDriver struct {
    *sip.BaseServer
    ports []*AHCIPort
    fs    *BlockDevice9PServer
}

func (d *AHCIDriver) Start(ctx context.Context) error {
    // Probe AHCI controller via /dev/mem
    devices := d.ProbeDevices()

    // Create 9P filesystem
    fs := New9PBlockServer()

    for i, port := range d.ports {
        // Add device to 9P tree
        dev := &BlockDevice{
            name: fmt.Sprintf("sdC%d", i),
            port: port,
        }
        fs.AddDevice(dev)
    }

    // Mount 9P server at /dev/sd
    return fs.Mount("/dev/sd")
}

func (d *AHCIDriver) HandleRead(dev *BlockDevice, lba uint64, count uint32) ([]byte, error) {
    // Issue AHCI read command
    cmd := d.buildReadCmd(lba, count)
    d.issueCommand(dev.port, cmd)

    // Wait for interrupt via /dev/irq/N
    <-dev.port.irqChan

    // Return data
    return dev.port.dmaBuffer[:count*512], nil
}
```

## 9P Server Implementation

```go
type BlockDevice9PServer struct {
    devices map[string]*BlockDevice
    root    *ninep.Dir
}

func (s *BlockDevice9PServer) Read(fid *ninep.Fid, offset uint64, count uint32) ([]byte, error) {
    // Parse path: /dev/sd/sdC0/raw
    dev, file := s.parsePath(fid.path)

    switch file {
    case "raw", "data":
        // Convert offset to LBA
        lba := offset / dev.sectorSize
        sectors := (count + dev.sectorSize - 1) / dev.sectorSize

        // Issue read to hardware driver
        return dev.driver.HandleRead(dev, lba, sectors)

    case "ctl":
        return []byte("disk ready\n"), nil

    case "geometry":
        return []byte(fmt.Sprintf(
            "sectors %d\nsecsize %d\n",
            dev.sectors, dev.sectorSize,
        )), nil
    }
}

func (s *BlockDevice9PServer) Write(fid *ninep.Fid, offset uint64, data []byte) (uint32, error) {
    dev, file := s.parsePath(fid.path)

    switch file {
    case "raw", "data":
        lba := offset / dev.sectorSize
        return dev.driver.HandleWrite(dev, lba, data)

    case "ctl":
        cmd := string(data)
        return s.handleCtl(dev, cmd)
    }
}
```

## Partition Table Support

Partitions are discovered at mount time:

```go
func (d *BlockDevice) ReadPartitionTable() error {
    // Read MBR/GPT from LBA 0
    sector0 := d.ReadSector(0)

    // Parse GPT
    partitions := parseGPT(sector0)

    // Add partition files to 9P tree
    for i, part := range partitions {
        d.fs.AddFile(fmt.Sprintf("part/%d", i), &PartitionFile{
            device: d,
            start:  part.StartLBA,
            end:    part.EndLBA,
        })
    }
}

type PartitionFile struct {
    device *BlockDevice
    start  uint64  // Starting LBA
    end    uint64  // Ending LBA
}

func (p *PartitionFile) Read(offset uint64, count uint32) ([]byte, error) {
    // Translate partition offset to absolute LBA
    lba := p.start + (offset / p.device.sectorSize)

    // Bounds check
    if lba > p.end {
        return nil, io.EOF
    }

    // Read from underlying device
    return p.device.driver.HandleRead(p.device, lba, count)
}
```

## Mounting Filesystems via 9P

Once block devices are exposed via 9P, filesystems mount them:

```bash
# Ext4 filesystem server mounts partition 2
# Reads /dev/sd/sdC0/part/2 via 9P
ext4fs -device /dev/sd/sdC0/part/2 -mount /root

# Or using mount command
mount /dev/sd/sdC0/part/2 /root ext4
```

The ext4 server:
1. Opens `/dev/sd/sdC0/part/2` via 9P
2. Reads ext4 superblock (Tread tag=X offset=1024 count=1024)
3. Caches inodes/blocks in memory
4. Exports another 9P server at `/root`
5. Translates file operations to block device reads/writes

## Complete Architecture

```
┌─────────────────────────────────────────────┐
│  Applications (userspace)                   │
│  - cat /root/file.txt                       │
└─────────────────┬───────────────────────────┘
                  │ 9P read
┌─────────────────▼───────────────────────────┐
│  ext4fs (SIP server)                        │
│  - Translates file ops to block I/O         │
│  - Exports 9P at /root                      │
└─────────────────┬───────────────────────────┘
                  │ 9P read /dev/sd/sdC0/part/2
┌─────────────────▼───────────────────────────┐
│  AHCI Driver (SIP server)                   │
│  - Exports 9P at /dev/sd                    │
│  - Translates reads to AHCI commands        │
└─────────────────┬───────────────────────────┘
                  │ /dev/mem + /dev/irq
┌─────────────────▼───────────────────────────┐
│  Kernel (microkernel)                       │
│  - devmem: MMIO access                      │
│  - devirq: Interrupt delivery               │
│  - devsip: Server lifecycle                 │
└─────────────────┬───────────────────────────┘
                  │ Hardware access
┌─────────────────▼───────────────────────────┐
│  AHCI Controller Hardware                   │
└─────────────────────────────────────────────┘
```

## Key Benefits

1. **Pure 9P**: No custom syscalls for block I/O
2. **Userspace Drivers**: AHCI/IDE drivers in userspace, crashes isolated
3. **Network Transparent**: Can mount remote disks via 9P
4. **Simple Tools**: `dd if=/dev/sd/sdC0/raw of=backup.img`
5. **Composable**: Stack filesystems easily
6. **Hot Plug**: Add/remove devices by updating 9P tree

## Implementation Files

```
userspace/go-servers/
├── ahci-driver/
│   ├── main.go              # AHCI driver SIP server
│   ├── ahci.go              # Hardware interaction via /dev/mem
│   ├── 9p_server.go         # Export block devices via 9P
│   └── partition.go         # GPT/MBR parsing
├── ide-driver/
│   └── ...                  # Similar for IDE
└── ext4fs/
    ├── main.go              # Ext4 filesystem SIP server
    ├── ext4.go              # Ext4 implementation
    └── 9p_server.go         # Export filesystem via 9P
```

This is the Plan 9 way - everything is a file, everything speaks 9P!
