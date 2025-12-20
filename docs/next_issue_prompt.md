# Task: Fix Allocator Hang during Sysexec CLR Load

## Context
The system successfully boots, runs `proc0`, sets up the namespace, and launches `sysexec` for `/boot/init`.
`sysexec` correctly identifies `/boot/init` as a CLR assembly (MZ header) and attempts to load the entire file into memory to execute it using `clr_execute_assembly`.
The file size is approx 13KB.

## Symptom
The system hangs or crashes silently during the memory allocation for the file content.
Debug logs show the sequence entering `malloc`, proceeding to `poolalloc`, then `poolnewarena`, and finally `xallocz`.
`xallocz` appears to succeed and allocate memory.
However, control never returns to `sysexec` to print the success message.

## Relevant Logs
```
DEBUG: sysexec allocating 13312 bytes
malloc: request size=13312
poolalloc enter p=ffffffff80221020 n=13328
poolallocl start p=ffffffff80221020 dsize=13328
poolallocl: need new arena asize=131104
poolnewarena enter p=ffffffff80221020 asize=131104 cursize=131104 max=841476000
xallocz start size=131104 zero=1 caller=0xffffffff813b94dc
xallocz: adjusted size 131128 bytes
xallocz: locked size=131128
xallocz success size=131128 addr=ffff80000a9909b0 data=ffff80000a9909c0
```
(No further output. Expected: "DEBUG: sysexec allocated asm_data=...")

## Code References
- **`kernel/9front-port/sysproc.c`**: `sysexec` function, specifically the CLR loading block (near "detected CLR assembly").
- **`kernel/libc9/pool.c`**: `poolalloc`, `poolnewarena`.
- **`kernel/9front-port/xalloc.c`**: `xallocz`.

## Task
Investigate why the system hangs after `xallocz` returns during this specific allocation.
Possible causes:
1.  Stack overflow (kernel stack is small).
2.  Locking issue (pool lock vs xalloc lock?).
3.  Memory corruption causing a silent fault.
4.  Issue with `poolnewarena` linking the new block.

Goal: Ensure `malloc` returns successfully and `sysexec` proceeds to call `clr_execute_assembly`.
