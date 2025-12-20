namespace Microsoft.FSharp.Core
open System

[<System.AttributeUsage(System.AttributeTargets.Method, AllowMultiple = false)>]
type EntryPointAttribute() =
    inherit System.Attribute()

[<AutoOpen>]
module Operators =
    let inline ignore _ = ()

type int = System.Int32
type obj = System.Object
type bool = System.Boolean
type string = System.String


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
