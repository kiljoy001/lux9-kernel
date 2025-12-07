package main

import (
	"log"
	"os"
	"time"
)

func main() {
	pid := os.Getpid()
	log.Printf("DRIVER: Console Driver Started [PID %d]", pid)
	
	// Simulate driver initialization
	time.Sleep(500 * time.Millisecond)
	log.Println("DRIVER: Hardware Initialized.")

	// Simulate work loop
	count := 0
	for {
		count++
		log.Printf("DRIVER: Processing IO batch %d...", count)
		time.Sleep(2 * time.Second)

		// Simulate a random crash every ~10 seconds to test RS recovery
		if count == 5 {
			log.Println("DRIVER: CRITICAL ERROR - NULL POINTER EXCEPTION! Crashing...")
			os.Exit(1) // Crash
		}
	}
}
