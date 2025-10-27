// 9P server for framebuffer access
package fbdriver

import (
	"context"
	"fmt"
	"io"
	"strconv"
	"strings"
	"sync"
)

// Framebuffer9PServer exports framebuffers via 9P
type Framebuffer9PServer struct {
	screens map[string]*Screen
	mu      sync.RWMutex
}

// NewFramebuffer9PServer creates a new 9P framebuffer server
func NewFramebuffer9PServer() *Framebuffer9PServer {
	return &Framebuffer9PServer{
		screens: make(map[string]*Screen),
	}
}

// AddScreen registers a screen for 9P export
func (fs *Framebuffer9PServer) AddScreen(screen *Screen) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	if _, exists := fs.screens[screen.Name]; exists {
		return fmt.Errorf("screen %s already exists", screen.Name)
	}

	fs.screens[screen.Name] = screen
	return nil
}

// RemoveScreen unregisters a screen
func (fs *Framebuffer9PServer) RemoveScreen(name string) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	screen, exists := fs.screens[name]
	if !exists {
		return fmt.Errorf("screen %s not found", name)
	}

	screen.Close()
	delete(fs.screens, name)
	return nil
}

// Read implements the 9P Tread operation
func (fs *Framebuffer9PServer) Read(path string, offset uint64, count uint32) ([]byte, error) {
	screen, file := fs.parsePath(path)
	if screen == nil {
		return nil, fmt.Errorf("screen not found in path: %s", path)
	}

	switch file {
	case "info":
		return []byte(screen.Info()), nil

	case "cursor":
		screen.mu.RLock()
		defer screen.mu.RUnlock()
		return []byte(fmt.Sprintf("%d %d\n", screen.CursorX, screen.CursorY)), nil

	case "font":
		screen.mu.RLock()
		defer screen.mu.RUnlock()
		return []byte(fmt.Sprintf("%dx%d\n", screen.Font.Width, screen.Font.Height)), nil

	case "data":
		// Read raw framebuffer data
		screen.mu.RLock()
		defer screen.mu.RUnlock()

		if offset >= uint64(len(screen.buffer)) {
			return nil, io.EOF
		}

		end := offset + uint64(count)
		if end > uint64(len(screen.buffer)) {
			end = uint64(len(screen.buffer))
		}

		return screen.buffer[offset:end], nil

	case "ctl":
		// Return current status
		return []byte("ready\n"), nil

	default:
		return nil, fmt.Errorf("cannot read from %s", file)
	}
}

// Write implements the 9P Twrite operation
func (fs *Framebuffer9PServer) Write(path string, offset uint64, data []byte) (uint32, error) {
	screen, file := fs.parsePath(path)
	if screen == nil {
		return 0, fmt.Errorf("screen not found in path: %s", path)
	}

	switch file {
	case "text":
		// Render text at current cursor position
		text := string(data)
		if err := screen.RenderText(text); err != nil {
			return 0, err
		}
		// Auto-flush after text write
		screen.Flush()
		return uint32(len(data)), nil

	case "data":
		// Write raw pixel data
		// Two modes: text format "x y r g b" or binary
		text := string(data)
		if strings.Contains(text, " ") {
			// Text format: "x y r g b"
			var x, y, r, g, b int
			n, err := fmt.Sscanf(text, "%d %d %d %d %d", &x, &y, &r, &g, &b)
			if err != nil || n != 5 {
				return 0, fmt.Errorf("invalid pixel format, expected: x y r g b")
			}
			color := Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
			if err := screen.PutPixel(x, y, color); err != nil {
				return 0, err
			}
			screen.Flush()
			return uint32(len(data)), nil
		} else {
			// Binary mode: write directly to framebuffer buffer
			return fs.writePixelsBinary(screen, offset, data)
		}

	case "rect":
		// Draw rectangle: "x y w h r g b"
		var x, y, w, h, r, g, b int
		n, err := fmt.Sscanf(string(data), "%d %d %d %d %d %d %d", &x, &y, &w, &h, &r, &g, &b)
		if err != nil || n != 7 {
			return 0, fmt.Errorf("invalid rect format, expected: x y w h r g b")
		}
		color := Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
		if err := screen.DrawRect(x, y, w, h, color); err != nil {
			return 0, err
		}
		screen.Flush()
		return uint32(len(data)), nil

	case "line":
		// Draw line: "x1 y1 x2 y2 r g b"
		var x1, y1, x2, y2, r, g, b int
		n, err := fmt.Sscanf(string(data), "%d %d %d %d %d %d %d", &x1, &y1, &x2, &y2, &r, &g, &b)
		if err != nil || n != 7 {
			return 0, fmt.Errorf("invalid line format, expected: x1 y1 x2 y2 r g b")
		}
		color := Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
		if err := screen.DrawLine(x1, y1, x2, y2, color); err != nil {
			return 0, err
		}
		screen.Flush()
		return uint32(len(data)), nil

	case "cursor":
		// Set cursor position: "x y"
		var x, y int
		n, err := fmt.Sscanf(string(data), "%d %d", &x, &y)
		if err != nil || n != 2 {
			return 0, fmt.Errorf("invalid cursor format, expected: x y")
		}
		screen.mu.Lock()
		screen.CursorX = x
		screen.CursorY = y
		screen.mu.Unlock()
		return uint32(len(data)), nil

	case "ctl":
		// Control commands
		return fs.handleControl(screen, string(data))

	case "refresh":
		// Force flush to hardware
		if err := screen.Flush(); err != nil {
			return 0, err
		}
		return uint32(len(data)), nil

	default:
		return 0, fmt.Errorf("cannot write to %s", file)
	}
}

