// Example device driver using SIP framework
// This demonstrates how to create a hardware driver as a SIP server
package sip

import (
	"context"
	"fmt"
	"time"
)

// ExampleDriver demonstrates a simple device driver implementation
type ExampleDriver struct {
	*BaseServer // Embed base server for default implementations
	devices     []string
	logger      *DefaultLogger
}

// Verify that ExampleDriver implements IDeviceDriver
var _ IDeviceDriver = (*ExampleDriver)(nil)

// NewExampleDriver creates a new example driver
func NewExampleDriver(config *ServerConfig) (IServer, error) {
	driver := &ExampleDriver{
		BaseServer: NewBaseServer(config),
		devices:    make([]string, 0),
		logger:     NewDefaultLogger(config.Name),
	}
	return driver, nil
}

// Initialize implements IDeviceDriver
func (d *ExampleDriver) Initialize(ctx context.Context, config *ServerConfig) error {
	// Call base initialization
	if err := d.BaseServer.Initialize(ctx, config); err != nil {
		return err
	}

	d.logger.Info("Initializing driver")

	// Verify capabilities
	if config.Capabilities&CapDeviceAccess == 0 {
		return fmt.Errorf("driver requires CapDeviceAccess capability")
	}

	d.logger.Info("Driver initialized successfully")
	return nil
}

// Start implements IDeviceDriver
func (d *ExampleDriver) Start(ctx context.Context) error {
	d.logger.Info("Starting driver")

	// Call base start
	if err := d.BaseServer.Start(ctx); err != nil {
		return err
	}

	// Probe for devices
	devices, err := d.Probe(ctx)
	if err != nil {
		d.logger.Error("Probe failed: %v", err)
		return err
	}

	d.logger.Info("Found %d devices", len(devices))

	// Attach each device
	for _, dev := range devices {
		if err := d.AttachDevice(ctx, dev); err != nil {
			d.logger.Error("Failed to attach %s: %v", dev, err)
			continue
		}
	}

	d.logger.Info("Driver started successfully")
	return nil
}

// Stop implements IDeviceDriver
func (d *ExampleDriver) Stop(ctx context.Context) error {
	d.logger.Info("Stopping driver")

	// Detach all devices
	for _, dev := range d.devices {
		if err := d.DetachDevice(ctx, dev); err != nil {
			d.logger.Error("Failed to detach %s: %v", dev, err)
		}
	}

	// Call base stop
	return d.BaseServer.Stop(ctx)
}

// Probe implements IDeviceDriver
func (d *ExampleDriver) Probe(ctx context.Context) ([]string, error) {
	d.logger.Info("Probing for devices")

	// In a real driver, this would:
	// 1. Scan PCI bus
	// 2. Check device IDs
	// 3. Return matching devices

	// Simulated device discovery
	devices := []string{
		"/dev/example0",
		"/dev/example1",
	}

	d.devices = devices
	return devices, nil
}

// AttachDevice implements IDeviceDriver
func (d *ExampleDriver) AttachDevice(ctx context.Context, devicePath string) error {
	d.logger.Info("Attaching device: %s", devicePath)

	// In a real driver, this would:
	// 1. Map device memory
	// 2. Initialize hardware
	// 3. Register interrupt handler
	// 4. Create /dev entries via 9P

	d.IncrementRequests()
	return nil
}

// DetachDevice implements IDeviceDriver
func (d *ExampleDriver) DetachDevice(ctx context.Context, devicePath string) error {
	d.logger.Info("Detaching device: %s", devicePath)

	// In a real driver, this would:
	// 1. Disable interrupts
	// 2. Flush pending I/O
	// 3. Unmap device memory
	// 4. Remove /dev entries

	return nil
}

// HandleInterrupt implements IDeviceDriver
func (d *ExampleDriver) HandleInterrupt(ctx context.Context, irq int) error {
	d.logger.Debug("Handling interrupt: %d", irq)

	// In a real driver, this would:
	// 1. Read interrupt status register
	// 2. Process completed operations
	// 3. Wake up waiting processes
	// 4. Acknowledge interrupt

	d.IncrementRequests()
	return nil
}

