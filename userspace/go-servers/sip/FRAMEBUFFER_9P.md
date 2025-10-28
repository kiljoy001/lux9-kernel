# Framebuffer 9P Interface Design

## Philosophy

The framebuffer driver runs in **userspace**, accessing video memory via `/dev/mem` and exporting a 9P filesystem for display operations. Applications write pixels/text by writing to 9P files.

## Architecture

```
┌─────────────────────────────────────┐
│  Applications                       │
│  echo "Hello" > /dev/draw/text      │
└─────────────┬───────────────────────┘
              │ 9P write
┌─────────────▼───────────────────────┐
│  Framebuffer Driver (userspace)    │
│  - Exports /dev/draw via 9P         │
│  - Renders text/pixels              │
│  - Manages fonts                    │
└─────────────┬───────────────────────┘
              │ /dev/mem (MMIO)
┌─────────────▼───────────────────────┐
│  Kernel (microkernel)               │
│  - devmem: maps framebuffer memory  │
└─────────────┬───────────────────────┘
              │ Hardware access
┌─────────────▼───────────────────────┐
│  Graphics Card / UEFI Framebuffer   │
└─────────────────────────────────────┘
```

## Filesystem Layout

```
/dev/draw/                  # Display devices
├── ctl                     # Global control
├── screen0/                # Primary display
│   ├── ctl                # Screen control
│   ├── data               # Raw framebuffer access (pixel data)
│   ├── refresh            # Trigger screen update
│   ├── text               # Write text (driver renders)
│   ├── rect               # Draw rectangles
│   ├── line               # Draw lines
│   ├── cursor             # Cursor position
│   ├── font               # Current font
│   └── info               # Screen info (resolution, depth, etc.)
└── screen1/                # Secondary display (if exists)
    └── ...
```

## Usage Examples

### Write Text to Screen

```bash
# Simple text output (driver handles rendering)
echo "Hello World" > /dev/draw/screen0/text

# Position cursor then write
echo "10 20" > /dev/draw/screen0/cursor  # x=10, y=20
echo "At position" > /dev/draw/screen0/text
```

### Draw Pixels

```bash
# Write raw pixel data
# Format: "x y r g b"
echo "100 100 255 0 0" > /dev/draw/screen0/data  # Red pixel at (100,100)

# Or write binary pixel data
dd if=image.raw of=/dev/draw/screen0/data bs=4 seek=1024
```

### Draw Shapes

```bash
# Draw rectangle: "x y width height r g b"
echo "50 50 200 100 0 255 0" > /dev/draw/screen0/rect

# Draw line: "x1 y1 x2 y2 r g b"
echo "0 0 800 600 255 255 255" > /dev/draw/screen0/line
```

### Get Screen Info

```bash
cat /dev/draw/screen0/info
# Output:
# width 1024
# height 768
# depth 32
# pitch 4096
# format RGB32
# framebuffer 0xE0000000
```

### Control Operations

```bash
# Clear screen
echo "clear" > /dev/draw/screen0/ctl

# Set mode (if supported)
echo "mode 1920x1080x32" > /dev/draw/screen0/ctl

# Refresh/flush
echo "1" > /dev/draw/screen0/refresh
```

## Implementation in Go

### Framebuffer Driver Structure

```go
type FramebufferDriver struct {
    *sip.BaseServer
    screens []*Screen
    fs      *Framebuffer9PServer
}

type Screen struct {
    name       string
    width      int
    height     int
    depth      int           // bits per pixel
    pitch      int           // bytes per scanline
    fbAddr     uint64        // physical framebuffer address
    fbSize     uint64        // framebuffer size in bytes
    fbMem      []byte        // mapped memory (via /dev/mem)

    // For text rendering
    cursorX    int
    cursorY    int
    font       *Font
    fgColor    Color
    bgColor    Color
}
```

### Accessing Framebuffer via /dev/mem

```go
func (s *Screen) Initialize(fbPhysAddr uint64, fbSize uint64) error {
    // Open /dev/mem
    memfd, err := os.OpenFile("/dev/mem", os.O_RDWR, 0)
    if err != nil {
        return err
    }

    // Seek to framebuffer physical address
    memfd.Seek(int64(fbPhysAddr), 0)

    // Read entire framebuffer into mapped memory
    s.fbMem = make([]byte, fbSize)

    // In real implementation, would use mmap() via syscall
    // For now, read/write operations on memfd

    return nil
}
```

