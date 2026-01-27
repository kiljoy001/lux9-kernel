# liblux9cs - C# Runtime for Lux9

C# bindings for Lux9 syscalls, designed for use with [bflat](https://github.com/bflat-lang/bflat).

## Overview

This library provides C# programs with access to Lux9 kernel services without requiring the .NET runtime. When compiled with bflat's bare-metal mode, C# code can run directly on Lux9.

## Files

- `Lux9.Syscalls.cs` - Low-level P/Invoke bindings to liblux
- `Lux9.Console.cs` - Console output (stdout/stderr)
- `Lux9.File.cs` - File I/O operations
- `Lux9.Process.cs` - Process control (fork, exec, exit)

## Usage

```csharp
using Lux9;

class Program
{
    static void Main()
    {
        Console.WriteLine("Hello from C#!");
        
        string content = File.ReadAllText("/dev/random");
        Console.WriteLine($"Read {content?.Length ?? 0} bytes");
        
        Process.Exit(null);
    }
}
```

## Building (Planned)

```bash
# When bflat Lux9 target is ready:
bflat build Hello.cs --os:lux9 -o hello.lux9
```

## Architecture

```
C# Code
    ↓
bflat (NativeAOT, no runtime)
    ↓
Native ELF binary
    ↓ P/Invoke
liblux.a (C syscall stubs)
    ↓ 9P messages
Lux9 Kernel
```

## Requirements

- bflat compiler with custom Lux9 target (TODO)
- liblux.a compiled for linking
- Lux9 kernel

## Status

**Work in Progress** - C# API designed, integration with bflat pending.
