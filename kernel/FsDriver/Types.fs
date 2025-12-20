namespace FsDriver

/// Device state
type DeviceState =
    | Uninitialized
    | Ready
    | Busy
    | Error of string

/// Memory region descriptor
type MemoryRegion = {
    BaseAddress: uint64
    Size: uint64
    Permissions: int
}

/// I/O request
type IORequest = {
    Operation: string
    Offset: uint64
    Buffer: byte array
    Length: int
}

/// Result type for operations
type Result<'T> =
    | Success of 'T
    | Failure of string
