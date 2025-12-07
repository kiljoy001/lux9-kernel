package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/sha256"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"syscall"
	"time"

	"github.com/fxamacker/cbor/v2"
)

// --- ROOT OF TRUST ---
// Generated via tools/security/keygen.go
var rootPubKey = ed25519.PublicKey{
    0x7f, 0x3f, 0x56, 0x51, 0xa2, 0xa6, 0xf3, 0x86,
    0x9b, 0x1d, 0x59, 0x13, 0x98, 0x57, 0x69, 0x5d,
    0xc0, 0x2d, 0xbb, 0xd3, 0x1a, 0x5b, 0x48, 0xa1,
    0xcf, 0x67, 0x46, 0xea, 0xa7, 0xe9, 0xab, 0xb0,
}

// Service definition mapping to CBOR keys
type Service struct {
	Name    string   `cbor:"name"`
	Binary  string   `cbor:"bin"`
	Args    []string `cbor:"args,omitempty"`
	Caps    []string `cbor:"caps,omitempty"` // SIP Capabilities
	Hash    []byte   `cbor:"hash,omitempty"`
}

func main() {
    // 1. Bootstrap Namespace (Essential for any PID 1)
    os.MkdirAll("/dev", 0777)
    os.MkdirAll("/proc", 0777)
    os.MkdirAll("/net", 0777)
    os.MkdirAll("/boot", 0777)

    mustBind("#c", "/dev", syscall.MREPL)
    mustBind("#p", "/proc", syscall.MREPL)
    mustBind("#I", "/net", syscall.MREPL)
    
	log.Println("RS: Secure Supervisor (CBOR Edition) Starting...")

	// 2. Secure Load
	services, err := loadManifest("/boot/services.cbor")
	if err != nil {
		log.Fatalf("RS: SECURITY PANIC - %v", err)
	}

	log.Printf("RS: Loaded %d verified services.", len(services))

	// 3. Supervisor Loop
	for _, svc := range services {
		go supervise(svc)
	}

	select {} // Block forever
}

func mustBind(new, old string, flags int) {
	if err := syscall.Bind(new, old, flags); err != nil {
		log.Printf("Warning: Failed to bind %s to %s: %v", new, old, err)
	}
}

func loadManifest(path string) ([]Service, error) {
	// A. Read Artifacts
	payload, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("cannot read manifest: %v", err)
	}

	signature, err := ioutil.ReadFile(path + ".sig")
	if err != nil {
		return nil, fmt.Errorf("cannot read signature: %v", err)
	}

	// B. Verify Integrity (Hash-then-Verify)
	hash := sha256.Sum256(payload)

	if !ed25519.Verify(rootPubKey, hash[:], signature) {
		return nil, fmt.Errorf("INVALID SIGNATURE: Config file has been tampered with")
	}

	// C. Parse CBOR
	var services []Service
	if err := cbor.Unmarshal(payload, &services); err != nil {
		return nil, fmt.Errorf("malformed CBOR: %v", err)
	}

	return services, nil
}

func supervise(s Service) {
	for {
		log.Printf("RS: Starting %s (%s)...", s.Name, s.Binary)
		
		cmd := exec.Command(s.Binary, s.Args...)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr

		if err := cmd.Start(); err != nil {
			log.Printf("RS: Failed to start %s: %v", s.Name, err)
			time.Sleep(5 * time.Second)
			continue
		}

        // Verify Hash immediately after start
        if s.Hash != nil {
            // Give the kernel a moment to setup the process (though exec is synchronous)
            // Ideally we check before it does anything dangerous.
            // But /proc is only available after it exists.
            // Since we are the parent, we know its PID.
            pid := cmd.Process.Pid
            
            actualHash, err := ioutil.ReadFile(fmt.Sprintf("/proc/%d/hash", pid))
            if err != nil {
                 log.Printf("RS: Failed to read hash for PID %d: %v. KILLING.", pid, err)
                 cmd.Process.Kill()
                 continue
            }
            
            if !bytes.Equal(actualHash, s.Hash) {
                log.Printf("RS: SECURITY ALERT! Hash mismatch for %s (PID %d). KILLING.", s.Name, pid)
                log.Printf("Expected: %x", s.Hash)
                log.Printf("Actual:   %x", actualHash)
                cmd.Process.Kill()
                // Maybe panic or halt system if core driver is compromised?
                continue 
            }
            log.Printf("RS: Service %s verified (Hash Match).", s.Name)
        }

		cmd.Wait()
		log.Printf("RS: Service %s exited. Restarting...", s.Name)
		time.Sleep(1 * time.Second)
	}
}