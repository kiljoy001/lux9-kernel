# Load symbols from the kernel FIRST
file lux9.elf

# Connect to QEMU
target remote localhost:1234

# Set breakpoints
b setuppagetables
b cr3_switch_trampoline

# Continue to setuppagetables
c

printf "\n=== AT SETUPPAGETABLES ===\n"
p/x $rip
info symbol $rip

# Continue to trampoline at 0x1000
c

printf "\n=== AT TRAMPOLINE (0x1000) ===\n"
printf "RDI (new_cr3) = "
p/x $rdi
printf "RSI (continuation) = "
p/x $rsi
printf "Current CR3 = "
p/x $cr3

# Disassemble trampoline
printf "\n=== TRAMPOLINE CODE ===\n"
x/10i $rip

# Step through trampoline instruction by instruction
printf "\n=== STEP 1: movq %%rsi, %%rax ===\n"
si
p/x $rip
p/x $rax

printf "\n=== STEP 2: cli ===\n"
si
p/x $rip

printf "\n=== STEP 3: movq %%rdi, %%cr3 (THE CRITICAL CR3 SWITCH) ===\n"
si
p/x $rip
printf "New CR3 = "
p/x $cr3

printf "\n=== STEP 4: jmp .Lafter_cr3 ===\n"
si
p/x $rip

printf "\n=== STEP 5: jmp *%%rax (JUMP TO CONTINUATION) ===\n"
printf "About to jump to: "
p/x $rax

# Check if the continuation address is mapped in new page tables
printf "\n=== CHECKING IF CONTINUATION IS MAPPED ===\n"
printf "Trying to read memory at continuation address...\n"
x/1gx $rax

printf "\n=== PERFORMING THE JUMP ===\n"
si
printf "RIP after jump = "
p/x $rip

# Disassemble where we landed
printf "\n=== CODE AT CURRENT RIP ===\n"
x/10i $rip

printf "\n=== QEMU MEMORY MAP ===\n"
monitor info mem

printf "\n=== DUMP PML4 ENTRIES ===\n"
set $pml4_phys = $cr3 & ~0xfff
printf "PML4 physical address: 0x%lx\n", $pml4_phys
# We need to access via HHDM
set $hhdm = 0xffff800000000000
set $pml4 = $hhdm + $pml4_phys
printf "PML4[0] (identity): "
x/1gx $pml4 + 0*8
printf "PML4[256] (HHDM): "
x/1gx $pml4 + 256*8
printf "PML4[511] (KZERO): "
x/1gx $pml4 + 511*8

quit
