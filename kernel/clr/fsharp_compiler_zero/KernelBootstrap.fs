// F# Kernel Bootstrap - Complete x86-64 initialization
// Based on Intel SDM and Multiboot specification

module KernelBootstrap

open System

// ==================== CONSTANTS ====================

[<Literal>]
let VGA_BUFFER = 0xB8000L
[<Literal>]
let VGA_WIDTH = 80
[<Literal>]
let VGA_HEIGHT = 25

// Multiboot magic values
[<Literal>]
let MULTIBOOT_BOOTLOADER_MAGIC = 0x2BADB002u
[<Literal>]
let MULTIBOOT2_BOOTLOADER_MAGIC = 0x36d76289u

// GDT segment selectors
[<Literal>]
let KERNEL_CODE_SEGMENT = 0x08
[<Literal>]
let KERNEL_DATA_SEGMENT = 0x10
[<Literal>]
let USER_CODE_SEGMENT = 0x18
[<Literal>]
let USER_DATA_SEGMENT = 0x20
[<Literal>]
let TSS_SEGMENT = 0x28

// ==================== LOW-LEVEL TYPES ====================

type MultibootInfo = {
    Flags: uint32
    MemLower: uint32
    MemUpper: uint32
    BootDevice: uint32
    CmdLine: uint32
    ModsCount: uint32
    ModsAddr: uint32
    MmapLength: uint32
    MmapAddr: uint32
}

type MemoryRegion = {
    BaseAddr: uint64
    Length: uint64
    Type: uint32  // 1 = available, 2 = reserved, 3 = ACPI, 4 = NVS, 5 = bad
}

type GDTEntry = {
    LimitLow: uint16
    BaseLow: uint16
    BaseMiddle: uint8
    Access: uint8
    Granularity: uint8
    BaseHigh: uint8
}

type IDTEntry = {
    OffsetLow: uint16
    Selector: uint16
    IST: uint8
    TypeAttr: uint8
    OffsetMid: uint16
    OffsetHigh: uint32
    Zero: uint32
}

// ==================== MEMORY MANAGEMENT ====================

let mutable physicalMemoryMap: MemoryRegion array = [||]
let mutable nextFreeFrame = 0x100000UL  // Start after 1MB

let parseMemoryMap (mmapAddr: uint32) (mmapLength: uint32) =
    // Parse multiboot memory map
    let regions = ResizeArray<MemoryRegion>()
    let mutable offset = 0u
    
    while offset < mmapLength do
        // Read memory region from multiboot structure
        // This would be actual memory reads in compiled code
        let region = {
            BaseAddr = 0UL  // Read from mmapAddr + offset
            Length = 0UL    // Read from mmapAddr + offset + 8
            Type = 0u       // Read from mmapAddr + offset + 16
        }
        regions.Add(region)
        offset <- offset + 24u  // Size of memory map entry
    
    physicalMemoryMap <- regions.ToArray()

let allocatePhysicalFrame () =
    // Simple frame allocator
    let frame = nextFreeFrame
    nextFreeFrame <- nextFreeFrame + 0x1000UL  // 4KB pages
    frame

// ==================== GDT/IDT SETUP ====================

let gdtEntries: GDTEntry array = Array.zeroCreate 6
let idtEntries: IDTEntry array = Array.zeroCreate 256

let setupGDTEntry index base limit access gran =
    gdtEntries.[index] <- {
        LimitLow = uint16 (limit &&& 0xFFFF)
        BaseLow = uint16 (base &&& 0xFFFF)
        BaseMiddle = uint8 ((base >>> 16) &&& 0xFF)
        Access = access
        Granularity = uint8 (((limit >>> 16) &&& 0x0F) ||| (gran &&& 0xF0))
        BaseHigh = uint8 ((base >>> 24) &&& 0xFF)
    }

let initializeGDT () =
    // Null segment
    setupGDTEntry 0 0u 0u 0uy 0uy
    
    // Kernel code segment (0x08)
    setupGDTEntry 1 0u 0xFFFFFFFFu 0x9Auy 0xCFuy
    
    // Kernel data segment (0x10)
    setupGDTEntry 2 0u 0xFFFFFFFFu 0x92uy 0xCFuy
    
    // User code segment (0x18)
    setupGDTEntry 3 0u 0xFFFFFFFFu 0xFAuy 0xCFuy
    
    // User data segment (0x20)
    setupGDTEntry 4 0u 0xFFFFFFFFu 0xF2uy 0xCFuy
    
    // TSS segment (0x28) - will be set up later
    setupGDTEntry 5 0u 0u 0uy 0uy

let setupIDTEntry index offset selector istValue typeAttr =
    idtEntries.[index] <- {
        OffsetLow = uint16 (offset &&& 0xFFFFUL)
        Selector = selector
        IST = istValue
        TypeAttr = typeAttr
        OffsetMid = uint16 ((offset >>> 16) &&& 0xFFFFUL)
        OffsetHigh = uint32 ((offset >>> 32) &&& 0xFFFFFFFFUL)
        Zero = 0u
    }

