# Why F# for Lux9 Userspace

## The Case for F# Over C#

### Safety Advantages

**Immutable by Default:**
```fsharp
// F# - immutable by default
let x = 10
x <- 20  // Compile error!

// C# - mutable by default
int x = 10;
x = 20;  // Works (dangerous)
```

**No Null by Default:**
```fsharp
// F# - Option type instead of null
let maybeValue : int option = Some 42
match maybeValue with
| Some x -> printfn "Got %d" x
| None -> printfn "No value"

// C# - null everywhere
string s = null;  // Compiles, crashes at runtime
Console.WriteLine(s.Length);  // NullReferenceException
```

**Pattern Matching (Exhaustiveness Checking):**
```fsharp
// F# - compiler enforces all cases
type Result =
    | Success of int
    | Error of string

let handle result =
    match result with
    | Success x -> x
    | Error msg -> 0
    // Compiler error if you forget a case!

// C# - easy to miss cases
if (result.IsSuccess) { ... }
// Forgot to handle Error? Runtime bug.
```

**Algebraic Data Types:**
```fsharp
// F# - discriminated unions (perfect for OS)
type FileDescriptor =
    | Console
    | File of path: string
    | Socket of port: int
    | Pipe of processId: int

type Capability =
    | Memory of address: uint64 * size: uint64
    | Channel of channelId: int
    | Device of deviceName: string

// Pattern matching is exhaustive and safe
let handleFd fd =
    match fd with
    | Console -> writeConsole()
    | File path -> writeFile path
    | Socket port -> writeSocket port
    | Pipe pid -> writePipe pid
```

**Functional Pipelining:**
```fsharp
// F# - composable, readable
let result =
    readFile "/etc/passwd"
    |> String.split '\n'
    |> List.filter (fun line -> line.Contains "root")
    |> List.map (fun line -> line.ToUpper())
    |> String.concat "\n"

// C# - imperative, verbose
var lines = File.ReadAllLines("/etc/passwd");
var filtered = new List<string>();
foreach (var line in lines) {
    if (line.Contains("root")) {
        filtered.Add(line.ToUpper());
    }
}
var result = String.Join("\n", filtered);
```

### Perfect for Systems Programming

**Type-Safe Syscalls:**
```fsharp
// Define syscall types
type SyscallResult<'T> =
    | Ok of 'T
    | Error of errno: int

// Syscall wrapper
let write (fd: FileDescriptor) (data: byte[]): SyscallResult<int> =
    let fdNum =
        match fd with
        | Console -> 1
        | File _ -> 3
        | Socket _ -> 4
        | Pipe _ -> 5

    let result = syscall_write fdNum data
    if result >= 0 then
        Ok result
    else
        Error result

// Usage - compiler enforces error handling
match write Console [| 72uy; 105uy |] with
| Ok n -> printfn "Wrote %d bytes" n
| Error errno -> printfn "Error: %d" errno
```

**Type-Safe Capabilities:**
```fsharp
// Blind Ledger capability wrapper
type Capability<'T> = private {
    Hash: byte[]
    Size: uint64
    Permissions: Permission list
}

// Can only create via minting
module Capability =
    let mint (pa: uint64) (size: uint64) (perms: Permission list) =
        let hash = ledger_mint pa size perms
        { Hash = hash; Size = size; Permissions = perms }

    let verify (cap: Capability<'T>) : bool =
        ledger_verify cap.Hash

    // Type safety: can't use Memory capability as Channel
    let readMemory (cap: Capability<Memory>) (offset: int) : byte[] =
        if verify cap then
            unsafe_read cap.Hash offset
        else
            failwith "Invalid capability"

// Usage
let memCap : Capability<Memory> = Capability.mint 0x1000UL 4096UL [Read; Write]
let data = Capability.readMemory memCap 0  // Type-safe!
// let chan = Capability.readMemory chanCap 0  // Compile error!
```

**Computation Expressions (Monads):**
```fsharp
// Define result monad for syscalls
type SyscallBuilder() =
    member _.Bind(x, f) =
        match x with
        | Ok value -> f value
        | Error e -> Error e

    member _.Return(x) = Ok x
    member _.ReturnFrom(x) = x

let syscall = SyscallBuilder()

// Use it for clean error handling
let copyFile src dst =
    syscall {
        let! srcFd = openFile src Read
        let! dstFd = openFile dst Write
        let! data = readAll srcFd
        let! written = write dstFd data
        do! close srcFd
        do! close dstFd
        return written
    }
// Automatically short-circuits on error!
```

**Active Patterns:**
```fsharp
// Parse syscall arguments
let (|Read|Write|Execute|) (perm: int) =
    if perm &&& 0x1 <> 0 then Read
    elif perm &&& 0x2 <> 0 then Write
    else Execute

// Use in pattern matching
match permissions with
| Read -> "r--"
| Write -> "-w-"
| Execute -> "--x"
```

