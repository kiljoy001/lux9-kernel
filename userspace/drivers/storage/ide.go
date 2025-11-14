package storage

import (
	"fmt"
	"log"
)

// IDEDriver implements the StorageDriver interface for IDE controllers
type IDEDriver struct {
	controllers []IDEController
}

// IDEController represents an IDE controller
type IDEController struct {
	CommandPort uint16
	ControlPort uint16
	Devices     []IDEDevice
}

// IDEDevice represents an IDE device
type IDEDevice struct {
	DeviceNumber int
	DeviceType   int
	Capacity     uint64 // in sectors
}

// NewIDEDriver creates a new IDE driver
func NewIDEDriver() *IDEDriver {
	return &IDEDriver{}
}

// Name returns the driver name
func (i *IDEDriver) Name() string {
	return "ide"
}

// Init initializes the IDE driver
func (i *IDEDriver) Init() error {
	// In a real implementation, this would scan for IDE controllers
	// and initialize them. For now, we'll simulate finding controllers.
	
	log.Printf("Initializing IDE driver...")
	
	// Simulate finding one IDE controller with two devices
	controller := IDEController{
		CommandPort: 0x1F0,
		ControlPort: 0x3F4,
		Devices: []IDEDevice{
			{
				DeviceNumber: 0,
				DeviceType:   1, // ATA device
				Capacity:     512 * 1024, // 512K sectors (256MB)
			},
			{
				DeviceNumber: 1,
				DeviceType:   1, // ATA device
				Capacity:     256 * 1024, // 256K sectors (128MB)
			},
		},
	}
	
	i.controllers = append(i.controllers, controller)
	log.Printf("Found %d IDE controller(s)", len(i.controllers))
	
	return nil
}

// ReadSector reads a sector from the IDE device
func (i *IDEDriver) ReadSector(device int, lba uint64, buffer []byte) error {
	if len(buffer) < 512 {
		return fmt.Errorf("buffer too small for sector read")
	}
	
	if len(i.controllers) == 0 || device >= len(i.controllers[0].Devices) {
		return fmt.Errorf("device %d not found", device)
	}
	
	// In a real implementation, this would:
	// 1. Send IDE read command
	// 2. Wait for DRQ bit
	// 3. Read data from data port
	// 4. Wait for completion
	
	// For simulation, just fill with test data
	for j := 0; j < 512; j++ {
		buffer[j] = byte((lba >> uint(j%8)) & 0xFF) ^ 0xFF
	}
	
	// Add some identifiable pattern for debugging
	buffer[0] = byte(device | 0x80) // High bit set to distinguish from AHCI
	buffer[1] = byte(lba & 0xFF)
	buffer[2] = byte((lba >> 8) & 0xFF)
	buffer[3] = byte((lba >> 16) & 0xFF)
	buffer[4] = byte((lba >> 24) & 0xFF)
	buffer[5] = 0xCC // Signature
	buffer[6] = 0x33 // Signature
	
	return nil
}

// WriteSector writes a sector to the IDE device
func (i *IDEDriver) WriteSector(device int, lba uint64, buffer []byte) error {
	if len(buffer) < 512 {
		return fmt.Errorf("buffer too small for sector write")
	}
	
	if len(i.controllers) == 0 || device >= len(i.controllers[0].Devices) {
		return fmt.Errorf("device %d not found", device)
	}
	
	// In a real implementation, this would:
	// 1. Send IDE write command
	// 2. Wait for DRQ bit
	// 3. Write data to data port
	// 4. Wait for completion
	
	// For simulation, just log the write
	log.Printf("IDE write: device=%d lba=%d", device, lba)
	
	return nil
}

// GetDeviceCapacity returns the device capacity in sectors
func (i *IDEDriver) GetDeviceCapacity(device int) (uint64, error) {
	if len(i.controllers) == 0 || len(i.controllers[0].Devices) == 0 {
		return 0, fmt.Errorf("no IDE devices found")
	}
	
	if device >= len(i.controllers[0].Devices) {
		return 0, fmt.Errorf("device %d not found", device)
	}
	
	return i.controllers[0].Devices[device].Capacity, nil
}

// GetDeviceCount returns the number of available devices
func (i *IDEDriver) GetDeviceCount() int {
	if len(i.controllers) == 0 {
		return 0
	}
	return len(i.controllers[0].Devices)
}
