# 9front Microkernel Build System
KERNEL := lux9.elf

CC := gcc
LD := ld
AS := as

# Internal GCC headers (stdarg.h, stdbool.h, etc.)
GCC_INC := $(shell $(CC) -print-file-name=include)

# Compiler flags - Plan 9 compatible with MAXIMUM SAFETY
# Phase 7: Security hardening enabled
CFLAGS := -Wall -Wextra -Wno-unused -Wno-unknown-pragmas -Wno-builtin-declaration-mismatch -Wno-discarded-qualifiers -Wno-missing-braces -Wno-incompatible-pointer-types -std=gnu11 \
           -O0 -g3 -gdwarf-4 \
           -ffreestanding -fno-stack-protector -fno-stack-check \
           -fno-lto -fno-pie -no-pie -fno-pic \
           -m64 -march=x86-64 -mcmodel=kernel \
           -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
           -nostdinc -I$(GCC_INC) \
           -Ikernel/include \
           -Ikernel/crypto \
           -Ikernel/clr/libmcu-cbor \
           -Iport \
           -I. \
           -D_PLAN9_SOURCE \
           -D__PLAN9_KERNEL__ \
           -DKERNEL \
           -D_KERNEL_QBE \
           -DUSE_PEBBLE_ALLOC=1 \
           -DKTZERO=0xffffffff80110000 \
           -fplan9-extensions -nostdlib -fno-builtin -fno-omit-frame-pointer \
           -Wformat-security -Wconversion -Wshadow

# Linker flags
# Phase 7: Security hardening - DEP/NX enabled
LDFLAGS := -m elf_x86_64 -nostdlib -static -no-pie --no-dynamic-linker \
           -z max-page-size=0x1000 \
           -z noexecstack \
           -T kernel/linker.ld

