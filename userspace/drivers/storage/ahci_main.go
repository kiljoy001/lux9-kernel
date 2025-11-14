package main

import (
	"log"
	"os"
	"os/signal"
	"syscall"
	
	"framework"
	"storage"
)

func main() {
	// Create AHCI driver
	ahciDriver := storage.NewAHCIDriver()
	blockDriver := storage.NewBlockDeviceDriver(ahciDriver)
	
	// Export the driver via 9P
	if err := framework.ExportDriver("ahci0", blockDriver); err != nil {
		log.Fatalf("Failed to export AHCI driver: %v", err)
	}
	
	log.Println("AHCI driver exported to /srv/ahci0")
	
	// Wait for interrupt signal
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	<-sigChan
	
	log.Println("AHCI driver shutting down...")
}
