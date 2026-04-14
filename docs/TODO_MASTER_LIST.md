# Lux9 Master TODO List

Goal: keep only mechanism in the kernel, treat hardware support as temporary
bootstrap glue, and move process, namespace, service, and driver policy into
user-space 9P services.

## Immediate Priorities
- [x] **Kinetic Defense Integration**
  - [x] Integrate `kernel/pow_gate.c` into `kernel/msgord.c`.
  - [x] Add PoW nonce check to message submission.
  - [x] Exempt TCB processes (`kp == 1`) to prevent circular dependencies.
  - [x] Enforce PoW for user-space drivers.

## Kernel Mechanism That Stays
- [x] **Exchange-page lifecycle**
  - [x] Replace lazy `P9SEG` stub allocation with the real per-process exchange
        page setup and teardown. (`kernel/9front-port/proc_p9setup_stub.c`,
        `kernel/9front-port/sysproc.c`,
        `kernel/9front-port/userinit.c`)
- [x] **Kernel `/proc` substrate**
  - [x] Keep a minimal kernel `/proc` substrate for process state exposure and
        control transport. (`kernel/router/proc.c`)
- [ ] **Define the minimum kernel `/proc` contract**
  - [x] Reduce kernel `/proc` responsibilities to mechanism only once `procd`
        owns policy and presentation. Public `/proc` attach is now bootstrap
        fallback only; raw `#p` remains the kernel substrate.
- [x] **Memory safety**
  - [x] Verify `bootstrap_alloc.c` separately from `xalloc.c`.
  - [x] Fix the raw address leak on verify failure in
        `kernel/9front-port/alloc.c`.
- [x] **Kernel-resident allocation accounting**
  - [x] Extend Pebble tracking to long-lived ring-0 allocations and keep
        pre-proc bootstrap allocations on the raw allocator until Pebble
        metadata is online. See `docs/PEBBLE_DRIVER_AUDIT.md`.
- [x] **Segment ownership**
  - [x] Record shared borrows during `dupseg()` and restore exclusivity
        correctly on COW collapse. (`kernel/9front-port/segment.c`)
- [x] **9P routing**
  - [x] Implement live server lookup in the active router.
        (`kernel/router/core.c`, `kernel/9front-pc64/globals.c`)
- [ ] **Exchange pool backlog**
  - [x] Finish Phase 4 dynamic resizing in `kernel/exchange_pool.c`.
- [ ] **Platform / trap correctness**
  - [x] Audit the interrupt control path in `kernel/9front-pc64/l.S`.
  - [x] Replace the global `m` trap fallback with trustworthy `%gs` handling in
        `kernel/9front-pc64/l.S`.
- [ ] **Low-level utility cleanup that may still belong in kernel**
  - [x] Use real timestamps for kernel device directories in
        `kernel/9front-port/dev.c`.
  - [x] Keep the UUID secondary index live in
        `kernel/9front-port/blind_ledger.c`.
  - [x] Audit the remaining utility, swap, VMX, link, and shutdown compatibility
        stubs in `kernel/9front-pc64/globals.c`.

## Kernel WASM / Isolation Work That Still Belongs In Kernel
- [x] **WASI environment export**
  - [x] Populate a real guest environment in
        `kernel/wasm/wasi_lux9_shim.c`.
- [x] **WASI path confinement**
  - [x] Enforce cleaned `/wasm` confinement and full dirfd-based path building.
        (`kernel/wasm/wasi_lux9_shim.c`)
- [x] **WASM memory model hardening**
  - [x] Keep stable linear-memory slots, guard gaps, dedicated grow/shrink
        backing, and wasm3 heap confinement.
        (`kernel/wasm/wasm_runtime.c`,
        `kernel/wasm/wasm_runtime/wasm3/m3_core.c`,
        `kernel/wasm/wasm_runtime/wasm3/m3_env.c`)
- [ ] **Verifier / runtime completeness**
  - [ ] Finish full abstract interpretation for stack verification in
        `kernel/wasm/fruity_ir.c`.
  - [ ] Finish missing WASI syscalls, errno normalization, and path / fd corner
        cases. (`kernel/wasm/wasi_lux9_shim.c`,
        `kernel/wasm/wasm_runtime.c`)
  - [ ] Finalize `/wasm` preopens, fd-rights inheritance, and per-process
        namespace isolation. (`kernel/9front-port/devroot.c`,
        `kernel/9front-port/userinit.c`,
        `kernel/wasm/wasi_lux9_shim.c`)
  - [ ] Reconcile branch-level linear-memory charging with physical-page
        ownership for xspanalloc-backed linear memory.
        (`kernel/wasm/wasm_runtime.c`, `kernel/9front-port/page.c`)
  - [ ] Define ring / exchange IPC for WASM and wire it into syscall entry.
        (`kernel/9front-port/devexchange.c`,
        `kernel/9front-port/devring.c`,
        `kernel/wasm/wasm_9p_integration.c`)
  - [ ] Map traps to stable WASI errno / exit behavior without panicking the
        kernel on malformed modules. (`kernel/wasm/wasm_runtime.c`,
        `kernel/wasm/wasm_runtime/wasm3`)
  - [ ] Finish syscall argument / return marshalling and remaining failure-path
        tightening in `kernel/wasm/wasm_runtime.c`.
  - [ ] Finish WASM fileserver load / parse / exchange marshalling.
        (`kernel/wasm/wasm_fileserver.c`)
