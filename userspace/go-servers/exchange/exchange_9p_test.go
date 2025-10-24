package main

import (
	"os"
	"testing"
	"time"
)

// Test9PDevice tests the 9P functionality of the exchange device
func Test9PDevice(t *testing.T) {
	// Skip if exchange device is not available
	device, err := os.OpenFile("#X/exchange", os.O_RDWR, 0)
	if err != nil {
		t.Skipf("Exchange device not available: %v", err)
	}
	defer device.Close()

	// Test basic read
	status := make([]byte, 1024)
	n, err := device.Read(status)
	if err != nil {
		t.Logf("Failed to read exchange status: %v", err)
	} else {
		t.Logf("Exchange status (%d bytes):\n%s", n, string(status[:n]))
	}

	// Test prepare command with a dummy address
	cmd := "prepare 0x10000000\n"
	n, err = device.Write([]byte(cmd))
	if err != nil {
		t.Errorf("Failed to send prepare command: %v", err)
	} else {
		t.Logf("Sent prepare command (%d bytes)", n)
	}

	// Small delay to allow processing
	time.Sleep(10 * time.Millisecond)

	// Read status again to see changes
	status = make([]byte, 1024)
	n, err = device.Read(status)
	if err != nil {
		t.Logf("Failed to read exchange status after prepare: %v", err)
	} else {
		t.Logf("Exchange status after prepare (%d bytes):\n%s", n, string(status[:n]))
	}

	t.Log("9P device test completed")
}