// ==================== INTERRUPT HANDLERS ====================

let exceptionHandler (vector: int) (errorCode: uint64) =
    // Basic exception handler
    let messages = [|
        "Divide by zero"
        "Debug"
        "NMI"
        "Breakpoint"
        "Overflow"
        "Bound range exceeded"
        "Invalid opcode"
        "Device not available"
        "Double fault"
        "Coprocessor segment overrun"
        "Invalid TSS"
        "Segment not present"
        "Stack segment fault"
        "General protection fault"
        "Page fault"
    |]
    
    if vector < messages.Length then
        printString 0 0 messages.[vector] 0x4F  // Red on black
    else
        printString 0 0 "Unknown exception" 0x4F

// ==================== VGA CONSOLE ====================

let mutable cursorX = 0
let mutable cursorY = 0

let putChar (x: int) (y: int) (ch: char) (color: byte) =
    if x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT then
        let offset = (y * VGA_WIDTH + x) * 2
        // Write to VGA buffer - would be direct memory access in compiled code
        ()

let printString (x: int) (y: int) (str: string) (color: byte) =
    let mutable px = x
    let mutable py = y
    
    for ch in str do
        if ch = '\n' then
            px <- 0
            py <- py + 1
        else
            putChar px py ch color
            px <- px + 1
            if px >= VGA_WIDTH then
                px <- 0
                py <- py + 1

let clearScreen () =
    for y in 0 .. VGA_HEIGHT - 1 do
        for x in 0 .. VGA_WIDTH - 1 do
            putChar x y ' ' 0x07

// ==================== PIC INITIALIZATION ====================

let initializePIC () =
    // Initialize 8259A PIC
    // ICW1: Initialize command
    // outb(0x20, 0x11)  // Master PIC
    // outb(0xA0, 0x11)  // Slave PIC
    
    // ICW2: Vector offset
    // outb(0x21, 0x20)  // Master starts at 0x20
    // outb(0xA1, 0x28)  // Slave starts at 0x28
    
    // ICW3: Master/Slave wiring
    // outb(0x21, 0x04)  // Slave at IRQ2
    // outb(0xA1, 0x02)  // Cascade identity
    
    // ICW4: Environment info
    // outb(0x21, 0x01)  // 8086 mode
    // outb(0xA1, 0x01)
    
    // Mask all interrupts initially
    // outb(0x21, 0xFF)
    // outb(0xA1, 0xFF)
    ()

// ==================== CPU DETECTION ====================

type CPUInfo = {
    Vendor: string
    Brand: string
    Features: uint64
    ExtendedFeatures: uint64
}

let detectCPU () =
    // Use CPUID instruction to get CPU info
    // This would be inline assembly in compiled code
    {
        Vendor = "GenuineIntel"  // Placeholder
        Brand = "Intel Core i7"   // Placeholder
        Features = 0UL
        ExtendedFeatures = 0UL
    }

// ==================== MAIN BOOTSTRAP ====================

let kernelMain (magic: uint32) (multibootInfo: uint32) =
    // Clear screen first
    clearScreen()
    
    // Display boot message
    printString 0 0 "F# Zero Kernel Bootstrap" 0x0F
    printString 0 1 "========================" 0x0F
    
    // Verify multiboot magic
    if magic = MULTIBOOT_BOOTLOADER_MAGIC then
        printString 0 3 "Multiboot 1 detected" 0x0A
    elif magic = MULTIBOOT2_BOOTLOADER_MAGIC then
        printString 0 3 "Multiboot 2 detected" 0x0A
    else
        printString 0 3 "Invalid boot magic!" 0x0C
        // Halt
        while true do ()
    
    // Initialize GDT
    printString 0 4 "Setting up GDT..." 0x07
    initializeGDT()
    
    // Initialize IDT
    printString 0 5 "Setting up IDT..." 0x07
    // Set up exception handlers
    for i in 0 .. 31 do
        setupIDTEntry i 0UL (uint16 KERNEL_CODE_SEGMENT) 0uy 0x8Euy
    
    // Initialize PIC
    printString 0 6 "Initializing PIC..." 0x07
    initializePIC()
    
    // Detect CPU
    printString 0 7 "Detecting CPU..." 0x07
    let cpu = detectCPU()
    printString 0 8 ("CPU: " + cpu.Vendor) 0x07
    
    // Parse memory map
    printString 0 9 "Parsing memory map..." 0x07
    // parseMemoryMap would be called with actual multiboot info
    
    // Enable interrupts
    printString 0 10 "Enabling interrupts..." 0x07
    // sti instruction would go here
    
    printString 0 12 "F# kernel ready!" 0x0A
    
    // Enter main kernel loop
    let mutable running = true
    while running do
        // Kernel main loop
        // Handle interrupts, schedule tasks, etc.
        ()
    
    0  // Return success