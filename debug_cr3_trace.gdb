# Load symbols
file lux9.elf

# Connect to QEMU
target remote localhost:1234

# First, get to the point where trampoline is copied
# Set breakpoint right after memmove copies the trampoline
b mmu.c:495

printf "\n=== Starting debugging ===\n"
c

printf "\n=== Trampoline has been copied to 0x1000 ===\n"
p/x $rip

# Now set hardware breakpoint at 0x1000 where trampoline will execute
printf "\n=== Setting hardware breakpoint at 0x1000 ===\n"
hbreak *0x1000

# Continue until we hit the trampoline
printf "\n=== Continuing to trampoline ===\n"
c

printf "\n\n"
printf "============================================\n"
printf "=== AT TRAMPOLINE (0x1000) ===\n"
printf "============================================\n"
printf "RDI (new_cr3 phys) = "
p/x $rdi
printf "RSI (continuation VA) = "
p/x $rsi
printf "Current CR3 = "
p/x $cr3

# Disassemble trampoline
printf "\n=== TRAMPOLINE CODE ===\n"
x/10i $rip

# Execute: movq %rsi, %rax
printf "\n[STEP 1: movq %%rsi, %%rax]\n"
si
printf "  RIP = 0x%lx, RAX = 0x%lx\n", $rip, $rax

# Execute: cli
printf "\n[STEP 2: cli]\n"
si
printf "  RIP = 0x%lx\n", $rip

# Execute: movq %rdi, %cr3 - THE CRITICAL INSTRUCTION
printf "\n[STEP 3: movq %%rdi, %%cr3 - CR3 SWITCH!]\n"
si
printf "  RIP = 0x%lx\n", $rip
printf "  NEW CR3 = 0x%lx\n", $cr3

# Execute: jmp .Lafter_cr3
printf "\n[STEP 4: jmp .Lafter_cr3]\n"
si
printf "  RIP = 0x%lx\n", $rip

# About to execute: jmp *%rax
printf "\n"
printf "============================================\n"
printf "=== ABOUT TO JUMP TO CONTINUATION ===\n"
printf "============================================\n"
printf "Target address in RAX = 0x%lx\n", $rax

# Try to examine memory at continuation - this will fail if not mapped!
printf "\n=== Checking if continuation address is mapped ===\n"
set $can_read = 1
# Try to access the memory
python
try:
    gdb.execute("x/1gx $rax")
except gdb.MemoryError:
    gdb.execute("set $can_read = 0")
    print("ERROR: Cannot read memory at continuation address!")
end

# Execute: jmp *%rax
printf "\n[STEP 5: jmp *%%rax - JUMP TO CONTINUATION]\n"
si
printf "  RIP after jump = 0x%lx\n", $rip

# Did we get to the right place?
printf "\n"
printf "============================================\n"
printf "=== AFTER JUMP - WHERE ARE WE? ===\n"
printf "============================================\n"
x/10i $rip

# Check page tables
printf "\n"
printf "============================================\n"
printf "=== PAGE TABLE ANALYSIS ===\n"
printf "============================================\n"
set $pml4_phys = $cr3 & ~0xfff
printf "PML4 physical address: 0x%lx\n", $pml4_phys

# Access PML4 via HHDM (hopefully it's still mapped!)
set $hhdm = 0xffff800000000000
set $pml4_va = $hhdm + $pml4_phys

printf "\nPML4 entries:\n"
printf "  PML4[0] (0x0000000000000000-0x0000007fffffffff): "
x/1gx $pml4_va + 0*8
printf "  PML4[256] (HHDM 0xffff800000000000-): "
x/1gx $pml4_va + 256*8
printf "  PML4[511] (KZERO 0xffffffff80000000-): "
x/1gx $pml4_va + 511*8

# Calculate which PML4 entry continuation address uses
set $cont_pml4_idx = ($rax >> 39) & 0x1ff
printf "\nContinuation address 0x%lx uses PML4[%d]\n", $rax, $cont_pml4_idx

printf "\nQEMU memory map:\n"
monitor info mem

quit
