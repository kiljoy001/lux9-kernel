# ADR: Abandoning Strict Plan 9 ABI Compatibility

## Status
Accepted

## Context
The Lux9 kernel was originally designed to be compatible with Plan 9 userland. However, the integration of advanced security features (Pebble, Borrow Checker, Blind Ledger) and the adoption of a 9P-centric architecture have created significant friction with the legacy Plan 9 ABI (register-based syscalls, a.out formats).

## Decision
We will **abandon strict Plan 9 ABI compatibility** as a primary goal.

## Consequences
1.  **Native ABI**: The primary interface is the **9P Exchange Protocol** (`Tsys*` messages in shared memory).
2.  **Legacy Support**: Support for standard Plan 9 binaries is deprecated/best-effort.
3.  **Security**: This allows us to enforce strict capability checks on structured messages rather than inspecting raw register state.
4.  **Simplicity**: Removing the "Compat Layer" complexity allows us to focus on the robust Native path.
