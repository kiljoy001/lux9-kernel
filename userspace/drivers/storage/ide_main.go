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
	// Create IDE driver
	ideDriver := storage.NewIDEDriver()
	blockDriver := storage.NewBlockDeviceDriver(ideDriver)
	
	// Export the driver via 9P
	if err := framework.ExportDriver("ide0", blockDriver); err != nil {
		log.Fatalf("Failed to export IDE driver: %v", err)
	}
	
	log.Println("IDE driver exported to /srv/ide0")
	
	// Wait for interrupt signal
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	<-sigChan
	
	log.Println("IDE driver shutting down...")
}
