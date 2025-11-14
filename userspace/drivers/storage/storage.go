package storage

import (
	"fmt"
	"log"
	"path/filepath"
	"syscall"
	"unsafe"
)

// StorageDriver represents a storage device driver
type StorageDriver interface {
	// Name returns the driver name
	Name() string
	
	// Init initializes the driver
	Init() error
	
	// ReadSector reads a sector from the device
	ReadSector(device int, lba uint64, buffer []byte) error
	
	// WriteSector writes a sector to the device
	WriteSector(device int, lba uint64, buffer []byte) error
	
	// GetDeviceCapacity returns the device capacity in sectors
	GetDeviceCapacity(device int) (uint64, error)
	
	// GetDeviceCount returns the number of available devices
	GetDeviceCount() int
}

// StorageDevice represents a storage device
type StorageDevice struct {
	ID       int
	Capacity uint64 // in sectors
	Model    string
}

// BlockDeviceDriver implements the driver interface for block devices
type BlockDeviceDriver struct {
	driver  StorageDriver
	devices []StorageDevice
}

// NewBlockDeviceDriver creates a new block device driver
func NewBlockDeviceDriver(storageDrv StorageDriver) *BlockDeviceDriver {
	return &BlockDeviceDriver{
		driver: storageDrv,
	}
}

// Name returns the driver name
func (b *BlockDeviceDriver) Name() string {
	return b.driver.Name()
}

// Init initializes the driver
func (b *BlockDeviceDriver) Init() error {
	if err := b.driver.Init(); err != nil {
		return err
	}
	
	// Initialize devices
	deviceCount := b.driver.GetDeviceCount()
	b.devices = make([]StorageDevice, deviceCount)
	
	for i := 0; i < deviceCount; i++ {
		capacity, err := b.driver.GetDeviceCapacity(i)
		if err != nil {
			log.Printf("Warning: Failed to get capacity for device %d: %v", i, err)
			capacity = 0
		}
		
		b.devices[i] = StorageDevice{
			ID:       i,
			Capacity: capacity,
			Model:    fmt.Sprintf("%s-device-%d", b.driver.Name(), i),
		}
	}
	
	log.Printf("Initialized %s driver with %d devices", b.driver.Name(), deviceCount)
	return nil
}

// HandleRead handles read operations
func (b *BlockDeviceDriver) HandleRead(path string, offset uint64, count uint32) ([]byte, error) {
	// Parse path: disk/N/data or disk/N/ctl
	if len(path) >= 5 && path[:5] == "disk/" {
		// Extract device number from path
		var deviceID int
		n, err := fmt.Sscanf(path[5:], "%d/", &deviceID)
		if err != nil || n != 1 {
			return nil, fmt.Errorf("invalid device path: %s", path)
		}
		
		if deviceID >= len(b.devices) {
			return nil, fmt.Errorf("device %d not found", deviceID)
		}
		
		// Check if this is a control file or data file
		if filepath.Base(path) == "ctl" {
			// Return control information
			ctlInfo := fmt.Sprintf("device: %d\nmodel: %s\ncapacity: %d sectors\nsector_size: 512\n",
				deviceID, b.devices[deviceID].Model, b.devices[deviceID].Capacity)
			return []byte(ctlInfo), nil
		}
		
		// This is a data read - convert byte offset to sector operations
		sectorSize := uint64(512)
		startSector := offset / sectorSize
		byteOffset := offset % sectorSize
		
		// Calculate number of sectors needed
		sectorsNeeded := (uint64(count) + byteOffset + sectorSize - 1) / sectorSize
		if sectorsNeeded == 0 && count > 0 {
			sectorsNeeded = 1
		}
		
		// Read sectors
		buffer := make([]byte, sectorsNeeded*sectorSize)
		for i := uint64(0); i < sectorsNeeded; i++ {
			err := b.driver.ReadSector(deviceID, startSector+i, buffer[i*sectorSize:(i+1)*sectorSize])
			if err != nil {
				return nil, fmt.Errorf("failed to read sector %d: %v", startSector+i, err)
			}
		}
		
		// Return requested data
		endOffset := byteOffset + uint64(count)
		if endOffset > uint64(len(buffer)) {
			endOffset = uint64(len(buffer))
		}
		
		return buffer[byteOffset:endOffset], nil
	}
	
	return nil, fmt.Errorf("invalid path: %s", path)
}

