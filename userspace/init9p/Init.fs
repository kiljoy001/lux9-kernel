module Lux9.Init

open System
open System.P9

/// Simple init - no routing, just lifecycle management
/// Children talk directly to kernel via their own exchange pages

/// Send a 9P message to kernel
let sendToKernel (message: byte[]) : byte[] =
    let pageCap = ExchangePage.GetMyPage()
    if not pageCap.IsValid then
        failwith "Init: Cannot get exchange page"

    let reply = Array.zeroCreate ExchangePage.ReplySize
    let replyLen = ExchangePage.SendMessage(pageCap, message, message.Length, reply, reply.Length)

    if replyLen > 0 then
        reply.[0..replyLen-1]
    else
        failwith "Init: Kernel communication failed"

/// 9P Protocol Helpers
module P9 =
    let mutable tag : uint16 = 0us

    let nextTag () =
        tag <- tag + 1us
        tag

    let putByte (b: byte) (buf: byte[]) (offset: int) =
        buf.[offset] <- b
        offset + 1

    let putShort (s: uint16) (buf: byte[]) (offset: int) =
        buf.[offset] <- byte (s &&& 0xFFus)
        buf.[offset+1] <- byte (s >>> 8)
        offset + 2

    let putInt (i: int) (buf: byte[]) (offset: int) =
        buf.[offset] <- byte (i &&& 0xFF)
        buf.[offset+1] <- byte ((i >>> 8) &&& 0xFF)
        buf.[offset+2] <- byte ((i >>> 16) &&& 0xFF)
        buf.[offset+3] <- byte ((i >>> 24) &&& 0xFF)
        offset + 4

    let putString (s: string) (buf: byte[]) (offset: int) =
        let bytes = System.Text.Encoding.UTF8.GetBytes(s)
        let off = putShort (uint16 bytes.Length) buf offset
        Array.Copy(bytes, 0, buf, off, bytes.Length)
        off + bytes.Length

    // Basic Tversion
    let tVersion (msize: int) (version: string) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4 + 2 + version.Length)
        let mutable off = 4 // Skip size for now
        off <- putByte 100uy buf off // Tversion
        off <- putShort 65535us buf off // NOTAG
        off <- putInt msize buf off
        off <- putString version buf off
        putInt (off) buf 0 |> ignore
        buf

    // Tattach
    let tAttach (fid: int) (afid: int) (uname: string) (aname: string) =
        let size = 4 + 1 + 2 + 4 + 4 + (2 + uname.Length) + (2 + aname.Length)
        let buf = Array.zeroCreate<byte> size
        let mutable off = 4
        off <- putByte 104uy buf off // Tattach
        off <- putShort (nextTag()) buf off
        off <- putInt fid buf off
        off <- putInt afid buf off
        off <- putString uname buf off
        off <- putString aname buf off
        putInt off buf 0 |> ignore
        buf

    // Twalk
    let tWalk (fid: int) (newfid: int) (names: string[]) =
        let mutable size = 4 + 1 + 2 + 4 + 4 + 2
        for n in names do size <- size + 2 + System.Text.Encoding.UTF8.GetBytes(n).Length
        let buf = Array.zeroCreate<byte> size
        let mutable off = 4
        off <- putByte 110uy buf off // Twalk
        off <- putShort (nextTag()) buf off
        off <- putInt fid buf off
        off <- putInt newfid buf off
        off <- putShort (uint16 names.Length) buf off
        for n in names do off <- putString n buf off
        putInt off buf 0 |> ignore
        buf

    // Topen
    let tOpen (fid: int) (mode: byte) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4 + 1)
        let mutable off = 4
        off <- putByte 112uy buf off // Topen
        off <- putShort (nextTag()) buf off
        off <- putInt fid buf off
        off <- putByte mode buf off
        putInt off buf 0 |> ignore
        buf

    // Tcreate
    let tCreate (fid: int) (name: string) (perm: int) (mode: byte) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4 + (2 + name.Length) + 4 + 1)
        let mutable off = 4
        off <- putByte 114uy buf off // Tcreate
        off <- putShort (nextTag()) buf off
        off <- putInt fid buf off
        off <- putString name buf off
        off <- putInt perm buf off
        off <- putByte mode buf off
        putInt off buf 0 |> ignore
        buf

    // Twrite
    let tWrite (fid: int) (offset: uint64) (data: byte[]) =
        let size = 4 + 1 + 2 + 4 + 8 + 4 + data.Length
        let buf = Array.zeroCreate<byte> size
        let mutable off = 4
        off <- putByte 118uy buf off // Twrite
        off <- putShort (nextTag()) buf off
        off <- putInt fid buf off
        // Write vlong offset manually
        buf.[off] <- byte (offset &&& 0xFFUL)
        buf.[off+1] <- byte ((offset >>> 8) &&& 0xFFUL)
        buf.[off+2] <- byte ((offset >>> 16) &&& 0xFFUL)
        buf.[off+3] <- byte ((offset >>> 24) &&& 0xFFUL)
        buf.[off+4] <- byte ((offset >>> 32) &&& 0xFFUL)
        buf.[off+5] <- byte ((offset >>> 40) &&& 0xFFUL)
        buf.[off+6] <- byte ((offset >>> 48) &&& 0xFFUL)
        buf.[off+7] <- byte ((offset >>> 56) &&& 0xFFUL)
        off <- off + 8
        off <- putInt data.Length buf off
        off <- putInt data.Length buf off
        Array.Copy(data, 0, buf, off, data.Length)
        putInt (off + data.Length) buf 0 |> ignore
        buf
        
    // Tclunk
    let tClunk (fid: int) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4)
        let mutable off = 4
        off <- putByte 120uy buf off // Tclunk
        off <- putShort (nextTag()) buf off
        off <- putInt fid buf off
        putInt off buf 0 |> ignore
        buf

