module Lux9.Init

open System
open System.Runtime.InteropServices

// WASM Import for Kernel 9P
[<DllImport("env", EntryPoint="lux9_send_9p")>]
extern int Lux9Send9P(byte[] msg, int64 len)

[<DllImport("env", EntryPoint="lux9_debug_print")>]
extern void Lux9Print(byte[] msg, int64 len)

[<DllImport("env", EntryPoint="lux9_spawn")>]
extern int64 Lux9Spawn(string path)

[<DllImport("env", EntryPoint="lux9_sleep")>]
extern void Lux9Sleep(int64 ms)

let print (s: string) =
    let bytes = System.Text.Encoding.UTF8.GetBytes(s)
    Lux9Print(bytes, int64 bytes.Length)

/// Send a 9P message to kernel via WASM host function
let sendToKernel (message: byte[]) =
    let res = Lux9Send9P(message, int64 message.Length)
    if res < 0 then
        failwith "Kernel communication failed"
    ()

/// 9P Protocol Helpers
module P9 =
    let mutable tag : uint16 = 0us
    let mutable fidCounter : uint32 = 100u

    let nextTag () =
        tag <- tag + 1us
        tag

    let nextFid () =
        fidCounter <- fidCounter + 1u
        fidCounter

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

    let finalize (buf: byte[]) (off: int) =
        putInt (off) buf 0 |> ignore
        buf

    // Tversion (100)
    let tVersion (msize: int) (version: string) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4 + 2 + version.Length)
        let mutable off = 4
        off <- putByte 100uy buf off
        off <- putShort 65535us buf off // NOTAG
        off <- putInt msize buf off
        off <- putString version buf off
        finalize buf off

    // Tattach (104): tag[2] fid[4] afid[4] uname[s] aname[s]
    let tAttach (fid: uint32) (afid: uint32) (uname: string) (aname: string) =
        let buf = Array.zeroCreate<byte> (100 + uname.Length + aname.Length) // Safe estimate
        let mutable off = 4
        off <- putByte 104uy buf off
        off <- putShort (nextTag()) buf off
        off <- putInt (int fid) buf off
        off <- putInt (int afid) buf off
        off <- putString uname buf off
        off <- putString aname buf off
        finalize buf off

    // Twalk (110): tag[2] fid[4] newfid[4] nwname[2] nwname*(wname[s])
    let tWalk (fid: uint32) (newfid: uint32) (names: string[]) =
        let mutable len = 100
        for n in names do len <- len + 2 + n.Length
        let buf = Array.zeroCreate<byte> len
        let mutable off = 4
        off <- putByte 110uy buf off
        off <- putShort (nextTag()) buf off
        off <- putInt (int fid) buf off
        off <- putInt (int newfid) buf off
        off <- putShort (uint16 names.Length) buf off
        for n in names do
            off <- putString n buf off
        finalize buf off

    // Topen (112): tag[2] fid[4] mode[1]
    let tOpen (fid: uint32) (mode: byte) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4 + 1)
        let mutable off = 4
        off <- putByte 112uy buf off
        off <- putShort (nextTag()) buf off
        off <- putInt (int fid) buf off
        off <- putByte mode buf off
        finalize buf off

    // Twrite (118): tag[2] fid[4] offset[8] count[4] data[count]
    let tWrite (fid: uint32) (offset: uint64) (data: string) =
        let bytes = System.Text.Encoding.UTF8.GetBytes(data)
        let buf = Array.zeroCreate<byte> (30 + bytes.Length)
        let mutable off = 4
        off <- putByte 118uy buf off
        off <- putShort (nextTag()) buf off
        off <- putInt (int fid) buf off
        // offset 64-bit
        off <- putInt (int (offset &&& 0xFFFFFFFFUL)) buf off
        off <- putInt (int (offset >>> 32)) buf off
        
        off <- putInt bytes.Length buf off
        Array.Copy(bytes, 0, buf, off, bytes.Length)
        off <- off + bytes.Length
        finalize buf off

    // Tclunk (120): tag[2] fid[4]
    let tClunk (fid: uint32) =
        let buf = Array.zeroCreate<byte> (4 + 1 + 2 + 4)
        let mutable off = 4
        off <- putByte 120uy buf off
        off <- putShort (nextTag()) buf off
        off <- putInt (int fid) buf off
        finalize buf off

// Globals
let NOFID = 0xFFFFFFFFu
let OREAD = 0uy
let OWRITE = 1uy
let ORDWR = 2uy

// Bind helper: bind new old
let bind (newPath: string) (oldPath: string) =
    try
        // 1. Attach to /mnt
        let rootFid = P9.nextFid()
        sendToKernel (P9.tAttach rootFid NOFID "root" "/mnt")
        // Note: Assuming success. Real impl should read Rattach.

        // 2. Walk to "ctl"
        let ctlFid = P9.nextFid()
        sendToKernel (P9.tWalk rootFid ctlFid [| "ctl" |])
        // Assuming success

        // 3. Open ctl
        sendToKernel (P9.tOpen ctlFid OWRITE)

        // 4. Write bind command: "bind #c /dev"
        let cmd = sprintf "bind %s %s" newPath oldPath
        sendToKernel (P9.tWrite ctlFid 0UL cmd)

        // 5. Cleanup
        sendToKernel (P9.tClunk ctlFid)
        sendToKernel (P9.tClunk rootFid)
        
        print (sprintf "[INIT] Bound %s -> %s" newPath oldPath)
    with ex ->
        print (sprintf "[INIT] Bind failed for %s -> %s: %s" newPath oldPath ex.Message)

/// Init entry point
[<EntryPoint>]
let main (args: string[]) : int =
    let hello = [| 0x48uy; 0x45uy; 0x4Cuy; 0x4Cuy; 0x4Fuy; 0x0Auy |] // HELLO\n
    Lux9Print(hello, 6L)
    print "=== Lux9 Init (WASM) - Namespace First ==="
    
    try
        print "[INIT] Sending Tversion..."
        let msg = P9.tVersion 8192 "9P2000"
        sendToKernel msg
        print "[INIT] Tversion Sent."
        
        // Setup Standard Namespace
        // Note: /dev, /env, /proc, /srv, /mnt must exist in root (#r)
        print "[INIT] Setting up namespace..."
        bind "#c" "/dev"
        bind "#e" "/env"
        bind "#p" "/proc"
        bind "#s" "/srv"
        // bind "#|" "/mnt" // Usually mounted to a specific mountpoint

        // Helper: Read plan9.ini?
        // Currently skipping read as 9P router doesn't fully support Tread on /env yet.
        print "[INIT] Namespace setup complete"

        print "[INIT] Spawning /bin/ramfs..."
        try
            let ramfs_pid = Lux9Spawn("/bin/ramfs")
            print (sprintf "[INIT] RamFS running with PID %d" ramfs_pid)
        with ex ->
            print (sprintf "[INIT] Failed to spawn RamFS: %s" ex.Message)

        print "[INIT] Spawning /bin/shell..."
        try
            let pid = Lux9Spawn("/bin/shell")
            print (sprintf "[INIT] Spawned shell with PID %d" pid)
        with ex ->
            print (sprintf "[INIT] Failed to spawn shell: %s" ex.Message)

        print "[INIT] Entering residency loop..."
        while true do
            Lux9Sleep(5000L)
            
        0
    with ex ->
        print (sprintf "[INIT] FATAL: %s" ex.Message)
        1