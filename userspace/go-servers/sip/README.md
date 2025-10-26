# SIP - Software Isolated Process Framework

A C#-style interface and factory pattern framework for building userspace servers in Lux9.

## Overview

The SIP framework provides a clean, consistent way to build isolated userspace services (drivers, file servers, etc.) that run as separate processes with hardware-enforced memory isolation.

## Key Concepts

### Software Isolated Processes (SIP)
- Each server runs in its own address space with MMU protection
- Servers communicate via 9P protocol and page exchange
- Hardware crashes are isolated - kernel stays stable
- True microkernel architecture

### Capabilities System
Servers declare required capabilities:
- `CapFileSystem` - Serve files via 9P
- `CapDeviceAccess` - Access hardware devices
- `CapPageExchange` - Exchange memory pages with other processes
- `CapNetworking` - Use network stack
- `CapInterrupt` - Register interrupt handlers
- `CapDMA` - Perform DMA operations

### Factory Pattern
Centralized server creation and registration:
- Register server types with constructors
- Create servers by type name
- Consistent lifecycle management
- Easy to extend with new server types

## Core Interfaces

### IServer
Base interface all servers must implement:
```go
type IServer interface {
    Initialize(ctx context.Context, config *ServerConfig) error
    Start(ctx context.Context) error
    Stop(ctx context.Context) error
    Health() ServerHealth
    GetConfig() *ServerConfig
}
```

### IDeviceDriver
Extended interface for hardware drivers:
```go
type IDeviceDriver interface {
    IServer
    Probe(ctx context.Context) ([]string, error)
    AttachDevice(ctx context.Context, devicePath string) error
    DetachDevice(ctx context.Context, devicePath string) error
    HandleInterrupt(ctx context.Context, irq int) error
}
```

### IFileServer
Extended interface for file system servers:
```go
type IFileServer interface {
    IServer
    Mount9P(ctx context.Context) error
    Unmount9P(ctx context.Context) error
}
```

## Usage Example

### 1. Create a Driver

```go
type MyDriver struct {
    *sip.BaseServer  // Embed for default implementations
    logger *sip.DefaultLogger
}

// Implement IDeviceDriver interface
func (d *MyDriver) Initialize(ctx context.Context, config *ServerConfig) error {
    if err := d.BaseServer.Initialize(ctx, config); err != nil {
        return err
    }
    d.logger.Info("Driver initialized")
    return nil
}

func (d *MyDriver) Probe(ctx context.Context) ([]string, error) {
    // Scan hardware, return device paths
    return []string{"/dev/mydevice0"}, nil
}

// ... implement other methods
```

### 2. Register with Factory

```go
factory := sip.NewServerFactory()
factory.Register("my-driver", func(config *sip.ServerConfig) (sip.IServer, error) {
    return &MyDriver{
        BaseServer: sip.NewBaseServer(config),
        logger: sip.NewDefaultLogger(config.Name),
    }, nil
})
```

### 3. Create and Run Server

```go
manager := sip.NewServerManager(factory)

config := &sip.ServerConfig{
    Name:         "ahci-driver",
    Capabilities: sip.CapDeviceAccess | sip.CapInterrupt | sip.CapDMA,
    MountPoint:   "/dev/sd",
    Priority:     10,
}

err := manager.StartServer(ctx, "my-driver", config)
```

## Architecture

```
┌─────────────────────────────────────────┐
│           Kernel (Microkernel)          │
│  - Memory Management                    │
│  - Process Isolation (MMU)              │
│  - IPC (9P, Page Exchange)              │
│  - Minimal Device Abstraction           │
└─────────────────────────────────────────┘
              ↕ 9P / Page Exchange
┌─────────────────────────────────────────┐
│         SIP Framework (Userspace)       │
│  ┌─────────────────────────────────┐   │
│  │    ServerFactory                │   │
│  │  - Register server types        │   │
│  │  - Create server instances      │   │
│  └─────────────────────────────────┘   │
│  ┌─────────────────────────────────┐   │
│  │    ServerManager                │   │
│  │  - Lifecycle management         │   │
│  │  - Health monitoring            │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
              ↕
┌─────────────────────────────────────────┐
│      Concrete Servers (Isolated)        │
│  ┌──────────┐  ┌──────────┐  ┌───────┐ │
│  │  AHCI    │  │   IDE    │  │ Ext4  │ │
│  │  Driver  │  │  Driver  │  │  FS   │ │
│  └──────────┘  └──────────┘  └───────┘ │
│  Each in separate memory-isolated       │
│  process with hardware protection       │
└─────────────────────────────────────────┘
```

## Benefits

### For Driver Development
- **Clean Interface**: C#-style interfaces provide clear contracts
- **Less Boilerplate**: BaseServer provides common functionality
- **Consistent Patterns**: Factory pattern makes all drivers look similar
- **Easy Testing**: Mock implementations for unit tests

### For System Stability
- **Isolation**: Driver crashes don't affect kernel
- **Hot Reload**: Restart drivers without kernel reboot
- **Security**: Capabilities limit what drivers can do
- **Monitoring**: Built-in health checks and metrics

### For Microkernel Goals
- **True Isolation**: Hardware MMU enforces boundaries
- **Minimal Kernel**: Drivers run in userspace
- **9P Integration**: File-based interface to everything
- **Go Safety**: Memory safety, goroutines, channels

## Next Steps

1. **Implement Kernel Interface**: System calls for device access, interrupts, DMA
2. **Port Existing Drivers**: Move AHCI/IDE from kernel to SIP servers
3. **Add 9P Integration**: Connect servers to actual 9P protocol
4. **Page Exchange**: Wire up zero-copy data transfer
5. **Boot Integration**: Start SIP servers during system initialization

## Design Philosophy

> "In Lux9, drivers aren't part of the kernel - they're isolated services that happen to talk to hardware"

This follows the Plan 9 philosophy where everything is a file server, extended with modern safety (Go, capabilities) and proper isolation (SIP, MMU).
