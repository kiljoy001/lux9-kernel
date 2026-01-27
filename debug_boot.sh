#!/bin/bash
qemu-system-x86_64 -cdrom lux9.iso -m 512M -serial stdio -display none -no-reboot -d int,cpu_reset -D /tmp/qemu_int.log > boot_capture.log 2>&1 &
PID=$!
sleep 15
kill -9 $PID

