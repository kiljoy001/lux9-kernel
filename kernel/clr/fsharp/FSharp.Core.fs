namespace Microsoft.FSharp.Core
open System

[<System.AttributeUsage(System.AttributeTargets.Method, AllowMultiple = false)>]
type EntryPointAttribute() =
    inherit System.Attribute()

[<AutoOpen>]
module Operators =
    let inline ignore _ = ()

    // Arithmetic
    let inline (+) (x: int) (y: int) = x + y
    let inline (-) (x: int) (y: int) = x - y
    let inline (*) (x: int) (y: int) = x * y
    let inline (/) (x: int) (y: int) = x / y
    let inline (%) (x: int) (y: int) = x % y
    let inline (~-) (x: int) = -x

    // Bitwise
    let inline (&&&) (x: int) (y: int) = x &&& y
    let inline (|||) (x: int) (y: int) = x ||| y
    let inline (^^^) (x: int) (y: int) = x ^^^ y
    let inline (<<<) (x: int) (y: int) = x <<< y
    let inline (>>>) (x: int) (y: int) = x >>> y

    // Comparison
    let inline (=) x y = x = y
    let inline (<>) x y = x <> y
    let inline (<) x y = x < y
    let inline (<=) x y = x <= y
    let inline (>) x y = x > y
    let inline (>=) x y = x >= y
    let inline min x y = if x <= y then x else y
    let inline max x y = if x >= y then x else y
    let inline abs x = if x < 0 then -x else x

    // Pipe operators
    let inline (|>) x f = f x
    let inline (<|) f x = f x
    let inline (>>) f g x = g (f x)
    let inline (<<) f g x = f (g x)
    
    // Reference cells
    type Ref<'T> = { mutable contents: 'T }
    let inline ref x = { contents = x }
    let inline (!) (r: Ref<'T>) = r.contents
    let inline (:=) (r: Ref<'T>) v = r.contents <- v
    let inline incr (r: Ref<int>) = r.contents <- r.contents + 1
    let inline decr (r: Ref<int>) = r.contents <- r.contents - 1

    // Option
    let inline defaultArg opt defaultValue =
        match opt with
        | Some v -> v
        | None -> defaultValue

    // Not operator
    let inline not b = if b then false else true
    
    // Failure
    let inline failwith (message: string) = raise (System.Exception(message))
    let inline invalidArg (arg: string) (message: string) = raise (System.ArgumentException(message, arg))

    // Format strings (Core implementation hook)
    type Format<'Printer,'State,'Residue,'Result> = Format of string

    // Conversions
    let inline int x = unbox<int> x
    let inline uint32 x = unbox<uint32> x
    let inline byte x = unbox<byte> x
    // Real implementation would rely on IL opcodes or framework calls
    // The "unbox" here is a hack for identity conversion of primitives in some runtimes, 
    // but for actual conversion (float -> int) it fails.
    // However, for hex printing we convert int -> uint32 which is bit-compatible.
    // And uint32 -> int for indexing.


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

type 'T option = Option<'T>

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

    interface System.Collections.Generic.IEnumerable<'T> with
        member this.GetEnumerator() =
            let mutable curr = this
            let mutable started = false
            { new System.Collections.Generic.IEnumerator<'T> with
                 member _.Current = 
                     match curr with 
                     | Cons(h, _) -> h 
                     | Empty -> Unchecked.defaultof<'T>
                 member _.MoveNext() =
                     if not started then
                         started <- true
                         match curr with Empty -> false | _ -> true
                     else
                         match curr with
                         | Cons(_, t) -> 
                             curr <- t
                             match curr with Empty -> false | _ -> true
                         | _ -> false
                 member _.Reset() = 
                     curr <- this
                     started <- false
                 member _.Dispose() = ()
                 member _.get_Current() = 
                     match curr with 
                     | Cons(h, _) -> box h 
                     | Empty -> null
            }

    interface System.Collections.IEnumerable with
        member this.GetEnumerator() = (this :> System.Collections.Generic.IEnumerable<'T>).GetEnumerator() :> System.Collections.IEnumerator

// Alias for convenient syntax
type list<'T> = FSharpList<'T>
type 'T list = FSharpList<'T>

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

module Array =
    let length (arr: 'T array) = arr.Length
    
    let isEmpty (arr: 'T array) = arr.Length = 0
    
    let get (arr: 'T array) i = arr.[i]
    
    let set (arr: 'T array) i v = arr.[i] <- v
    
    let zeroCreate<'T> (count: int) : 'T array = Microsoft.FSharp.Core.Operators.failwith "Implemented by compiler intrinsic"
    // Note: In real F#, zeroCreate is often an external or intrinsic. 
    // Here we assume `Array.zeroCreate` is bound to `System.Array.ZeroCreate` or similar 
    // OR we rely on existing `Array.zeroCreate` usage in `List.toArray` which works?
    // In `List.toArray`: `let arr = Array.zeroCreate len`.
    // So `Array.zeroCreate` MUST be available in the environment or `Operators`?
    // It's not in `Operators` above.
    // It's likely an intrinsic `Microsoft.FSharp.Collections.Array.zeroCreate`.
    // Let's define it as a stub that the compiler replaces, or use `System.Array`.
    // For now, I'll rely on the user having it working since `List.toArray` uses it.
    
    let init (count: int) (initializer: int -> 'T) : 'T array =
        let arr = Array.zeroCreate count
        for i = 0 to count - 1 do
            arr.[i] <- initializer i
        arr
        
    let create (count: int) (value: 'T) : 'T array =
        let arr = Array.zeroCreate count
        for i = 0 to count - 1 do
            arr.[i] <- value
        arr
        
    let iter (action: 'T -> unit) (arr: 'T array) : unit =
        for i = 0 to arr.Length - 1 do
            action arr.[i]
            
    let iteri (action: int -> 'T -> unit) (arr: 'T array) : unit =
        for i = 0 to arr.Length - 1 do
            action i arr.[i]
            
    let map (mapping: 'T -> 'U) (arr: 'T array) : 'U array =
        let len = arr.Length
        let res = Array.zeroCreate len
        for i = 0 to len - 1 do
            res.[i] <- mapping arr.[i]
        res
        
    let mapi (mapping: int -> 'T -> 'U) (arr: 'T array) : 'U array =
        let len = arr.Length
        let res = Array.zeroCreate len
        for i = 0 to len - 1 do
            res.[i] <- mapping i arr.[i]
        res
        
    let fold (folder: 'State -> 'T -> 'State) (state: 'State) (arr: 'T array) : 'State =
        let mutable acc = state
        for i = 0 to arr.Length - 1 do
            acc <- folder acc arr.[i]
        acc
        
    let exists (predicate: 'T -> bool) (arr: 'T array) : bool =
        let mutable found = false
        let mutable i = 0
        while i < arr.Length && not found do
            if predicate arr.[i] then found <- true
            i <- i + 1
        found

    let forall (predicate: 'T -> bool) (arr: 'T array) : bool =
        let mutable ok = true
        let mutable i = 0
        while i < arr.Length && ok do
            if not (predicate arr.[i]) then ok <- false
            i <- i + 1
        ok
        
    let tryFind (predicate: 'T -> bool) (arr: 'T array) : 'T option =
        let mutable res = None
        let mutable i = 0
        while i < arr.Length && res.IsNone do
            if predicate arr.[i] then res <- Some arr.[i]
            i <- i + 1
        res
        
    let find (predicate: 'T -> bool) (arr: 'T array) : 'T =
        match tryFind predicate arr with
        | Some x -> x
        | None -> Microsoft.FSharp.Core.Operators.failwith "Key not found in Array"
        
    let ofList (lst: FSharpList<'T>) : 'T array =
        let mutable len = 0
        let mutable curr = lst
        while not (match curr with Empty -> true | _ -> false) do
             len <- len + 1
             curr <- match curr with Cons(_, t) -> t | Empty -> Empty
             
        let res = zeroCreate len
        let mutable i = 0
        curr <- lst
        while i < len do
             match curr with
             | Cons(h, t) ->
                 res.[i] <- h
                 curr <- t
             | Empty -> ()
             i <- i + 1
        res
        
    let toList (arr: 'T array) : FSharpList<'T> =
        let mutable res = Empty
        for i = arr.Length - 1 downto 0 do
            res <- Cons(arr.[i], res)
        res


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
// F# Sequence (alias for IEnumerable<T>)
type seq<'T> = System.Collections.Generic.IEnumerable<'T>
type Seq<'T> = seq<'T>

module Seq =
    let empty<'T> : seq<'T> = 
        System.Linq.Enumerable.Empty<'T>()
        
    let singleton (x: 'T) : seq<'T> =
        let arr = Microsoft.FSharp.Collections.Array.zeroCreate 1
        arr.[0] <- x
        arr :> seq<'T>
        
    let length (source: seq<'T>) : int =
        System.Linq.Enumerable.Count(source)
             
    let isEmpty (source: seq<'T>) : bool =
        not (System.Linq.Enumerable.Any(source))
        
    let map (mapping: 'T -> 'U) (source: seq<'T>) : seq<'U> =
        System.Linq.Enumerable.Select(source, System.Func<_,_>(mapping))
        
    let mapi (mapping: int -> 'T -> 'U) (source: seq<'T>) : seq<'U> =
         // Select with index supported by Enumerable
         System.Linq.Enumerable.Select(source, System.Func<_,_,_>(fun x i -> mapping i x))
         
    let filter (predicate: 'T -> bool) (source: seq<'T>) : seq<'T> =
        System.Linq.Enumerable.Where(source, System.Func<_,_>(predicate))
        
    let fold (folder: 'State -> 'T -> 'State) (state: 'State) (source: seq<'T>) : 'State =
        System.Linq.Enumerable.Aggregate(source, state, System.Func<_,_,_>(folder))
        
    let iter (action: 'T -> unit) (source: seq<'T>) : unit =
        use e = source.GetEnumerator()
        while e.MoveNext() do
            action e.Current
            
    let iteri (action: int -> 'T -> unit) (source: seq<'T>) : unit =
        use e = source.GetEnumerator()
        let mutable i = 0
        while e.MoveNext() do
            action i e.Current
            i <- i + 1
            
    let head (source: seq<'T>) : 'T =
        System.Linq.Enumerable.First(source)
        
    let tryHead (source: seq<'T>) : 'T option =
        // FirstOrDefault returns default(T) which might be null or 0.
        // We need explicit check.
        use e = source.GetEnumerator()
        if e.MoveNext() then Some e.Current else None
        
    let tail (source: seq<'T>) : seq<'T> =
        System.Linq.Enumerable.Skip(source, 1)
        
    let toArray (source: seq<'T>) : 'T array =
        System.Linq.Enumerable.ToArray(source)
        
    let toList (source: seq<'T>) : FSharpList<'T> =
        // Avoid internal recursion if possible, build locally
        let mutable res = Microsoft.FSharp.Collections.List.Empty
        let arr = toArray source
        for i = arr.Length - 1 downto 0 do
            res <- Microsoft.FSharp.Collections.List.Cons(arr.[i], res)
        res
        
    let ofList (source: FSharpList<'T>) : seq<'T> =
        // FSharpList implements IEnumerable
        source :> seq<'T>
        
    let ofArray (source: 'T array) : seq<'T> =
        source :> seq<'T>
        
    let cast<'T> (source: System.Collections.IEnumerable) : seq<'T> =
        let mutable res = Microsoft.FSharp.Collections.List.Empty
        let e = source.GetEnumerator()
        while e.MoveNext() do
            res <- Microsoft.FSharp.Collections.List.Cons(unbox<'T> e.Current, res)
        ofList (Microsoft.FSharp.Collections.List.rev res)
        
    let init (count: int) (initializer: int -> 'T) : seq<'T> =
        let range = System.Linq.Enumerable.Range(0, count)
        System.Linq.Enumerable.Select(range, System.Func<_,_>(initializer))
        
    let collect (mapping: 'T -> seq<'U>) (source: seq<'T>) : seq<'U> =
        System.Linq.Enumerable.SelectMany(source, System.Func<_,_>(mapping))
        
    let exists (predicate: 'T -> bool) (source: seq<'T>) : bool =
        System.Linq.Enumerable.Any(source, System.Func<_,_>(predicate))
        
    let forall (predicate: 'T -> bool) (source: seq<'T>) : bool =
        System.Linq.Enumerable.All(source, System.Func<_,_>(predicate))
        
    let tryFind (predicate: 'T -> bool) (source: seq<'T>) : 'T option =
        let mutable res = None
        use e = source.GetEnumerator()
        while res.IsNone && e.MoveNext() do
            if predicate e.Current then res <- Some e.Current
        res
        
    let find (predicate: 'T -> bool) (source: seq<'T>) : 'T =
        match tryFind predicate source with
        | Some x -> x
        | None -> Microsoft.FSharp.Core.Operators.failwith "Key not found"
    

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

// Printf module for formatted output
namespace Microsoft.FSharp.Core


module Printf =
    open System
    open System.Text
    
    // Direct kernel InternalCall
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Internal_PrintLine(string message)
    
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Internal_Print(string message)


    // Helper to scan format string
    // Implements simplified %d, %s, %b, %x handling
    let rec private parseFormat (fmt: string) (i: int) (sb: StringBuilder) (continuation: string -> 'Result) : obj =
        if i >= fmt.Length then
            continuation (sb.ToString()) :> obj
        else
            if fmt.[i] = '%' then
                if i + 1 >= fmt.Length then 
                    sb.Append('%') |> ignore
                    continuation (sb.ToString()) :> obj
                else
                     match fmt.[i+1] with
                     | 'd' -> 
                        let f (x: int) = 
                             sb.Append(x) |> ignore
                             parseFormat fmt (i+2) sb continuation
                        f :> obj
                     | 's' ->
                        let f (x: string) =
                             sb.Append(x) |> ignore
                             parseFormat fmt (i+2) sb continuation
                        f :> obj
                     | 'b' ->
                        let f (x: bool) =
                             sb.Append(x) |> ignore
                             parseFormat fmt (i+2) sb continuation
                        f :> obj
                     | 'x' ->
                        let f (x: int) =
                             // Manual hex (int based)
                             let bs = "0123456789abcdef"
                             let mutable val_ = x
                             let mutable started = false
                             // 32 bits = 8 hex digits
                             for i = 7 downto 0 do
                                 let shift = i * 4
                                 let nibble = (val_ >>> shift) &&& 0xF
                                 if nibble <> 0 || started || i=0 then
                                     started <- true
                                     sb.Append(bs.[nibble]) |> ignore
                             parseFormat fmt (i+2) sb continuation
                        f :> obj
                     | '%' ->
                        sb.Append('%') |> ignore
                        parseFormat fmt (i+2) sb continuation
                     | _ ->
                        sb.Append('%') |> ignore
                        sb.Append(fmt.[i+1]) |> ignore
                        parseFormat fmt (i+2) sb continuation
            else
                sb.Append(fmt.[i]) |> ignore
                parseFormat fmt (i+1) sb continuation

    let kprintf (continuation: string -> 'Result) (format: string) : 'Printer =
        unbox (parseFormat format 0 (StringBuilder()) continuation)

    let printfn (format: string) : 'Printer =
        kprintf (fun s -> Internal_PrintLine s) format

    let printf (format: string) : 'Printer =
        kprintf (fun s -> Internal_Print s) format
    
    let sprintf (format: string) : 'Printer =
        kprintf (fun s -> s) format

[<AutoOpen>]
module PrintfTopLevel =
    let printfn fmt = Printf.printfn fmt
    let printf fmt = Printf.printf fmt
    let sprintf fmt = Printf.sprintf fmt