# Source files
PORT_C := $(wildcard kernel/9front-port/*.c)
# Filter out conflicting/duplicate files
PORT_C := $(filter-out kernel/9front-port/rbtree.c, $(PORT_C))

# Ensure TPM drivers are included
TPM_C := kernel/9front-port/tpm2_driver.c kernel/9front-port/tpm2_sapi_minimal.c
PC64_C := $(wildcard kernel/9front-pc64/*.c)
LIBC_C := $(wildcard kernel/libc9/*.c)
MEMDRAW_C := $(wildcard kernel/libmemdraw/*.c)
FAMILY_C := $(wildcard kernel/family/*.c)
CRYPTO_C := $(wildcard kernel/crypto/*.c)
BORROW_C := kernel/borrowchecker.c
LOCKDAG_C := kernel/lock_dag.c
PROCSTATEDAG_C := kernel/proc_state_dag.c
PROCFSM_C := kernel/proc_fsm.c
P9ROUTER_C := kernel/9p_router.c
# SYSCALL9P_C removed - Phase 6: TRUE syscall elimination via exchange page doorbell
# GHOSTDAG renamed to msgord - see MSGORD_C below
MSGORD_C := kernel/msgord.c
CONSENSUS_DEPTH_C := kernel/consensus_depth.c
REAL_DRIVERS_C := $(wildcard real_drivers/*.c)
PEBBLE_C := kernel/pebble.c
POW_GATE_C := kernel/pow_gate.c
BENCHMARK_C := kernel/benchmark.c
CBOR_C := kernel/clr/libmcu-cbor/common.c kernel/clr/libmcu-cbor/decoder.c kernel/clr/libmcu-cbor/encoder.c kernel/clr/libmcu-cbor/parser.c
CLR_C := kernel/clr/fruity/fruity_ir.c kernel/clr/fruity/fruity_to_qbe.c kernel/clr/fruity/fruity_cbor.c kernel/clr/fruity/qbe_buffer.c kernel/clr/qbe_compile.c kernel/clr/qbe/kernel_compat.c kernel/clr/qbe/exchange_io.c kernel/clr/qbe/clr_p9_internal.c kernel/clr/qbe/clr_core.c kernel/clr/qbe/clr_console.c kernel/clr/qbe/clr_bcl_helpers.c kernel/clr/clr_exchange_ops.c kernel/clr/qbe/amd64/targ.c kernel/clr/qbe/qbe_globals.c kernel/clr/clr_runtime.c kernel/clr/il_parser.c kernel/clr/il_to_fruity.c kernel/clr/clr-kernel/clr_pebble_integration.c $(CBOR_C)

# TPM2-TSS sources - REMOVED, using minimal SAPI instead
# TPM2_MU_C := $(wildcard kernel/tpm2-tss/mu/*.c)
# TPM2_SAPI_C := $(wildcard kernel/tpm2-tss/sapi/*.c) $(wildcard kernel/tpm2-tss/sapi/api/*.c)
# TPM2_TCTI_C := kernel/tpm2-tss/tcti_kernel.c
# TPM2_TSS_C := $(TPM2_MU_C) $(TPM2_SAPI_C) $(TPM2_TCTI_C)

# QBE compiler core sources (for qbe.a)
QBE_CORE_C := kernel/clr/qbe/alias.c kernel/clr/qbe/cfg.c kernel/clr/qbe/copy.c kernel/clr/qbe/fold.c kernel/clr/qbe/gas.c kernel/clr/qbe/live.c kernel/clr/qbe/load.c kernel/clr/qbe/mem.c kernel/clr/qbe/parse.c kernel/clr/qbe/rega.c kernel/clr/qbe/spill.c kernel/clr/qbe/ssa.c kernel/clr/qbe/util.c kernel/clr/qbe/amd64/emit.c kernel/clr/qbe/amd64/isel.c kernel/clr/qbe/amd64/sysv.c
QBE_CORE_O := $(QBE_CORE_C:.c=.o)

# SD/FIS support files already included by wildcard above

# Assembly files
ASM_S := kernel/9front-pc64/l.S kernel/9front-pc64/entry.S kernel/crypto/hwcrypto.S

# Object files
PORT_O := $(PORT_C:.c=.o)
PC64_O := $(PC64_C:.c=.o)
LIBC_O := $(LIBC_C:.c=.o)
FAMILY_O := $(FAMILY_C:.c=.o)
CRYPTO_O := $(CRYPTO_C:.c=.o)
ASM_O := $(ASM_S:.S=.o)

MEMDRAW_O := $(MEMDRAW_C:.c=.o)
BORROW_O := $(BORROW_C:.c=.o)
LOCKDAG_O := $(LOCKDAG_C:.c=.o)
PROCSTATEDAG_O := $(PROCSTATEDAG_C:.c=.o)
PROCFSM_O := $(PROCFSM_C:.c=.o)
P9ROUTER_O := $(P9ROUTER_C:.c=.o)
# SYSCALL9P_O removed - Phase 6 pure 9P via doorbell
# GHOSTDAG_O removed - using MSGORD_O
MSGORD_O := $(MSGORD_C:.c=.o)
CONSENSUS_DEPTH_O := $(CONSENSUS_DEPTH_C:.c=.o)
REAL_DRIVERS_O := $(REAL_DRIVERS_C:.c=.o)
PEBBLE_O := $(PEBBLE_C:.c=.o)
POW_GATE_O := $(POW_GATE_C:.c=.o)
BENCHMARK_O := $(BENCHMARK_C:.c=.o)
CLR_O := $(CLR_C:.c=.o)
# TPM2_TSS_O := $(TPM2_TSS_C:.c=.o)  # Removed - using minimal SAPI

# External archives
QBE_A := kernel/clr/qbe/qbe.a

# QBE_GHOSTDAG_O removed - renamed to msgord

ALL_O := $(ASM_O) $(PORT_O) $(PC64_O) $(LIBC_O) $(FAMILY_O) $(CRYPTO_O) $(MEMDRAW_O) $(BORROW_O) $(PEBBLE_O) $(POW_GATE_O) $(BENCHMARK_O) $(REAL_DRIVERS_O) $(LOCKDAG_O) $(PROCSTATEDAG_O) $(PROCFSM_O) $(P9ROUTER_O) $(MSGORD_O) $(CONSENSUS_DEPTH_O) $(CLR_O) $(QBE_A)
# TPM already included in PORT_O

.PHONY: all clean count iso run help

all: $(KERNEL)

$(KERNEL): $(ALL_O)
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) $(ALL_O) -o $@
	@echo "Build complete: $(KERNEL)"
	@ls -lh $(KERNEL)

# Build QBE static library
$(QBE_A): $(QBE_CORE_O)
	@echo "AR $@"
	@ar rcs $@ $(QBE_CORE_O)

# QBE needs SSE for floating point and doesn't use GNU extensions
kernel/clr/qbe/%.o: kernel/clr/qbe/%.c
	@echo "CC $< (QBE)"
	@$(CC) $(filter-out -mno-sse -mno-sse2 -std=gnu11,$(CFLAGS)) -std=c11 -msse -msse2 -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

kernel/clr/qbe/amd64/%.o: kernel/clr/qbe/amd64/%.c
	@echo "CC $< (QBE)"
	@$(CC) $(filter-out -mno-sse -mno-sse2 -std=gnu11,$(CFLAGS)) -std=c11 -msse -msse2 -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

# CBOR library needs special flags
kernel/clr/libmcu-cbor/%.o: kernel/clr/libmcu-cbor/%.c
	@echo "CC $< (CBOR)"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

%.o: %.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_O) $(CLR_O) $(KERNEL)
	rm -f kernel/clr/qbe/qbe.a kernel/clr/qbe/**/*.o
	rm -rf iso_root lux9.iso

count:
	@echo "=== Line counts ==="
	@echo "Port layer:"
	@wc -l kernel/9front-port/*.c 2>/dev/null | tail -1
	@echo "Platform (pc64):"
	@wc -l kernel/9front-pc64/*.c 2>/dev/null | tail -1
	@echo "LibC:"
	@wc -l kernel/libc9/*.c 2>/dev/null | tail -1
	@echo "Total C code:"
	@find kernel -name "*.c" | xargs wc -l 2>/dev/null | tail -1

list:
	@echo "Source files to compile:"
	@echo "Port: $(words $(PORT_C)) files"
	@echo "PC64: $(words $(PC64_C)) files"
	@echo "LibC: $(words $(LIBC_C)) files"
	@echo "Total: $(words $(ALL_O)) objects"

test-build:
	@echo "Testing compilation of first file..."
	$(CC) $(CFLAGS) -c kernel/9front-port/alloc.c -o /tmp/test.o
	@echo "✓ Basic compilation works!"

userspace/build/initrd.tar:
	@echo "Building userspace..."
	@$(MAKE) -C userspace initrd

iso: $(KERNEL) userspace/build/initrd.tar
	@echo "Creating ISO image..."
	@echo "Checking for xorriso..."
	@which xorriso > /dev/null || (echo "Error: xorriso not found. Please install xorriso package." && exit 1)
	@echo "Setting up directory structure..."
	@rm -rf iso_root
	@mkdir -p iso_root/boot iso_root/boot/limine iso_root/EFI/BOOT
	@echo "Copying kernel and userspace..."
	@cp $(KERNEL) iso_root/boot/
	@cp userspace/build/initrd.tar iso_root/boot/
	@cp limine.conf iso_root/boot/limine/
	@echo "Checking for Limine bootloader files..."
	@[ -f boot/limine/bin/limine-bios.sys ] && echo "Found local Limine files" || echo "No local Limine found"
	@cp boot/limine/bin/limine-bios.sys iso_root/boot/limine/ 2>/dev/null || echo "No limine-bios.sys found"
	@cp boot/limine/bin/limine-bios-cd.bin iso_root/boot/limine/ 2>/dev/null || echo "No limine-bios-cd.bin found"
	@cp boot/limine/bin/limine-uefi-cd.bin iso_root/boot/limine/ 2>/dev/null || echo "No limine-uefi-cd.bin found"
	@cp boot/limine/bin/BOOTX64.EFI iso_root/EFI/BOOT/ 2>/dev/null || echo "No BOOTX64.EFI found"
	@echo "Creating ISO with xorriso..."
	@xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o lux9.iso
	@echo "Installing boot loader..."
	@if [ -f boot/limine/bin/limine ]; then \
		echo "Using local limine binary"; \
		if boot/limine/bin/limine bios-install lux9.iso; then \
			echo "✓ BIOS bootloader installed successfully"; \
		else \
			echo "⚠ BIOS bootloader installation failed (exit code $$?)"; \
			echo "  ISO will only boot via UEFI"; \
		fi; \
	elif command -v limine > /dev/null 2>&1; then \
		echo "Using system limine binary"; \
		if limine bios-install lux9.iso; then \
			echo "✓ BIOS bootloader installed successfully"; \
		else \
			echo "⚠ BIOS bootloader installation failed (exit code $$?)"; \
			echo "  ISO will only boot via UEFI"; \
		fi; \
	else \
		echo "⚠ No limine binary found - skipping BIOS bootloader installation"; \
		echo "  ISO will only boot via UEFI"; \
	fi
	@rm -rf iso_root
	@echo "✓ Created lux9.iso"

run: $(KERNEL)
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 not found. Please install qemu package." && exit 1)
	qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL) -no-reboot -display none -serial file:qemu.log

direct-run: $(KERNEL)
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 not found. Please install qemu package." && exit 1)
	qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL) -no-reboot -display none -serial stdio

help:
	@echo "Lux9 Build System (GNUmakefile):"
	@echo "  make          - Build kernel only"
	@echo "  make all      - Build kernel only"
	@echo "  make iso      - Build complete bootable ISO (RECOMMENDED)"
	@echo "  make clean    - Clean all build artifacts"
	@echo "  make run      - Run kernel in QEMU (with logging)"
	@echo "  make direct-run - Run kernel in QEMU (direct stdio)"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Note: The Makefile in root directory provides additional targets"
	@echo "Run 'make -f Makefile help' for more detailed build options"