### Drawing Operations

```go
// PutPixel writes a single pixel
func (s *Screen) PutPixel(x, y int, color Color) error {
    if x < 0 || x >= s.width || y < 0 || y >= s.height {
        return fmt.Errorf("pixel out of bounds")
    }

    offset := y*s.pitch + x*4  // 4 bytes per pixel (RGBA)

    // Write to framebuffer memory
    s.fbMem[offset+0] = color.B
    s.fbMem[offset+1] = color.G
    s.fbMem[offset+2] = color.R
    s.fbMem[offset+3] = color.A

    // Flush to hardware via /dev/mem
    return s.flush(offset, 4)
}

// DrawRect fills a rectangle
func (s *Screen) DrawRect(x, y, w, h int, color Color) error {
    for dy := 0; dy < h; dy++ {
        for dx := 0; dx < w; dx++ {
            s.PutPixel(x+dx, y+dy, color)
        }
    }
    return nil
}

// RenderText renders text at cursor position
func (s *Screen) RenderText(text string) error {
    for _, ch := range text {
        if ch == '\n' {
            s.cursorX = 0
            s.cursorY += s.font.height
            continue
        }

        // Get glyph bitmap
        glyph := s.font.GetGlyph(ch)

        // Draw glyph
        s.drawGlyph(s.cursorX, s.cursorY, glyph, s.fgColor)

        // Advance cursor
        s.cursorX += s.font.width
        if s.cursorX >= s.width {
            s.cursorX = 0
            s.cursorY += s.font.height
        }
    }
    return nil
}
```

### 9P Server Implementation

```go
type Framebuffer9PServer struct {
    screens map[string]*Screen
}

func (fs *Framebuffer9PServer) Write(path string, offset uint64, data []byte) (uint32, error) {
    screen, file := fs.parsePath(path)

    switch file {
    case "text":
        // Render text
        text := string(data)
        return uint32(len(data)), screen.RenderText(text)

    case "data":
        // Raw pixel write
        // Parse: "x y r g b" or binary data
        if isBinary(data) {
            return fs.writePixelsBinary(screen, offset, data)
        } else {
            return fs.writePixelText(screen, data)
        }

    case "rect":
        // Parse: "x y w h r g b"
        var x, y, w, h, r, g, b int
        fmt.Sscanf(string(data), "%d %d %d %d %d %d %d", &x, &y, &w, &h, &r, &g, &b)
        color := Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
        return uint32(len(data)), screen.DrawRect(x, y, w, h, color)

    case "line":
        // Parse: "x1 y1 x2 y2 r g b"
        var x1, y1, x2, y2, r, g, b int
        fmt.Sscanf(string(data), "%d %d %d %d %d %d %d", &x1, &y1, &x2, &y2, &r, &g, &b)
        color := Color{R: uint8(r), G: uint8(g), B: uint8(b), A: 255}
        return uint32(len(data)), screen.DrawLine(x1, y1, x2, y2, color)

    case "cursor":
        // Parse: "x y"
        fmt.Sscanf(string(data), "%d %d", &screen.cursorX, &screen.cursorY)
        return uint32(len(data)), nil

    case "ctl":
        return fs.handleControl(screen, string(data))

    case "refresh":
        // Flush entire framebuffer to hardware
        return uint32(len(data)), screen.flush(0, len(screen.fbMem))
    }

    return 0, fmt.Errorf("unknown file: %s", file)
}

func (fs *Framebuffer9PServer) Read(path string, offset uint64, count uint32) ([]byte, error) {
    screen, file := fs.parsePath(path)

    switch file {
    case "info":
        info := fmt.Sprintf("width %d\nheight %d\ndepth %d\npitch %d\nformat RGB32\nframebuffer 0x%x\n",
            screen.width, screen.height, screen.depth, screen.pitch, screen.fbAddr)
        return []byte(info), nil

    case "data":
        // Read raw framebuffer
        if offset >= uint64(len(screen.fbMem)) {
            return nil, io.EOF
        }
        end := offset + uint64(count)
        if end > uint64(len(screen.fbMem)) {
            end = uint64(len(screen.fbMem))
        }
        return screen.fbMem[offset:end], nil

    case "cursor":
        return []byte(fmt.Sprintf("%d %d\n", screen.cursorX, screen.cursorY)), nil
    }

    return nil, fmt.Errorf("cannot read from %s", file)
}
```

