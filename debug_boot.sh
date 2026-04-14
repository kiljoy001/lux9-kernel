#!/bin/bash
qemu-system-x86_64 -cdrom os/lux9.iso -m 512M -serial file:boot_capture.log -display none -no-reboot -d int,cpu_reset -D /tmp/qemu_int.log &
PID=$!
sleep 180
kill $PID
sleep 2
kill -9 $PID
