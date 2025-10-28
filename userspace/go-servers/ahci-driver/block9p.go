// Package ahcidriver implements an AHCI block device driver as a 9P server
// This runs as a userspace SIP server and exposes block devices via 9P protocol
package ahcidriver

import (
	"context"
	"fmt"
	"io"
	"sync"
)

// BlockDevice represents a single block storage device (disk)
type BlockDevice struct {
	Name       string // e.g., "sdC0"
	SectorSize uint32 // Usually 512 or 4096
	Sectors    uint64 // Total number of sectors
	Model      string // Drive model string
	Serial     string // Drive serial number

	// Hardware specific
	Port       *AHCIPort        // AHCI port this device is on
	Partitions []*Partition     // Discovered partitions

	// Synchronization
	mu sync.RWMutex
}

// Partition represents a disk partition
type Partition struct {
	Number   int    // Partition number
	StartLBA uint64 // Starting LBA
	EndLBA   uint64 // Ending LBA (inclusive)
	Type     string // Partition type (GUID or MBR type)
	Name     string // Partition name (GPT only)
}

// Block9PServer exports block devices via 9P protocol
type Block9PServer struct {
	devices map[string]*BlockDevice // Key: device name (e.g., "sdC0")
	mu      sync.RWMutex

	// 9P filesystem structure
	// We'll implement the actual 9P protocol later
	// For now, define the logical structure
}

// NewBlock9PServer creates a new 9P block device server
func NewBlock9PServer() *Block9PServer {
	return &Block9PServer{
		devices: make(map[string]*BlockDevice),
	}
}

// AddDevice registers a new block device for 9P export
func (s *Block9PServer) AddDevice(dev *BlockDevice) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if _, exists := s.devices[dev.Name]; exists {
		return fmt.Errorf("device %s already exists", dev.Name)
	}

	s.devices[dev.Name] = dev
	return nil
}

// RemoveDevice unregisters a block device
func (s *Block9PServer) RemoveDevice(name string) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if _, exists := s.devices[name]; !exists {
		return fmt.Errorf("device %s not found", name)
	}

	delete(s.devices, name)
	return nil
}

// File represents a virtual file in the 9P filesystem
type File struct {
	Name     string
	IsDir    bool
	Device   *BlockDevice
	FileType FileType
	PartNum  int // For partition files
}

// FileType identifies what kind of file this is
type FileType int

const (
	FileTypeRoot FileType = iota
	FileTypeDeviceDir
	FileTypeRaw
	FileTypeData
	FileTypeCtl
	FileTypeGeometry
	FileTypePartDir
	FileTypePartition
)

// 9P Operations Interface
// These methods handle actual 9P protocol messages

// Read implements the 9P Tread operation for block devices
func (s *Block9PServer) Read(path string, offset uint64, count uint32) ([]byte, error) {
	file, err := s.resolvePath(path)
	if err != nil {
		return nil, err
	}

	switch file.FileType {
	case FileTypeRaw, FileTypeData:
		// Read from raw device
		return s.readRawDevice(file.Device, offset, count)

	case FileTypePartition:
		// Read from partition
		return s.readPartition(file.Device, file.PartNum, offset, count)

	case FileTypeCtl:
		// Read control info
		return s.readCtl(file.Device)

	case FileTypeGeometry:
		// Read geometry info
		return s.readGeometry(file.Device)

	default:
		return nil, fmt.Errorf("cannot read from %s", path)
	}
}

// Write implements the 9P Twrite operation for block devices
func (s *Block9PServer) Write(path string, offset uint64, data []byte) (uint32, error) {
	file, err := s.resolvePath(path)
	if err != nil {
		return 0, err
	}

	switch file.FileType {
	case FileTypeRaw, FileTypeData:
		// Write to raw device
		return s.writeRawDevice(file.Device, offset, data)

	case FileTypePartition:
		// Write to partition
		return s.writePartition(file.Device, file.PartNum, offset, data)

	case FileTypeCtl:
		// Write control command
		return s.writeCtl(file.Device, data)

	default:
		return 0, fmt.Errorf("cannot write to %s", path)
	}
}

