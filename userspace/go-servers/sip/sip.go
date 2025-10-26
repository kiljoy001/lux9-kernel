// Package sip provides the Software Isolated Process framework for Lux9
// This package defines interfaces and factory patterns for creating isolated
// userspace servers that communicate via 9P protocol and page exchange.
package sip

import (
	"context"
	"fmt"
	"log"
	"os"
	"sync"
)

// ServerCapability defines what resources a SIP server can access
type ServerCapability uint64

const (
	CapNone         ServerCapability = 0
	CapFileSystem   ServerCapability = 1 << iota // Can serve files via 9P
	CapDeviceAccess                              // Can access hardware devices
	CapPageExchange                              // Can exchange pages with other processes
	CapNetworking                                // Can use network stack
	CapInterrupt                                 // Can register interrupt handlers
	CapDMA                                       // Can perform DMA operations
	CapAll          ServerCapability = ^ServerCapability(0)
)

// ServerConfig holds configuration for a SIP server
type ServerConfig struct {
	Name         string             // Server name (e.g., "ahci-driver", "ext4fs")
	Capabilities ServerCapability   // Required capabilities
	MountPoint   string             // Where to mount in namespace (e.g., "/dev/sd")
	Priority     int                // Scheduling priority
	MemoryLimit  uint64             // Maximum memory in bytes (0 = unlimited)
	Metadata     map[string]string  // Additional metadata
}

// IServer is the core interface that all SIP servers must implement
// Similar to C# interface pattern
type IServer interface {
	// Initialize is called once during server startup
	// Returns error if initialization fails
	Initialize(ctx context.Context, config *ServerConfig) error

	// Start begins serving requests (non-blocking)
	// Should spawn goroutines as needed
	Start(ctx context.Context) error

	// Stop gracefully shuts down the server
	// Should cleanup resources and wait for pending operations
	Stop(ctx context.Context) error

	// Health returns the current health status
	// Used for monitoring and restart decisions
	Health() ServerHealth

	// GetConfig returns the server's configuration
	GetConfig() *ServerConfig
}

// IDeviceDriver extends IServer for hardware device drivers
type IDeviceDriver interface {
	IServer

	// Probe detects and enumerates hardware devices
	// Returns list of detected device paths
	Probe(ctx context.Context) ([]string, error)

	// AttachDevice configures and enables a specific device
	AttachDevice(ctx context.Context, devicePath string) error

	// DetachDevice safely removes a device
	DetachDevice(ctx context.Context, devicePath string) error

	// HandleInterrupt processes hardware interrupts
	// Called by kernel when interrupt occurs
	HandleInterrupt(ctx context.Context, irq int) error
}

// IFileServer extends IServer for file system servers
type IFileServer interface {
	IServer

	// Mount9P starts serving 9P protocol on the mount point
	Mount9P(ctx context.Context) error

	// Unmount9P stops serving and unmounts
	Unmount9P(ctx context.Context) error
}

// ServerHealth represents server health status
type ServerHealth struct {
	Status    HealthStatus
	Message   string
	Uptime    int64 // seconds
	Requests  uint64
	Errors    uint64
	LastError error
}

// HealthStatus enum
type HealthStatus int

const (
	HealthUnknown HealthStatus = iota
	HealthStarting
	HealthHealthy
	HealthDegraded
	HealthFailing
	HealthStopped
)

func (h HealthStatus) String() string {
	switch h {
	case HealthUnknown:
		return "Unknown"
	case HealthStarting:
		return "Starting"
	case HealthHealthy:
		return "Healthy"
	case HealthDegraded:
		return "Degraded"
	case HealthFailing:
		return "Failing"
	case HealthStopped:
		return "Stopped"
	default:
		return fmt.Sprintf("HealthStatus(%d)", h)
	}
}

// BaseServer provides default implementation of IServer
// Concrete servers can embed this and override specific methods
type BaseServer struct {
	config      *ServerConfig
	health      ServerHealth
	healthMutex sync.RWMutex
	startTime   int64
	ctx         context.Context
	cancel      context.CancelFunc
}

// NewBaseServer creates a new base server
func NewBaseServer(config *ServerConfig) *BaseServer {
	return &BaseServer{
		config: config,
		health: ServerHealth{
			Status: HealthUnknown,
		},
	}
}

// Initialize implements IServer
func (s *BaseServer) Initialize(ctx context.Context, config *ServerConfig) error {
	s.config = config
	s.ctx, s.cancel = context.WithCancel(ctx)
	s.updateHealth(HealthStarting, "Initializing", nil)
	return nil
}

// Start implements IServer
func (s *BaseServer) Start(ctx context.Context) error {
	s.updateHealth(HealthHealthy, "Running", nil)
	return nil
}

// Stop implements IServer
func (s *BaseServer) Stop(ctx context.Context) error {
	if s.cancel != nil {
		s.cancel()
	}
	s.updateHealth(HealthStopped, "Stopped", nil)
	return nil
}

// Health implements IServer
func (s *BaseServer) Health() ServerHealth {
	s.healthMutex.RLock()
	defer s.healthMutex.RUnlock()
	return s.health
}

// GetConfig implements IServer
func (s *BaseServer) GetConfig() *ServerConfig {
	return s.config
}