## F# Features Perfect for Lux9

### 1. Units of Measure (Type-Safe Arithmetic)

```fsharp
[<Measure>] type bytes
[<Measure>] type pages

let pageSize = 4096<bytes>

let pagesNeeded (size: int<bytes>) : int<pages> =
    let s = size / 1<bytes>
    let ps = pageSize / 1<bytes>
    ((s + ps - 1) / ps) * 1<pages>

let allocPages (n: int<pages>) : uint64 =
    pebble_alloc (n * (pageSize / 1<bytes>))

// Type safety prevents bugs
let size = 8192<bytes>
let pages = pagesNeeded size  // Correct
// let pages = size / pageSize  // Compile error! (bytes/bytes ≠ pages)
```

### 2. Type Providers (Future: Generate Syscalls from Spec)

```fsharp
// Future: Auto-generate syscall bindings
type Lux9Syscalls = SyscallProvider<"syscalls.json">

// Usage
let result = Lux9Syscalls.write(1, data)
// Compiler knows types from JSON spec!
```

### 3. Async/Await (Better than C#)

```fsharp
// F# async is lightweight, composable
let readFileAsync path = async {
    let! fd = openFileAsync path
    let! data = readAsync fd 4096
    do! closeAsync fd
    return data
}

// Run multiple async operations
let processFiles files =
    files
    |> List.map readFileAsync
    |> Async.Parallel
    |> Async.RunSynchronously
```

### 4. Discriminated Unions for Errors

```fsharp
type FileError =
    | NotFound of path: string
    | PermissionDenied of path: string
    | AlreadyExists of path: string
    | IOError of errno: int

type FileResult<'T> = Result<'T, FileError>

let openFile (path: string) : FileResult<FileDescriptor> =
    match syscall_open path with
    | -1 -> Error (NotFound path)
    | -2 -> Error (PermissionDenied path)
    | fd when fd > 0 -> Ok (File path)
    | errno -> Error (IOError errno)

// Pattern match on errors
match openFile "/etc/passwd" with
| Ok fd -> useFile fd
| Error (NotFound path) -> printfn "File not found: %s" path
| Error (PermissionDenied path) -> printfn "Access denied: %s" path
| Error err -> printfn "Error: %A" err
```

## Example: F# Init Process

```fsharp
// init.fs - Process 1 for Lux9

module Init

type MountPoint = {
    Source: string
    Target: string
    FsType: string
}

type InitError =
    | MountFailed of MountPoint * errno: int
    | SpawnFailed of program: string * errno: int
    | DeviceError of device: string

// Result monad for init operations
let initResult = SyscallBuilder()

let mountFs (mp: MountPoint) : Result<unit, InitError> =
    match syscall_mount mp.Source mp.Target mp.FsType with
    | 0 -> Ok ()
    | errno -> Error (MountFailed (mp, errno))

let spawnShell () : Result<int, InitError> =
    match syscall_fork () with
    | 0 ->
        // Child: exec shell
        syscall_exec "/bin/sh" [||] |> ignore
        Ok 0
    | pid when pid > 0 -> Ok pid
    | errno -> Error (SpawnFailed ("/bin/sh", errno))

// Main init sequence
let initSystem () =
    initResult {
        // Mount essential filesystems
        do! mountFs { Source = "devtmpfs"; Target = "/dev"; FsType = "devfs" }
        do! mountFs { Source = "proc"; Target = "/proc"; FsType = "procfs" }

        // Setup console
        let! cons = openFile "/dev/cons"
        do! dup2 cons 0  // stdin
        do! dup2 cons 1  // stdout
        do! dup2 cons 2  // stderr

        // Spawn shell
        let! shellPid = spawnShell ()

        printfn "Init: Started shell (pid %d)" shellPid

        // Wait for children
        return! waitLoop ()
    }

// Wait for child processes
let rec waitLoop () =
    match syscall_wait () with
    | pid when pid > 0 ->
        printfn "Init: Process %d exited" pid
        waitLoop ()
    | _ -> Ok ()

// Entry point
[<EntryPoint>]
let main args =
    match initSystem () with
    | Ok () -> 0
    | Error (MountFailed (mp, errno)) ->
        eprintfn "Failed to mount %s: errno %d" mp.Target errno
        1
    | Error (SpawnFailed (prog, errno)) ->
        eprintfn "Failed to spawn %s: errno %d" prog errno
        1
    | Error (DeviceError dev) ->
        eprintfn "Device error: %s" dev
        1
```

## Example: F# Shell (rc equivalent)

