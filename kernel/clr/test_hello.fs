// test_hello.fs - Simple F# test program
//
// Compile:
//   fsc test_hello.fs
//
// This produces test_hello.dll with IL bytecode

[<EntryPoint>]
let main args =
    printfn "Hello from F#!"
    0
