#!/bin/bash
rm -f /tmp/debug_boot.log
qemu-system-x86_64 -cdrom lux9.iso -m 512M -serial file:/tmp/debug_boot.log -display none -no-reboot -d int,cpu_reset -D /tmp/qemu_int.log &
PID=$!
sleep 60
kill $PID
