namespace Init

module Task =
    let Add (a: int) (b: int) : int =
        a + b

module Lux9Start =
    // Custom entry point for Lux9 Kernel
    let KernelEntry () : int = 
        let result = Task.Add 10 20
        result