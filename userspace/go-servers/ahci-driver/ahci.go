// AHCI hardware driver implementation
// Accesses AHCI controller via /dev/mem and /dev/irq
package ahcidriver

import (
	"encoding/binary"
	"fmt"
	"os"
	"sync"
	"unsafe"
)

// AHCI Register offsets (HBA Generic Host Control)
const (
	AHCIRegCAP     = 0x00 // Host Capabilities
	AHCIRegGHC     = 0x04 // Global Host Control
	AHCIRegIS      = 0x08 // Interrupt Status
	AHCIRegPI      = 0x0C // Ports Implemented
	AHCIRegVS      = 0x10 // Version
	AHCIRegCCCCTL  = 0x14 // Command Completion Coalescing Control
	AHCIRegCCCPORTS = 0x18 // Command Completion Coalescing Ports
	AHCIRegEMLOC   = 0x1C // Enclosure Management Location
	AHCIRegEMCTL   = 0x20 // Enclosure Management Control
	AHCIRegCAP2    = 0x24 // Host Capabilities Extended
)

// AHCI Port register offsets (from port base)
const (
	PortRegCLB   = 0x00 // Command List Base Address
	PortRegCLBU  = 0x04 // Command List Base Address Upper
	PortRegFB    = 0x08 // FIS Base Address
	PortRegFBU   = 0x0C // FIS Base Address Upper
	PortRegIS    = 0x10 // Interrupt Status
	PortRegIE    = 0x14 // Interrupt Enable
	PortRegCMD   = 0x18 // Command and Status
	PortRegTFD   = 0x20 // Task File Data
	PortRegSIG   = 0x24 // Signature
	PortRegSSTS  = 0x28 // Serial ATA Status
	PortRegSCTL  = 0x2C // Serial ATA Control
	PortRegSERR  = 0x30 // Serial ATA Error
	PortRegSACT  = 0x34 // Serial ATA Active
	PortRegCI    = 0x38 // Command Issue
)

// Port base offset calculation
func portBase(portNum int) uint64 {
	return 0x100 + uint64(portNum)*0x80
}

// AHCIController represents the AHCI HBA
type AHCIController struct {
	baseAddr  uint64        // Physical address of AHCI registers
	memFile   *os.File      // /dev/mem file descriptor
	irqFile   *os.File      // /dev/irq/N file descriptor
	irqNum    int           // IRQ number
	ports     []*AHCIPort   // Active ports
	mu        sync.Mutex
}

// AHCIPort represents a single AHCI port
type AHCIPort struct {
	controller *AHCIController
	portNum    int
	baseOffset uint64 // Offset from controller base

	// DMA buffers (allocated via /dev/dma)
	cmdListVirt  uintptr // Virtual address of command list
	cmdListPhys  uint64  // Physical address for hardware
	fisVirt      uintptr // FIS receive area virtual
	fisPhys      uint64  // FIS receive area physical
	ctVirt       []uintptr // Command tables virtual
	ctPhys       []uint64  // Command tables physical

	// Synchronization
	irqChan chan struct{} // Closed when interrupt received
	mu      sync.Mutex
}

// NewAHCIController creates a new AHCI controller instance
func NewAHCIController(pciBar uint64, irq int) (*AHCIController, error) {
	// Open /dev/mem for MMIO access
	memFile, err := os.OpenFile("/dev/mem", os.O_RDWR, 0)
	if err != nil {
		return nil, fmt.Errorf("failed to open /dev/mem: %w", err)
	}

	ctrl := &AHCIController{
		baseAddr: pciBar,
		memFile:  memFile,
		irqNum:   irq,
		ports:    make([]*AHCIPort, 0),
	}

	return ctrl, nil
}

// Initialize sets up the AHCI controller
func (c *AHCIController) Initialize() error {
	// Enable AHCI mode
	ghc := c.readReg32(AHCIRegGHC)
	ghc |= (1 << 31) // Set AHCI Enable bit
	c.writeReg32(AHCIRegGHC, ghc)

	// Request interrupt ownership
	ghc |= (1 << 1) // Set Interrupt Enable bit
	c.writeReg32(AHCIRegGHC, ghc)

	// Register for interrupts via /dev/irq
	if err := c.registerIRQ(); err != nil {
		return fmt.Errorf("failed to register IRQ: %w", err)
	}

	// Detect implemented ports
	portsImpl := c.readReg32(AHCIRegPI)

	for i := 0; i < 32; i++ {
		if portsImpl&(1<<uint(i)) != 0 {
			port, err := c.initializePort(i)
			if err != nil {
				fmt.Printf("Failed to initialize port %d: %v\n", i, err)
				continue
			}
			c.ports = append(c.ports, port)
		}
	}

	return nil
}

