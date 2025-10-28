// Package fbdriver implements a userspace framebuffer driver
// Accesses video memory via /dev/mem and exports display via 9P
package fbdriver

import (
	"fmt"
	"os"
	"sync"
)

// Color represents an RGBA color
type Color struct {
	R, G, B, A uint8
}

var (
	ColorBlack   = Color{0, 0, 0, 255}
	ColorWhite   = Color{255, 255, 255, 255}
	ColorRed     = Color{255, 0, 0, 255}
	ColorGreen   = Color{0, 255, 0, 255}
	ColorBlue    = Color{0, 0, 255, 255}
	ColorYellow  = Color{255, 255, 0, 255}
	ColorCyan    = Color{0, 255, 255, 255}
	ColorMagenta = Color{255, 0, 255, 255}
)

// PixelFormat defines how pixels are stored in memory
type PixelFormat int

const (
	PixelFormatRGB32  PixelFormat = iota // 32-bit RGBA
	PixelFormatBGR32                     // 32-bit BGRA
	PixelFormatRGB24                     // 24-bit RGB
	PixelFormatRGB565                    // 16-bit RGB (5-6-5)
)

// Screen represents a physical display screen
type Screen struct {
	Name   string
	Width  int
	Height int
	Depth  int // Bits per pixel
	Pitch  int // Bytes per scanline

	FBAddr uint64 // Physical framebuffer address
	FBSize uint64 // Framebuffer size in bytes
	Format PixelFormat

	// Memory access
	memFile *os.File // /dev/mem file descriptor
	buffer  []byte   // Local framebuffer copy for double buffering

	// Text rendering state
	CursorX int
	CursorY int
	Font    *Font
	FgColor Color
	BgColor Color

	// Synchronization
	mu sync.RWMutex

	// Dirty tracking for optimization
	dirty     bool
	dirtyX1   int
	dirtyY1   int
	dirtyX2   int
	dirtyY2   int
}

// NewScreen creates a new screen instance
func NewScreen(name string, width, height, depth, pitch int, fbAddr, fbSize uint64) *Screen {
	return &Screen{
		Name:    name,
		Width:   width,
		Height:  height,
		Depth:   depth,
		Pitch:   pitch,
		FBAddr:  fbAddr,
		FBSize:  fbSize,
		Format:  PixelFormatRGB32, // Default
		buffer:  make([]byte, fbSize),
		FgColor: ColorWhite,
		BgColor: ColorBlack,
		Font:    LoadVGAFont(),
	}
}

// Initialize opens /dev/mem and maps the framebuffer
func (s *Screen) Initialize() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	// Open /dev/mem for MMIO access
	memFile, err := os.OpenFile("/dev/mem", os.O_RDWR, 0)
	if err != nil {
		return fmt.Errorf("failed to open /dev/mem: %w", err)
	}
	s.memFile = memFile

	// Read initial framebuffer contents
	if _, err := s.memFile.Seek(int64(s.FBAddr), 0); err != nil {
		return fmt.Errorf("failed to seek to framebuffer: %w", err)
	}

	// In a real implementation, we'd use mmap here
	// For now, we use read/write operations
	n, err := s.memFile.Read(s.buffer)
	if err != nil && n == 0 {
		return fmt.Errorf("failed to read framebuffer: %w", err)
	}

	return nil
}

// Close releases resources
func (s *Screen) Close() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if s.memFile != nil {
		return s.memFile.Close()
	}
	return nil
}

// PutPixel sets a single pixel at (x, y)
func (s *Screen) PutPixel(x, y int, color Color) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if x < 0 || x >= s.Width || y < 0 || y >= s.Height {
		return fmt.Errorf("pixel (%d, %d) out of bounds", x, y)
	}

	offset := y*s.Pitch + x*4 // 4 bytes per pixel for RGB32

	switch s.Format {
	case PixelFormatRGB32:
		s.buffer[offset+0] = color.R
		s.buffer[offset+1] = color.G
		s.buffer[offset+2] = color.B
		s.buffer[offset+3] = color.A

	case PixelFormatBGR32:
		s.buffer[offset+0] = color.B
		s.buffer[offset+1] = color.G
		s.buffer[offset+2] = color.R
		s.buffer[offset+3] = color.A

	case PixelFormatRGB24:
		s.buffer[offset+0] = color.R
		s.buffer[offset+1] = color.G
		s.buffer[offset+2] = color.B

	case PixelFormatRGB565:
		// Pack into 16-bit RGB565
		rgb565 := uint16((color.R>>3)<<11 | (color.G>>2)<<5 | (color.B >> 3))
		s.buffer[offset+0] = uint8(rgb565 & 0xFF)
		s.buffer[offset+1] = uint8(rgb565 >> 8)
	}

	s.markDirty(x, y, x, y)
	return nil
}

