# Debug script to check memory state before CR3 switch

echo === PRE-CR3 SWITCH MEMORY CHECK ===\n

# Check if we can access key memory regions
echo Testing HHDM access at 0 (first physical page)...\n
echo x/10i 0x1000 (trampoline location in low memory)...\n
x/10i 0x1000

echo \nTesting HHDM base address...\n
print hhdm_base

echo \nChecking kernel relocation area (2MB)...\n
echo x/20i 0x200000\n
x/20i 0x200000

echo \nChecking current kernel location...\n
print &end
print (u64int)kend - KZERO  # kernel size

echo \n=== PAGE TABLE CONTENTS BEFORE SWITCH ===\n
echo Current PML4 entries...\n
print m->pml4[0]    # Identity mapping
print m->pml4[256]  # KZERO mapping  
print m->pml4[511]  # HHDM mapping

echo \nChecking if trampoline was copied correctly...\\n
echo Memory at 0x1000 (should be trampoline):\n
x/10xb 0x1000

echo \n=== CRITICAL CHECK:continuation address validity ===\n
print continuation
echo If this is not in KZERO range, that's the problem!\n

echo \n=== MEMORY LAYOUT CHECK ===\n
echo Physical memory regions:\n
for i=0; i<3; i++
  print conf.mem[i].base
  print conf.mem[i].npage
end

echo \nIf you see issues above, the CR3 switch will fail!\n