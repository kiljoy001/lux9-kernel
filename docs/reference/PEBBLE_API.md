# Pebble API Reference

The Pebble primitives are now available in the Lux 9 kernel behind the `pebble_enabled`
runtime flag (enabled by default; pass `pebble=0` on the kernel command line to
disable).  Processes interact with Pebble by issuing white capabilities, verifying
them, allocating black handles, and—when required—acquiring red snapshots of blue
objects.  This document captures the concrete syscall surface and the invariants
the kernel enforces today.

## Syscall Surface

| Syscall | Prototype | Purpose |
| --- | --- | --- |
| `PEBBLE_WHITE_ISSUE` | `int pebble_white_issue(ulong size, PebbleWhite **tokenp);` | Mint a white token authorising up to `size` bytes of black allocation. The token pointer is opaque and must be passed unchanged to `pebble_white_verify`. |
| `PEBBLE_WHITE_VERIFY` | `int pebble_white_verify(PebbleWhite *white, void **black_hint);` | Validate a white token and transfer its budget to the caller. `black_hint` receives the white token’s `data_ptr` (currently `nil`). |
| `PEBBLE_BLACK_ALLOC` | `int pebble_black_alloc(uintptr size, void **handle);` | Consume one verified white token, allocate a zeroed kernel buffer of `size`, and return the opaque black handle. |
| `PEBBLE_BLACK_FREE` | `int pebble_black_free(void *handle);` | Release a black handle once its blue/red lifecycle has completed. |
| `PEBBLE_RED_COPY` | `int pebble_red_copy(PebbleBlue *blue, PebbleRed **redp);` | Materialise (or return an existing) red snapshot for a blue object created alongside a black allocation. |
| `PEBBLE_BLUE_DISCARD` | `int pebble_blue_discard(PebbleBlue *blue);` | Drop a speculative blue object after a red copy exists. |

### Invariants

* White tokens carry the byte budget for upcoming black allocations.  Verification
  increases a per-process counter (`white_verified`) and adds the authorised size
  to `white_pending`.  Each successful `pebble_black_alloc` consumes one verified
  token and deducts the allocation size from the pending byte total.
* If a black allocation fails after the token is consumed, the kernel rolls back
  both the budget and the verification state to keep accounting correct.
* Blue objects must never be discarded before a matching red copy exists; the
  kernel enforces this in `pebble_blue_discard`.
* Black frees are blocked while a live blue (without a red snapshot) remains.

### Errors

All entry points return `0` on success and `-1` on failure, setting `up->errstr`
to one of:

* `PEBBLE_E_PERM` – capability or state violation (token missing, pebble disabled).
* `PEBBLE_E_BADARG` – nil pointer or size outside `[64 B, 1 GiB]`.
* `PEBBLE_E_AGAIN` – insufficient token budget or all white slots exhausted.
* `PEBBLE_E_BUSY` – attempt to discard blue/black while snapshots are outstanding.
* `PEBBLE_E_NOMEM` – allocation failure inside the pebble allocator.

## Kernel Self-Test

During bootstrap the first kernel process (`proc0`) now runs `pebble_selftest`.
The sequence:

1. Issues a 64 B white token (`PEBBLE_MIN_ALLOC`).
2. Verifies the token, performs a black allocation, and obtains the paired blue handle.
3. Forces a red snapshot, discards the blue side, and frees the black handle.

Success emits `PEBBLE: selftest PASS` on the serial console; failures print the
kernel error string.  The helper honours the runtime flag, so it is skipped when
`pebble=0`.

`shell_scripts/run_test.sh` automates the check by rebuilding the kernel, running
it under QEMU, and grepping `qemu.log` for the PASS marker.

## `/dev/sip/issue`

White tokens are available over the new `/dev/sip/issue` control file.  Writing a
decimal or hexadecimal byte count (e.g. `echo 4096 >/dev/sip/issue`) mints a white
token and a subsequent read returns the token pointer as hexadecimal text:

```
; echo 4096 >/dev/sip/issue
; cat /dev/sip/issue
0xffffffff80abc000
```

The channel exposes one-shot semantics—each write produces a single token result
that disappears after it is read.  Kernel debug builds also log the issuance when
`pebbledebug=1`.

## Capability Integration

* `/dev/mem` and `/dev/pci` retain their existing capability gates (`CapDeviceAccess`,
  `CapIOPort`, `CapPCI`).  Pebble does not yet auto-mint white tokens; user space
  should call `pebble_white_issue` explicitly before requesting a black allocation.
* Processes without the relevant capability bits still receive `Eperm` at the
  device layer; pebble accounting remains per-process and unaffected by device opens.

## Using the API from User Space

The short-term wrapper plan is:

1. Create an argument block (`Sargs`) on the user stack.
2. Store the syscall number in `%rbp` and execute the `syscall` instruction.
3. Provide thin C helpers:

```c
void *pebble_issue_white(ulong size)
{
	PebbleWhite *token;
	if(syscall(PEBBLE_WHITE_ISSUE, size, &token) < 0)
		return nil;
	return token;
}
```

A complete userland shim will live under `userspace/lib/` once the SYSRET path
is solid.  Until then, the kernel self-test plus QEMU harness provide regression
coverage for the allocator and snapshot logic.

## Future Work

* Surface successful white issuance through `/dev/sip` so 9P servers can hand
  tokens to drivers without invoking syscalls directly.
* Extend the self-test to exercise multiple concurrent tokens and red/blue error
  paths once multi-core scheduling stabilises.
* Add Go bindings (planned `userspace/go-servers/sip/pebble.go`) once the syscall
  ABI is nailed down.
