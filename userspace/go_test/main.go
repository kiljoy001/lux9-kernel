package main

import (
	"log"
	"net/http"
	"os"
	"syscall"
)

func main() {
	log.Println("Go Init: Bootstrapping Lux9...")

	// 1. Create Mount Points
	os.MkdirAll("/dev", 0777)
	os.MkdirAll("/proc", 0777)
	os.MkdirAll("/net", 0777)
	os.MkdirAll("/env", 0777)

	// 2. Bind Kernel Devices (The "Magic" Step)
	mustBind("#c", "/dev", syscall.MREPL)  // Console & drivers
	mustBind("#p", "/proc", syscall.MREPL) // Process control
	mustBind("#I", "/net", syscall.MREPL)  // TCP/IP Stack
	mustBind("#e", "/env", syscall.MREPL)  // Environment variables

	// 3. Start Your Server
	log.Println("System ready. Starting HTTP server on :80...")
	
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("Hello from Native Go on Lux9!"))
	})

	// Use :8080 locally if :80 is privileged/blocked, but on bare metal :80 is fine.
	if err := http.ListenAndServe(":80", nil); err != nil {
		log.Fatalf("Server died: %v", err)
	}
}

func mustBind(new, old string, flags int) {
	if err := syscall.Bind(new, old, flags); err != nil {
		log.Printf("Warning: Failed to bind %s to %s: %v", new, old, err)
	} else {
		log.Printf("Bound %s to %s", new, old)
	}
}
