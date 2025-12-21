module Lux9.Init

open System
open System.Runtime.InteropServices

// WASM Import for Kernel 9P
[<DllImport("env", EntryPoint="lux9_send_9p")>]
extern int Lux9Send9P(byte[] msg, int len)

[<DllImport("env", EntryPoint="lux9_debug_print")>]
extern void Lux9Print(string msg, int len)

[<DllImport("env", EntryPoint="lux9_spawn")>]
extern int Lux9Spawn(string path)

[<DllImport("env", EntryPoint="lux9_sleep")>]
extern void Lux9Sleep(int ms)

let print (s: string) =
    let bytes = System.Text.Encoding.UTF8.GetBytes(s)
    Lux9Print(s, bytes.Length)

/// Send a 9P message to kernel via WASM host function
let sendToKernel (message: byte[]) =
    let res = Lux9Send9P(message, message.Length)
    if res < 0 then
        failwith "Kernel communication failed"
    // Note: Return value is currently just status, response is in exchange page.
    // Ideally we would read it back. For now, assume success/async.
    ()

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

/// Init entry point
[<EntryPoint>]
let main (args: string[]) : int =
    print "=== Lux9 Init (WASM) ==="
    
    try
        print "[INIT] Sending Tversion..."
        let msg = P9.tVersion 8192 "9P2000"
        sendToKernel msg
        print "[INIT] Tversion Sent."
        
        print "[INIT] Spawning /bin/shell..."
        try
            let pid = Lux9Spawn("/bin/shell")
            print (sprintf "[INIT] Spawned shell with PID %d" pid)
        with ex ->
            print (sprintf "[INIT] Failed to spawn shell: %s" ex.Message)

        print "[INIT] Entering residency loop..."
        while true do
            Lux9Sleep(5000)
            
        0
    with ex ->
        print (sprintf "[INIT] FATAL: %s" ex.Message)
        1