// readRawDevice reads from the raw device
func (s *Block9PServer) readRawDevice(dev *BlockDevice, offset uint64, count uint32) ([]byte, error) {
	dev.mu.RLock()
	defer dev.mu.RUnlock()

	// Convert offset to LBA
	lba := offset / uint64(dev.SectorSize)

	// Calculate number of sectors to read
	// Round up to include partial sectors
	sectors := (count + dev.SectorSize - 1) / dev.SectorSize

	// Bounds check
	if lba+uint64(sectors) > dev.Sectors {
		return nil, io.EOF
	}

	// Issue read command to hardware
	// This will be implemented in ahci.go
	return dev.Port.Read(lba, sectors)
}

// writeRawDevice writes to the raw device
func (s *Block9PServer) writeRawDevice(dev *BlockDevice, offset uint64, data []byte) (uint32, error) {
	dev.mu.Lock()
	defer dev.mu.Unlock()

	lba := offset / uint64(dev.SectorSize)
	sectors := uint32((len(data) + int(dev.SectorSize) - 1) / int(dev.SectorSize))

	if lba+uint64(sectors) > dev.Sectors {
		return 0, fmt.Errorf("write beyond end of device")
	}

	// Issue write command to hardware
	err := dev.Port.Write(lba, data)
	if err != nil {
		return 0, err
	}

	return uint32(len(data)), nil
}

// readPartition reads from a partition
func (s *Block9PServer) readPartition(dev *BlockDevice, partNum int, offset uint64, count uint32) ([]byte, error) {
	dev.mu.RLock()
	defer dev.mu.RUnlock()

	if partNum >= len(dev.Partitions) {
		return nil, fmt.Errorf("partition %d not found", partNum)
	}

	part := dev.Partitions[partNum]

	// Translate partition offset to absolute LBA
	partLBA := offset / uint64(dev.SectorSize)
	absoluteLBA := part.StartLBA + partLBA

	// Bounds check within partition
	sectors := (count + dev.SectorSize - 1) / dev.SectorSize
	if absoluteLBA+uint64(sectors) > part.EndLBA {
		return nil, io.EOF
	}

	// Read from underlying device
	return dev.Port.Read(absoluteLBA, sectors)
}

// writePartition writes to a partition
func (s *Block9PServer) writePartition(dev *BlockDevice, partNum int, offset uint64, data []byte) (uint32, error) {
	dev.mu.Lock()
	defer dev.mu.Unlock()

	if partNum >= len(dev.Partitions) {
		return 0, fmt.Errorf("partition %d not found", partNum)
	}

	part := dev.Partitions[partNum]
	partLBA := offset / uint64(dev.SectorSize)
	absoluteLBA := part.StartLBA + partLBA

	sectors := uint32((len(data) + int(dev.SectorSize) - 1) / int(dev.SectorSize))
	if absoluteLBA+uint64(sectors) > part.EndLBA {
		return 0, fmt.Errorf("write beyond partition boundary")
	}

	err := dev.Port.Write(absoluteLBA, data)
	if err != nil {
		return 0, err
	}

	return uint32(len(data)), nil
}

// readCtl reads control information
func (s *Block9PServer) readCtl(dev *BlockDevice) ([]byte, error) {
	dev.mu.RLock()
	defer dev.mu.RUnlock()

	info := fmt.Sprintf("device %s\nmodel %s\nserial %s\nstatus ready\n",
		dev.Name, dev.Model, dev.Serial)
	return []byte(info), nil
}

// readGeometry reads geometry information
func (s *Block9PServer) readGeometry(dev *BlockDevice) ([]byte, error) {
	dev.mu.RLock()
	defer dev.mu.RUnlock()

	// Calculate CHS geometry (legacy, but useful)
	cylinders := dev.Sectors / (255 * 63)
	if cylinders == 0 {
		cylinders = 1
	}

	info := fmt.Sprintf("sectors %d\nsecsize %d\ncylinders %d\nheads 255\nsectors/track 63\n",
		dev.Sectors, dev.SectorSize, cylinders)
	return []byte(info), nil
}

