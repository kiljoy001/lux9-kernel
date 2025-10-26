package sip

import (
	"context"
	"fmt"
	"testing"
)

func TestServerFactory(t *testing.T) {
	factory := NewServerFactory()

	// Register example servers
	if err := RegisterExampleServers(factory); err != nil {
		t.Fatalf("Failed to register servers: %v", err)
	}

	// Verify registration
	types := factory.ListTypes()
	if len(types) != 2 {
		t.Errorf("Expected 2 server types, got %d", len(types))
	}

	// Create a driver
	config := &ServerConfig{
		Name:         "test-driver",
		Capabilities: CapDeviceAccess,
		MountPoint:   "/dev/test",
	}

	server, err := factory.Create("example-driver", config)
	if err != nil {
		t.Fatalf("Failed to create server: %v", err)
	}

	if server == nil {
		t.Fatal("Server is nil")
	}

	// Verify it implements IDeviceDriver
	_, ok := server.(IDeviceDriver)
	if !ok {
		t.Error("Server does not implement IDeviceDriver")
	}
}

func TestServerLifecycle(t *testing.T) {
	ctx := context.Background()

	factory := NewServerFactory()
	if err := RegisterExampleServers(factory); err != nil {
		t.Fatalf("Failed to register servers: %v", err)
	}

	config := &ServerConfig{
		Name:         "lifecycle-test",
		Capabilities: CapDeviceAccess | CapInterrupt,
		MountPoint:   "/dev/lifecycle",
	}

	server, err := factory.Create("example-driver", config)
	if err != nil {
		t.Fatalf("Failed to create server: %v", err)
	}

	// Initialize
	if err := server.Initialize(ctx, config); err != nil {
		t.Fatalf("Initialize failed: %v", err)
	}

	// Check health after init
	health := server.Health()
	if health.Status != HealthStarting {
		t.Errorf("Expected HealthStarting, got %v", health.Status)
	}

	// Start
	if err := server.Start(ctx); err != nil {
		t.Fatalf("Start failed: %v", err)
	}

	// Check health after start
	health = server.Health()
	if health.Status != HealthHealthy {
		t.Errorf("Expected HealthHealthy, got %v", health.Status)
	}

	// Stop
	if err := server.Stop(ctx); err != nil {
		t.Fatalf("Stop failed: %v", err)
	}

	// Check health after stop
	health = server.Health()
	if health.Status != HealthStopped {
		t.Errorf("Expected HealthStopped, got %v", health.Status)
	}
}

func TestServerManager(t *testing.T) {
	ctx := context.Background()

	factory := NewServerFactory()
	if err := RegisterExampleServers(factory); err != nil {
		t.Fatalf("Failed to register servers: %v", err)
	}

	manager := NewServerManager(factory)

	// Start multiple servers
	driverConfig := &ServerConfig{
		Name:         "test-driver-1",
		Capabilities: CapDeviceAccess,
		MountPoint:   "/dev/test1",
	}

	if err := manager.StartServer(ctx, "example-driver", driverConfig); err != nil {
		t.Fatalf("Failed to start driver: %v", err)
	}

	fsConfig := &ServerConfig{
		Name:         "test-fs-1",
		Capabilities: CapFileSystem,
		MountPoint:   "/srv/testfs",
	}

	if err := manager.StartServer(ctx, "example-fileserver", fsConfig); err != nil {
		t.Fatalf("Failed to start file server: %v", err)
	}

	// Verify servers are running
	servers := manager.ListServers()
	if len(servers) != 2 {
		t.Errorf("Expected 2 servers, got %d", len(servers))
	}

	// Get specific server
	server, ok := manager.GetServer("test-driver-1")
	if !ok {
		t.Error("Failed to get server")
	}
	if server == nil {
		t.Error("Server is nil")
	}

	// Stop specific server
	if err := manager.StopServer(ctx, "test-driver-1"); err != nil {
		t.Fatalf("Failed to stop server: %v", err)
	}

	// Verify server was removed
	servers = manager.ListServers()
	if len(servers) != 1 {
		t.Errorf("Expected 1 server after stop, got %d", len(servers))
	}

	// Stop all
	if err := manager.StopAll(ctx); err != nil {
		t.Fatalf("Failed to stop all: %v", err)
	}

	servers = manager.ListServers()
	if len(servers) != 0 {
		t.Errorf("Expected 0 servers after stop all, got %d", len(servers))
	}
}

