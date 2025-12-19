module Lux9.Shell

open System
open System.IO
open System.Diagnostics

[<EntryPoint>]
let main args =
    Console.WriteLine("=== Lux9 F# Shell ===")
    Console.WriteLine("Type 'exit' to quit.")
    
    let mutable running = true
    while running do
        Console.Write("$ ")
        let line = Console.ReadLine()
        
        if not (String.IsNullOrEmpty(line)) then
            // manual split since we lack StringSplitOptions
            let rawParts = line.Split(' ')
            // Filter empty
            let parts = 
                let mutable cnt = 0
                for s in rawParts do
                    if not (String.IsNullOrEmpty(s)) then cnt <- cnt + 1
                let arr = Array.zeroCreate cnt
                let mutable idx = 0
                for s in rawParts do
                    if not (String.IsNullOrEmpty(s)) then
                        arr.[idx] <- s
                        idx <- idx + 1
                arr

            if parts.Length > 0 then
                let cmd = parts.[0]
                
                match cmd with
                | "exit" -> 
                    running <- false
                    Console.WriteLine("Bye!")
                | "cd" ->
                    if parts.Length > 1 then
                        Console.WriteLine("cd: Not implemented in minimal BCL yet")
                        // Environment.CurrentDirectory <- parts.[1]
                    else
                        Console.WriteLine("Usage: cd <path>")
                | _ ->
                    try
                        // Try to execute as binary
                        let argsStr = 
                            if parts.Length > 1 then 
                                let sb = System.Text.StringBuilder()
                                for i = 1 to parts.Length - 1 do
                                    if i > 1 then sb.Append(" ") |> ignore
                                    sb.Append(parts.[i]) |> ignore
                                sb.ToString()
                            else ""
                        
                        let _ = Process.Start(cmd, argsStr)
                        // TODO: Wait for child? Process.Start returns Process object
                        // Currently we don't have Wait() implemented in BCL fully
                        System.Threading.Thread.Sleep(500) // Hacky wait
                    with ex ->
                        // Manual crash?
                        Console.WriteLine("Error executing command")
                        // Console.WriteLine(ex.Message) // ex.Message might rely on stuff
    0
