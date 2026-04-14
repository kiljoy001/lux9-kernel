# Lux9 Kernel

Lux9 is a formally verified microkernel for ARM64 processors (and x86_64) built from scratch to demonstrate how we can stop memory-safety attacks at the driver level.

It integrates:
*   **GHOSTDAG Consensus** for non-cryptographic BFT ordering.
*   **WASM Runtime** (wasm3) running in kernel-space for drivers and apps, isolated by software fault isolation (SFI) and formal verification.
*   **Formal Verification** using Coq and Frama-C.
*   **Rump Kernel** userspace for POSIX compatibility.
