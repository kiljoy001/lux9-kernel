package sip

import (
	"fmt"
	"os"
	"syscall"
	"unsafe"
)

// KernelSession represents an active session with the kernel SIP driver
type KernelSession struct {
	id       int
	ctlFile  *os.File
	ipc      *ExchangeIPC
}

// GetIPC returns the ExchangeIPC for this session
func (k *KernelSession) GetIPC() *ExchangeIPC {
	return k.ipc
}

// RegisterWithKernel performs the handshake with /dev/sip to register the current process
func (s *BaseServer) RegisterWithKernel() error {
	// 1. Clone a new server instance
	cloneFile, err := os.OpenFile("/dev/sip/clone", os.O_RDWR, 0)
	if err != nil {
		return fmt.Errorf("failed to open /dev/sip/clone: %v", err)
	}
	// The clone file *becomes* the ctl file for the new instance in Plan 9 style,
	// or we might need to read the ID.
	// Based on devsip.c: "Redirect c to the ctl file of the new server"
	// So cloneFile IS the ctl file.

	// We need to find out our ID. devsip.c read() on ctl returns status info including ID.
	buf := make([]byte, 1024)
	n, err := cloneFile.Read(buf)
	if err != nil {
		cloneFile.Close()
		return fmt.Errorf("failed to read server info: %v", err)
	}
	
	var id int
	// Output format: "id: %d\nname: %s..."
	_, err = fmt.Sscanf(string(buf[:n]), "id: %d", &id)
	if err != nil {
		cloneFile.Close()
		return fmt.Errorf("failed to parse server ID: %v", err)
	}

	session := &KernelSession{
		id:      id,
		ctlFile: cloneFile,
	}

	// 2. Configure capabilities
	if err := session.configure(s.config); err != nil {
		cloneFile.Close()
		return err
	}

	// 3. Setup Exchange IPC
	ipc, err := NewExchangeIPC()
	if err != nil {
		cloneFile.Close()
		return fmt.Errorf("failed to setup exchange IPC: %v", err)
	}
	session.ipc = ipc

	// 4. Start (binds current PID to the server instance)
	if _, err := cloneFile.WriteString("start"); err != nil {
		cloneFile.Close()
		return fmt.Errorf("failed to start server: %v", err)
	}

	s.session = session
	return nil
}

func (k *KernelSession) configure(config *ServerConfig) error {
	// Set Name
	if _, err := fmt.Fprintf(k.ctlFile, "name %s", config.Name); err != nil {
		return err
	}

	// Set Capabilities
	if config.Capabilities&CapDeviceAccess != 0 {
		if _, err := k.ctlFile.WriteString("capability device"); err != nil {
			return err
		}
	}
	if config.Capabilities&CapInterrupt != 0 {
		if _, err := k.ctlFile.WriteString("capability interrupt"); err != nil {
			return err
		}
	}
	if config.Capabilities&CapDMA != 0 {
		if _, err := k.ctlFile.WriteString("capability dma"); err != nil {
			return err
		}
	}
	if config.Capabilities&CapFileSystem != 0 {
		if _, err := k.ctlFile.WriteString("capability fs"); err != nil {
			return err
		}
	}
	
	// Could add others (networking, etc) matching devsip.c logic

	return nil
}

// ExchangeIPC handles both ring buffer (small messages) and page exchange (large messages)
type ExchangeIPC struct {
	// Ring buffer for batched small messages
	ringFd      int
	ringChannel *IpcChannel // Mapped control page from /dev/ring

	// Page exchange for large individual messages
	exchangeFd  int
	recvAddr    uintptr // Virtual address to map incoming pages
	sendAddr    uintptr // Virtual address for outgoing pages

	// Buffers backed by the mapped pages
	recvBuf     []byte
	sendBuf     []byte
}

// IPC Ring structures (matching kernel ipc_ring.h)
const (
	RingSize       = 128
	RingMask       = 127
	BatchPageMagic = 0xB47C4831
	RingMagic      = 0x52494E47
	BatchDataStart = 16 // sizeof(BatchHeader)
)

