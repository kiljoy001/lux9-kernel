namespace Lux9.Kernel

open System

// Primitive types mapped to kernel structures
type PebbleToken = nativeint
type ChannelId = uint32
type MsgId = uint32

// Kernel Panic (maps to kernel panic())
module Kernel =
    // External method - mapped to C function 'panic'
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Panic(string message)

    // External method - mapped to C function 'print'
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Print(string message)

// Pebble Memory Management
module Pebble =
    // Maps to FRUITY_LIME (clr_object_alloc)
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern 'T Allocate<'T>()

    // Maps to FRUITY_VANILLA (clr_object_addref)
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Share<'T>('T obj)

    // Maps to FRUITY_BURN (clr_object_release)
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Release<'T>('T obj)

// Message Ordering (GHOSTDAG/MSGORD)
module MsgOrd =
    // Submit message for ordering
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern MsgId Submit(string path, object payload)

    // Check if message is ordered (BLUE)
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern bool IsOrdered(MsgId id)

// Tasklet Management
module Tasklet =
    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Yield()

    [<System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)>]
    extern void Sleep(int milliseconds)
