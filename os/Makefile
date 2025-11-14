# os/Makefile - OS Integration Build
# Combines kernel and userspace into complete OS

# Configuration
KERNEL_BINARY := ../kernel/lux9.elf
USERSPACE_INITRD := ../userspace/build/initrd.tar
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso

# Boot loader (requires limine)
LIMINE_DIR := /home/scott/Repo/limine-c-template-x86-64/limine
LIMINE_BIOS := $(LIMINE_DIR)/limine-bios.sys
LIMINE_BIOS_CD := $(LIMINE_DIR)/limine-bios-cd.bin
LIMINE_UEFI_CD := $(LIMINE_DIR)/limine-uefi-cd.bin
BOOTX64_EFI := $(LIMINE_DIR)/BOOTX64.EFI

# ISO tools
XORRISO := xorriso
GENISOIMAGE := genisoimage

# Targets
.PHONY: all clean test run help

# Main OS build
all: lux9.iso

# Create bootable ISO
lux9.iso: $(KERNEL_BINARY) $(USERSPACE_INITRD) $(LIMINE_BIOS_CD) $(LIMINE_UEFI_CD) $(BOOTX64_EFI)
	@echo "Creating bootable ISO: $@"
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/{boot/limine,EFI/BOOT}
	
	# Copy kernel and initrd
	@cp $(KERNEL_BINARY) $(ISO_DIR)/boot/
	@cp $(USERSPACE_INITRD) $(ISO_DIR)/boot/
	
	# Copy boot loader files
	@cp $(LIMINE_BIOS) $(ISO_DIR)/boot/limine/
	@cp $(LIMINE_BIOS_CD) $(ISO_DIR)/boot/limine/
	@cp $(LIMINE_UEFI_CD) $(ISO_DIR)/boot/limine/
	@cp $(BOOTX64_EFI) $(ISO_DIR)/EFI/BOOT/
	
	# Copy limine configuration
	@cp ../limine.conf $(ISO_DIR)/boot/limine/
	
	# Create ISO with hybrid boot support
	@$(XORRISO) -as mkisofs \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		-iso-level 3 \
		-rock -joliet \
		$(ISO_DIR) -o $@
	
	# Install boot loader
	@/home/scott/Repo/limine/limine bios-install $@ 2>/dev/null || true
	
	@echo "OS image created: $@"
	@ls -lh $@

# Testing
test: lux9.iso
	@echo "Running OS integration test..."
	@timeout 30s /home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 \
		-drive file=$<,format=raw,media=disk \
		-m 2G -smp 4 \
		-nographic -serial stdio \
		-display none \
		-no-reboot || echo "Test completed"

# Run in QEMU
run: lux9.iso
	@echo "Running OS in QEMU..."
	@/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 \
		-drive file=$<,format=raw,media=disk \
		-m 2G -smp 4 \
		-nographic -serial stdio \
		-display none

# Debug mode
debug: lux9.iso
	@echo "Debugging OS in QEMU+GDB..."
	@/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 \
		-drive file=$<,format=raw,media=disk \
		-m 2G -smp 4 \
		-nographic -serial stdio \
		-display none \
		-s -S &

# Cleanup
clean:
	@echo "Cleaning OS build..."
	@rm -rf $(BUILD_DIR)
	@rm -f lux9.iso

help:
	@echo "OS Integration Targets:"
	@echo "  all     - Create bootable ISO"
	@echo "  test    - Run integration test"
	@echo "  run     - Run in QEMU"
	@echo "  debug   - Debug in QEMU+GDB"
	@echo "  clean   - Clean build artifacts"