type IpcPageRing struct {
	Head  uint32
	Tail  uint32
	Mask  uint32
	Flags uint32
	Pages [RingSize]uint64
}

type IpcChannel struct {
	Magic      uint32
	Status     uint32
	Submission IpcPageRing
	Completion IpcPageRing
}

type BatchHeader struct {
	NumMessages uint16
	UsedBytes   uint16
	Magic       uint32
	Nonce       uint64
}

func NewExchangeIPC() (*ExchangeIPC, error) {
	pageSize := syscall.Getpagesize()

	// 1. Open and setup ring buffer device
	ringFd, err := syscall.Open("/dev/ring/0", syscall.O_RDWR, 0)
	if err != nil {
		return nil, fmt.Errorf("failed to open /dev/ring/0: %v", err)
	}

	// mmap the ring control page
	ringMem, err := syscall.Mmap(ringFd, 0, pageSize, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_SHARED)
	if err != nil {
		syscall.Close(ringFd)
		return nil, fmt.Errorf("mmap ring failed: %v", err)
	}
	ringChannel := (*IpcChannel)(unsafe.Pointer(&ringMem[0]))

	// Verify ring magic
	if ringChannel.Magic != RingMagic {
		syscall.Munmap(ringMem)
		syscall.Close(ringFd)
		return nil, fmt.Errorf("invalid ring magic: got 0x%x, want 0x%x", ringChannel.Magic, RingMagic)
	}

	// 2. Open the page exchange device (for large messages)
	exchangeFd, err := syscall.Open("/dev/exchange", syscall.O_RDWR, 0)
	if err != nil {
		syscall.Munmap(ringMem)
		syscall.Close(ringFd)
		return nil, fmt.Errorf("failed to open /dev/exchange: %v", err)
	}

	// 3. Allocate page-aligned buffers for page exchange
	recvMem, err := syscall.Mmap(-1, 0, pageSize, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_ANON|syscall.MAP_PRIVATE)
	if err != nil {
		syscall.Close(exchangeFd)
		syscall.Munmap(ringMem)
		syscall.Close(ringFd)
		return nil, fmt.Errorf("mmap recv failed: %v", err)
	}

	sendMem, err := syscall.Mmap(-1, 0, pageSize, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_ANON|syscall.MAP_PRIVATE)
	if err != nil {
		syscall.Munmap(recvMem)
		syscall.Close(exchangeFd)
		syscall.Munmap(ringMem)
		syscall.Close(ringFd)
		return nil, fmt.Errorf("mmap send failed: %v", err)
	}

	return &ExchangeIPC{
		ringFd:      ringFd,
		ringChannel: ringChannel,
		exchangeFd:  exchangeFd,
		recvAddr:    uintptr(unsafe.Pointer(&recvMem[0])),
		sendAddr:    uintptr(unsafe.Pointer(&sendMem[0])),
		recvBuf:     recvMem,
		sendBuf:     sendMem,
	}, nil
}

// ReceiveLoop processes incoming 9P messages from ring buffer (batched small messages)
// and page exchange (large individual messages)
func (ipc *ExchangeIPC) ReceiveLoop(handler func([]byte) []byte) {
	for {
		// 1. Check ring buffer for batched small messages (primary path)
		ipc.processRingBatches(handler)

		// 2. Check page exchange for large individual messages (fallback)
		// Note: In a real implementation, this would be event-driven (select/poll)
		// For now, we prioritize ring buffer processing
	}
}

// processRingBatches handles batched messages from the ring buffer
func (ipc *ExchangeIPC) processRingBatches(handler func([]byte) []byte) {
	ring := &ipc.ringChannel.Submission

	// Process all pending batch pages
	for ring.Head != ring.Tail {
		// Get page handle (user virtual address)
		pageHandle := ring.Pages[ring.Head&RingMask]

		// Map and process the batch page
		ipc.processBatchPage(pageHandle, handler)

		// Return page to completion ring
		compRing := &ipc.ringChannel.Completion
		compRing.Pages[compRing.Tail&RingMask] = pageHandle
		compRing.Tail++

		// Advance submission ring head
		ring.Head++
	}
}

