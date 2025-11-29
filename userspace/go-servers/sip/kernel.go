package sip

import (
	"fmt"
	"os"
	"strings"
	"syscall"
	"unsafe"
)

// KernelSession represents an active session with the kernel SIP driver
type KernelSession struct {
	id       int
	ctlFile  *os.File
	ipc      *ExchangeIPC
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

// ExchangeIPC handles the page exchange protocol
type ExchangeIPC struct {
	fd       int
	recvAddr uintptr // Virtual address to map incoming pages
	sendAddr uintptr // Virtual address for outgoing pages
	
	// Buffers backed by the mapped pages
	recvBuf  []byte
	sendBuf  []byte
}

func NewExchangeIPC() (*ExchangeIPC, error) {
	// 1. Open the exchange device
	fd, err := syscall.Open("/dev/exchange", syscall.O_RDWR, 0)
	if err != nil {
		return nil, err
	}

	pageSize := syscall.Getpagesize()

	// 2. Allocate page-aligned buffers via mmap
	// In a real system, we might use a specific allocator or the sip/alloc device 
	// to ensure these are physical pages we own. 
	// For now, anonymous mmap gives us pages the kernel can grab.
	
	recvMem, err := syscall.Mmap(-1, 0, pageSize, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_ANON|syscall.MAP_PRIVATE)
	if err != nil {
		syscall.Close(fd)
		return nil, fmt.Errorf("mmap recv failed: %v", err)
	}

	sendMem, err := syscall.Mmap(-1, 0, pageSize, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_ANON|syscall.MAP_PRIVATE)
	if err != nil {
		syscall.Munmap(recvMem)
		syscall.Close(fd)
		return nil, fmt.Errorf("mmap send failed: %v", err)
	}

	return &ExchangeIPC{
		fd:       fd,
		recvAddr: uintptr(unsafe.Pointer(&recvMem[0])),
		sendAddr: uintptr(unsafe.Pointer(&sendMem[0])),
		recvBuf:  recvMem,
		sendBuf:  sendMem,
	}, nil
}

// ReceiveLoop processes incoming 9P messages
// This should be run in a goroutine
func (ipc *ExchangeIPC) ReceiveLoop(handler func([]byte) []byte) {
	buf := make([]byte, 1024)
	
	for {
		// 1. Poll for pending pages (Read blocks in devirq, but devexchange might act differently)
		// devexchange.c read() returns a list of prepared pages.
		// We probably want a blocking wait or a loop. 
		// If devexchange read is non-blocking/snapshot, we poll.
		
		n, err := syscall.Read(ipc.fd, buf)
		if err != nil {
			// Log error
			continue
		}
		if n == 0 {
			// EOF?
			continue
		}

		// output is text: "PID Handle VAddr\n..."
		lines := strings.Split(string(buf[:n]), "\n")
		for _, line := range lines {
			var pid, handle, vaddr int
			if _, err := fmt.Sscanf(line, "%d %d %x", &pid, &handle, &vaddr); err == nil {
				if handle > 0 {
					ipc.processMessage(handle, handler)
				}
			}
		}
	}
}

func (ipc *ExchangeIPC) processMessage(handle int, handler func([]byte) []byte) {
	// 2. Accept the page: "accept <handle> <local_vaddr> <prot>"
	// This maps the physical page from the client into our recvAddr
	cmd := fmt.Sprintf("accept %d %d %d", handle, ipc.recvAddr, 0600)
	if _, err := syscall.Write(ipc.fd, []byte(cmd)); err != nil {
		return 
	}

	// 3. Process 9P message directly from memory (Zero Copy)
	// The data is now at ipc.recvAddr (ipc.recvBuf)
	// We assume the message length is encoded in 9P or fits in page.
	// Standard 9P starts with size[4].
	
	// Basic safety check (though we trust handler)
	// reqData := ipc.recvBuf[:]
	
	// Handle request and generate response
	respData := handler(ipc.recvBuf)

	// 4. Write response to Send Buffer
	// We copy to our send page. 
	// Optimization: We could modify recvPage in place if protocol allows, 
	// but standard exchange might require a fresh 'prepare' of a page we own.
	// 'recvPage' is now ours. We can reuse it!
	copy(ipc.recvBuf, respData)

	// 5. Give page back to client: "prepare <local_vaddr>"
	// We send back the same page we received (ping-pong)
	replyCmd := fmt.Sprintf("prepare %d", ipc.recvAddr)
	syscall.Write(ipc.fd, []byte(replyCmd))
}