// writePixelsBinary writes binary pixel data
func (fs *Framebuffer9PServer) writePixelsBinary(screen *Screen, offset uint64, data []byte) (uint32, error) {
	screen.mu.Lock()
	defer screen.mu.Unlock()

	if offset >= uint64(len(screen.buffer)) {
		return 0, fmt.Errorf("offset beyond framebuffer")
	}

	// Copy to buffer
	n := copy(screen.buffer[offset:], data)

	// Mark as dirty
	// Calculate approximate dirty rectangle from offset
	pixelsPerRow := screen.Pitch / 4
	startY := int(offset) / screen.Pitch
	endY := int(offset+uint64(n)) / screen.Pitch

	screen.markDirty(0, startY, screen.Width-1, endY)

	return uint32(n), nil
}

// handleControl processes control commands
func (fs *Framebuffer9PServer) handleControl(screen *Screen, cmd string) (uint32, error) {
	cmd = strings.TrimSpace(cmd)

	switch {
	case cmd == "clear":
		// Clear screen to background color
		if err := screen.Clear(screen.BgColor); err != nil {
			return 0, err
		}
		screen.Flush()
		return uint32(len(cmd)), nil

	case cmd == "flush":
		// Flush to hardware
		return uint32(len(cmd)), screen.Flush()

	case strings.HasPrefix(cmd, "fgcolor "):
		// Set foreground color: "fgcolor r g b"
		var r, g, b int
		n, err := fmt.Sscanf(cmd, "fgcolor %d %d %d", &r, &g, &b)
		if err != nil || n != 3 {
			return 0, fmt.Errorf("invalid fgcolor format, expected: fgcolor r g b")
		}
		screen.mu.Lock()
		screen.FgColor = Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
		screen.mu.Unlock()
		return uint32(len(cmd)), nil

	case strings.HasPrefix(cmd, "bgcolor "):
		// Set background color: "bgcolor r g b"
		var r, g, b int
		n, err := fmt.Sscanf(cmd, "bgcolor %d %d %d", &r, &g, &b)
		if err != nil || n != 3 {
			return 0, fmt.Errorf("invalid bgcolor format, expected: bgcolor r g b")
		}
		screen.mu.Lock()
		screen.BgColor = Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
		screen.mu.Unlock()
		return uint32(len(cmd)), nil

	case strings.HasPrefix(cmd, "mode "):
		// Mode change: "mode 1920x1080x32"
		// TODO: Implement mode switching if hardware supports it
		return 0, fmt.Errorf("mode switching not yet implemented")

	default:
		return 0, fmt.Errorf("unknown control command: %s", cmd)
	}
}

// parsePath splits path into screen name and file
// Example: "/dev/draw/screen0/text" -> ("screen0", "text")
func (fs *Framebuffer9PServer) parsePath(path string) (*Screen, string) {
	parts := strings.Split(strings.Trim(path, "/"), "/")

	// Expected format: dev/draw/screenN/file
	if len(parts) < 4 {
		return nil, ""
	}

	screenName := parts[2] // e.g., "screen0"
	fileName := parts[3]   // e.g., "text"

	fs.mu.RLock()
	screen, exists := fs.screens[screenName]
	fs.mu.RUnlock()

	if !exists {
		return nil, ""
	}

	return screen, fileName
}

// Mount starts serving the 9P filesystem at the given mount point
func (fs *Framebuffer9PServer) Mount(ctx context.Context, mountPoint string) error {
	// TODO: Integrate with actual 9P server library
	// For now, just log
	fmt.Printf("Framebuffer9P: Would mount at %s\n", mountPoint)
	return nil
}

// ProbeFramebuffer reads boot info to find framebuffer details
func ProbeFramebuffer() (*Screen, error) {
	// Read framebuffer info from /dev/bootinfo
	// This would be provided by the kernel from UEFI GOP or multiboot2

	// For now, use example values
	// In real implementation: data, err := os.ReadFile("/dev/bootinfo")

	// Parse format: "fb_addr=0xE0000000 fb_width=1024 fb_height=768 fb_pitch=4096 fb_depth=32"

	// Example hardcoded values (would come from bootinfo)
	fbAddr := uint64(0xE0000000)
	fbWidth := 1024
	fbHeight := 768
	fbPitch := 4096
	fbDepth := 32
	fbSize := uint64(fbHeight * fbPitch)

	screen := NewScreen("screen0", fbWidth, fbHeight, fbDepth, fbPitch, fbAddr, fbSize)
	return screen, nil
}

// ParseBootInfo parses kernel boot info for framebuffer parameters
func ParseBootInfo(bootInfo string) (addr uint64, width, height, pitch, depth int, err error) {
	// Parse format: "fb_addr=0xE0000000 fb_width=1024 fb_height=768 fb_pitch=4096 fb_depth=32"

	for _, param := range strings.Fields(bootInfo) {
		kv := strings.Split(param, "=")
		if len(kv) != 2 {
			continue
		}

		key := kv[0]
		value := kv[1]

		switch key {
		case "fb_addr":
			fmt.Sscanf(value, "0x%x", &addr)
		case "fb_width":
			width, _ = strconv.Atoi(value)
		case "fb_height":
			height, _ = strconv.Atoi(value)
		case "fb_pitch":
			pitch, _ = strconv.Atoi(value)
		case "fb_depth":
			depth, _ = strconv.Atoi(value)
		}
	}

	if addr == 0 || width == 0 || height == 0 {
		return 0, 0, 0, 0, 0, fmt.Errorf("invalid boot info")
	}

	return
}