func TestDeviceDriverInterface(t *testing.T) {
	ctx := context.Background()

	config := &ServerConfig{
		Name:         "driver-interface-test",
		Capabilities: CapDeviceAccess | CapInterrupt,
		MountPoint:   "/dev/test",
	}

	driver, err := NewExampleDriver(config)
	if err != nil {
		t.Fatalf("Failed to create driver: %v", err)
	}

	deviceDriver, ok := driver.(IDeviceDriver)
	if !ok {
		t.Fatal("Driver does not implement IDeviceDriver")
	}

	// Initialize
	if err := deviceDriver.Initialize(ctx, config); err != nil {
		t.Fatalf("Initialize failed: %v", err)
	}

	// Probe devices
	devices, err := deviceDriver.Probe(ctx)
	if err != nil {
		t.Fatalf("Probe failed: %v", err)
	}

	if len(devices) == 0 {
		t.Error("Expected at least one device")
	}

	// Attach device
	if len(devices) > 0 {
		if err := deviceDriver.AttachDevice(ctx, devices[0]); err != nil {
			t.Fatalf("AttachDevice failed: %v", err)
		}
	}

	// Handle interrupt
	if err := deviceDriver.HandleInterrupt(ctx, 42); err != nil {
		t.Fatalf("HandleInterrupt failed: %v", err)
	}

	// Detach device
	if len(devices) > 0 {
		if err := deviceDriver.DetachDevice(ctx, devices[0]); err != nil {
			t.Fatalf("DetachDevice failed: %v", err)
		}
	}
}

func TestFileServerInterface(t *testing.T) {
	ctx := context.Background()

	config := &ServerConfig{
		Name:         "fs-interface-test",
		Capabilities: CapFileSystem,
		MountPoint:   "/srv/test",
	}

	fs, err := NewExampleFileServer(config)
	if err != nil {
		t.Fatalf("Failed to create file server: %v", err)
	}

	fileServer, ok := fs.(IFileServer)
	if !ok {
		t.Fatal("Server does not implement IFileServer")
	}

	// Initialize
	if err := fileServer.Initialize(ctx, config); err != nil {
		t.Fatalf("Initialize failed: %v", err)
	}

	// Mount
	if err := fileServer.Mount9P(ctx); err != nil {
		t.Fatalf("Mount9P failed: %v", err)
	}

	// Unmount
	if err := fileServer.Unmount9P(ctx); err != nil {
		t.Fatalf("Unmount9P failed: %v", err)
	}
}

func TestCapabilities(t *testing.T) {
	// Test capability combinations
	caps := CapDeviceAccess | CapInterrupt | CapDMA

	if caps&CapDeviceAccess == 0 {
		t.Error("DeviceAccess capability not set")
	}

	if caps&CapInterrupt == 0 {
		t.Error("Interrupt capability not set")
	}

	if caps&CapDMA == 0 {
		t.Error("DMA capability not set")
	}

	if caps&CapFileSystem != 0 {
		t.Error("FileSystem capability should not be set")
	}
}

func TestHealthStatus(t *testing.T) {
	statuses := []HealthStatus{
		HealthUnknown,
		HealthStarting,
		HealthHealthy,
		HealthDegraded,
		HealthFailing,
		HealthStopped,
	}

	for _, status := range statuses {
		str := status.String()
		if str == "" {
			t.Errorf("Empty string for status %d", status)
		}
	}
}

func TestConcurrentAccess(t *testing.T) {
	ctx := context.Background()
	factory := NewServerFactory()
	if err := RegisterExampleServers(factory); err != nil {
		t.Fatalf("Failed to register servers: %v", err)
	}

	manager := NewServerManager(factory)

	// Start multiple servers concurrently
	done := make(chan bool)
	for i := 0; i < 5; i++ {
		go func(n int) {
			config := &ServerConfig{
				Name:         fmt.Sprintf("concurrent-%d", n),
				Capabilities: CapDeviceAccess,
				MountPoint:   fmt.Sprintf("/dev/test%d", n),
			}
			if err := manager.StartServer(ctx, "example-driver", config); err != nil {
				t.Errorf("Failed to start server %d: %v", n, err)
			}
			done <- true
		}(i)
	}

	// Wait for all to complete
	for i := 0; i < 5; i++ {
		<-done
	}

	// Verify all servers started
	servers := manager.ListServers()
	if len(servers) != 5 {
		t.Errorf("Expected 5 servers, got %d", len(servers))
	}

	// Stop all
	if err := manager.StopAll(ctx); err != nil {
		t.Fatalf("Failed to stop all: %v", err)
	}
}

// Benchmark server creation
func BenchmarkServerCreation(b *testing.B) {
	factory := NewServerFactory()
	RegisterExampleServers(factory)

	config := &ServerConfig{
		Name:         "bench-server",
		Capabilities: CapDeviceAccess,
		MountPoint:   "/dev/bench",
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, err := factory.Create("example-driver", config)
		if err != nil {
			b.Fatalf("Failed to create server: %v", err)
		}
	}
}

// Benchmark health check
func BenchmarkHealthCheck(b *testing.B) {
	config := &ServerConfig{
		Name:         "health-bench",
		Capabilities: CapDeviceAccess,
	}

	server, _ := NewExampleDriver(config)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = server.Health()
	}
}
