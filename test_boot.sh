#!/bin/bash
# Simple boot test script
cd /home/scott/Repo/lux9-kernel
/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 \
  -kernel lux9.elf \
  -m 2G \
  -nographic \
  -no-reboot