// initializePort sets up a single AHCI port
func (c *AHCIController) initializePort(portNum int) (*AHCIPort, error) {
	port := &AHCIPort{
		controller: c,
		portNum:    portNum,
		baseOffset: portBase(portNum),
		irqChan:    make(chan struct{}, 1),
	}

	// Check if device is present
	ssts := c.readPortReg32(port, PortRegSSTS)
	det := ssts & 0xF
	if det != 3 {
		return nil, fmt.Errorf("no device present (DET=%d)", det)
	}

	// Allocate DMA buffers
	if err := port.allocateDMABuffers(); err != nil {
		return nil, err
	}

	// Set command list and FIS base addresses
	c.writePortReg32(port, PortRegCLB, uint32(port.cmdListPhys))
	c.writePortReg32(port, PortRegCLBU, uint32(port.cmdListPhys>>32))
	c.writePortReg32(port, PortRegFB, uint32(port.fisPhys))
	c.writePortReg32(port, PortRegFBU, uint32(port.fisPhys>>32))

	// Enable FIS receive
	cmd := c.readPortReg32(port, PortRegCMD)
	cmd |= (1 << 4) // FRE - FIS Receive Enable
	c.writePortReg32(port, PortRegCMD, cmd)

	// Start command engine
	cmd |= (1 << 0) // ST - Start
	c.writePortReg32(port, PortRegCMD, cmd)

	// Enable interrupts for this port
	c.writePortReg32(port, PortRegIE, 0xFFFFFFFF)

	return port, nil
}

// allocateDMABuffers allocates DMA-capable memory for AHCI structures
func (p *AHCIPort) allocateDMABuffers() error {
	// TODO: Use /dev/dma/alloc when implemented
	// For now, use placeholder values
	// In real implementation:
	// 1. Open /dev/dma/alloc
	// 2. Write allocation request
	// 3. Read back virtual/physical addresses

	// Command list: 32 entries * 32 bytes = 1024 bytes
	// FIS area: 256 bytes
	// Command tables: 32 * 256 bytes = 8192 bytes

	// Placeholder - these would come from DMA allocator
	p.cmdListVirt = 0x80000000
	p.cmdListPhys = 0x10000000
	p.fisVirt = 0x80001000
	p.fisPhys = 0x10001000

	p.ctVirt = make([]uintptr, 32)
	p.ctPhys = make([]uint64, 32)
	for i := 0; i < 32; i++ {
		p.ctVirt[i] = 0x80002000 + uintptr(i*256)
		p.ctPhys[i] = 0x10002000 + uint64(i*256)
	}

	return nil
}

// registerIRQ registers for interrupt notifications via /dev/irq
func (c *AHCIController) registerIRQ() error {
	// Open /dev/irq/ctl to register
	ctlFile, err := os.OpenFile("/dev/irq/ctl", os.O_WRONLY, 0)
	if err != nil {
		return fmt.Errorf("failed to open /dev/irq/ctl: %w", err)
	}
	defer ctlFile.Close()

	// Write registration command
	cmd := fmt.Sprintf("register %d ahci-driver\n", c.irqNum)
	if _, err := ctlFile.Write([]byte(cmd)); err != nil {
		return fmt.Errorf("failed to register IRQ: %w", err)
	}

	// Open IRQ event file
	irqPath := fmt.Sprintf("/dev/irq/%d", c.irqNum)
	c.irqFile, err = os.OpenFile(irqPath, os.O_RDONLY, 0)
	if err != nil {
		return fmt.Errorf("failed to open %s: %w", irqPath, err)
	}

	// Start IRQ handler goroutine
	go c.irqHandler()

	return nil
}

// irqHandler waits for interrupts and dispatches to ports
func (c *AHCIController) irqHandler() {
	buf := make([]byte, 32)
	for {
		// Read blocks until interrupt
		n, err := c.irqFile.Read(buf)
		if err != nil {
			fmt.Printf("IRQ read error: %v\n", err)
			return
		}

		if n > 0 {
			// Interrupt received, read status
			is := c.readReg32(AHCIRegIS)

			// Dispatch to ports
			for i, port := range c.ports {
				if is&(1<<uint(i)) != 0 {
					// Clear port interrupt
					pis := c.readPortReg32(port, PortRegIS)
					c.writePortReg32(port, PortRegIS, pis)

					// Signal port
					select {
					case port.irqChan <- struct{}{}:
					default:
					}
				}
			}

			// Clear global interrupt status
			c.writeReg32(AHCIRegIS, is)
		}
	}
}

