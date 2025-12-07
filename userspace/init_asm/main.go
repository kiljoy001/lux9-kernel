package main

import (
	"runtime"
	"syscall"
)

func main() {
	// Print Go version and OS/arch
	println("Go version:", runtime.Version())
	println("GOOS:", runtime.GOOS, "GOARCH:", runtime.GOARCH)
	
        // Try a simple syscall to see if runtime is working
        // On Plan 9, syscall.Open only takes two arguments
        fd, err := syscall.Open("#c/cons", syscall.O_WRONLY)
        if err != nil {
                println("Open failed:", err.Error())
        } else {
                println("Open succeeded! fd =", fd)
                syscall.Write(fd, []byte("Hello from Plan 9!\n"))
        }
}
