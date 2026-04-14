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
           -Iport \
           -I. \
           -D_PLAN9_SOURCE \
           -D__PLAN9_KERNEL__ \
           -DKERNEL \
           -D_KERNEL_QBE \
           -DUSE_PEBBLE_ALLOC=1 \
           -DKTZERO=0xffffffff80110000 \
           -fplan9-extensions -nostdlib -fno-builtin -fno-omit-frame-pointer \
           -Wformat-security -Wconversion -Wshadow \
           -fcf-protection=none

# Linker flags
# Phase 7: Security hardening - DEP/NX enabled
LDFLAGS := -m elf_x86_64 -nostdlib -static -no-pie --no-dynamic-linker \
           -z max-page-size=0x1000 \
           -z noexecstack \
           -T kernel/linker.ld

LIBGCC := $(shell $(CC) -print-libgcc-file-name)

# Source files
PORT_C := $(wildcard kernel/9front-port/*.c)
# Filter out conflicting/duplicate files
PORT_C := $(filter-out kernel/9front-port/rbtree.c kernel/9front-port/devsym.c, $(PORT_C))

# Ensure TPM drivers are included
TPM_C := kernel/9front-port/tpm2_driver.c kernel/9front-port/tpm2_sapi_minimal.c
PC64_C := $(wildcard kernel/9front-pc64/*.c)
LIBC_C := $(wildcard kernel/libc9/*.c)
MEMDRAW_C := $(wildcard kernel/libmemdraw/*.c)
FAMILY_C := $(wildcard kernel/family/*.c)
CRYPTO_C := $(wildcard kernel/crypto/*.c)
BORROW_C := kernel/borrowchecker.c kernel/borrow_enforce.c
LOCKDAG_C := kernel/lock_dag.c
PROCSTATEDAG_C := kernel/proc_state_dag.c
PROCFSM_C := kernel/proc_fsm.c
P9ROUTER_C := kernel/router/core.c kernel/router/fs.c kernel/router/proc.c kernel/router/ipc.c kernel/router/wasm.c kernel/router/doorbell.c kernel/router/srv.c kernel/router/srv_compat.c
MNTDRIVER_C := userspace/ns/nsd/mnt_driver.c
# SYSCALL9P_C removed - Phase 6: TRUE syscall elimination via exchange page doorbell
# GHOSTDAG renamed to msgord - see MSGORD_C below
MSGORD_C := kernel/msgord.c
CONSENSUS_DEPTH_C := kernel/consensus_depth.c
REAL_DRIVERS_C := $(wildcard real_drivers/*.c)
PEBBLE_C := kernel/pebble.c kernel/pebble_kernel.c kernel/distributed_pebble.c
EXCHANGE_POOL_C := kernel/exchange_pool.c kernel/exchange_pool_ipc.c
POW_GATE_C := kernel/pow_gate.c
BENCHMARK_C := kernel/benchmark.c
CAPABILITY_C := kernel/capability/lux_capability.c
# mini-gmp wrapper for symbolic math
SYMBOLIC_C := kernel/symbolic/minigmp_kernel.c
BPRINT_C := kernel/bprint.c

# CLR removed - archived in old_clr_pipeline/


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
MNTDRIVER_O := $(MNTDRIVER_C:.c=.o)
# SYSCALL9P_O removed - Phase 6 pure 9P via doorbell
# GHOSTDAG_O removed - using MSGORD_O
MSGORD_O := $(MSGORD_C:.c=.o)
CONSENSUS_DEPTH_O := $(CONSENSUS_DEPTH_C:.c=.o)
REAL_DRIVERS_O := $(REAL_DRIVERS_C:.c=.o)
UUID_O := kernel/lib/uuid.o
PEBBLE_O := $(PEBBLE_C:.c=.o)
EXCHANGE_POOL_O := $(EXCHANGE_POOL_C:.c=.o)
POW_GATE_O := $(POW_GATE_C:.c=.o)
BENCHMARK_O := $(BENCHMARK_C:.c=.o)
CAPABILITY_O := $(CAPABILITY_C:.c=.o)
WASM3_C := $(filter-out kernel/wasm/wasm_runtime/wasm3/m3_api_libc.c, $(wildcard kernel/wasm/wasm_runtime/wasm3/*.c))
WASM_FILESERVER_C := kernel/wasm/wasm_runtime.c kernel/wasm/wasm_fileserver.c kernel/wasm/wasm_9p_integration.c kernel/wasm/wasm_capability_bindings.c kernel/wasm/wasi_lux9_shim.c kernel/wasm/wasm_host_lux9.c kernel/wasm/wasm_arena.c
WASM_C := $(WASM3_C) $(WASM_FILESERVER_C)
WASM_O := $(WASM_C:.c=.o)
SYMBOLIC_O := $(SYMBOLIC_C:.c=.o)
BPRINT_O := $(BPRINT_C:.c=.o)
# TPM2_TSS_O := $(TPM2_TSS_C:.c=.o)  # Removed - using minimal SAPI

# QBE_GHOSTDAG_O removed - renamed to msgord

ALL_O := $(ASM_O) $(PORT_O) $(PC64_O) $(LIBC_O) $(FAMILY_O) $(CRYPTO_O) $(MEMDRAW_O) $(BORROW_O) $(PEBBLE_O) $(EXCHANGE_POOL_O) $(POW_GATE_O) $(BENCHMARK_O) $(CAPABILITY_O) $(REAL_DRIVERS_O) $(LOCKDAG_O) $(PROCSTATEDAG_O) $(PROCFSM_O) $(P9ROUTER_O) $(MNTDRIVER_O) $(MSGORD_O) $(CONSENSUS_DEPTH_O) $(WASM_O) $(SYMBOLIC_O) $(BPRINT_O) $(UUID_O) kernel/kconf.o
# TPM already included in PORT_O

.PHONY: all clean count iso run kunit test help userspace-all

all: $(KERNEL)

$(KERNEL): $(ALL_O)
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) $(ALL_O) $(LIBGCC) -o $@
	@echo "Build complete: $(KERNEL)"
	@ls -lh $(KERNEL)

# Build QBE static library
$(QBE_A): $(QBE_CORE_O)
	@echo "AR $@"
	@ar rcs $@ $(QBE_CORE_O)

# WASM3 Runtime build - use WASM3's compatibility headers
kernel/wasm/wasm_runtime/wasm3/%.o: kernel/wasm/wasm_runtime/wasm3/%.c
	@echo "CC $< (WASM3)"
	@$(CC) $(CFLAGS) -msse -msse2 -Wno-conversion -Wno-sign-conversion -Wno-shadow -Wno-macro-redefined -Dd_m3HasFloat=0 -Dd_m3HasSIMD=0 -Ikernel/wasm/wasm_runtime/wasm3/include -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -include kernel/include/dat.h -c $< -o $@

# mini-gmp wrapper for symbolic math
kernel/symbolic/minigmp_kernel.o: kernel/symbolic/minigmp_kernel.c
	@echo "CC $< (minigmp)"
	@$(CC) $(CFLAGS) -Wno-conversion -Wno-sign-conversion -Wno-sign-compare -Wno-unused-function -Wno-shadow -DMINI_GMP_LIMB_TYPE="long" -include kernel/include/u.h -include kernel/include/mem.h -c $< -o $@

# WASM file server code also needs WASM3 headers
kernel/wasm/%.o: kernel/wasm/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -msse -msse2 -Wno-shadow -Ikernel/wasm/wasm_runtime/wasm3/include -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

# Relax warnings for 9p_router.c due to extensive use of mixed integer types
kernel/9p_router.o: kernel/9p_router.c
	@echo "CC $< (Relaxed)"
	@$(CC) $(CFLAGS) -Wno-conversion -Wno-sign-conversion -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

# Relax warnings for aml.c (ACPI bytecode interpreter is legacy code)
kernel/9front-pc64/aml.o: kernel/9front-pc64/aml.c
	@echo "CC $< (Relaxed)"
	@$(CC) $(CFLAGS) -Wno-conversion -Wno-sign-conversion -Wno-shadow -Wno-parentheses -Wno-implicit-fallthrough -Wno-sign-compare -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -include kernel/include/u.h -include kernel/include/portlib.h -include kernel/include/mem.h -c $< -o $@

%.o: %.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_O) $(UUID_O) $(KERNEL)
	rm -rf iso_root lux9.iso
	@$(MAKE) -C userspace clean

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

userspace-all:
	@echo "Building userspace..."
	@$(MAKE) -C userspace
	@echo "Copying initrd to boot/..."
	@mkdir -p boot
	@cp userspace/build/initrd.tar boot/initrd.tar
	@echo "✓ initrd.tar copied to boot/"

userspace/build/initrd.tar: userspace-all
	@:

iso: $(KERNEL) userspace-all
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
	@echo "Applying optional Limine BIOS install patch..."
	@if [ -x boot/limine/bin/limine ]; then \
		echo "Using local limine binary"; \
		if boot/limine/bin/limine bios-install lux9.iso; then \
			echo "✓ BIOS bootloader installed successfully"; \
		else \
			echo "⚠ limine bios-install failed (exit code $$?)"; \
			echo "  ISO still has BIOS+UEFI El Torito entries for CD boot"; \
			echo "  bios-install is only needed for some raw-disk/USB BIOS paths"; \
		fi; \
	elif command -v limine > /dev/null 2>&1; then \
		echo "Using system limine binary"; \
		if limine bios-install lux9.iso; then \
			echo "✓ BIOS bootloader installed successfully"; \
		else \
			echo "⚠ limine bios-install failed (exit code $$?)"; \
			echo "  ISO still has BIOS+UEFI El Torito entries for CD boot"; \
			echo "  bios-install is only needed for some raw-disk/USB BIOS paths"; \
		fi; \
	else \
		echo "ℹ No limine installer binary found; skipping optional bios-install step"; \
		echo "  ISO still has BIOS+UEFI El Torito entries for CD boot"; \
	fi
	@rm -rf iso_root
	@echo "✓ Created lux9.iso"

run: $(KERNEL)
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 not found. Please install qemu package." && exit 1)
	qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL) -no-reboot -display none -serial file:qemu.log

direct-run: $(KERNEL)
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 not found. Please install qemu package." && exit 1)
	qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL) -no-reboot -display none -serial stdio

KUNIT_BIN := test/kunit/build/kunit_tests
KUNIT_HOST_SRCS := \
	test/kunit/kunit_main.c \
	test/kunit/libc9_string_test.c \
	test/kunit/libc9_string_ops_test.c \
	test/kunit/libc9_memory_test.c \
	test/kunit/libc9_numeric_test.c \
	test/kunit/capability_stubs.c \
	test/kunit/blind_cap_test.c \
	test/kunit/critical_contracts_test.c \
	kernel/libc9/kstrlen.c \
	kernel/libc9/kstrcmp.c \
	kernel/libc9/memcmp.c \
	kernel/libc9/kmemmove.c \
	kernel/libc9/kstrcpy.c \
	kernel/libc9/kstrncpy.c \
	kernel/libc9/kstrcat.c \
	kernel/libc9/strncat.c \
	kernel/libc9/kstrchr.c \
	kernel/libc9/strncmp.c \
	kernel/libc9/cistrncmp.c \
	kernel/libc9/strstr.c \
	kernel/libc9/kmemset.c \
	kernel/libc9/memchr.c \
	kernel/libc9/memccpy.c \
	kernel/libc9/strecpy.c \
	kernel/libc9/atoi.c \
	kernel/libc9/strtol.c \
	kernel/libc9/strtoul.c \
	kernel/libc9/strtoull.c \
	kernel/crypto/blind_cap.c \
	kernel/crypto/monocypher.c
KUNIT_HOST_CFLAGS := -std=gnu11 -O0 -g3 -Wall -Wextra -Werror \
	-Wno-unknown-pragmas \
	-Wno-error=sign-compare \
	-Wno-error=parentheses \
	-Wno-error=type-limits \
	-Wno-error=discarded-qualifiers \
	-Wno-error=unused-parameter \
	-Wno-error=unused-variable \
	-fno-builtin \
	-fno-builtin-strcmp -fno-builtin-strlen \
	-fno-builtin-memcmp -fno-builtin-memmove -fno-builtin-memcpy \
	-Ikernel/include

kunit: $(KUNIT_BIN)
	@$(KUNIT_BIN)

test: kunit

$(KUNIT_BIN): $(KUNIT_HOST_SRCS)
	@mkdir -p test/kunit/build
	@echo "Building KUnit-style host tests..."
	@$(CC) $(KUNIT_HOST_CFLAGS) $(KUNIT_HOST_SRCS) -o $(KUNIT_BIN)

help:
	@echo "Lux9 Build System (GNUmakefile):"
	@echo "  make          - Build kernel only"
	@echo "  make all      - Build kernel only"
	@echo "  make iso      - Build complete bootable ISO (RECOMMENDED)"
	@echo "  make clean    - Clean all build artifacts"
	@echo "  make run      - Run kernel in QEMU (with logging)"
	@echo "  make direct-run - Run kernel in QEMU (direct stdio)"
	@echo "  make kunit    - Run KUnit-style host unit tests"
	@echo "  make test     - Alias for make kunit"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Note: The Makefile in root directory provides additional targets"
	@echo "Run 'make -f Makefile help' for more detailed build options"
