// AHCI Driver - Userspace AHCI driver as a SIP server
// Exposes block devices via 9P at /dev/sd
package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"

	ahcidriver "lux9/userspace/go-servers/ahci-driver"
	"lux9/userspace/go-servers/sip"
)

// AHCIDriverServer implements the SIP IDeviceDriver interface
type AHCIDriverServer struct {
	*sip.BaseServer
	controller *ahcidriver.AHCIController
	blockServer *ahcidriver.Block9PServer
	logger     *sip.DefaultLogger
}

// Ensure we implement the interface
var _ sip.IDeviceDriver = (*AHCIDriverServer)(nil)

// NewAHCIDriver creates a new AHCI driver instance
func NewAHCIDriver(config *sip.ServerConfig) (sip.IServer, error) {
	return &AHCIDriverServer{
		BaseServer:  sip.NewBaseServer(config),
		logger:      sip.NewDefaultLogger(config.Name),
		blockServer: ahcidriver.NewBlock9PServer(),
	}, nil
}

// Initialize implements IDeviceDriver
func (d *AHCIDriverServer) Initialize(ctx context.Context, config *sip.ServerConfig) error {
	if err := d.BaseServer.Initialize(ctx, config); err != nil {
		return err
	}

	d.logger.Info("Initializing AHCI driver")

	// Verify capabilities
	requiredCaps := sip.CapDeviceAccess | sip.CapInterrupt | sip.CapDMA
	if config.Capabilities&requiredCaps != requiredCaps {
		return fmt.Errorf("AHCI driver requires CapDeviceAccess, CapInterrupt, and CapDMA")
	}

	d.logger.Info("AHCI driver initialized")
	return nil
}

// Start implements IDeviceDriver
func (d *AHCIDriverServer) Start(ctx context.Context) error {
	d.logger.Info("Starting AHCI driver")

	if err := d.BaseServer.Start(ctx); err != nil {
		return err
	}

	// Probe for devices
	devices, err := d.Probe(ctx)
	if err != nil {
		d.logger.Error("Probe failed: %v", err)
		return err
	}

	d.logger.Info("Found %d AHCI devices", len(devices))

	// Attach each device
	for _, devPath := range devices {
		if err := d.AttachDevice(ctx, devPath); err != nil {
			d.logger.Error("Failed to attach %s: %v", devPath, err)
			continue
		}
		d.logger.Info("Attached device: %s", devPath)
	}

	// Mount 9P filesystem at /dev/sd
	if err := d.blockServer.Mount(ctx, "/dev/sd"); err != nil {
		return fmt.Errorf("failed to mount 9P server: %w", err)
	}

	d.logger.Info("AHCI driver started, serving at /dev/sd")
	return nil
}

// Stop implements IDeviceDriver
func (d *AHCIDriverServer) Stop(ctx context.Context) error {
	d.logger.Info("Stopping AHCI driver")

	// TODO: Unmount 9P filesystem
	// TODO: Stop command processing
	// TODO: Disable interrupts

	return d.BaseServer.Stop(ctx)
}

// Probe implements IDeviceDriver
func (d *AHCIDriverServer) Probe(ctx context.Context) ([]string, error) {
	d.logger.Info("Probing for AHCI controllers")

	// TODO: Scan PCI bus via /dev/pci or similar
	// For now, use hardcoded values from common locations
	// In real implementation, would read PCI config space

	// Common AHCI BAR address (from PCI BAR5)
	// This would normally come from PCI enumeration
	pciBar := uint64(0xFEDC0000) // Example AHCI base address
	irq := 11                     // Example IRQ number

	d.logger.Info("Found AHCI controller at 0x%x, IRQ %d", pciBar, irq)

	// Create controller
	ctrl, err := ahcidriver.NewAHCIController(pciBar, irq)
	if err != nil {
		return nil, fmt.Errorf("failed to create controller: %w", err)
	}

	d.controller = ctrl

	// Initialize controller
	if err := ctrl.Initialize(); err != nil {
		return nil, fmt.Errorf("failed to initialize controller: %w", err)
	}

	// Return device paths for each port
	devices := make([]string, 0)
	for i := range ctrl.Ports() {
		devices = append(devices, fmt.Sprintf("sdC%d", i))
	}

	return devices, nil
}

