// Test stub for verifying the kernel interface
// This simulates the "userspace AHCI driver" using the new kernel interface

package main

import (
	"context"
	"log"
	"os"
	"time"

	"lux9/userspace/go-servers/sip"
)

// Mock AHCI driver
type TestAHCIDriver struct {
	*sip.BaseServer
}

func (d *TestAHCIDriver) Initialize(ctx context.Context, config *sip.ServerConfig) error {
	log.Println("TestDriver: Initialize called")
	return d.BaseServer.Initialize(ctx, config)
}

func (d *TestAHCIDriver) Start(ctx context.Context) error {
	log.Println("TestDriver: Start called")
	// In a real driver, we would start the 9P listener here
	// For this test, we just sleep to keep the process alive
	
	if d.BaseServer.GetConfig().Capabilities&sip.CapDeviceAccess != 0 {
		// Try to access /dev/mem just to verify capability
		f, err := os.OpenFile("/dev/mem", os.O_RDWR, 0)
		if err != nil {
			log.Printf("TestDriver: /dev/mem access failed (expected if not running as root/capability): %v", err)
		} else {
			log.Println("TestDriver: /dev/mem access successful")
			f.Close()
		}
	}

	go func() {
		for {
			select {
			case <-ctx.Done():
				return
			case <-time.After(5 * time.Second):
				log.Println("TestDriver: still running...")
			}
		}
	}()
	return d.BaseServer.Start(ctx)
}

func (d *TestAHCIDriver) Stop(ctx context.Context) error {
	log.Println("TestDriver: Stop called")
	return d.BaseServer.Stop(ctx)
}

func (d *TestAHCIDriver) Health() sip.ServerHealth {
	return d.BaseServer.Health()
}

func (d *TestAHCIDriver) GetConfig() *sip.ServerConfig {
	return d.BaseServer.GetConfig()
}

// Stub methods for IDeviceDriver
func (d *TestAHCIDriver) Probe(ctx context.Context) ([]string, error) { return nil, nil }
func (d *TestAHCIDriver) AttachDevice(ctx context.Context, path string) error { return nil }
func (d *TestAHCIDriver) DetachDevice(ctx context.Context, path string) error { return nil }
func (d *TestAHCIDriver) HandleInterrupt(ctx context.Context, irq int) error { return nil }

func main() {
	log.Println("Starting Test AHCI SIP Server...")

	factory := sip.NewServerFactory()
	factory.Register("ahci", func(config *sip.ServerConfig) (sip.IServer, error) {
		return &TestAHCIDriver{
			BaseServer: sip.NewBaseServer(config),
		}, nil
	})

	manager := sip.NewServerManager(factory)
	ctx := context.Background()

	config := &sip.ServerConfig{
		Name:         "test-ahci",
		Capabilities: sip.CapDeviceAccess | sip.CapInterrupt | sip.CapDMA,
		MountPoint:   "/dev/sd",
	}

	if err := manager.StartServer(ctx, "ahci", config); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}

	// Keep main process alive
	select {}
}