// updateHealth updates the health status (thread-safe)
func (s *BaseServer) updateHealth(status HealthStatus, message string, err error) {
	s.healthMutex.Lock()
	defer s.healthMutex.Unlock()
	s.health.Status = status
	s.health.Message = message
	if err != nil {
		s.health.LastError = err
		s.health.Errors++
	}
}

// IncrementRequests increments the request counter
func (s *BaseServer) IncrementRequests() {
	s.healthMutex.Lock()
	defer s.healthMutex.Unlock()
	s.health.Requests++
}

// ServerFactory creates SIP servers based on configuration
type ServerFactory struct {
	registry map[string]ServerConstructor
	mu       sync.RWMutex
}

// ServerConstructor is a function that creates a new server instance
type ServerConstructor func(config *ServerConfig) (IServer, error)

// NewServerFactory creates a new server factory
func NewServerFactory() *ServerFactory {
	return &ServerFactory{
		registry: make(map[string]ServerConstructor),
	}
}

// Register adds a server constructor to the factory
func (f *ServerFactory) Register(serverType string, constructor ServerConstructor) error {
	f.mu.Lock()
	defer f.mu.Unlock()

	if _, exists := f.registry[serverType]; exists {
		return fmt.Errorf("server type %s already registered", serverType)
	}

	f.registry[serverType] = constructor
	log.Printf("SIP Factory: Registered server type '%s'", serverType)
	return nil
}

// Create instantiates a new server of the specified type
func (f *ServerFactory) Create(serverType string, config *ServerConfig) (IServer, error) {
	f.mu.RLock()
	constructor, exists := f.registry[serverType]
	f.mu.RUnlock()

	if !exists {
		return nil, fmt.Errorf("unknown server type: %s", serverType)
	}

	server, err := constructor(config)
	if err != nil {
		return nil, fmt.Errorf("failed to create server %s: %w", serverType, err)
	}

	log.Printf("SIP Factory: Created server '%s' of type '%s'", config.Name, serverType)
	return server, nil
}

// ListTypes returns all registered server types
func (f *ServerFactory) ListTypes() []string {
	f.mu.RLock()
	defer f.mu.RUnlock()

	types := make([]string, 0, len(f.registry))
	for t := range f.registry {
		types = append(types, t)
	}
	return types
}

// ServerManager manages the lifecycle of multiple SIP servers
type ServerManager struct {
	servers map[string]IServer
	factory *ServerFactory
	mu      sync.RWMutex
}

// NewServerManager creates a new server manager
func NewServerManager(factory *ServerFactory) *ServerManager {
	return &ServerManager{
		servers: make(map[string]IServer),
		factory: factory,
	}
}

// StartServer creates and starts a new server
func (m *ServerManager) StartServer(ctx context.Context, serverType string, config *ServerConfig) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	// Check if server already exists
	if _, exists := m.servers[config.Name]; exists {
		return fmt.Errorf("server %s already running", config.Name)
	}

	// Create server via factory
	server, err := m.factory.Create(serverType, config)
	if err != nil {
		return err
	}

	// Initialize
	if err := server.Initialize(ctx, config); err != nil {
		return fmt.Errorf("failed to initialize %s: %w", config.Name, err)
	}

	// Start
	if err := server.Start(ctx); err != nil {
		return fmt.Errorf("failed to start %s: %w", config.Name, err)
	}

	m.servers[config.Name] = server
	log.Printf("ServerManager: Started server '%s'", config.Name)
	return nil
}

// StopServer stops a running server
func (m *ServerManager) StopServer(ctx context.Context, name string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	server, exists := m.servers[name]
	if !exists {
		return fmt.Errorf("server %s not found", name)
	}

	if err := server.Stop(ctx); err != nil {
		return fmt.Errorf("failed to stop %s: %w", name, err)
	}

	delete(m.servers, name)
	log.Printf("ServerManager: Stopped server '%s'", name)
	return nil
}

// GetServer retrieves a running server
func (m *ServerManager) GetServer(name string) (IServer, bool) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	server, exists := m.servers[name]
	return server, exists
}

// ListServers returns all running servers
func (m *ServerManager) ListServers() []string {
	m.mu.RLock()
	defer m.mu.RUnlock()

	names := make([]string, 0, len(m.servers))
	for name := range m.servers {
		names = append(names, name)
	}
	return names
}

// StopAll stops all running servers
func (m *ServerManager) StopAll(ctx context.Context) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	var lastErr error
	for name, server := range m.servers {
		if err := server.Stop(ctx); err != nil {
			log.Printf("Error stopping %s: %v", name, err)
			lastErr = err
		}
	}

	m.servers = make(map[string]IServer)
	return lastErr
}

// DefaultLogger provides basic logging for SIP servers
type DefaultLogger struct {
	prefix string
}

// NewDefaultLogger creates a logger with prefix
func NewDefaultLogger(prefix string) *DefaultLogger {
	return &DefaultLogger{prefix: prefix}
}

func (l *DefaultLogger) Info(format string, args ...interface{}) {
	log.Printf("[%s] INFO: "+format, append([]interface{}{l.prefix}, args...)...)
}

func (l *DefaultLogger) Error(format string, args ...interface{}) {
	log.Printf("[%s] ERROR: "+format, append([]interface{}{l.prefix}, args...)...)
}

func (l *DefaultLogger) Debug(format string, args ...interface{}) {
	if os.Getenv("SIP_DEBUG") != "" {
		log.Printf("[%s] DEBUG: "+format, append([]interface{}{l.prefix}, args...)...)
	}
}
