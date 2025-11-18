// Lux9 Init - First userspace process (Go version)
// This replaces the C init with a cleaner Go implementation
package main

import (
	"fmt"
	"os"
	"syscall"
	"time"
)

func panic_init(msg string) {
	fmt.Printf("\n")
	fmt.Printf("=====================================\n")
	fmt.Printf("   INIT PANIC\n")
	fmt.Printf("=====================================\n")
	fmt.Printf("ERROR: %s\n", msg)
	fmt.Printf("\n")

	// Try to run emergency shell
	emergencyShell()

	// If that fails, halt
	for {
		time.Sleep(time.Hour)
	}
}

func emergencyShell() {
	fmt.Printf("\n")
	fmt.Printf("========================================\n")
	fmt.Printf("   EMERGENCY SHELL - System Recovery\n")
	fmt.Printf("========================================\n")
	fmt.Printf("\n")
	fmt.Printf("Something went wrong during boot.\n")
	fmt.Printf("Attempting to start minimal shell...\n")
	fmt.Printf("\n")

	// Try various shell paths
	shells := []string{"/bin/rc", "/bin/sh", "/bin/rush"}
	for _, shell := range shells {
		fmt.Printf("Trying %s...\n", shell)
		err := syscall.Exec(shell, []string{shell, "-i"}, os.Environ())
		if err != nil {
			fmt.Printf("  Failed: %v\n", err)
		}
	}

	fmt.Printf("\nNo shell available!\n")
	fmt.Printf("System halted.\n")
}

func bindDevice(device, path string) error {
	fmt.Printf("[init] Binding %s to %s\n", device, path)
	err := syscall.Bind(device, path, syscall.MAFTER)
	if err != nil {
		return fmt.Errorf("bind %s to %s failed: %v", device, path, err)
	}
	return nil
}

func startServer(path string, args []string) (int, error) {
	fmt.Printf("[init] Starting %s", path)
	if len(args) > 1 {
		fmt.Printf(" with args: %v", args[1:])
	}
	fmt.Printf("\n")

	// Fork
	pid, err := syscall.ForkExec(path, args, &syscall.ProcAttr{
		Files: []uintptr{os.Stdin.Fd(), os.Stdout.Fd(), os.Stderr.Fd()},
		Env:   os.Environ(),
	})
	if err != nil {
		return -1, fmt.Errorf("failed to start %s: %v", path, err)
	}

	return pid, nil
}

