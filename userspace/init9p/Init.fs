namespace Init

open System

module Task =
    let Add (a: int) (b: int) : int =
        a + b
    
    // Test conditional control flow
    let Max (a: int) (b: int) : int =
        if a > b then
            a
        else
            b
    
    // Test loop control flow
    let Sum (n: int) : int =
        let mutable acc = 0
        let mutable i = 1
        while i <= n do
            acc <- acc + i
            i <- i + 1
        acc
    
    // Test nested conditionals
    let Classify (x: int) : int =
        if x < 0 then
            -1  // negative
        else if x = 0 then
            0   // zero
        else
            1   // positive

module Lux9Start =
    // Custom entry point for Lux9 Kernel
    let KernelEntry () : int = 
        Console.WriteLine("Lux9 F# Init: Starting...")

        // Test simple addition
        let sum = Task.Add 10 20
        Console.WriteLine("Add Result:")
        Console.WriteLine(sum)
        
        // Test conditional (if/else)
        let maxVal = Task.Max 15 25
        Console.WriteLine("Max Result:")
        Console.WriteLine(maxVal)
        
        // Test loop (while)
        let total = Task.Sum 5  // 1+2+3+4+5 = 15
        Console.WriteLine("Sum Result:")
        Console.WriteLine(total)
        
        // Test nested conditional
        let cls = Task.Classify 42
        Console.WriteLine("Classify Result:")
        Console.WriteLine(cls)
        
        let finalResult = sum + maxVal + total + cls
        Console.WriteLine("Final Result:")
        Console.WriteLine(finalResult)

        // Return combined result
        finalResult  // 30 + 25 + 15 + 1 = 71

module Program =
    [<EntryPoint>]
    let main argv =
        0