// writeCtl handles control commands
func (s *Block9PServer) writeCtl(dev *BlockDevice, data []byte) (uint32, error) {
	cmd := string(data)

	switch cmd {
	case "flush\n", "flush":
		// Flush device cache
		return uint32(len(data)), dev.Port.Flush()

	case "rescan\n", "rescan":
		// Rescan for partitions
		return uint32(len(data)), s.rescanPartitions(dev)

	default:
		return 0, fmt.Errorf("unknown control command: %s", cmd)
	}
}

// RescanPartitions reads partition table and updates partition list (exported)
func (s *Block9PServer) RescanPartitions(dev *BlockDevice) error {
	return s.rescanPartitions(dev)
}

// rescanPartitions reads partition table and updates partition list
func (s *Block9PServer) rescanPartitions(dev *BlockDevice) error {
	// Read sector 0 (MBR) and sector 1 (GPT header)
	mbr, err := dev.Port.Read(0, 1)
	if err != nil {
		return fmt.Errorf("failed to read MBR: %w", err)
	}

	// Check for GPT
	if len(mbr) >= 512 && mbr[510] == 0x55 && mbr[511] == 0xAA {
		// Could be GPT or MBR
		gptHeader, err := dev.Port.Read(1, 1)
		if err == nil && len(gptHeader) >= 512 {
			if string(gptHeader[0:8]) == "EFI PART" {
				// GPT partition table
				return s.parseGPT(dev, gptHeader)
			}
		}

		// Try MBR
		return s.parseMBR(dev, mbr)
	}

	return fmt.Errorf("no valid partition table found")
}

// parseMBR parses an MBR partition table
func (s *Block9PServer) parseMBR(dev *BlockDevice, mbr []byte) error {
	dev.Partitions = make([]*Partition, 0)

	// MBR partition entries start at offset 446
	for i := 0; i < 4; i++ {
		offset := 446 + (i * 16)
		if offset+16 > len(mbr) {
			break
		}

		entry := mbr[offset : offset+16]

		// Check if partition is active (non-zero type)
		partType := entry[4]
		if partType == 0 {
			continue
		}

		// Read LBA start and size (little endian)
		startLBA := uint64(entry[8]) | uint64(entry[9])<<8 | uint64(entry[10])<<16 | uint64(entry[11])<<24
		numSectors := uint64(entry[12]) | uint64(entry[13])<<8 | uint64(entry[14])<<16 | uint64(entry[15])<<24

		part := &Partition{
			Number:   i + 1,
			StartLBA: startLBA,
			EndLBA:   startLBA + numSectors - 1,
			Type:     fmt.Sprintf("0x%02x", partType),
			Name:     fmt.Sprintf("partition%d", i+1),
		}

		dev.Partitions = append(dev.Partitions, part)
	}

	return nil
}

// parseGPT parses a GPT partition table
func (s *Block9PServer) parseGPT(dev *BlockDevice, header []byte) error {
	// TODO: Full GPT parsing
	// For now, just placeholder
	dev.Partitions = make([]*Partition, 0)
	return fmt.Errorf("GPT parsing not yet implemented")
}

// resolvePath converts a 9P path to a File object
func (s *Block9PServer) resolvePath(path string) (*File, error) {
	// Example paths:
	// "/dev/sd/sdC0/raw"
	// "/dev/sd/sdC0/part/2"
	// "/dev/sd/sdC0/ctl"

	// Simple path parsing (TODO: make more robust)
	// For now, just extract device name and file type

	// This is a placeholder - real implementation needs proper path parsing
	return nil, fmt.Errorf("path resolution not yet implemented")
}

// Mount starts serving the 9P filesystem at the given mount point
func (s *Block9PServer) Mount(ctx context.Context, mountPoint string) error {
	// TODO: Integrate with actual 9P server library
	// For now, just log
	fmt.Printf("Block9P: Would mount at %s\n", mountPoint)
	return nil
}
