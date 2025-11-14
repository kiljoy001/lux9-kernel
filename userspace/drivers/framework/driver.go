package framework

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
	"syscall"
)

// Driver represents a hardware driver
type Driver interface {
	// Name returns the driver name
	Name() string
	
	// Init initializes the driver
	Init() error
	
	// HandleRead handles read operations on a file
	HandleRead(path string, offset uint64, count uint32) ([]byte, error)
	
	// HandleWrite handles write operations on a file
	HandleWrite(path string, offset uint64, data []byte) (uint32, error)
	
	// HandleOpen handles file open operations
	HandleOpen(path string, mode uint8) error
	
	// HandleCreate handles file creation
	HandleCreate(path string, perm uint32, mode uint8) error
	
	// HandleRemove handles file removal
	HandleRemove(path string) error
	
	// HandleStat returns file statistics
	HandleStat(path string) (Stat, error)
	
	// HandleWstat updates file statistics
	HandleWstat(path string, stat Stat) error
}

// Stat represents file statistics
type Stat struct {
	Type   uint16
	Dev    uint32
	Qid    Qid
	Mode   uint32
	Atime  uint32
	Mtime  uint32
	Length uint64
	Name   string
	UID    string
	GID    string
	MUID   string
}

// Qid represents a file's unique identifier
type Qid struct {
	Type    uint8
	Version uint32
	Path    uint64
}

// Marshal serializes a Qid
func (q *Qid) Marshal() []byte {
	buf := make([]byte, 13)
	buf[0] = q.Type
	buf[1] = byte(q.Version)
	buf[2] = byte(q.Version >> 8)
	buf[3] = byte(q.Version >> 16)
	buf[4] = byte(q.Version >> 24)
	buf[5] = byte(q.Path)
	buf[6] = byte(q.Path >> 8)
	buf[7] = byte(q.Path >> 16)
	buf[8] = byte(q.Path >> 24)
	buf[9] = byte(q.Path >> 32)
	buf[10] = byte(q.Path >> 40)
	buf[11] = byte(q.Path >> 48)
	buf[12] = byte(q.Path >> 56)
	return buf
}

// DriverServer implements a 9P server for hardware drivers
type DriverServer struct {
	driver Driver
	root   string
}

// NewDriverServer creates a new driver server
func NewDriverServer(driver Driver, root string) *DriverServer {
	return &DriverServer{
		driver: driver,
		root:   root,
	}
}

// Serve starts the driver server
func (ds *DriverServer) Serve() error {
	// Create the driver directory
	if err := os.MkdirAll(ds.root, 0755); err != nil {
		return fmt.Errorf("failed to create driver directory: %v", err)
	}
	
	// Initialize the driver
	if err := ds.driver.Init(); err != nil {
		return fmt.Errorf("driver initialization failed: %v", err)
	}
	
	log.Printf("Driver server '%s' started at %s", ds.driver.Name(), ds.root)
	return nil
}

// GetPath resolves a path relative to the driver root
func (ds *DriverServer) GetPath(path string) string {
	if path == "" {
		return ds.root
	}
	return filepath.Join(ds.root, path)
}

// ExportDriver exports a driver to the 9P namespace
func ExportDriver(name string, driver Driver) error {
	// Create /srv directory if it doesn't exist
	if err := os.MkdirAll("/srv", 0755); err != nil {
		return fmt.Errorf("failed to create /srv: %v", err)
	}
	
	// Create the driver server
	srvPath := filepath.Join("/srv", name)
	server := NewDriverServer(driver, srvPath)
	
	// Serve the driver
	return server.Serve()
}
