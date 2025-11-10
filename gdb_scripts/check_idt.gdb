# Check IDT entry for vector 0x46
set architecture i386:x86-64
target remote localhost:1234
symbol-file lux9.elf

# Get IDT location
printf "Reading IDTR...\n"

# We need to find where the IDT is. It's either at temp_idt or was moved.
# Let's check the temp_idt symbol
set $idt_addr = (unsigned long long)&temp_idt

printf "IDT at: %#llx\n", $idt_addr

# Each IDT entry is 16 bytes (128 bits) in long mode
# Entry for vector 0x46 is at idt_base + (0x46 * 16)
set $entry_addr = $idt_addr + (0x46 * 16)

printf "IDT[0x46] at: %#llx\n", $entry_addr

# Read the IDT entry (16 bytes)
printf "IDT[0x46] raw bytes:\n"
x/16bx $entry_addr

# Decode the IDT entry
# Format: offset_low:16 | selector:16 | IST:3 zeros:5 type:5 DPL:2 P:1 | offset_mid:16 | offset_high:32 | reserved:32
set $word0 = *(unsigned short*)($entry_addr + 0)
set $word1 = *(unsigned short*)($entry_addr + 2)
set $word2 = *(unsigned short*)($entry_addr + 4)
set $word3 = *(unsigned short*)($entry_addr + 6)
set $word4 = *(unsigned int*)($entry_addr + 8)
set $word5 = *(unsigned int*)($entry_addr + 12)

set $handler = ((unsigned long long)$word4 << 32) | ((unsigned long long)$word3 << 16) | $word0

printf "Handler address for vector 0x46: %#llx\n", $handler

# Disassemble at that address
printf "Disassembly at handler:\n"
x/5i $handler

# Compare with expected vectortable entry
set $vectortable_addr = (unsigned long long)&vectortable
set $expected = $vectortable_addr + (0x46 * 6)

printf "Expected (vectortable[0x46]): %#llx\n", $expected
printf "Actual handler: %#llx\n", $handler

if ($handler == $expected)
    printf "✓ IDT entry is CORRECT\n"
else
    printf "✗ IDT entry is WRONG! Off by %lld bytes\n", ($handler - $expected)
end

detach
quit
