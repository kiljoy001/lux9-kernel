# Boot Bring-up Status

## Current milestone
- Kernel boots through `proc0()` setup, maps user stack/text, and executes the real `touser()`/`sysretq` path.
- Serial breadcrumb `'U'` confirms SYSRET is issued; no further output, and QEMU resets after watchdog timeout.

## What’s working
- Early mmupool lock leak fixed (mmualloc cleanup committed as `prep: guard fp exit and seed user stack`).
- User stack page allocated, zeroed, and mapped into the new page tables.
- Plan 9 `touser`, `syscallentry`, `forkret`, `_strayintr[x]`, and full 256-entry `vectortable` ported from 9front (`pc64: port touser and syscallentry`).
- `touser`/`syscallentry` instrumented with `'U'`/`'s'` serial markers for transition tracing.

## Remaining issues
- SYSRET fails to land in user mode: no initcode output, no trap markers. Likely causes:
  - UESEL/UDSEL descriptors missing or not loaded into GDT used after CR3 switch.
  - `touser` clearing RMACH/RUSER before SYSRET (Plan 9 loads them from GS base after swapgs) may zero pointers needed later.
  - Stack pointer or return address not canonical or misaligned (Plan 9 expects extra 8-byte gap beneath stack frame).
  - CR4/STAR/SFMask MSRs not set for SYSCALL/SYSRET (9front configures them during CPU init).

## Next steps
1. Capture register state just before SYSRET using GDB (`info registers`) to confirm RCX/RSP canonical values and R11 bits.
2. Verify GDT entries for UESEL/UDSEL exist after `setuppagetables()`/`mmuinit()`; ensure IDT/GDT pointer still valid once we build our own tables.
3. Cross-check `syscallentry` prologue/epilogue against Plan 9’s exact Ureg layout (DS/ES/FS/GS slots).
4. Consider temporarily leaving RMACH/RUSER intact through SYSRET to see if fault handlers rely on them before GS reload.
5. If SYSRET still reboots silently, install a double-fault handler breakpoint via GDB to catch the failure path.
