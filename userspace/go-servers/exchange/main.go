package main

import (
	"fmt"
	"os"
	"time"
)

func main() {
	fmt.Println("Exchange 9P Test Program")
	fmt.Println("========================")

	// Try to open the exchange device
	fmt.Println("\n--- Opening exchange device ---")
	device, err := os.OpenFile("#X/exchange", os.O_RDWR, 0)
	if err != nil {
		fmt.Printf("Failed to open exchange device: %v\n", err)
		fmt.Println("This is expected when running outside the kernel environment.")
		os.Exit(1)
	}
	defer device.Close()
	fmt.Println("Successfully opened exchange device")

	// Read current status
	fmt.Println("\n--- Reading initial status ---")
	status := make([]byte, 1024)
	n, err := device.Read(status)
	if err != nil {
		fmt.Printf("Failed to read exchange status: %v\n", err)
	} else {
		fmt.Printf("Exchange status (%d bytes):\n%s\n", n, string(status[:n]))
	}

	// Test prepare command with a dummy address
	fmt.Println("\n--- Sending prepare command ---")
	cmd := "prepare 0x10000000\n"
	n, err = device.Write([]byte(cmd))
	if err != nil {
		fmt.Printf("Failed to send prepare command: %v\n", err)
	} else {
		fmt.Printf("Sent prepare command (%d bytes)\n", n)
	}

	// Small delay to allow processing
	time.Sleep(100 * time.Millisecond)

	// Read status again to see changes
	fmt.Println("\n--- Reading status after prepare ---")
	status = make([]byte, 1024)
	n, err = device.Read(status)
	if err != nil {
		fmt.Printf("Failed to read exchange status after prepare: %v\n", err)
	} else {
		fmt.Printf("Exchange status after prepare (%d bytes):\n%s\n", n, string(status[:n]))
	}

	fmt.Println("\n--- Exchange test completed ---")
}
