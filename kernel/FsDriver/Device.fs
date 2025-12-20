namespace FsDriver

module Device =
    open Utils

    /// Device descriptor
    type DeviceDescriptor = {
        mutable State: DeviceState
        Name: string
        BaseAddr: uint64
        IrqNumber: int
        Regions: MemoryRegion list
    }

    /// Create new device
    let create (name: string) (baseAddr: uint64) (irq: int) : DeviceDescriptor =
        {
            State = Uninitialized
            Name = name
            BaseAddr = baseAddr
            IrqNumber = irq
            Regions = []
        }

    /// Initialize device
    let initialize (device: DeviceDescriptor) : Result<unit> =
        if device.State <> Uninitialized then
            Failure "Device already initialized"
        else
            device.State <- Ready
            Success ()

    /// Add memory region to device
    let addRegion (device: DeviceDescriptor) (region: MemoryRegion) : Result<unit> =
        if not (isAligned region.BaseAddress 4096UL) then
            Failure "Region not page-aligned"
        else
            let newRegions = region :: device.Regions
            let device' = { device with Regions = newRegions }
            Success ()

    /// Process I/O request
    let processIO (device: DeviceDescriptor) (request: IORequest) : Result<int> =
        match device.State with
        | Ready ->
            device.State <- Busy
            let bytesProcessed = request.Length
            device.State <- Ready
            Success bytesProcessed
        | Busy ->
            Failure "Device busy"
        | Error msg ->
            Failure ("Device error: " + msg)
        | Uninitialized ->
            Failure "Device not initialized"

    /// Handle interrupt
    let handleInterrupt (device: DeviceDescriptor) (irq: int) : Result<unit> =
        if irq <> device.IrqNumber then
            Failure "Wrong IRQ"
        else
            match device.State with
            | Busy ->
                device.State <- Ready
                Success ()
            | _ ->
                Failure "Spurious interrupt"

    /// Reset device
    let reset (device: DeviceDescriptor) : Result<unit> =
        device.State <- Uninitialized
        Success ()
