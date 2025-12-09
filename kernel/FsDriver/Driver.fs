namespace FsDriver

module Driver =
    open Device
    open Utils

    /// Driver state
    type DriverState = {
        mutable Devices: DeviceDescriptor list
        MaxDevices: int
        mutable RequestCount: uint64
    }

    /// Create driver state
    let createState (maxDevices: int) : DriverState =
        {
            Devices = []
            MaxDevices = maxDevices
            RequestCount = 0UL
        }

    /// Register device with driver
    let registerDevice (state: DriverState) (device: DeviceDescriptor) : Result<unit> =
        if List.length state.Devices >= state.MaxDevices then
            Failure "Too many devices"
        else
            state.Devices <- device :: state.Devices
            Success ()

    /// Find device by name
    let findDevice (state: DriverState) (name: string) : Result<DeviceDescriptor> =
        match List.tryFind (fun d -> d.Name = name) state.Devices with
        | Some device -> Success device
        | None -> Failure ("Device not found: " + name)

    /// Process request on named device
    let processRequest (state: DriverState) (deviceName: string) (request: IORequest) : Result<int> =
        state.RequestCount <- state.RequestCount + 1UL
        match findDevice state deviceName with
        | Success device ->
            processIO device request
        | Failure msg ->
            Failure msg

    /// Get statistics
    let getStats (state: DriverState) : uint64 * int =
        (state.RequestCount, List.length state.Devices)

    /// Shutdown all devices
    let shutdown (state: DriverState) : Result<unit> =
        let resetResults =
            state.Devices
            |> List.map reset

        let failures =
            resetResults
            |> List.filter (function Failure _ -> true | _ -> false)

        match failures with
        | [] ->
            state.Devices <- []
            Success ()
        | _ ->
            Failure "Some devices failed to reset"
