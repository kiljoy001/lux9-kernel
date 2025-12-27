module Test

[<EntryPoint>]
let main argv =
    let x = 42
    let res = match x with | 42 -> 100 | _ -> 0
    printfn "%d" res
    0