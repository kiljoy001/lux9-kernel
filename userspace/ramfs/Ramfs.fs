module Lux9.Ramfs

open System
open System.Runtime.InteropServices
open System.Collections.Generic

// WASM imports for kernel communication
[<DllImport("env", EntryPoint="lux9_debug_print")>]
extern void Lux9Print(string msg, int len)

let print (s: string) =
    let bytes = System.Text.Encoding.UTF8.GetBytes(s)
    Lux9Print(s, bytes.Length)

/// File entry in the RamFS tree
type FileEntry = {
    Name: string
    mutable Content: byte[]
    IsDirectory: bool
    mutable Children: Map<string, FileEntry>
    mutable Size: int64
    Permissions: int
}

/// In-memory filesystem
type RamFS() =
    let mutable root = {
        Name = "/"
        Content = [||]
        IsDirectory = true
        Children = Map.empty
        Size = 0L
        Permissions = 0o755
    }
    
    /// Split path into components
    let splitPath (path: string) =
        path.Split([|'/'|], StringSplitOptions.RemoveEmptyEntries)
        |> Array.toList
    
    /// Navigate to parent directory and get entry name
    let rec navigateTo (components: string list) (current: FileEntry) : (FileEntry * string) option =
        match components with
        | [] -> None
        | [name] -> Some (current, name)
        | name :: rest ->
            match Map.tryFind name current.Children with
            | Some child when child.IsDirectory ->
                navigateTo rest child
            | _ -> None
    
    /// Find entry at path
    let rec findEntry (components: string list) (current: FileEntry) : FileEntry option =
        match components with
        | [] -> Some current
        | name :: rest ->
            match Map.tryFind name current.Children with
            | Some child -> findEntry rest child
            | None -> None
    
    /// Create file or directory
    member this.Create(path: string, isDir: bool, perm: int) : int =
        try
            let components = splitPath path
            match navigateTo components root with
            | Some (parent, name) ->
                if Map.containsKey name parent.Children then
                    -1 // File exists
                else
                    let newEntry = {
                        Name = name
                        Content = [||]
                        IsDirectory = isDir
                        Children = Map.empty
                        Size = 0L
                        Permissions = perm
                    }
                    parent.Children <- Map.add name newEntry parent.Children
                    print (sprintf "[RAMFS] Created %s: %s" (if isDir then "dir" else "file") path)
                    0
            | None -> -1 // Parent not found
        with ex ->
            print (sprintf "[RAMFS] Create error: %s" ex.Message)
            -1
    
    /// Read from file
    member this.Read(path: string, offset: int64, count: int) : byte[] =
        try
            let components = splitPath path
            match findEntry components root with
            | Some entry when not entry.IsDirectory ->
                let actualOffset = min offset entry.Size
                let actualCount = min count (int (entry.Size - actualOffset))
                if actualCount <= 0 then
                    [||]
                else
                    Array.sub entry.Content (int actualOffset) actualCount
            | _ -> [||]
        with ex ->
            print (sprintf "[RAMFS] Read error: %s" ex.Message)
            [||]
    
    /// Write to file
    member this.Write(path: string, offset: int64, data: byte[]) : int =
        try
            let components = splitPath path
            match findEntry components root with
            | Some entry when not entry.IsDirectory ->
                let newSize = max entry.Size (offset + int64 data.Length)
                if int64 entry.Content.Length < newSize then
                    // Expand buffer
                    let newContent = Array.zeroCreate<byte> (int newSize)
                    Array.Copy(entry.Content, newContent, entry.Content.Length)
                    entry.Content <- newContent
                
                Array.Copy(data, 0, entry.Content, int offset, data.Length)
                entry.Size <- newSize
                print (sprintf "[RAMFS] Wrote %d bytes to %s at offset %d" data.Length path offset)
                data.Length
            | _ -> -1
        with ex ->
            print (sprintf "[RAMFS] Write error: %s" ex.Message)
            -1
    
    /// Remove file or directory
    member this.Remove(path: string) : int =
        try
            let components = splitPath path
            match navigateTo components root with
            | Some (parent, name) ->
                match Map.tryFind name parent.Children with
                | Some entry ->
                    if entry.IsDirectory && not (Map.isEmpty entry.Children) then
                        -1 // Directory not empty
                    else
                        parent.Children <- Map.remove name parent.Children
                        print (sprintf "[RAMFS] Removed %s" path)
                        0
                | None -> -1
            | None -> -1
        with ex ->
            print (sprintf "[RAMFS] Remove error: %s" ex.Message)
            -1
    
    /// Get file/directory info
    member this.Stat(path: string) : (bool * int64 * int) option =
        try
            let components = splitPath path
            match findEntry components root with
            | Some entry -> Some (entry.IsDirectory, entry.Size, entry.Permissions)
            | None -> None
        with ex ->
            print (sprintf "[RAMFS] Stat error: %s" ex.Message)
            None
    
    /// List directory contents
    member this.List(path: string) : string[] =
        try
            let components = splitPath path
            match findEntry components root with
            | Some entry when entry.IsDirectory ->
                entry.Children
                |> Map.toArray
                |> Array.map fst
            | _ -> [||]
        with ex ->
            print (sprintf "[RAMFS] List error: %s" ex.Message)
            [||]

// Global RamFS instance
let ramfs = RamFS()

// Export functions for kernel to call
[<EntryPoint>]
let main (args: string[]) : int =
    print "=== Lux9 RamFS (WASM) ==="
    print "[RAMFS] Filesystem initialized"
    
    // Create some default directories
    ramfs.Create("/tmp", true, 0o777) |> ignore
    ramfs.Create("/var", true, 0o755) |> ignore
    ramfs.Create("/home", true, 0o755) |> ignore
    
    print "[RAMFS] Default directories created"
    print "[RAMFS] Ready for operations"
    
    // Stay resident
    while true do
        System.Threading.Thread.Sleep(1000)
    
    0