// Read reads sectors from the device
func (p *AHCIPort) Read(lba uint64, sectorCount uint32) ([]byte, error) {
	p.mu.Lock()
	defer p.mu.Unlock()

	// Find free command slot
	slot := p.findFreeSlot()
	if slot < 0 {
		return nil, fmt.Errorf("no free command slots")
	}

	// Build command
	p.buildReadCommand(slot, lba, sectorCount)

	// Issue command
	p.controller.writePortReg32(p, PortRegCI, 1<<uint(slot))

	// Wait for completion via interrupt
	<-p.irqChan

	// Check for errors
	tfd := p.controller.readPortReg32(p, PortRegTFD)
	if tfd&0x01 != 0 { // ERR bit
		return nil, fmt.Errorf("read error: TFD=0x%x", tfd)
	}

	// Copy data from DMA buffer
	data := make([]byte, sectorCount*512)
	// TODO: Copy from actual DMA buffer
	// For now, return empty data

	return data, nil
}

// Write writes sectors to the device
func (p *AHCIPort) Write(lba uint64, data []byte) error {
	p.mu.Lock()
	defer p.mu.Unlock()

	sectorCount := uint32((len(data) + 511) / 512)

	slot := p.findFreeSlot()
	if slot < 0 {
		return fmt.Errorf("no free command slots")
	}

	// TODO: Copy data to DMA buffer
	// Build write command
	p.buildWriteCommand(slot, lba, sectorCount)

	// Issue command
	p.controller.writePortReg32(p, PortRegCI, 1<<uint(slot))

	// Wait for completion
	<-p.irqChan

	tfd := p.controller.readPortReg32(p, PortRegTFD)
	if tfd&0x01 != 0 {
		return fmt.Errorf("write error: TFD=0x%x", tfd)
	}

	return nil
}

// Flush flushes the device cache
func (p *AHCIPort) Flush() error {
	p.mu.Lock()
	defer p.mu.Unlock()

	slot := p.findFreeSlot()
	if slot < 0 {
		return fmt.Errorf("no free command slots")
	}

	// Build flush command
	p.buildFlushCommand(slot)

	// Issue and wait
	p.controller.writePortReg32(p, PortRegCI, 1<<uint(slot))
	<-p.irqChan

	return nil
}

// findFreeSlot finds a free command slot
func (p *AHCIPort) findFreeSlot() int {
	ci := p.controller.readPortReg32(p, PortRegCI)
	for i := 0; i < 32; i++ {
		if ci&(1<<uint(i)) == 0 {
			return i
		}
	}
	return -1
}

// buildReadCommand builds an AHCI READ DMA EXT command
func (p *AHCIPort) buildReadCommand(slot int, lba uint64, count uint32) {
	// TODO: Build actual command structure in DMA memory
	// This involves:
	// 1. Setting up command header in command list
	// 2. Setting up FIS in command table
	// 3. Setting up PRDT entries
}

// buildWriteCommand builds an AHCI WRITE DMA EXT command
func (p *AHCIPort) buildWriteCommand(slot int, lba uint64, count uint32) {
	// TODO: Build actual command structure
}

// buildFlushCommand builds an AHCI FLUSH CACHE EXT command
func (p *AHCIPort) buildFlushCommand(slot int) {
	// TODO: Build actual command structure
}

// Register access helpers using /dev/mem

func (c *AHCIController) readReg32(offset uint64) uint32 {
	c.mu.Lock()
	defer c.mu.Unlock()

	addr := c.baseAddr + offset
	c.memFile.Seek(int64(addr), 0)

	var val uint32
	binary.Read(c.memFile, binary.LittleEndian, &val)
	return val
}

func (c *AHCIController) writeReg32(offset uint64, val uint32) {
	c.mu.Lock()
	defer c.mu.Unlock()

	addr := c.baseAddr + offset
	c.memFile.Seek(int64(addr), 0)
	binary.Write(c.memFile, binary.LittleEndian, val)
}

func (c *AHCIController) readPortReg32(p *AHCIPort, offset uint64) uint32 {
	return c.readReg32(p.baseOffset + offset)
}

func (c *AHCIController) writePortReg32(p *AHCIPort, offset uint64, val uint32) {
	c.writeReg32(p.baseOffset+offset, val)
}

// Identify reads device identification information
func (p *AHCIPort) Identify() (model string, serial string, sectors uint64, err error) {
	// TODO: Issue IDENTIFY DEVICE command
	// For now, return placeholder
	return "AHCI Device", "0000000000", 1000000, nil
}

// Ports returns all active ports on this controller
func (c *AHCIController) Ports() []*AHCIPort {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.ports
}

// Keep compiler happy
var _ = unsafe.Sizeof(0)
