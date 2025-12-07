#!/bin/bash
set -e
# Build raw assembly init as Plan 9 binary

# 1. Compile and Link to ELF
gcc -c init_raw.S -o init_raw.o
ld -Ttext=0x200020 --entry=_start -o init_raw.elf init_raw.o

# 2. Extract binary text
objcopy -O binary -j .text init_raw.elf init_raw.text
objcopy -O binary -j .data init_raw.elf init_raw.data 2>/dev/null || touch init_raw.data

TEXT_SIZE=$(stat -c%s init_raw.text)
DATA_SIZE=$(stat -c%s init_raw.data)

# 3. Create Plan 9 Header (32 bytes)
# Magic (4), Text(4), Data(4), Bss(4), Syms(4), Entry(4), Spsz(4), Gpsz(4)
# Amd64 Magic: 0x8a97 (Little Endian: 97 8A 00 00)
# Entry: 0x200020 + 32 (header) ? No, entry is virtual address. 0x200020.
# Text starts after header. So Load address is 0x200020.
# Wait, if entry is 0x200020, and header is not loaded?
# Plan 9: Header is part of file, but not mapped?
# sysexec skips header?
# sysexec reads header.
# sysexec maps text at entry?
# If entry is 0x200020.
# I'll assume 0x200020.

python3 -c "import sys, struct; sys.stdout.buffer.write(b'\x97\x8a\x00\x00' + struct.pack('<I', $TEXT_SIZE) + struct.pack('<I', $DATA_SIZE) + b'\x00'*4 + b'\x00'*4 + struct.pack('<I', 0x200020) + b'\x00'*8)" > header

# 4. Concatenate
cat header init_raw.text init_raw.data > init

# Cleanup
rm init_raw.o init_raw.elf init_raw.text init_raw.data header
echo "Built Plan 9 raw init: init"
