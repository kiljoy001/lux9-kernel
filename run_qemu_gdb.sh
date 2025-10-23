#!/bin/bash
cd /home/scott/Repo/lux9-kernel
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -serial stdio -no-reboot -s -S