## Detecting Framebuffer

The driver needs to discover the framebuffer from UEFI/BIOS:

```go
func (d *FramebufferDriver) Probe(ctx context.Context) ([]string, error) {
    // Read UEFI GOP (Graphics Output Protocol) info
    // This could be passed from bootloader via multiboot2
    // Or read from /dev/bootinfo (kernel exposes boot parameters)

    bootInfo, err := os.ReadFile("/dev/bootinfo")
    if err != nil {
        return nil, err
    }

    // Parse framebuffer info
    // Format: "fb_addr=0xE0000000 fb_width=1024 fb_height=768 fb_pitch=4096 fb_depth=32"
    var fbAddr uint64
    var fbWidth, fbHeight, fbPitch, fbDepth int

    fmt.Sscanf(string(bootInfo),
        "fb_addr=%x fb_width=%d fb_height=%d fb_pitch=%d fb_depth=%d",
        &fbAddr, &fbWidth, &fbHeight, &fbPitch, &fbDepth)

    // Create screen
    screen := &Screen{
        name:   "screen0",
        width:  fbWidth,
        height: fbHeight,
        depth:  fbDepth,
        pitch:  fbPitch,
        fbAddr: fbAddr,
        fbSize: uint64(fbHeight * fbPitch),
    }

    // Initialize (map framebuffer memory)
    if err := screen.Initialize(fbAddr, screen.fbSize); err != nil {
        return nil, err
    }

    d.screens = append(d.screens, screen)
    return []string{"screen0"}, nil
}
```

## Font Rendering

Simple bitmap font:

```go
type Font struct {
    width  int
    height int
    glyphs map[rune][]byte  // Bitmap data for each character
}

// Load built-in 8x16 VGA font
func LoadVGAFont() *Font {
    font := &Font{
        width:  8,
        height: 16,
        glyphs: make(map[rune][]byte),
    }

    // Embed font data
    // Each glyph is 16 bytes (8 pixels wide, 16 rows)
    // ...font data...

    return font
}

func (f *Font) GetGlyph(ch rune) []byte {
    if glyph, ok := f.glyphs[ch]; ok {
        return glyph
    }
    return f.glyphs['?']  // Default glyph
}

func (s *Screen) drawGlyph(x, y int, bitmap []byte, color Color) {
    for row := 0; row < s.font.height; row++ {
        bits := bitmap[row]
        for col := 0; col < s.font.width; col++ {
            if bits&(1<<(7-col)) != 0 {
                s.PutPixel(x+col, y+row, color)
            } else {
                s.PutPixel(x+col, y+row, s.bgColor)
            }
        }
    }
}
```

## Benefits of Userspace Framebuffer

1. **Crash Isolation**: Graphics driver crash doesn't kill kernel
2. **Hot Reload**: Update driver without reboot
3. **Multiple Implementations**: Software renderer, GPU acceleration
4. **Network Transparent**: VNC-like remote display via 9P
5. **Composition**: Multiple programs can draw (with proper locking)
6. **Testing**: Easy to test rendering without hardware

## Example: Simple Terminal

```go
// A simple terminal that uses the framebuffer
func main() {
    // Open framebuffer
    textfd, _ := os.OpenFile("/dev/draw/screen0/text", os.O_WRONLY, 0)

    // Clear screen
    ctlfd, _ := os.OpenFile("/dev/draw/screen0/ctl", os.O_WRONLY, 0)
    ctlfd.Write([]byte("clear"))

    // Write some text
    textfd.Write([]byte("Lux9 Operating System\n"))
    textfd.Write([]byte("Framebuffer Console v1.0\n"))
    textfd.Write([]byte("\n"))
    textfd.Write([]byte("# "))

    // Read input, echo to screen
    scanner := bufio.NewScanner(os.Stdin)
    for scanner.Scan() {
        line := scanner.Text()
        textfd.Write([]byte(line + "\n# "))
    }
}
```

## Kernel Requirements

The kernel only needs to provide:
- `/dev/mem` access with capability checking
- `/dev/bootinfo` to pass UEFI framebuffer info
- Page mapping for framebuffer physical memory

Everything else (rendering, fonts, drawing) is in userspace!

This is the Plan 9 way: minimal kernel, powerful userspace services.