func main() {
	fmt.Printf("\n")
	fmt.Printf("==============================================\n")
	fmt.Printf("   Lux9 Init (Go version)\n")
	fmt.Printf("==============================================\n")
	fmt.Printf("First userspace process starting...\n")
	fmt.Printf("\n")

	// Step 1: Bind essential device drivers
	fmt.Printf("[init] Step 1: Binding device drivers\n")

	// Bind console device (#c) to /dev
	err := bindDevice("#c", "/dev")
	if err != nil {
		fmt.Printf("WARNING: %v\n", err)
		fmt.Printf("Continuing anyway...\n")
	}

	// Bind environment device (#e) to /env
	err = bindDevice("#e", "/env")
	if err != nil {
		fmt.Printf("WARNING: %v\n", err)
		fmt.Printf("Continuing anyway...\n")
	}

	// Step 2: Set up stdio
	fmt.Printf("[init] Step 2: Setting up console I/O\n")

	// Open console
	consfd, err := syscall.Open("/dev/cons", syscall.O_RDWR)
	if err != nil {
		fmt.Printf("WARNING: Cannot open /dev/cons: %v\n", err)
		fmt.Printf("Stdin/stdout may not work properly\n")
	} else {
		// Dup to stdin, stdout, stderr
		syscall.Dup(consfd, 0)
		syscall.Dup(consfd, 1)
		syscall.Dup(consfd, 2)
		if consfd > 2 {
			syscall.Close(consfd)
		}
		fmt.Printf("[init] Console configured (fds 0,1,2)\n")
	}

	// Step 3: Set up environment
	fmt.Printf("[init] Step 3: Setting up environment\n")
	os.Setenv("path", "/bin")
	os.Setenv("home", "/root")
	os.Setenv("user", "root")
	os.Setenv("prompt", "; ")
	fmt.Printf("[init] Environment variables set\n")

	// Step 4: Start filesystem server (if available)
	fmt.Printf("[init] Step 4: Starting filesystem server\n")

	// Get root device from kernel params (TODO: read from actual kernel cmdline)
	rootdev := os.Getenv("rootdev")
	if rootdev == "" {
		rootdev = "hd0:0"
	}
	fmt.Printf("[init] Root device: %s\n", rootdev)

	// Try to start ext4fs server
	fsPid, err := startServer("/bin/ext4fs", []string{"/bin/ext4fs", rootdev})
	if err != nil {
		fmt.Printf("WARNING: Cannot start filesystem server: %v\n", err)
		fmt.Printf("Continuing without root filesystem...\n")
	} else {
		fmt.Printf("[init] Filesystem server started (pid %d)\n", fsPid)

		// Wait for server to initialize
		time.Sleep(200 * time.Millisecond)

		// Mount root (Plan 9 mount takes fd, not pid)
		// For now, skip mounting - this requires 9P file descriptor
		// TODO: Implement proper 9P connection to server
		fmt.Printf("[init] TODO: Mount root filesystem at /\n")
		fmt.Printf("[init] (Requires 9P connection setup)\n")
	}

	// Step 5: Start crypto server
	fmt.Printf("[init] Step 5: Starting crypto server\n")
	cryptoPid, err := startServer("/bin/crypto", []string{"/bin/crypto"})
	if err != nil {
		fmt.Printf("WARNING: Cannot start crypto server: %v\n", err)
	} else {
		fmt.Printf("[init] Crypto server started (pid %d)\n", cryptoPid)

		// Mount crypto at /crypto (Plan 9 mount takes fd, not pid)
		// TODO: Implement proper 9P connection
		time.Sleep(100 * time.Millisecond)
		fmt.Printf("[init] TODO: Mount crypto service at /crypto\n")
		fmt.Printf("[init] (Requires 9P connection setup)\n")
	}

	// Step 6: Start other essential servers
	fmt.Printf("[init] Step 6: Starting additional servers\n")

	// TODO: Start other servers (devfs, procfs, network, etc.)

	// Step 7: Run tests if TEST environment variable is set
	if os.Getenv("TEST") != "" {
		fmt.Printf("[init] Running tests...\n")

		testPid, err := startServer("/bin/test-syscalls", []string{"/bin/test-syscalls"})
		if err != nil {
			fmt.Printf("WARNING: Test failed to start: %v\n", err)
		} else {
			fmt.Printf("[init] Test started (pid %d)\n", testPid)
		}
	}

	// Step 8: Exec shell
	fmt.Printf("[init] Step 8: Starting user shell\n")
	fmt.Printf("\n")
	fmt.Printf("==============================================\n")
	fmt.Printf("   Lux9 boot complete!\n")
	fmt.Printf("==============================================\n")
	fmt.Printf("\n")

	// Try to exec rc shell
	shells := []struct {
		path string
		args []string
	}{
		{"/bin/rc", []string{"/bin/rc", "-i"}},
		{"/bin/sh", []string{"/bin/sh", "-i"}},
		{"/sbin/init", []string{"/sbin/init"}},
	}

	for _, shell := range shells {
		fmt.Printf("[init] Trying to exec %s...\n", shell.path)
		err := syscall.Exec(shell.path, shell.args, os.Environ())
		if err != nil {
			fmt.Printf("[init] %s not available: %v\n", shell.path, err)
		}
	}

	// If we get here, no shell was available
	fmt.Printf("\n")
	fmt.Printf("[init] No shell available!\n")
	fmt.Printf("[init] Available options:\n")
	fmt.Printf("  - Build /bin/rc (rc shell from 9front)\n")
	fmt.Printf("  - Build a simple shell in Go\n")
	fmt.Printf("  - Mount a filesystem with a shell\n")
	fmt.Printf("\n")
	fmt.Printf("[init] System is running but no interactive shell.\n")
	fmt.Printf("[init] Entering idle loop...\n")

	// Idle loop - system is running but no shell
	for {
		time.Sleep(time.Hour)
	}
}
