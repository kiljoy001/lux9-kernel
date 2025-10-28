// Framebuffer Driver - Userspace framebuffer driver as a SIP server
// Exposes displays via 9P at /dev/draw
package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"

	fbdriver "lux9/userspace/go-servers/fbdriver"
	"lux9/userspace/go-servers/sip"
)

// FramebufferDriverServer implements the SIP IDeviceDriver interface
type FramebufferDriverServer struct {
	*sip.BaseServer
	screens   []*fbdriver.Screen
	fbServer  *fbdriver.Framebuffer9PServer
	logger    *sip.DefaultLogger
}

// Ensure we implement the interface
var _ sip.IDeviceDriver = (*FramebufferDriverServer)(nil)

// NewFramebufferDriver creates a new framebuffer driver instance
func NewFramebufferDriver(config *sip.ServerConfig) (sip.IServer, error) {
	return &FramebufferDriverServer{
		BaseServer: sip.NewBaseServer(config),
		logger:     sip.NewDefaultLogger(config.Name),
		fbServer:   fbdriver.NewFramebuffer9PServer(),
	}, nil
}

// Initialize implements IDeviceDriver
func (d *FramebufferDriverServer) Initialize(ctx context.Context, config *sip.ServerConfig) error {
	if err := d.BaseServer.Initialize(ctx, config); err != nil {
		return err
	}

	d.logger.Info("Initializing framebuffer driver")

	// Verify capabilities
	if config.Capabilities&sip.CapDeviceAccess == 0 {
		return fmt.Errorf("framebuffer driver requires CapDeviceAccess")
	}

	d.logger.Info("Framebuffer driver initialized")
	return nil
}

// Start implements IDeviceDriver
func (d *FramebufferDriverServer) Start(ctx context.Context) error {
	d.logger.Info("Starting framebuffer driver")

	if err := d.BaseServer.Start(ctx); err != nil {
		return err
	}

	// Probe for displays
	devices, err := d.Probe(ctx)
	if err != nil {
		d.logger.Error("Probe failed: %v", err)
		return err
	}

	d.logger.Info("Found %d display(s)", len(devices))

	// Attach each display
	for _, devPath := range devices {
		if err := d.AttachDevice(ctx, devPath); err != nil {
			d.logger.Error("Failed to attach %s: %v", devPath, err)
			continue
		}
		d.logger.Info("Attached display: %s", devPath)
	}

	// Mount 9P filesystem at /dev/draw
	if err := d.fbServer.Mount(ctx, "/dev/draw"); err != nil {
		return fmt.Errorf("failed to mount 9P server: %w", err)
	}

	d.logger.Info("Framebuffer driver started, serving at /dev/draw")
	return nil
}

// Stop implements IDeviceDriver
func (d *FramebufferDriverServer) Stop(ctx context.Context) error {
	d.logger.Info("Stopping framebuffer driver")

	// Close all screens
	for _, screen := range d.screens {
		screen.Close()
	}

	return d.BaseServer.Stop(ctx)
}

// Probe implements IDeviceDriver
func (d *FramebufferDriverServer) Probe(ctx context.Context) ([]string, error) {
	d.logger.Info("Probing for framebuffer devices")

	// Read framebuffer info from kernel
	// In real implementation, would read /dev/bootinfo
	screen, err := fbdriver.ProbeFramebuffer()
	if err != nil {
		return nil, fmt.Errorf("failed to probe framebuffer: %w", err)
	}

	d.logger.Info("Found framebuffer: %dx%d @ 0x%x",
		screen.Width, screen.Height, screen.FBAddr)

	d.screens = append(d.screens, screen)

	return []string{"screen0"}, nil
}

// AttachDevice implements IDeviceDriver
func (d *FramebufferDriverServer) AttachDevice(ctx context.Context, devicePath string) error {
	d.logger.Info("Attaching display: %s", devicePath)

	// Find the corresponding screen
	var screen *fbdriver.Screen
	for _, s := range d.screens {
		if s.Name == devicePath {
			screen = s
			break
		}
	}

	if screen == nil {
		return fmt.Errorf("screen %s not found", devicePath)
	}

	// Initialize screen (map framebuffer memory via /dev/mem)
	if err := screen.Initialize(); err != nil {
		return fmt.Errorf("failed to initialize screen: %w", err)
	}

	// Clear screen to black
	screen.Clear(fbdriver.ColorBlack)
	screen.Flush()

	// Draw welcome message
	screen.FgColor = fbdriver.ColorWhite
	screen.BgColor = fbdriver.ColorBlack
	screen.RenderText("Lux9 Operating System\n")
	screen.RenderText("Framebuffer Driver v1.0\n")
	screen.RenderText("\n")
	screen.Flush()

	// Add to 9P server
	if err := d.fbServer.AddScreen(screen); err != nil {
		return fmt.Errorf("failed to add screen to 9P server: %w", err)
	}

	d.IncrementRequests()
	return nil
}

// DetachDevice implements IDeviceDriver
func (d *FramebufferDriverServer) DetachDevice(ctx context.Context, devicePath string) error {
	d.logger.Info("Detaching display: %s", devicePath)

	if err := d.fbServer.RemoveScreen(devicePath); err != nil {
		return err
	}

	return nil
}

// HandleInterrupt implements IDeviceDriver
func (d *FramebufferDriverServer) HandleInterrupt(ctx context.Context, irq int) error {
	// Framebuffer typically doesn't use interrupts
	// This is here for interface compliance
	return nil
}

func main() {
	log.SetPrefix("[fb-driver] ")
	log.SetFlags(log.Ldate | log.Ltime | log.Lmicroseconds)

	// Create SIP factory and register framebuffer driver
	factory := sip.NewServerFactory()
	if err := factory.Register("fb-driver", NewFramebufferDriver); err != nil {
		log.Fatalf("Failed to register framebuffer driver: %v", err)
	}

	// Create server manager
	manager := sip.NewServerManager(factory)

	// Configure framebuffer driver server
	config := &sip.ServerConfig{
		Name:         "fb-driver",
		Capabilities: sip.CapDeviceAccess, // Needs /dev/mem for framebuffer access
		MountPoint:   "/dev/draw",
		Priority:     5,
	}

	// Start the driver
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	if err := manager.StartServer(ctx, "fb-driver", config); err != nil {
		log.Fatalf("Failed to start framebuffer driver: %v", err)
	}

	log.Printf("Framebuffer driver running")

	// Wait for signals
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	<-sigChan
	log.Printf("Shutting down...")

	// Stop all servers
	if err := manager.StopAll(ctx); err != nil {
		log.Printf("Error during shutdown: %v", err)
	}

	log.Printf("Framebuffer driver stopped")
}
