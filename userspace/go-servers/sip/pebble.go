package sip

import (
	"fmt"
	"os"
)

// PebbleHandle is an opaque handle to a kernel-managed pebble resource
type PebbleHandle uintptr

// AllocBlack allocates a kernel-managed "black" buffer of `size`.
// This consumes a verified white token (conceptually, though the API might track state per process).
// This implementation uses /dev/pebble/alloc via 9P.
func AllocBlack(size uint64) (PebbleHandle, error) {
	f, err := os.OpenFile("/dev/pebble/alloc", os.O_RDWR, 0)
	if err != nil {
		return 0, fmt.Errorf("failed to open /dev/pebble/alloc: %v", err)
	}
	defer f.Close()

	// Write request: "size <bytes>"
	req := fmt.Sprintf("size %d", size)
	if _, err := f.WriteString(req); err != nil {
		return 0, fmt.Errorf("pebble alloc write failed: %v", err)
	}

	// Read response: "addr: 0x..."
	// Note: In devpebble.c currently, write performs alloc but doesn't easily return data
	// without a stateful file descriptor logic (read-after-write on same FD).
	// Assuming devpebble.c implements read-after-write statefulness:
	
	buf := make([]byte, 64)
	n, err := f.Read(buf)
	if err != nil {
		return 0, fmt.Errorf("pebble alloc read failed: %v", err)
	}

	var addr uintptr
	_, err = fmt.Sscanf(string(buf[:n]), "addr: 0x%x", &addr)
	if err != nil {
		return 0, fmt.Errorf("pebble alloc parse failed: %q", string(buf[:n]))
	}

	return PebbleHandle(addr), nil
}

// FreeBlack releases a black handle.
// Writes "free <addr>" to /dev/pebble/free
func FreeBlack(handle PebbleHandle) error {
	f, err := os.OpenFile("/dev/pebble/free", os.O_WRONLY, 0)
	if err != nil {
		return fmt.Errorf("failed to open /dev/pebble/free: %v", err)
	}
	defer f.Close()

	cmd := fmt.Sprintf("free 0x%x", handle)
	if _, err := f.WriteString(cmd); err != nil {
		return fmt.Errorf("pebble free write failed: %v", err)
	}
	return nil
}

// SetNoSwap attempts to prevent the current process from swapping.
// It writes "noswap" to /proc/self/ctl.
func SetNoSwap() error {
	f, err := os.OpenFile("/proc/self/ctl", os.O_WRONLY, 0)
	if err != nil {
		return fmt.Errorf("failed to open /proc/self/ctl: %v", err)
	}
	defer f.Close()

	_, err = f.WriteString("noswap")
	if err != nil {
		return fmt.Errorf("failed to write noswap to ctl: %v", err)
	}
	return nil
}
