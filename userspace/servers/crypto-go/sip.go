// SIP (Secure Interface Paging) support for Lux9 servers
// Uses exchange pages for zero-copy 9P message passing
package main

import (
	"fmt"
	"syscall"
)

// SIPConnection represents a connection via the exchange device
type SIPConnection struct {
	exchangeFd int    // File descriptor for #X/exchange
	msgBuf     []byte // Buffer for 9P messages
}

// OpenExchangeDevice opens the SIP exchange device
// This is how userspace servers communicate via 9P in Lux9
func OpenExchangeDevice() (*SIPConnection, error) {
	// Open the exchange device (#X/exchange)
	// This is Lux9's zero-copy IPC mechanism
	fd, err := syscall.Open("#X/exchange", syscall.O_RDWR)
	if err != nil {
		return nil, fmt.Errorf("failed to open exchange device: %v", err)
	}

	fmt.Printf("[sip] Opened exchange device (fd=%d)\n", fd)

	conn := &SIPConnection{
		exchangeFd: fd,
		msgBuf:     make([]byte, 8192), // 8KB buffer for 9P messages
	}

	return conn, nil
}

// Close closes the SIP connection
func (c *SIPConnection) Close() error {
	return syscall.Close(c.exchangeFd)
}

// ReadStatus reads the exchange device status
// This shows current exchange page state
func (c *SIPConnection) ReadStatus() (string, error) {
	buf := make([]byte, 1024)
	n, err := syscall.Read(c.exchangeFd, buf)
	if err != nil {
		return "", err
	}
	return string(buf[:n]), nil
}

// PrepareExchangePage prepares a virtual address for page exchange
// Format: "prepare 0xVADDR\n"
func (c *SIPConnection) PrepareExchangePage(vaddr uintptr) error {
	cmd := fmt.Sprintf("prepare 0x%x\n", vaddr)
	n, err := syscall.Write(c.exchangeFd, []byte(cmd))
	if err != nil {
		return err
	}
	if n != len(cmd) {
		return fmt.Errorf("short write: wrote %d of %d bytes", n, len(cmd))
	}
	return nil
}

// Send9PMessage sends a 9P message through the exchange device
func (c *SIPConnection) Send9PMessage(msg []byte) error {
	n, err := syscall.Write(c.exchangeFd, msg)
	if err != nil {
		return fmt.Errorf("failed to send 9P message: %v", err)
	}
	if n != len(msg) {
		return fmt.Errorf("short write: wrote %d of %d bytes", n, len(msg))
	}
	return nil
}

// Receive9PMessage receives a 9P message from the exchange device
func (c *SIPConnection) Receive9PMessage() ([]byte, error) {
	n, err := syscall.Read(c.exchangeFd, c.msgBuf)
	if err != nil {
		return nil, fmt.Errorf("failed to read 9P message: %v", err)
	}
	if n == 0 {
		return nil, fmt.Errorf("connection closed")
	}

	// Return a copy of the message
	msg := make([]byte, n)
	copy(msg, c.msgBuf[:n])
	return msg, nil
}

// Serve9P is the main server loop that handles 9P messages via SIP
func (c *SIPConnection) Serve9P(handler func([]byte) ([]byte, error)) error {
	fmt.Println("[sip] Starting 9P server loop...")

	for {
		// Read 9P message from exchange device
		reqMsg, err := c.Receive9PMessage()
		if err != nil {
			return fmt.Errorf("receive error: %v", err)
		}

		fmt.Printf("[sip] Received 9P message (%d bytes)\n", len(reqMsg))

		// Handle the message
		respMsg, err := handler(reqMsg)
		if err != nil {
			fmt.Printf("[sip] Handler error: %v\n", err)
			continue
		}

		// Send response
		err = c.Send9PMessage(respMsg)
		if err != nil {
			return fmt.Errorf("send error: %v", err)
		}

		fmt.Printf("[sip] Sent 9P response (%d bytes)\n", len(respMsg))
	}
}

// Example 9P message handler
func handle9PMessage(msg []byte) ([]byte, error) {
	// TODO: Parse 9P message
	// - Extract message type (Tversion, Tattach, Topen, Tread, Twrite, etc.)
	// - Dispatch to appropriate handler
	// - Build response message

	// For now, just echo back
	fmt.Printf("[sip] Handling message: %x\n", msg[:min(16, len(msg))])

	// In reality, you would:
	// 1. Parse the Fcall structure from msg
	// 2. Handle based on message type
	// 3. Generate appropriate Rcall response
	// 4. Serialize back to bytes

	return msg, nil // Placeholder
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
