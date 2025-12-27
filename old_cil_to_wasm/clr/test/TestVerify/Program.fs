namespace Test
open Microsoft.FSharp.Core
open Microsoft.FSharp.Collections

module BCLTest =
    let run () =
        // Test Operators
        let x = 10 + 20
        Printf.printfn "10 + 20 = %d" x
        
        if x <> 30 then failwith "Addition failed"

        // Test Compare
        if 10 >= 20 then failwith "Compare failed"
        
        // Test Array
        let arr = Array.create 5 1
        arr.[0] <- 42
        Printf.printfn "Arr[0] = %d" arr.[0]
        
        if arr.[0] <> 42 then failwith "Array set failed"
        
        // Test Array.map
        let arr2 = Array.map (fun x -> x * 2) arr
        Printf.printfn "Arr2[0] = %d" arr2.[0]
        
        if arr2.[0] <> 84 then failwith "Array map failed"
        
        // Test Option
        let o = Some 5
        let v = match o with Some x -> x | None -> 0
        Printf.printfn "Option: %d" v
        
        Printf.printfn "FSharp BCL Verification Complete"
