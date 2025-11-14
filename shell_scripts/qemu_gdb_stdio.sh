#!/bin/bash
set -euo pipefail

cd /home/scott/Repo/lux9-kernel

LOGFILE="/home/scott/Repo/lux9-kernel/qemu.log"

exec qemu-system-x86_64 \
  -M q35 \
  -m 2G \
  -cdrom lux9.iso \
  -boot d \
  -display none \
  -serial file:"$LOGFILE" \
  -no-reboot \
  -S \
  -gdb stdio