```fsharp
// sh.fs - Simple shell for Lux9

module Shell

type Command = {
    Program: string
    Args: string list
    Stdin: FileDescriptor option
    Stdout: FileDescriptor option
}

type Pipeline =
    | Simple of Command
    | Pipe of Command * Pipeline

let rec executePipeline = function
    | Simple cmd -> executeCommand cmd
    | Pipe (cmd, rest) ->
        // Create pipe
        match syscall_pipe () with
        | Ok (readFd, writeFd) ->
            // Fork for first command
            match syscall_fork () with
            | 0 ->
                // Child: exec with stdout = write end
                syscall_dup2 writeFd 1 |> ignore
                syscall_close readFd |> ignore
                executeCommand cmd
            | pid when pid > 0 ->
                // Parent: continue pipeline with stdin = read end
                syscall_close writeFd |> ignore
                executePipeline rest
            | _ -> Error "Fork failed"
        | Error e -> Error e

// REPL
let rec repl () =
    printf "# "
    match readLine () with
    | Some line ->
        let pipeline = parseLine line
        match executePipeline pipeline with
        | Ok status -> ()
        | Error msg -> eprintfn "Error: %s" msg
        repl ()
    | None -> ()

[<EntryPoint>]
let main args =
    printfn "Lux9 Shell (F#)"
    repl ()
    0
```

## Example: F# ls Command

```fsharp
// ls.fs - List directory contents

module Ls

type FileInfo = {
    Name: string
    Size: int64
    Permissions: Permission list
    IsDirectory: bool
}

let listDirectory path =
    result {
        let! dir = openDir path
        let! entries = readDir dir
        do! closeDir dir
        return entries
    }

let formatLong (info: FileInfo) =
    let perms = formatPermissions info.Permissions
    let size = info.Size
    let name = if info.IsDirectory then info.Name + "/" else info.Name
    sprintf "%s %10d %s" perms size name

[<EntryPoint>]
let main args =
    let path = if args.Length > 0 then args.[0] else "."

    match listDirectory path with
    | Ok entries ->
        entries
        |> List.sortBy (fun e -> e.Name)
        |> List.iter (fun e -> printfn "%s" (formatLong e))
        0
    | Error (NotFound p) ->
        eprintfn "ls: %s: No such file or directory" p
        1
    | Error err ->
        eprintfn "ls: %A" err
        1
```

## F# vs C# for Lux9

| Feature | F# | C# |
|---------|----|----|
| **Null Safety** | ✅ Option types | ❌ Null everywhere |
| **Immutability** | ✅ Default | ❌ Opt-in |
| **Pattern Matching** | ✅ Exhaustive | ⚠️ Limited |
| **Discriminated Unions** | ✅ Native | ❌ Emulated |
| **Type Inference** | ✅ Excellent | ⚠️ Basic |
| **Functional Composition** | ✅ Built-in | ⚠️ LINQ |
| **Units of Measure** | ✅ Yes | ❌ No |
| **Active Patterns** | ✅ Yes | ❌ No |
| **Computation Expressions** | ✅ Yes | ⚠️ Async only |
| **Syntax Noise** | ✅ Minimal | ❌ Verbose |
| **Safety** | ✅ Maximum | ⚠️ Medium |

## Compilation Pipeline

```
F# Source (.fs)
    ↓
F# Compiler (fsc)
    ↓
.NET IL (CIL bytecode)
    ↓
Fruity IR Converter
    ↓
Fruity IR (in-memory)
    ↓
QBE Backend
    ↓
x86-64 Assembly
    ↓
Native Binary
```

## Next Steps for F# Integration

1. **Test F# → IL compilation**
   ```bash
   fsc init.fs
   # Produces init.dll with IL bytecode
   ```

2. **Parse IL → Fruity IR**
   - IL is simpler than parsing C# AST
   - Stack-based → easy to convert

3. **Implement F# runtime basics**
   - FSharp.Core (minimal subset)
   - Option type
   - List type
   - Result type

4. **Test simple F# program**
   ```fsharp
   let main args =
       printfn "Hello from F#!"
       0
   ```

## F# Standard Library Subset for Lux9

**Core (must have):**
- Option<'T>
- Result<'T, 'E>
- List<'T>
- Array<'T>
- String operations

**Nice to have:**
- Seq<'T> (lazy sequences)
- Map<'K, 'V>
- Set<'T>
- Async<'T>

**Can skip:**
- Reflection
- LINQ
- Large collections (ResizeArray, etc.)

## Summary

**Why F# is perfect for Lux9:**
- ✅ **Safety**: Immutable, no null, exhaustive patterns
- ✅ **Expressiveness**: ADTs, pattern matching, pipelines
- ✅ **Systems fit**: Great for OS utilities, syscalls, capabilities
- ✅ **Smaller runtime**: Less bloat than C#
- ✅ **Better for CLR**: Simpler IL, easier to compile

**Decision:** Use F# for all userspace programs.
- init process (F#)
- Shell (F#)
- Core utilities (F#)
- System services (F#)

This is the right call - F# is **much** safer and cleaner than C#.
