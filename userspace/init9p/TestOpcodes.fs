module TestOpcodes

open System
// open Lux9.Core

let TestArithmetic () =
    let a = 10L
    let b = 20L
    let c = a + b
    if c = 30L then
        Console.WriteLine "Arithmetic: PASS"
    else
        Console.WriteLine "Arithmetic: FAIL"

let TestLDSTR () =
    Console.WriteLine 12345
    let s = "Hello"
    Console.WriteLine s
    // We expect "Hello" to be printed. 
    // If we see garbage, we know LDSTR or Print is broken.

let TestFlow () =
    let mutable x = 0L
    if x = 0L then
        x <- 1L
    else
        x <- 2L
    
    if x = 1L then
        Console.WriteLine "Flow: PASS"
    else
        Console.WriteLine "Flow: FAIL"

let RunAll () =
    Console.WriteLine "Starting Opcode Tests..."
    TestArithmetic()
    TestLDSTR()
    TestFlow()
    Console.WriteLine "Tests Complete."