- [ ] **Vendored wasm3 upstream TODOs**
  - [ ] Track remaining upstream TODOs in `m3_api_wasi.c`,
        `m3_api_uvwasi.c`, `m3_api_meta_wasi.c`, `m3_parse.c`,
        `m3_compile.c`, `m3_env.c`, `m3_info.c`, `m3_exec.h`,
        `m3_math_utils.h`, `m3_simd_hal.h`, and `lux9_math.c`.

## Transitional Kernel Glue To Evict
These items may need to exist temporarily to keep the ISO bootable, but they
are not target architecture and should not accrete permanent policy.

- [ ] **Create the generic HAL diverter**
  - [ ] Add `kernel/9front-port/devdivert.c` as the thin kernel-side client for
        user-space HAL-backed devices. See `docs/DRIVER_MIGRATION_STRATEGY.md`.
- [ ] **Move hardware probing out of kernel**
  - [ ] Treat `kernel/9front-port/pci_stubs.c` as bootstrap-only and replace it
        with a user-space HAL discovery path.
- [ ] **Retire temporary in-kernel storage / device backends**
  - [ ] Replace the current bootstrap implementations in
        `kernel/9front-port/sdio.c`,
        `kernel/9front-port/sdfis.c`,
        `kernel/9front-port/devram_stubs.c`,
        `kernel/9front-port/devcons_minimal.c`, and related shims with
        user-space services.
- [ ] **Retire temporary kernel compatibility backends**
  - [ ] Replace or delete the remaining compatibility backends in
        `kernel/9front-port/kernel_stubs.c`,
        `kernel/9front-port/vault_syscalls.c`,
        `kernel/9front-port/devsip.c`,
        `kernel/9front-port/devdistress.c`, and `kernel/9front-pc64/globals.c`
        once their user-space owners exist.
- [ ] **Decide the fate of compatibility-only devices**
  - [ ] Decide whether `/dev/ram` remains as bootstrap scaffolding or becomes a
        user-space service. (`kernel/9front-port/devram.c`,
        `kernel/9front-port/devram_stubs.c`)
  - [ ] Decide whether CGA snapshotting is needed at all before spending more
        time on `kernel/9front-pc64/cga.c`.
  - [ ] Decide whether TPM hardware support remains in kernel or moves behind a
        service boundary before reviving `kernel/9front-port/tpm_minimal.c.old`.
- [ ] **Architecture leftovers that are probably not TCB work**
  - [ ] Handle unsupported AML address spaces without `"not implemented"`
        fallthroughs if ACPI remains kernel-resident.
        (`kernel/9front-pc64/archacpi.c`)

## User-Space Process / Namespace / Service Policy
This is the primary backlog if the goal is a truer microkernel.

- [ ] **Finish `procd` as the real `/proc` service**
  - [x] Replace the old status-only stub with a real user-space `/proc` facade
        that proxies root, `/proc/<pid>`, `status`, `ctl`, and `stat` through
        kernel `#p`. (`userspace/procd/procd.c`)
  - [x] Expand the proxied per-process file set to include `mem`, `note`,
        `pid2`, and `ppid` so more `/proc` presentation lives in policy space.
        (`userspace/procd/procd.c`)
  - [x] Add the remaining `/proc` surface that should live in policy space
        instead of inside the kernel router, including synthetic `/proc` and
        `/proc/<pid>` directory presentation. (`userspace/procd/procd.c`)
  - [ ] Decide whether `procd` needs any explicit heartbeat / health reporting
        to resurrection beyond endpoint publication and supervisor ownership.
  - [ ] Decide which process-management operations stay as kernel mechanism and
        which become `procd` policy.
- [ ] **Finish `nsd` as namespace assembly, not a storage engine**
  - [ ] Keep mutable state in ordinary services and ordered control-plane data,
        not inside `nsd`. See `docs/ZX_GATEWAY.md`.
  - [ ] Finish the ordered namespace intent integration and remote-backed mount
        materialization in `userspace/ns/nsd/nsd.c`.
- [ ] **Service management policy**
  - [ ] Keep resurrection as the service supervisor and move ad hoc bootstrap
        policy out of direct kernel dependencies where possible.
        (`userspace/resurrection/resurrection.c`)
- [ ] **Environment policy**
  - [ ] Audit `envd` ownership / lifecycle behavior and keep environment
        management out of the kernel. (`userspace/envd/envd.c`)
- [ ] **Process-service split**
  - [ ] Document and implement the final split between kernel process mechanism
        and user-space process policy in `procd`, `resurrection`, and `nsd`.

## User-Space Driver / HAL Migration
- [ ] **PoW compliance**
  - [ ] Audit user-space drivers and services for PoW compliance and correct
        `liblux` usage.
- [ ] **HAL capability enforcement**
  - [ ] Implement `CAP_HW_IO` checks in the WASM host functions used by the HAL
        path.
- [ ] **Storage migration**
  - [ ] Port `devsd_hw.c` / storage-family logic into a user-space HAL module.
- [ ] **Broader driver migration**
  - [ ] Move remaining hardware families out of kernel one family at a time
        behind the diverter contract.
- [ ] **Rump / POSIX bridge**
  - [ ] Finish the remaining WASI / Rump POSIX bridge operations in
        `userspace/rump/rump_server.c`.

## Testing / Tooling
- [ ] Add targeted runtime tests for `procd`, `nsd`, and the HAL path instead of
      relying only on boot smoke tests.
- [ ] Add direct guest-side WASM runtime tests beyond the current smoke module.

## Documentation
- [ ] Update `SECURITY.md` with the Kinetic Defense IPC model.
- [ ] Mark `ghostdag_kernel.c` as deprecated / historical in comments.
