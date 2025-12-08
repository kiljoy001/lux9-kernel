package main

import (
	"syscall"
)

func main() {
	b := []byte("Hello from Go Low Level!\n")
	syscall.Write(1, b)
	for {}
}

