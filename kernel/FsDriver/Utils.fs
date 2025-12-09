namespace FsDriver

module Utils =
    /// Compute checksum of byte array
    let checksum (data: byte array) : uint32 =
        data
        |> Array.fold (fun acc b -> acc + uint32 b) 0u

    /// Align address to page boundary
    let alignUp (addr: uint64) (alignment: uint64) : uint64 =
        let mask = alignment - 1UL
        (addr + mask) &&& (~~~mask)

    /// Check if address is aligned
    let isAligned (addr: uint64) (alignment: uint64) : bool =
        (addr % alignment) = 0UL

    /// Convert error to result
    let toResult (condition: bool) (error: string) : Result<unit> =
        if condition then Success ()
        else Failure error

    /// Retry operation up to N times
    let rec retry (n: int) (f: unit -> Result<'T>) : Result<'T> =
        if n <= 0 then
            Failure "Max retries exceeded"
        else
            match f() with
            | Success v -> Success v
            | Failure _ -> retry (n - 1) f
