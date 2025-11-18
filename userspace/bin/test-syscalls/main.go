// Test Go syscalls on Lux9
// This program verifies that Go's Plan 9 syscalls work with Lux9 kernel
package main

import (
	"fmt"
	"os"
	"syscall"
)

func main() {
	fmt.Println("=== Lux9 Go Syscall Test ===")

	// Test 1: Basic write to stdout
	fmt.Println("\nTest 1: Write to stdout")
	msg := "Hello from Go on Lux9!\n"
	n, err := syscall.Write(1, []byte(msg))
	if err != nil {
		fmt.Printf("FAIL: Write returned error: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("PASS: Wrote %d bytes\n", n)

	// Test 2: Open a file for write
	fmt.Println("\nTest 2: Create and open file")
	fd, err := syscall.Open("/tmp/test.txt", syscall.O_WRONLY|syscall.O_CREAT|syscall.O_TRUNC)
	if err != nil {
		fmt.Printf("FAIL: Open failed: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("PASS: Opened file, fd=%d\n", fd)

	// Test 3: Write to file
	fmt.Println("\nTest 3: Write to file")
	testData := []byte("Test data from Go\n")
	n, err = syscall.Write(fd, testData)
	if err != nil {
		fmt.Printf("FAIL: Write to file failed: %v\n", err)
		syscall.Close(fd)
		os.Exit(1)
	}
	fmt.Printf("PASS: Wrote %d bytes to file\n", n)

	// Test 4: Close file
	fmt.Println("\nTest 4: Close file")
	err = syscall.Close(fd)
	if err != nil {
		fmt.Printf("FAIL: Close failed: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("PASS: File closed")

	// Test 5: Read back the file
	fmt.Println("\nTest 5: Read file back")
	fd, err = syscall.Open("/tmp/test.txt", syscall.O_RDONLY)
	if err != nil {
		fmt.Printf("FAIL: Open for read failed: %v\n", err)
		os.Exit(1)
	}

	buf := make([]byte, 1024)
	n, err = syscall.Read(fd, buf)
	if err != nil {
		fmt.Printf("FAIL: Read failed: %v\n", err)
		syscall.Close(fd)
		os.Exit(1)
	}
	fmt.Printf("PASS: Read %d bytes: %s", n, string(buf[:n]))
	syscall.Close(fd)

	// Test 6: Pipe creation
	fmt.Println("\nTest 6: Create pipe")
	var p [2]int
	err = syscall.Pipe(p[:])
	if err != nil {
		fmt.Printf("FAIL: Pipe failed: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("PASS: Created pipe, fds=%d,%d\n", p[0], p[1])

	// Test 7: Write to pipe
	fmt.Println("\nTest 7: Write to pipe")
	pipeData := []byte("pipe test")
	n, err = syscall.Write(p[1], pipeData)
	if err != nil {
		fmt.Printf("FAIL: Pipe write failed: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("PASS: Wrote %d bytes to pipe\n", n)

	// Test 8: Read from pipe
	fmt.Println("\nTest 8: Read from pipe")
	pipeBuf := make([]byte, 100)
	n, err = syscall.Read(p[0], pipeBuf)
	if err != nil {
		fmt.Printf("FAIL: Pipe read failed: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("PASS: Read %d bytes from pipe: %s\n", n, string(pipeBuf[:n]))

	syscall.Close(p[0])
	syscall.Close(p[1])

	// Test 9: Environment variables
	fmt.Println("\nTest 9: Environment variables")
	os.Setenv("TEST_VAR", "test_value")
	val := os.Getenv("TEST_VAR")
	if val != "test_value" {
		fmt.Printf("FAIL: Environment variable mismatch: got %s\n", val)
		os.Exit(1)
	}
	fmt.Println("PASS: Environment variables work")

	fmt.Println("\n=== All Tests PASSED ===")
	fmt.Println("Go syscalls are working correctly on Lux9!")
}
