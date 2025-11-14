# 9front Microkernel Build System
KERNEL := lux9.elf

CC := gcc
LD := ld
AS := as

# Compiler flags - Plan 9 compatible
CFLAGS := -Wall -Wno-unused -Wno-unknown-pragmas -Wno-builtin-declaration-mismatch -Wno-discarded-qualifiers -Wno-missing-braces -Wno-incompatible-pointer-types -std=gnu11 \
           -O0 -g3 -gdwarf-4 \
           -ffreestanding -fno-stack-protector -fno-stack-check \
           -fno-lto -fno-pie -no-pie -fno-pic \
           -m64 -march=x86-64 -mcmodel=kernel \
           -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
           -Ikernel/include \
           -Iport \
           -I. \
           -D_PLAN9_SOURCE \
           -DKTZERO=0xffffffff80110000

# Linker flags
LDFLAGS := -m elf_x86_64 -nostdlib -static -no-pie --no-dynamic-linker \
           -z max-page-size=0x1000 \
           -T kernel/linker.ld

# Source files
PORT_C := $(wildcard kernel/9front-port/*.c)
PC64_C := $(wildcard kernel/9front-pc64/*.c)
LIBC_C := $(wildcard kernel/libc9/*.c)
MEMDRAW_C := $(wildcard kernel/libmemdraw/*.c)
BORROW_C := kernel/borrowchecker.c
LOCKDAG_C := kernel/lock_dag.c
REAL_DRIVERS_C := $(wildcard real_drivers/*.c)
PEBBLE_C := kernel/pebble.c

# SD/FIS support files already included by wildcard above

# Assembly files
ASM_S := kernel/9front-pc64/l.S kernel/9front-pc64/entry.S

# Object files
PORT_O := $(PORT_C:.c=.o)
PC64_O := $(PC64_C:.c=.o)
LIBC_O := $(LIBC_C:.c=.o)
ASM_O := $(ASM_S:.S=.o)

MEMDRAW_O := $(MEMDRAW_C:.c=.o)
BORROW_O := $(BORROW_C:.c=.o)
LOCKDAG_O := $(LOCKDAG_C:.c=.o)
REAL_DRIVERS_O := $(REAL_DRIVERS_C:.c=.o)
PEBBLE_O := $(PEBBLE_C:.c=.o)

ALL_O := $(PORT_O) $(PC64_O) $(LIBC_O) $(MEMDRAW_O) $(ASM_O) $(BORROW_O) $(PEBBLE_O) $(REAL_DRIVERS_O) $(LOCKDAG_O)

.PHONY: all clean count iso run

all: $(KERNEL)

$(KERNEL): $(ALL_O)
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) $(ALL_O) -o $@
	@echo "Build complete: $(KERNEL)"
	@ls -lh $(KERNEL)

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_O) $(KERNEL)
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

iso: $(KERNEL) userspace/initrd.tar
	@echo "Creating ISO image..."
	@# Check for required tools
	@which xorriso > /dev/null || (echo "Error: xorriso not found. Please install xorriso package." && exit 1)
	@# Use local limine if available, otherwise try system limine
	@if [ -d "boot/limine" ] && [ -f "boot/limine/limine-bios.sys" ]; then \
		LIMINE_DIR="boot/limine"; \
	elif [ -d "/usr/local/share/limine" ]; then \
		LIMINE_DIR="/usr/local/share/limine"; \
	elif [ -d "/usr/share/limine" ]; then \
		LIMINE_DIR="/usr/share/limine"; \
	else \
		echo "Warning: Limine files not found. ISO may not be bootable."; \
		LIMINE_DIR=""; \
	fi; \
	rm -rf iso_root; \
	mkdir -p iso_root/boot iso_root/boot/limine iso_root/EFI/BOOT; \
	cp $(KERNEL) iso_root/boot/; \
	cp userspace/initrd.tar iso_root/boot/; \
	cp limine.conf iso_root/boot/limine/; \
	if [ -n "$$LIMINE_DIR" ]; then \
		cp "$$LIMINE_DIR/limine-bios.sys" iso_root/boot/limine/ 2>/dev/null || true; \
		cp "$$LIMINE_DIR/limine-bios-cd.bin" iso_root/boot/limine/ 2>/dev/null || true; \
		cp "$$LIMINE_DIR/limine-uefi-cd.bin" iso_root/boot/limine/ 2>/dev/null || true; \
		cp "$$LIMINE_DIR/BOOTX64.EFI" iso_root/EFI/BOOT/ 2>/dev/null || true; \
	fi; \
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o lux9.iso 2>/dev/null || true; \
	if [ -n "$$LIMINE_DIR" ] && [ -f "$$LIMINE_DIR/limine" ]; then \
		"$$LIMINE_DIR/limine" bios-install lux9.iso 2>/dev/null || true; \
	elif command -v limine >/dev/null 2>&1; then \
		limine bios-install lux9.iso 2>/dev/null || true; \
	fi; \
	rm -rf iso_root; \
	@echo "✓ Created lux9.iso"

run: $(KERNEL)
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 not found. Please install qemu package." && exit 1)
	qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL) -no-reboot -display none -serial file:qemu.log

direct-run: $(KERNEL)
	@which qemu-system-x86_64 > /dev/null || (echo "Error: qemu-system-x86_64 not found. Please install qemu package." && exit 1)
	qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL) -no-reboot -display none -serial stdio
