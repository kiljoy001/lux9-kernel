namespace Microsoft.FSharp.Core
open System

[<System.AttributeUsage(System.AttributeTargets.Method, AllowMultiple = false)>]
type EntryPointAttribute() =
    inherit System.Attribute()

[<AutoOpen>]
module Operators =
    let inline ignore _ = ()
    
    // Pipe operators for functional composition
    let inline (|>) x f = f x
    let inline (<|) f x = f x
    let inline (>>) f g x = g (f x)
    let inline (<<) f g x = f (g x)
    
    // Default value
    let inline defaultArg opt defaultValue =
        match opt with
        | Some v -> v
        | None -> defaultValue

    // Not operator
    let inline not b = if b then false else true
    
    // Failure
    let inline failwith (message: string) = raise (System.Exception(message))

type int = System.Int32
type obj = System.Object
type bool = System.Boolean
type string = System.String
type float = System.Double
type byte = System.Byte


[<Struct>]
type Option<'T> =
    | None
    | Some of 'T

type Result<'T,'TError> =
    | Ok of 'T
    | Error of 'TError

module Option =
    let isSome = function Some _ -> true | None -> false
    let isNone = function None -> true | Some _ -> false
    let map f = function Some x -> Some (f x) | None -> None
    let bind f = function Some x -> f x | None -> None
    let defaultValue v = function Some x -> x | None -> v
    let get = function Some x -> x | None -> failwith "Option has no value"

[<Struct>]
type Unit =
    member x.ToString() = "()"
    // Stub

type SourceConstructFlags =
    | Module = 1
    | Closure = 2
    | Exception = 4
    | UnionCase = 8

[<System.AttributeUsage(System.AttributeTargets.All, AllowMultiple = false)>]
type CompilationMappingAttribute(sourceConstructFlags: System.Int32) =
    inherit System.Attribute()
    member _.SourceConstructFlags = sourceConstructFlags


// F# Immutable List
namespace Microsoft.FSharp.Collections
open Microsoft.FSharp.Core

type FSharpList<'T> =
    | Empty
    | Cons of head: 'T * tail: FSharpList<'T>
    
    member this.Head =
        match this with
        | Cons(h, _) -> h
        | Empty -> failwith "List is empty"
    
    member this.Tail =
        match this with
        | Cons(_, t) -> t
        | Empty -> failwith "List is empty"
    
    member this.Length =
        let rec loop acc lst =
            match lst with
            | Empty -> acc
            | Cons(_, t) -> loop (acc + 1) t
        loop 0 this

// Alias for convenient syntax
type list<'T> = FSharpList<'T>