// HandleWrite handles write operations
func (b *BlockDeviceDriver) HandleWrite(path string, offset uint64, data []byte) (uint32, error) {
	// Parse path: disk/N/data or disk/N/ctl
	if len(path) >= 5 && path[:5] == "disk/" {
		// Extract device number from path
		var deviceID int
		n, err := fmt.Sscanf(path[5:], "%d/", &deviceID)
		if err != nil || n != 1 {
			return 0, fmt.Errorf("invalid device path: %s", path)
		}
		
		if deviceID >= len(b.devices) {
			return 0, fmt.Errorf("device %d not found", deviceID)
		}
		
		// Check if this is a control file
		if filepath.Base(path) == "ctl" {
			// Handle control commands (for now, reject writes)
			return 0, fmt.Errorf("control file is read-only")
		}
		
		// This is a data write - convert byte offset to sector operations
		sectorSize := uint64(512)
		startSector := offset / sectorSize
		byteOffset := offset % sectorSize
		
		// Calculate number of sectors needed
		sectorsNeeded := (uint64(len(data)) + byteOffset + sectorSize - 1) / sectorSize
		if sectorsNeeded == 0 && len(data) > 0 {
			sectorsNeeded = 1
		}
		
		// For partial sector writes, we need to read-modify-write
		buffer := make([]byte, sectorsNeeded*sectorSize)
		
		// Read existing data for partial sectors
		if byteOffset > 0 || (uint64(len(data))%sectorSize) != 0 {
			for i := uint64(0); i < sectorsNeeded; i++ {
				err := b.driver.ReadSector(deviceID, startSector+i, buffer[i*sectorSize:(i+1)*sectorSize])
				if err != nil {
					// If read fails, zero-fill the buffer
					for j := i * sectorSize; j < (i+1)*sectorSize; j++ {
						buffer[j] = 0
					}
				}
			}
		}
		
		// Copy new data into buffer
		copy(buffer[byteOffset:], data)
		
		// Write sectors
		for i := uint64(0); i < sectorsNeeded; i++ {
			err := b.driver.WriteSector(deviceID, startSector+i, buffer[i*sectorSize:(i+1)*sectorSize])
			if err != nil {
				return uint32(i * sectorSize), fmt.Errorf("failed to write sector %d: %v", startSector+i, err)
			}
		}
		
		return uint32(len(data)), nil
	}
	
	return 0, fmt.Errorf("invalid path: %s", path)
}

// HandleOpen handles file open operations
func (b *BlockDeviceDriver) HandleOpen(path string, mode uint8) error {
	// Allow opening of existing files
	return nil
}

// HandleCreate handles file creation
func (b *BlockDeviceDriver) HandleCreate(path string, perm uint32, mode uint8) error {
	// Device files are created automatically, no manual creation allowed
	return fmt.Errorf("cannot create device files")
}

// HandleRemove handles file removal
func (b *BlockDeviceDriver) HandleRemove(path string) error {
	// Device files cannot be removed
	return fmt.Errorf("cannot remove device files")
}

// HandleStat returns file statistics
func (b *BlockDeviceDriver) HandleStat(path string) (interface{}, error) {
	// Root directory
	if path == "" || path == "/" {
		return struct {
			Name string
			Mode uint32
		}{"storage", 0755 | syscall.S_IFDIR}, nil
	}
	
	// Disk directory
	if path == "disk" {
		return struct {
			Name string
			Mode uint32
		}{"disk", 0755 | syscall.S_IFDIR}, nil
	}
	
	// Device directories and files
	if len(path) >= 5 && path[:5] == "disk/" {
		var deviceID int
		rest := path[5:]
		
		// Check if this is a device directory
		n, err := fmt.Sscanf(rest, "%d", &deviceID)
		if err == nil && n == 1 {
			if deviceID < len(b.devices) {
				// This is a device directory
				if rest == fmt.Sprintf("%d", deviceID) || rest == fmt.Sprintf("%d/", deviceID) {
					return struct {
						Name string
						Mode uint32
					}{fmt.Sprintf("%d", deviceID), 0755 | syscall.S_IFDIR}, nil
				}
				
				// Check for specific files within device directory
				if len(rest) > len(fmt.Sprintf("%d/", deviceID)) {
					filename := rest[len(fmt.Sprintf("%d/", deviceID)):]
					switch filename {
					case "data":
						return struct {
							Name string
							Mode uint32
							Size uint64
						}{"data", 0644 | syscall.S_IFREG, b.devices[deviceID].Capacity * 512}, nil
					case "ctl":
						return struct {
							Name string
							Mode uint32
							Size uint64
						}{"ctl", 0644 | syscall.S_IFREG, 128}, nil
					}
				}
			}
		}
	}
	
	return nil, fmt.Errorf("file not found: %s", path)
}

// HandleWstat updates file statistics
func (b *BlockDeviceDriver) HandleWstat(path string, stat interface{}) error {
	// Device files are read-only in terms of metadata
	return fmt.Errorf("cannot modify device file metadata")
}