/// Set up namespace by sending bind commands to /mnt/ctl
let setupNamespace () =
    printfn "[INIT] Setting up namespace..."

    try
        // 0. Initialize 9P session (Tversion)
        let msgVer = P9.tVersion 8192 "9P2000"
        let _ = sendToKernel msgVer
        printfn "[INIT] 9P Version OK"

        // 1. Attach to /mnt using aname="#M". Use Fid 1.
        // Assuming Fid 1 is free.
        let msgAttach = P9.tAttach 1 -1 "init" "#M"
        let _ = sendToKernel msgAttach
        printfn "[INIT] Attached #M to Fid 1"
        
        // 2. Walk to ctl (Fid 1 -> Fid 2)
        let msgWalk = P9.tWalk 1 2 [| "ctl" |]
        let _ = sendToKernel msgWalk
        printfn "[INIT] Walked to ctl (Fid 2)"

        // 3. Open ctl (Fid 2, OWRITE only = 1)
        let msgOpen = P9.tOpen 2 1uy
        let _ = sendToKernel msgOpen
        printfn "[INIT] Opened ctl"
        
        // Ensure /dev and /proc exist in root
        // Clone root (Fid 1) to Fid 3
        let _ = sendToKernel (P9.tWalk 1 3 [||])
        // Create /dev (DMDIR = 0x80000000)
        // 0x80000000 | 0755
        // Tcreate(fid, name, perm, mode)
        // Note: Tcreate requires parent fid, creates file, and modifies fid to new file.
        // We catch exception in case it already exists (create fails)
        try
            let _ = sendToKernel (P9.tCreate 3 "dev" -2147483189 0uy) // DMDIR | 0755
            printfn "[INIT] Created /dev"
        with _ -> printfn "[INIT] /dev likely exists"
        let _ = sendToKernel (P9.tClunk 3) // Close fid 3

        // Clone root (Fid 1) to Fid 3
        let _ = sendToKernel (P9.tWalk 1 3 [||])
        try
            let _ = sendToKernel (P9.tCreate 3 "proc" -2147483189 0uy)
            printfn "[INIT] Created /proc"
        with _ -> printfn "[INIT] /proc likely exists"
        let _ = sendToKernel (P9.tClunk 3)

        // 4. Write "bind #c /dev 0"
        let cmd1 = System.Text.Encoding.UTF8.GetBytes("bind #c /dev 0")
        let msgWrite1 = P9.tWrite 2 0UL cmd1
        let _ = sendToKernel msgWrite1
        printfn "[INIT] Wrote bind #c /dev"

        // 5. Write "bind #p /proc 0"
        let cmd2 = System.Text.Encoding.UTF8.GetBytes("bind #p /proc 0")
        let msgWrite2 = P9.tWrite 2 0UL cmd2
        let _ = sendToKernel msgWrite2
        printfn "[INIT] Wrote bind #p /proc"
        
        // Cleanup
        let _ = sendToKernel (P9.tClunk 2)
        let _ = sendToKernel (P9.tClunk 1)
        ()
        
    with ex ->
        printfn "[INIT] Namespace setup FAILED: %s" ex.Message
        // Don't fail completely, try to spawn shell anyway
        
    printfn "[INIT] Namespace setup complete"

/// Spawn a child process
let spawnChild (path: string) =
    printfn "[INIT] Spawning: %s" path

    // Use the new Process.Start API
    try
        let _ = System.Diagnostics.Process.Start(path, "")
        printfn "[INIT] Spawned: %s" path
    with ex ->
        printfn "[INIT] FAILED to spawn %s: %s" path ex.Message
    ()

    printfn "[INIT] Spawned: %s" path

/// Reap exited children
let rec reapChildren () =
    // TODO: Send wait() request to kernel via 9P
    // For now, just sleep
    System.Threading.Thread.Sleep(1000)
    reapChildren()

/// Init entry point
[<EntryPoint>]
let main (args: string[]) : int =
    printfn "=== Lux9 Init (Simple) ==="
    printfn "PID: 1"
    printfn "Architecture: Direct kernel communication, no routing"
    printfn ""

    try
        // Set up namespace
        setupNamespace()

        // Spawn shell
        spawnChild "/bin/shell"

        // Loop forever reaping children
        printfn "[INIT] Entering reap loop..."
        reapChildren()

        0
    with ex ->
        printfn "[INIT] FATAL: %s" ex.Message
        1
