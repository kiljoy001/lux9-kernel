package main

import (
	"fmt"
	"os"
	"testing"
)

func TestMinimalDevice(t *testing.T) {
	fmt.Println("Trying to open exchange device...")
	device, err := os.OpenFile("#X/exchange", os.O_RDWR, 0)
	if err != nil {
		t.Logf("Failed to open exchange device: %v", err)
		t.Skip("Exchange device not available")
	}
	defer device.Close()
	fmt.Println("Successfully opened exchange device")
}
