module Lux9.Init

// Init with Console.WriteLine - direct InternalCall test

open System

[<EntryPoint>]
let main (args: string[]) : int =
    Console.WriteLine("=== Lux9 Init (Resident) ===")
    
    let mutable running = true
    while running do
        Console.WriteLine("Init resident...")
    0