// processBatchPage processes a single batch page containing multiple 9P messages
func (ipc *ExchangeIPC) processBatchPage(pageHandle uint64, handler func([]byte) []byte) {
	var batch *BatchHeader

	// SECURITY: Validate page handle
	if pageHandle == 0 {
		fmt.Fprintf(os.Stderr, "ring: invalid page handle: 0\n")
		return
	}

	// SECURITY: Check page alignment (must be 4KB aligned)
	if pageHandle&0xFFF != 0 {
		fmt.Fprintf(os.Stderr, "ring: page handle not aligned: %#x\n", pageHandle)
		return
	}

	// Map the batch page via page exchange
	// Write "accept <handle> <vaddr> <prot>" to /dev/exchange
	cmd := fmt.Sprintf("accept %d %d %d", pageHandle, ipc.recvAddr, 0600)
	if _, err := syscall.Write(ipc.exchangeFd, []byte(cmd)); err != nil {
		fmt.Fprintf(os.Stderr, "ring: failed to accept page: %v\n", err)
		return
	}

	// Access the mapped batch page
	batchMem := ipc.recvBuf

	// Parse batch header
	if len(batchMem) < BatchDataStart {
		fmt.Fprintf(os.Stderr, "ring: batch too small: %d bytes\n", len(batchMem))
		goto cleanup
	}

	batch = (*BatchHeader)(unsafe.Pointer(&batchMem[0]))

	// SECURITY: Validate magic number
	if batch.Magic != BatchPageMagic {
		fmt.Fprintf(os.Stderr, "ring: invalid batch magic: %#x (expected %#x)\n",
			batch.Magic, BatchPageMagic)
		goto cleanup
	}

	// SECURITY: Validate num_messages
	if batch.NumMessages > 256 {
		fmt.Fprintf(os.Stderr, "ring: too many messages: %d (max 256)\n", batch.NumMessages)
		goto cleanup
	}

	// SECURITY: Validate used_bytes
	if batch.UsedBytes < BatchDataStart || batch.UsedBytes > 4096 {
		fmt.Fprintf(os.Stderr, "ring: invalid used_bytes: %d\n", batch.UsedBytes)
		goto cleanup
	}

	// Process each message in the batch
	{
		offset := BatchDataStart
		for i := 0; i < int(batch.NumMessages); i++ {
			// SECURITY: Check we can read message length header
			if offset+2 > int(batch.UsedBytes) {
				fmt.Fprintf(os.Stderr, "ring: message header exceeds used_bytes at offset %d\n", offset)
				break
			}

			// Read message length (u16 little-endian)
			msgLen := uint16(batchMem[offset]) | (uint16(batchMem[offset+1]) << 8)
			offset += 2

			// SECURITY: Validate message length
			if msgLen > 8192 {
				fmt.Fprintf(os.Stderr, "ring: message too large: %d bytes\n", msgLen)
				break
			}

			// SECURITY: Check message fits in batch
			if offset+int(msgLen) > int(batch.UsedBytes) {
				fmt.Fprintf(os.Stderr, "ring: message data exceeds used_bytes\n")
				break
			}

			// SECURITY: Check for integer overflow
			if offset+int(msgLen) < offset {
				fmt.Fprintf(os.Stderr, "ring: integer overflow in message bounds\n")
				break
			}

			// Extract message data
			msgData := batchMem[offset : offset+int(msgLen)]

			// Process 9P message
			respData := handler(msgData)

			// TODO: Write response back to batch page
			// For now, we just process and discard responses
			// Full implementation would batch responses similarly

			_ = respData
			offset += int(msgLen)
		}
	}

cleanup:
	// Release the page back via exchange
	releaseCmd := fmt.Sprintf("cancel %d", pageHandle)
	if _, err := syscall.Write(ipc.exchangeFd, []byte(releaseCmd)); err != nil {
		fmt.Fprintf(os.Stderr, "ring: failed to release page: %v\n", err)
	}
}