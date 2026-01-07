type Point = { X: int; Y: int }

let createPoint x y = { X = x; Y = y }

let sumPoint (p: Point) = p.X + p.Y

[<EntryPoint>]
let main args =
    let p = createPoint 10 20
    sumPoint p // Should return 30