// AttachDevice implements IDeviceDriver
func (d *AHCIDriverServer) AttachDevice(ctx context.Context, devicePath string) error {
	d.logger.Info("Attaching device: %s", devicePath)

	// Find the corresponding AHCI port
	// devicePath is like "sdC0" where C is controller, 0 is port
	var portNum int
	fmt.Sscanf(devicePath, "sdC%d", &portNum)

	ports := d.controller.Ports()
	if portNum >= len(ports) {
		return fmt.Errorf("invalid port number: %d", portNum)
	}

	port := ports[portNum]

	// Identify the device to get capacity and model info
	model, serial, sectors, err := port.Identify()
	if err != nil {
		return fmt.Errorf("failed to identify device: %w", err)
	}

	// Create BlockDevice structure
	dev := &ahcidriver.BlockDevice{
		Name:       devicePath,
		SectorSize: 512, // Standard sector size
		Sectors:    sectors,
		Model:      model,
		Serial:     serial,
		Port:       port,
	}

	// Read partition table
	if err := d.blockServer.RescanPartitions(dev); err != nil {
		d.logger.Error("Failed to read partitions for %s: %v", devicePath, err)
		// Continue anyway - raw device access still works
	}

	// Add to 9P server
	if err := d.blockServer.AddDevice(dev); err != nil {
		return fmt.Errorf("failed to add device to 9P server: %w", err)
	}

	d.IncrementRequests()
	return nil
}

// DetachDevice implements IDeviceDriver
func (d *AHCIDriverServer) DetachDevice(ctx context.Context, devicePath string) error {
	d.logger.Info("Detaching device: %s", devicePath)

	// TODO: Flush pending I/O
	// TODO: Stop command processing
	// TODO: Remove from 9P server

	if err := d.blockServer.RemoveDevice(devicePath); err != nil {
		return err
	}

	return nil
}

// HandleInterrupt implements IDeviceDriver
func (d *AHCIDriverServer) HandleInterrupt(ctx context.Context, irq int) error {
	d.logger.Debug("Handling interrupt: IRQ %d", irq)
	// Interrupts are handled by the AHCI controller's irqHandler goroutine
	// This method is here for interface compliance
	d.IncrementRequests()
	return nil
}

// Helper method (not part of interface)
func (d *AHCIDriverServer) Ports() []*ahcidriver.AHCIPort {
	if d.controller == nil {
		return nil
	}
	return d.controller.Ports()
}

func main() {
	log.SetPrefix("[ahci-driver] ")
	log.SetFlags(log.Ldate | log.Ltime | log.Lmicroseconds)

	// Create SIP factory and register AHCI driver
	factory := sip.NewServerFactory()
	if err := factory.Register("ahci-driver", NewAHCIDriver); err != nil {
		log.Fatalf("Failed to register AHCI driver: %v", err)
	}

	// Create server manager
	manager := sip.NewServerManager(factory)

	// Configure AHCI driver server
	config := &sip.ServerConfig{
		Name:         "ahci-driver",
		Capabilities: sip.CapDeviceAccess | sip.CapInterrupt | sip.CapDMA,
		MountPoint:   "/dev/sd",
		Priority:     10,
	}

	// Start the driver
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	if err := manager.StartServer(ctx, "ahci-driver", config); err != nil {
		log.Fatalf("Failed to start AHCI driver: %v", err)
	}

	log.Printf("AHCI driver running")

	// Wait for signals
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	<-sigChan
	log.Printf("Shutting down...")

	// Stop all servers
	if err := manager.StopAll(ctx); err != nil {
		log.Printf("Error during shutdown: %v", err)
	}

	log.Printf("AHCI driver stopped")
}