// GetPixel reads a pixel at (x, y)
func (s *Screen) GetPixel(x, y int) (Color, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	if x < 0 || x >= s.Width || y < 0 || y >= s.Height {
		return ColorBlack, fmt.Errorf("pixel (%d, %d) out of bounds", x, y)
	}

	offset := y*s.Pitch + x*4

	switch s.Format {
	case PixelFormatRGB32:
		return Color{
			R: s.buffer[offset+0],
			G: s.buffer[offset+1],
			B: s.buffer[offset+2],
			A: s.buffer[offset+3],
		}, nil

	case PixelFormatBGR32:
		return Color{
			R: s.buffer[offset+2],
			G: s.buffer[offset+1],
			B: s.buffer[offset+0],
			A: s.buffer[offset+3],
		}, nil

	case PixelFormatRGB24:
		return Color{
			R: s.buffer[offset+0],
			G: s.buffer[offset+1],
			B: s.buffer[offset+2],
			A: 255,
		}, nil

	case PixelFormatRGB565:
		rgb565 := uint16(s.buffer[offset+0]) | uint16(s.buffer[offset+1])<<8
		return Color{
			R: uint8((rgb565 >> 11) << 3),
			G: uint8(((rgb565 >> 5) & 0x3F) << 2),
			B: uint8((rgb565 & 0x1F) << 3),
			A: 255,
		}, nil
	}

	return ColorBlack, nil
}

// DrawRect fills a rectangle with color
func (s *Screen) DrawRect(x, y, w, h int, color Color) error {
	for dy := 0; dy < h; dy++ {
		for dx := 0; dx < w; dx++ {
			if err := s.PutPixel(x+dx, y+dy, color); err != nil {
				return err
			}
		}
	}
	return nil
}

// DrawLine draws a line using Bresenham's algorithm
func (s *Screen) DrawLine(x1, y1, x2, y2 int, color Color) error {
	dx := abs(x2 - x1)
	dy := abs(y2 - y1)
	sx := 1
	if x1 > x2 {
		sx = -1
	}
	sy := 1
	if y1 > y2 {
		sy = -1
	}

	err := dx - dy
	x, y := x1, y1

	for {
		if err := s.PutPixel(x, y, color); err != nil {
			return err
		}

		if x == x2 && y == y2 {
			break
		}

		e2 := 2 * err
		if e2 > -dy {
			err -= dy
			x += sx
		}
		if e2 < dx {
			err += dx
			y += sy
		}
	}

	return nil
}

// Clear fills the entire screen with a color
func (s *Screen) Clear(color Color) error {
	return s.DrawRect(0, 0, s.Width, s.Height, color)
}

// RenderText renders text at the current cursor position
func (s *Screen) RenderText(text string) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	for _, ch := range text {
		if ch == '\n' {
			s.CursorX = 0
			s.CursorY += s.Font.Height
			// Scroll if needed
			if s.CursorY+s.Font.Height > s.Height {
				s.scroll()
				s.CursorY = s.Height - s.Font.Height
			}
			continue
		}

		if ch == '\r' {
			s.CursorX = 0
			continue
		}

		if ch == '\t' {
			s.CursorX = ((s.CursorX / s.Font.Width) + 1) * s.Font.Width
			if s.CursorX >= s.Width {
				s.CursorX = 0
				s.CursorY += s.Font.Height
			}
			continue
		}

		// Get glyph bitmap
		glyph := s.Font.GetGlyph(ch)

		// Draw glyph
		s.drawGlyph(s.CursorX, s.CursorY, glyph, s.FgColor, s.BgColor)

		// Advance cursor
		s.CursorX += s.Font.Width
		if s.CursorX+s.Font.Width > s.Width {
			s.CursorX = 0
			s.CursorY += s.Font.Height
			// Scroll if needed
			if s.CursorY+s.Font.Height > s.Height {
				s.scroll()
				s.CursorY = s.Height - s.Font.Height
			}
		}
	}

	return nil
}

