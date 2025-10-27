# SIP/HIP Control Filesystem Plan

This document captures the current plan for implementing `/dev/sip`, the 9P control filesystem that manages Software or Hardware Isolated Processes (SIP/HIP) and their capabilities.

## Guiding Principles

- **9P everywhere**: Every lifecycle operation is exposed as a file read or write under `/dev/sip`. Driver-facing devices (`/dev/mem`, `/dev/dma`, `/dev/irq`, `/dev/pci`) already speak 9P; `/dev/sip` extends that model to capability and lifecycle management.
- **Capability-backed access**: `/dev/sip` is the single place where capabilities are granted or revoked. All hardware-facing devices must consult the capability bitmap attached to `up->sip` before serving a request.
- **Unified SIP/HIP handling**: The same control surface configures traditional SIP servers (user-mode isolation) and HIP servers (hardware-isolated contexts). HIP metadata hangs off the same per-server directory.

## Filesystem Layout

```
/dev/sip/
├── ctl             # Global control (rescan, dump)
├── status          # Aggregated system status/metrics
├── caps/           # Debug view of per-process capabilities
│   └── <pid>
└── servers/
    ├── clone       # Write type → returns new server id
    └── srv<ID>/
        ├── ctl     # Command channel (name, cap, bind, start, stop, kill, notes)
        ├── status  # State machine, restarts, last-crash info
        ├── pid     # Read: bound pid, Write: bind existing proc (HIP attach)
        ├── ns      # Future namespace overrides
        ├── hip_mode    # Desired HIP mode (write)
        ├── hip_status  # Hardware isolation attestation (read)
        └── caps    # Debug readback of granted caps
```

## Kernel Structures

- `SipServer`: id, human-readable name, owner pid, capability bitmask, bound device list, lifecycle state (Created/Starting/Running/Stopped/Crashed), restart counters, crash diagnostics.
- `SipRegistry`: global table (hash by id and by pid) guarded by `RWlock`, freelists for server ids.
- `SipDeviceBinding`: records exclusive claims on `/dev/mem` regions, DMA allocations, IRQ lines, etc.
- `SipProc` attachment: pointer on `Proc` holding current server membership and capability mask.

## Lifecycle Flow

1. **Clone**: writing a server type to `/dev/sip/servers/clone` creates a `SipServer` in `Created` state with no capabilities. The read side of `clone` reports pending crash notifications.
2. **Configure**: user writes commands to `/dev/sip/servers/srv<ID>/ctl`:
   - `name <string>`: set label.
   - `cap add|remove <cap>`: mutate capability mask.
   - `bind <device>`: declare hardware resources (e.g. IRQ 11, PCI TBDF).
   - `start`: transition to `Starting`; the init supervisor (userspace) may react by spawning the driver.
   - `stop`, `kill`, `notes <text>`: manage lifecycle and telemetry.
3. **Attach**: when the driver process starts, it writes its pid to `pid`. Kernel links `Proc` to the server and enforces caps on subsequent 9P opens.
4. **Running**: server state becomes `Running`. `status` exposes pid, uptime, restart budget, crash history.
5. **Crash/Exit**: `procctl` hook notices process death, moves server to `Crashed`, increments counters, queues optional auto-restart notifications. Entries under `/dev/sip/servers/clone` surface crash reports.
6. **Stop**: `stop` command revokes bindings, clears caps, transitions to `Stopped`.

## Capability Enforcement

- Add helper `sip_check(up, CAP_DMA)` used by `/dev/mem`, `/dev/dma`, `/dev/irq`, `/dev/pci`.
- Cap bitmap stored in `SipProc`. `/dev/sip/caps/<pid>` offers read-only debugging view.
- Capabilities envisioned: `CapDeviceAccess`, `CapIOPort`, `CapDMA`, `CapInterrupt`, `CapPCI`, others as needed.

## HIP Extensions

- `hip_mode`: write desired isolation mode (`none`, `trusted`, `sev`, etc.).
- `hip_status`: read attestation from hardware monitor (populated asynchronously).
- `ctl` command `hip attach <token>` binds to a hardware-enforced container.
- Future link with attestation subsystem to validate HIP claims before granting caps.

## Instrumentation & Tooling

- Add `SIP_DEBUG` guarded prints in the kernel registry for traceability.
- Provide userspace helper (`userspace/bin/sip_ctl.c` or rc script) to script clone/start/stop flows.
- Extend existing `/dev/sip/events` (future) to stream lifecycle events for monitoring daemons.

## Testing Roadmap

1. Unit-test capability checks by attempting to open hardware devices without caps.
2. Simulate lifecycle via rc scripts: clone → cap → start → bind driver stub → stop.
3. Introduce synthetic crash (send note) and verify `status`/`clone` reporting.
4. Once SYSRET path is stable, run the Go AHCI driver atop the new `/dev/sip` interface.

## Next Implementation Steps

1. Build `SipRegistry`, `/dev/sip` devtab, and minimal `clone/ctl/status` handling in the kernel.
2. Wire capability checks into existing device drivers (replace TODO stubs).
3. Hook process exit path (`procexit`) to update server state and clean DMA allocations.
4. Add HIP metadata files and stubbed attestation hooks (to be filled when hardware isolation lands).

This plan will evolve, but anchoring the design around 9P semantics ensures the SIP/HIP framework stays consistent with the rest of Lux9.