// ExampleFileServer demonstrates a file system server implementation
type ExampleFileServer struct {
	*BaseServer
	logger    *DefaultLogger
	mounted   bool
	fileCache map[string][]byte
}

// Verify that ExampleFileServer implements IFileServer
var _ IFileServer = (*ExampleFileServer)(nil)

// NewExampleFileServer creates a new example file server
func NewExampleFileServer(config *ServerConfig) (IServer, error) {
	server := &ExampleFileServer{
		BaseServer: NewBaseServer(config),
		logger:     NewDefaultLogger(config.Name),
		fileCache:  make(map[string][]byte),
	}
	return server, nil
}

// Initialize implements IFileServer
func (f *ExampleFileServer) Initialize(ctx context.Context, config *ServerConfig) error {
	if err := f.BaseServer.Initialize(ctx, config); err != nil {
		return err
	}

	f.logger.Info("Initializing file server")

	// Verify capabilities
	if config.Capabilities&CapFileSystem == 0 {
		return fmt.Errorf("file server requires CapFileSystem capability")
	}

	return nil
}

// Start implements IFileServer
func (f *ExampleFileServer) Start(ctx context.Context) error {
	f.logger.Info("Starting file server")

	if err := f.BaseServer.Start(ctx); err != nil {
		return err
	}

	// Mount 9P server
	if err := f.Mount9P(ctx); err != nil {
		return err
	}

	return nil
}

// Stop implements IFileServer
func (f *ExampleFileServer) Stop(ctx context.Context) error {
	f.logger.Info("Stopping file server")

	// Unmount 9P server
	if err := f.Unmount9P(ctx); err != nil {
		return err
	}

	return f.BaseServer.Stop(ctx)
}

// Mount9P implements IFileServer
func (f *ExampleFileServer) Mount9P(ctx context.Context) error {
	f.logger.Info("Mounting 9P at %s", f.config.MountPoint)

	// In a real implementation:
	// 1. Create 9P server
	// 2. Register file operations
	// 3. Mount at namespace path
	// 4. Start serving requests

	f.mounted = true
	return nil
}

// Unmount9P implements IFileServer
func (f *ExampleFileServer) Unmount9P(ctx context.Context) error {
	f.logger.Info("Unmounting 9P from %s", f.config.MountPoint)

	// In a real implementation:
	// 1. Flush pending operations
	// 2. Close open files
	// 3. Unmount from namespace
	// 4. Stop 9P server

	f.mounted = false
	return nil
}

// RegisterExampleServers registers example servers with the factory
func RegisterExampleServers(factory *ServerFactory) error {
	if err := factory.Register("example-driver", NewExampleDriver); err != nil {
		return err
	}

	if err := factory.Register("example-fileserver", NewExampleFileServer); err != nil {
		return err
	}

	return nil
}

// RunExample demonstrates using the SIP framework
func RunExample() {
	ctx := context.Background()

	// Create factory and register server types
	factory := NewServerFactory()
	if err := RegisterExampleServers(factory); err != nil {
		panic(err)
	}

	// Create server manager
	manager := NewServerManager(factory)

	// Start a driver
	driverConfig := &ServerConfig{
		Name:         "my-driver",
		Capabilities: CapDeviceAccess | CapInterrupt,
		MountPoint:   "/dev/mydriver",
		Priority:     10,
	}

	if err := manager.StartServer(ctx, "example-driver", driverConfig); err != nil {
		panic(err)
	}

	// Start a file server
	fsConfig := &ServerConfig{
		Name:         "my-fileserver",
		Capabilities: CapFileSystem,
		MountPoint:   "/srv/myfs",
		Priority:     5,
	}

	if err := manager.StartServer(ctx, "example-fileserver", fsConfig); err != nil {
		panic(err)
	}

	// Let servers run for a bit
	time.Sleep(2 * time.Second)

	// Check health
	for _, name := range manager.ListServers() {
		if server, ok := manager.GetServer(name); ok {
			health := server.Health()
			fmt.Printf("Server %s: %s - %s\n", name, health.Status, health.Message)
		}
	}

	// Stop all servers
	if err := manager.StopAll(ctx); err != nil {
		panic(err)
	}
}
