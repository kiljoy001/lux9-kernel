#!/usr/bin/env bash

# Launch QEMU with a gdb stub on localhost:1234 and the kernel halted.
# Serial output goes to the current terminal; press Ctrl+C to stop QEMU.

set -euo pipefail

qemu-system-x86_64 \
  -M q35 \
  -m 2G \
  -smp 1 \
  -cdrom lux9.iso \
  -boot d \
  -serial stdio \
  -display none \
  -no-reboot \
  -s \
  -S