module List =
    let empty<'T> : FSharpList<'T> = Empty
    
    let isEmpty (lst: FSharpList<'T>) = 
        match lst with
        | Empty -> true
        | Cons _ -> false
    
    let head (lst: FSharpList<'T>) = lst.Head
    
    let tail (lst: FSharpList<'T>) = lst.Tail
    
    let length (lst: FSharpList<'T>) = lst.Length
    
    let cons h t = Cons(h, t)
    
    let rec map (f: 'a -> 'b) (lst: FSharpList<'a>) : FSharpList<'b> =
        match lst with
        | Empty -> Empty
        | Cons(h, t) -> Cons(f h, map f t)
    
    let rec filter (predicate: 'a -> bool) (lst: FSharpList<'a>) : FSharpList<'a> =
        match lst with
        | Empty -> Empty
        | Cons(h, t) ->
            if predicate h then Cons(h, filter predicate t)
            else filter predicate t
    
    let rec fold (folder: 'state -> 'a -> 'state) (state: 'state) (lst: FSharpList<'a>) : 'state =
        match lst with
        | Empty -> state
        | Cons(h, t) -> fold folder (folder state h) t
    
    let rec foldBack (folder: 'a -> 'state -> 'state) (lst: FSharpList<'a>) (state: 'state) : 'state =
        match lst with
        | Empty -> state
        | Cons(h, t) -> folder h (foldBack folder t state)
    
    let rec rev (lst: FSharpList<'a>) : FSharpList<'a> =
        fold (fun acc x -> Cons(x, acc)) Empty lst
    
    let rec concat (lists: FSharpList<FSharpList<'a>>) : FSharpList<'a> =
        match lists with
        | Empty -> Empty
        | Cons(h, t) -> append h (concat t)
    
    and append (lst1: FSharpList<'a>) (lst2: FSharpList<'a>) : FSharpList<'a> =
        foldBack cons lst1 lst2
    
    let rec forall (predicate: 'a -> bool) (lst: FSharpList<'a>) : bool =
        match lst with
        | Empty -> true
        | Cons(h, t) -> predicate h && forall predicate t
    
    let rec exists (predicate: 'a -> bool) (lst: FSharpList<'a>) : bool =
        match lst with
        | Empty -> false
        | Cons(h, t) -> predicate h || exists predicate t
    
    let rec iter (action: 'a -> unit) (lst: FSharpList<'a>) : unit =
        match lst with
        | Empty -> ()
        | Cons(h, t) -> action h; iter action t
    
    let rec sumBy (projection: 'a -> int) (lst: FSharpList<'a>) : int =
        fold (fun acc x -> acc + projection x) 0 lst
    
    let ofArray (arr: 'a array) : FSharpList<'a> =
        let mutable result = Empty
        for i = arr.Length - 1 downto 0 do
            result <- Cons(arr.[i], result)
        result
    
    let toArray (lst: FSharpList<'a>) : 'a array =
        let len = length lst
        let arr = Array.zeroCreate len
        let mutable current = lst
        let mutable i = 0
        while not (isEmpty current) do
            arr.[i] <- head current
            current <- tail current
            i <- i + 1
        arr

// F# Immutable Map (simplified AVL tree implementation)
type FSharpMap<'Key, 'Value when 'Key : comparison> =
    | MapEmpty
    | MapNode of key: 'Key * value: 'Value * left: FSharpMap<'Key, 'Value> * right: FSharpMap<'Key, 'Value> * height: int

module Map =
    let empty<'Key, 'Value when 'Key : comparison> : FSharpMap<'Key, 'Value> = MapEmpty
    
    let isEmpty (map: FSharpMap<'Key, 'Value>) =
        match map with
        | MapEmpty -> true
        | MapNode _ -> false
    
    let private height (map: FSharpMap<'Key, 'Value>) =
        match map with
        | MapEmpty -> 0
        | MapNode(_, _, _, _, h) -> h
    
    let private makeNode key value left right =
        let h = 1 + max (height left) (height right)
        MapNode(key, value, left, right, h)
    
    let rec add (key: 'Key) (value: 'Value) (map: FSharpMap<'Key, 'Value>) : FSharpMap<'Key, 'Value> =
        match map with
        | MapEmpty -> MapNode(key, value, MapEmpty, MapEmpty, 1)
        | MapNode(k, v, left, right, _) ->
            if key < k then
                makeNode k v (add key value left) right
            elif key > k then
                makeNode k v left (add key value right)
            else
                makeNode key value left right
    
    let rec tryFind (key: 'Key) (map: FSharpMap<'Key, 'Value>) : Microsoft.FSharp.Core.Option<'Value> =
        match map with
        | MapEmpty -> Microsoft.FSharp.Core.None
        | MapNode(k, v, left, right, _) ->
            if key < k then tryFind key left
            elif key > k then tryFind key right
            else Microsoft.FSharp.Core.Some v
    
    let find (key: 'Key) (map: FSharpMap<'Key, 'Value>) : 'Value =
        match tryFind key map with
        | Microsoft.FSharp.Core.Some v -> v
        | Microsoft.FSharp.Core.None -> failwith "Key not found"
    
    let containsKey (key: 'Key) (map: FSharpMap<'Key, 'Value>) : bool =
        match tryFind key map with
        | Microsoft.FSharp.Core.Some _ -> true
        | Microsoft.FSharp.Core.None -> false
    
    let rec fold (folder: 'State -> 'Key -> 'Value -> 'State) (state: 'State) (map: FSharpMap<'Key, 'Value>) : 'State =
        match map with
        | MapEmpty -> state
        | MapNode(k, v, left, right, _) ->
            let s1 = fold folder state left
            let s2 = folder s1 k v
            fold folder s2 right
    
    let count (map: FSharpMap<'Key, 'Value>) : int =
        fold (fun acc _ _ -> acc + 1) 0 map
    
    let toList (map: FSharpMap<'Key, 'Value>) : FSharpList<'Key * 'Value> =
        fold (fun acc k v -> Cons((k, v), acc)) Empty map
    
    let ofList (lst: FSharpList<'Key * 'Value>) : FSharpMap<'Key, 'Value> =
        List.fold (fun acc (k, v) -> add k v acc) empty lst

// F# Immutable Set (simplified using Map)
type FSharpSet<'T when 'T : comparison> =
    | SetData of FSharpMap<'T, unit>

module Set =
    let empty<'T when 'T : comparison> : FSharpSet<'T> = SetData(MapEmpty)
    
    let isEmpty (set: FSharpSet<'T>) =
        match set with
        | SetData(map) -> Map.isEmpty map
    
    let add (value: 'T) (set: FSharpSet<'T>) : FSharpSet<'T> =
        match set with
        | SetData(map) -> SetData(Map.add value () map)
    
    let contains (value: 'T) (set: FSharpSet<'T>) : bool =
        match set with
        | SetData(map) -> Map.containsKey value map
    
    let count (set: FSharpSet<'T>) : int =
        match set with
        | SetData(map) -> Map.count map
    
    let toList (set: FSharpSet<'T>) : FSharpList<'T> =
        match set with
        | SetData(map) -> 
            Map.fold (fun acc k _ -> Cons(k, acc)) Empty map
    
    let ofList (lst: FSharpList<'T>) : FSharpSet<'T> =
        List.fold (fun acc x -> add x acc) empty lst

// F# Lazy Sequence (minimal implementation)
type Seq<'T> =
    | SeqEmpty
    | SeqCons of head: 'T * tail: (unit -> Seq<'T>)

module Seq =
    let empty<'T> : Seq<'T> = SeqEmpty
    
    let isEmpty (s: Seq<'T>) =
        match s with
        | SeqEmpty -> true
        | SeqCons _ -> false
    
    let head (s: Seq<'T>) : 'T =
        match s with
        | SeqCons(h, _) -> h
        | SeqEmpty -> failwith "Sequence is empty"
    
    let tail (s: Seq<'T>) : Seq<'T> =
        match s with
        | SeqCons(_, t) -> t()
        | SeqEmpty -> failwith "Sequence is empty"
    
    let rec map (f: 'a -> 'b) (s: Seq<'a>) : Seq<'b> =
        match s with
        | SeqEmpty -> SeqEmpty
        | SeqCons(h, t) -> SeqCons(f h, fun () -> map f (t()))
    
    let rec filter (predicate: 'a -> bool) (s: Seq<'a>) : Seq<'a> =
        match s with
        | SeqEmpty -> SeqEmpty
        | SeqCons(h, t) ->
            if predicate h then SeqCons(h, fun () -> filter predicate (t()))
            else filter predicate (t())
    
    let rec take (n: int) (s: Seq<'a>) : Seq<'a> =
        if n <= 0 then SeqEmpty
        else
            match s with
            | SeqEmpty -> SeqEmpty
            | SeqCons(h, t) -> SeqCons(h, fun () -> take (n - 1) (t()))
    
    let toList (s: Seq<'a>) : FSharpList<'a> =
        let rec loop acc s =
            match s with
            | SeqEmpty -> List.rev acc
            | SeqCons(h, t) -> loop (Cons(h, acc)) (t())
        loop Empty s
    
    let ofList (lst: FSharpList<'a>) : Seq<'a> =
        let rec loop lst =
            match lst with
            | Empty -> SeqEmpty
            | Cons(h, t) -> SeqCons(h, fun () -> loop t)
        loop lst

// F# String module
module String =
    open System
    
    let length (s: string) : int = s.Length
    
    let isEmpty (s: string) : bool = s.Length = 0
    
    let isNullOrEmpty (s: string) : bool = 
        s = null || s.Length = 0
    
    let concat (sep: string) (strings: FSharpList<string>) : string =
        let sb = System.Text.StringBuilder()
        let rec loop first lst =
            match lst with
            | Empty -> ()
            | Cons(h, t) ->
                if not first then sb.Append(sep : string) |> ignore
                sb.Append(h : string) |> ignore
                loop false t
        loop true strings
        sb.ToString()
    
    let split (separator: char) (s: string) : FSharpList<string> =
        let parts = s.Split(separator)
        let mutable result = Empty
        for i = parts.Length - 1 downto 0 do
            result <- Cons(parts.[i], result)
        result
    
    let contains (substring: string) (s: string) : bool =
        s.Contains(substring)
    
    let startsWith (prefix: string) (s: string) : bool =
        s.StartsWith(prefix)
    
    let endsWith (suffix: string) (s: string) : bool =
        s.EndsWith(suffix)
    
    let trim (s: string) : string = s.Trim()
    
    let toLower (s: string) : string = s.ToLower()
    
    let toUpper (s: string) : string = s.ToUpper()
    
    let substring (start: int) (length: int) (s: string) : string =
        s.Substring(start, length)
    
    let replicate (count: int) (s: string) : string =
        let sb = System.Text.StringBuilder()
        for _ in 1..count do
            sb.Append(s) |> ignore
        sb.ToString()

// Printf module for formatted output (minimal implementation)
namespace Microsoft.FSharp.Core

module Printf =
    open System
    
    // Direct kernel InternalCall - bypasses cross-assembly resolution
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Internal_PrintLine(string message)
    
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Internal_Print(string message)
    
    // Simple printfn implementation that handles basic format specifiers
    let printfn (format: string) : 'T =
        // Direct kernel print - no cross-assembly call needed
        Internal_PrintLine(format)
        Unchecked.defaultof<'T>
    
    let printf (format: string) : 'T =
        Internal_Print(format)
        Unchecked.defaultof<'T>
    
    let sprintf (format: string) : 'T =
        // Return the format string for now
        Unchecked.defaultof<'T>

[<AutoOpen>]
module PrintfTopLevel =
    let printfn fmt = Printf.printfn fmt
    let printf fmt = Printf.printf fmt
    let sprintf fmt = Printf.sprintf fmt

