package main

import (
	"context"
	"fmt"
	"log"

	"lux9/servers/p9"
	"lux9/userspace/go-servers/sip"
)

// CryptoServer implements the SIP IServer interface
type CryptoServer struct {
	*sip.BaseServer
	fs     *CryptoFS
	p9srv  *p9.Server
	ctx    context.Context
	cancel context.CancelFunc
}

// Initialize sets up the crypto server
func (cs *CryptoServer) Initialize(ctx context.Context, config *sip.ServerConfig) error {
	if err := cs.BaseServer.Initialize(ctx, config); err != nil {
		return fmt.Errorf("BaseServer.Initialize failed: %w", err)
	}

	// Create crypto filesystem
	cs.fs = NewCryptoFS()

	// Create 9P server
	cs.p9srv = p9.NewServer(cs.fs)

	// Store context
	cs.ctx, cs.cancel = context.WithCancel(ctx)

	log.Printf("CryptoServer initialized: %s", config.Name)
	return nil
}

// Start begins serving 9P requests via page exchange
func (cs *CryptoServer) Start(ctx context.Context) error {
	log.Printf("CryptoServer starting 9P service via page exchange")

	// Get the ExchangeIPC from the session (set up during RegisterWithKernel)
	session := cs.GetSession()
	if session == nil {
		return fmt.Errorf("KernelSession not initialized - did RegisterWithKernel succeed?")
	}

	ipc := session.GetIPC()
	if ipc == nil {
		return fmt.Errorf("ExchangeIPC not initialized - did RegisterWithKernel succeed?")
	}

	// Create a 9P message handler that processes requests from exchange pages
	handler := func(reqData []byte) []byte {
		// Parse 9P request from the page
		// Process it through our 9P server
		// Return response data
		return cs.handle9PRequest(reqData)
	}

	// Start the receive loop - this blocks until shutdown
	ipc.ReceiveLoop(handler)

	return nil
}

// Stop gracefully shuts down the server
func (cs *CryptoServer) Stop(ctx context.Context) error {
	log.Printf("CryptoServer stopping")
	if cs.cancel != nil {
		cs.cancel()
	}
	return nil
}

// handle9PRequest processes a 9P request from an exchange page
func (cs *CryptoServer) handle9PRequest(reqData []byte) []byte {
	// Read 9P message size (first 4 bytes, little-endian)
	if len(reqData) < 4 {
		log.Printf("Invalid 9P message: too short (%d bytes)", len(reqData))
		return nil
	}

	// Standard 9P message format: size[4] type[1] tag[2] ...
	// For now, we'll need to manually parse and dispatch
	// This is a simplified implementation - a full version would use the p9 library's parsing

	// Create a response buffer
	respBuf := make([]byte, 8192) // Max 9P message size

	// TODO: Properly integrate with p9.Server's message parsing
	// For now, this is a placeholder that shows the architecture

	log.Printf("Processing 9P message of %d bytes", len(reqData))

	// Return error response for now
	// In production, this would call into cs.p9srv to handle the message
	return respBuf[:128] // Placeholder response
}

// Factory function for creating CryptoServer instances
func newCryptoServer(config *sip.ServerConfig) (sip.IServer, error) {
	return &CryptoServer{
		BaseServer: sip.NewBaseServer(config),
	}, nil
}

func main() {
	log.SetPrefix("cryptosrv: ")
	log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)

	log.Println("Crypto Server starting...")

	// Create server factory and register crypto server
	factory := sip.NewServerFactory()
	factory.Register("crypto", newCryptoServer)

	// Configure server
	config := &sip.ServerConfig{
		Name:         "cryptosrv",
		Capabilities: sip.CapFileSystem,
		MountPoint:   "/crypto",
	}

	// Create server manager
	manager := sip.NewServerManager(factory)

	// Start server (blocks until shutdown)
	ctx := context.Background()
	if err := manager.StartServer(ctx, "crypto", config); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}

	log.Println("Crypto Server exiting")
}