// drawGlyph draws a single character glyph (called with lock held)
func (s *Screen) drawGlyph(x, y int, bitmap []byte, fg, bg Color) {
	for row := 0; row < s.Font.Height; row++ {
		if y+row >= s.Height {
			break
		}

		bits := bitmap[row]
		for col := 0; col < s.Font.Width; col++ {
			if x+col >= s.Width {
				break
			}

			offset := (y+row)*s.Pitch + (x+col)*4

			var color Color
			if bits&(1<<(7-col)) != 0 {
				color = fg
			} else {
				color = bg
			}

			// Write directly to buffer (lock already held)
			switch s.Format {
			case PixelFormatRGB32:
				s.buffer[offset+0] = color.R
				s.buffer[offset+1] = color.G
				s.buffer[offset+2] = color.B
				s.buffer[offset+3] = color.A
			case PixelFormatBGR32:
				s.buffer[offset+0] = color.B
				s.buffer[offset+1] = color.G
				s.buffer[offset+2] = color.R
				s.buffer[offset+3] = color.A
			}
		}
	}

	s.markDirty(x, y, x+s.Font.Width, y+s.Font.Height)
}

// scroll scrolls the screen up by one line
func (s *Screen) scroll() {
	// Copy all lines up by one font height
	lineBytes := s.Font.Height * s.Pitch
	copy(s.buffer, s.buffer[lineBytes:])

	// Clear the bottom line
	bottomStart := (s.Height - s.Font.Height) * s.Pitch
	for i := bottomStart; i < len(s.buffer); i += 4 {
		s.buffer[i+0] = s.BgColor.R
		s.buffer[i+1] = s.BgColor.G
		s.buffer[i+2] = s.BgColor.B
		s.buffer[i+3] = s.BgColor.A
	}

	s.dirty = true
	s.dirtyX1 = 0
	s.dirtyY1 = 0
	s.dirtyX2 = s.Width - 1
	s.dirtyY2 = s.Height - 1
}

// markDirty marks a region as dirty for refresh
func (s *Screen) markDirty(x1, y1, x2, y2 int) {
	if !s.dirty {
		s.dirty = true
		s.dirtyX1 = x1
		s.dirtyY1 = y1
		s.dirtyX2 = x2
		s.dirtyY2 = y2
	} else {
		if x1 < s.dirtyX1 {
			s.dirtyX1 = x1
		}
		if y1 < s.dirtyY1 {
			s.dirtyY1 = y1
		}
		if x2 > s.dirtyX2 {
			s.dirtyX2 = x2
		}
		if y2 > s.dirtyY2 {
			s.dirtyY2 = y2
		}
	}
}

// Flush writes the buffer to actual framebuffer memory
func (s *Screen) Flush() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if !s.dirty {
		return nil // Nothing to flush
	}

	// Seek to framebuffer start
	if _, err := s.memFile.Seek(int64(s.FBAddr), 0); err != nil {
		return fmt.Errorf("failed to seek to framebuffer: %w", err)
	}

	// Write entire buffer to hardware
	// In optimized version, would only write dirty region
	if _, err := s.memFile.Write(s.buffer); err != nil {
		return fmt.Errorf("failed to write to framebuffer: %w", err)
	}

	// Clear dirty flag
	s.dirty = false

	return nil
}

// FlushRegion flushes only a specific region (optimization)
func (s *Screen) FlushRegion(x, y, w, h int) error {
	s.mu.RLock()
	defer s.mu.RUnlock()

	// Clamp to screen bounds
	if x < 0 {
		x = 0
	}
	if y < 0 {
		y = 0
	}
	if x+w > s.Width {
		w = s.Width - x
	}
	if y+h > s.Height {
		h = s.Height - y
	}

	// Write each scanline in the region
	for row := 0; row < h; row++ {
		offset := (y+row)*s.Pitch + x*4
		length := w * 4

		// Seek to this scanline
		if _, err := s.memFile.Seek(int64(s.FBAddr)+int64(offset), 0); err != nil {
			return err
		}

		// Write this scanline
		if _, err := s.memFile.Write(s.buffer[offset : offset+length]); err != nil {
			return err
		}
	}

	return nil
}

// Info returns screen information as a string
func (s *Screen) Info() string {
	s.mu.RLock()
	defer s.mu.RUnlock()

	format := "RGB32"
	switch s.Format {
	case PixelFormatBGR32:
		format = "BGR32"
	case PixelFormatRGB24:
		format = "RGB24"
	case PixelFormatRGB565:
		format = "RGB565"
	}

	return fmt.Sprintf("width %d\nheight %d\ndepth %d\npitch %d\nformat %s\nframebuffer 0x%x\n",
		s.Width, s.Height, s.Depth, s.Pitch, format, s.FBAddr)
}

// Helper functions

func abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}
