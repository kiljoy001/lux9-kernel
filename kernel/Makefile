# kernel/Makefile - Kernel Build Configuration
# Pure kernel build with no userspace dependencies

# Configuration
KERNEL_NAME := lux9
KERNEL_EXT := elf
KERNEL_BINARY := $(KERNEL_NAME).$(KERNEL_EXT)

# Compiler and tools
CC := gcc
LD := ld
AS := as
OBJCOPY := objcopy
STRIP := strip

# Build directories
SRC_DIR := .
INCLUDE_DIR := ../kernel/include
PORT_DIR := ../kernel/9front-port
PC64_DIR := ../kernel/9front-pc64
LIBC_DIR := ../kernel/libc9
PORT_DIR := ../kernel/9front-port
AML_DIR := ../kernel/aml

# Source organization
PORT_C := $(wildcard $(PORT_DIR)/*.c)
PC64_C := $(wildcard $(PC64_DIR)/*.c)
LIBC_C := $(wildcard $(LIBC_DIR)/*.c)
AML_C := $(wildcard $(AML_DIR)/*.c)
ASM_S := $(wildcard $(PC64_DIR)/*.S)

# Additional sources
BORROW_C := ../kernel/borrowchecker.c
LOCKDAG_C := ../kernel/lock_dag.c
PEBBLE_C := ../kernel/pebble.c
CRYPTO_C := $(wildcard ../kernel/crypto/*.c)
CRYPTO_S := ../kernel/crypto/hwcrypto.S
REAL_DRIVERS_C := $(wildcard ../real_drivers/*.c)
FAMILY_C := family/family.c family/pci_family.c family/pci_resource_pool.c family/pci_channel.c family/pci_transactions.c family/pci_9p.c family/pebble.c family/stubs.c family/devfamily.c
CLR_FRUITY_C := clr/fruity/fruity_ir.c clr/fruity/fruity_cbor.c clr/fruity/fruity_to_qbe.c clr/fruity/qbe_buffer.c
CLR_CBOR_C := clr/libmcu-cbor/common.c clr/libmcu-cbor/decoder.c clr/libmcu-cbor/encoder.c clr/libmcu-cbor/helper.c clr/libmcu-cbor/ieee754.c clr/libmcu-cbor/parser.c
CLR_QBE_C := clr/qbe_compile.c
MISC_KERNEL_C := ../kernel/ghostdag_kernel.c ../kernel/9p_router.c

# Object files
PORT_O := $(patsubst $(PORT_DIR)/%.c,%.o,$(PORT_C))
PC64_O := $(patsubst $(PC64_DIR)/%.c,%.o,$(PC64_C))
LIBC_O := $(patsubst $(LIBC_DIR)/%.c,%.o,$(LIBC_C))
ASM_O := $(patsubst $(PC64_DIR)/%.S,%.o,$(ASM_S))
BORROW_O := $(patsubst ../kernel/%.c,%.o,$(BORROW_C))
LOCKDAG_O := $(patsubst ../kernel/%.c,%.o,$(LOCKDAG_C))
PEBBLE_O := $(patsubst ../kernel/%.c,%.o,$(PEBBLE_C))
CRYPTO_O := $(patsubst ../kernel/%.c,%.o,$(CRYPTO_C)) $(patsubst ../kernel/%.S,%.o,$(CRYPTO_S))
REAL_DRIVERS_O := $(patsubst ../real_drivers/%.c,%.o,$(REAL_DRIVERS_C))
FAMILY_O := family.o pci_family.o pci_resource_pool.o pci_channel.o pci_transactions.o pci_9p.o stubs.o devfamily.o
AML_O := $(patsubst $(AML_DIR)/%.c,%.o,$(AML_C))
CLR_FRUITY_O := fruity_ir.o fruity_cbor.o fruity_to_qbe.o qbe_buffer.o il_parser.o il_to_fruity.o
CLR_CBOR_O := cbor_common.o cbor_decoder.o cbor_encoder.o cbor_helper.o cbor_ieee754.o cbor_parser.o
CLR_QBE_O := qbe_compile.o clr_console.o clr_core.o clr_bcl_helpers.o clr_runtime.o
MISC_KERNEL_O := $(patsubst ../kernel/%.c,%.o,$(MISC_KERNEL_C))

ALL_O := $(PORT_O) $(PC64_O) $(LIBC_O) $(ASM_O) $(BORROW_O) $(PEBBLE_O) $(CRYPTO_O) $(REAL_DRIVERS_O) $(LOCKDAG_O) $(FAMILY_O) $(CLR_FRUITY_O) $(CLR_CBOR_O) $(CLR_QBE_O) $(MISC_KERNEL_O)
# AML_O removed temporarily to fix build
# $(AML_O)

# Compiler flags - Optimized for Plan 9 compatibility
CFLAGS := -Wall -Wno-unused -Wno-unknown-pragmas \
          -Wno-builtin-declaration-mismatch \
          -Wno-discarded-qualifiers \
          -Wno-missing-braces \
          -Wno-incompatible-pointer-types \
          -std=gnu11 \
          -O0 -g3 -gdwarf-4 \
          -ffreestanding -fno-stack-protector \
          -fno-stack-check -fno-lto -fno-pie \
          -m64 -march=x86-64 -mcmodel=kernel \
          -mno-80387 -mno-mmx -mno-sse -mno-sse2 \
          -mno-red-zone \
          -I$(INCLUDE_DIR) \
          -I../include \
          -I../port \
          -I.. \
          -Iclr/libmcu-cbor \
          -Iclr/fruity \
          -D_PLAN9_SOURCE \
          -D__PLAN9_KERNEL__ \
          -DKERNEL \
          -DKTZERO=0xffffffff80110000

# Linker configuration
LDFLAGS := -m elf_x86_64 -nostdlib -static -no-pie \
           --no-dynamic-linker \
           -z max-page-size=0x1000 \
           -T ../kernel/linker.ld

# Targets
.PHONY: all clean test run debug install help

# Main kernel build
all: $(KERNEL_BINARY)

$(KERNEL_BINARY): $(ALL_O)
	@echo "Linking kernel: $@"
	$(LD) $(LDFLAGS) $(ALL_O) -o $@
	@echo "Kernel build complete: $(KERNEL_BINARY)"
	@ls -lh $(KERNEL_BINARY)

# Object file compilation
%.o: $(PORT_DIR)/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: $(PC64_DIR)/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: $(AML_DIR)/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: $(LIBC_DIR)/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: $(PC64_DIR)/%.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: ../kernel/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: ../kernel/%.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: ../real_drivers/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

pebble_family.o: family/pebble.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: family/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# CLR Fruity IR compilation
fruity_ir.o: clr/fruity/fruity_ir.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

fruity_cbor.o: clr/fruity/fruity_cbor.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

fruity_to_qbe.o: clr/fruity/fruity_to_qbe.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

qbe_buffer.o: clr/fruity/qbe_buffer.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# CLR CBOR library compilation
cbor_common.o: clr/libmcu-cbor/common.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

cbor_decoder.o: clr/libmcu-cbor/decoder.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

cbor_encoder.o: clr/libmcu-cbor/encoder.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

cbor_helper.o: clr/libmcu-cbor/helper.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

cbor_ieee754.o: clr/libmcu-cbor/ieee754_stub.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

cbor_parser.o: clr/libmcu-cbor/parser.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DCBOR_NO_FLOAT -c $< -o $@

# CLR QBE compiler
qbe_compile.o: clr/qbe_compile.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clr_console.o: clr/qbe/clr_console.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clr_core.o: clr/qbe/clr_core.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clr_bcl_helpers.o: clr/qbe/clr_bcl_helpers.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

il_parser.o: clr/il_parser.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

il_to_fruity.o: clr/il_to_fruity.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clr_runtime.o: clr/clr_runtime.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Testing targets (placeholder)

run: $(KERNEL_BINARY)
	@echo "Running kernel in QEMU..."
	@/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL_BINARY) -no-reboot -display none -serial stdio

debug: $(KERNEL_BINARY)
	@echo "Running kernel in QEMU with GDB..."
	@/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 -M q35 -m 2G -kernel $(KERNEL_BINARY) -no-reboot -display none -serial stdio -s -S &

# Installation (for distribution builds)
install: $(KERNEL_BINARY)
	@mkdir -p $(DESTDIR)/boot
	@cp $(KERNEL_BINARY) $(DESTDIR)/boot/
	@echo "Kernel installed to $(DESTDIR)/boot/"

# Cleanup
clean:
	@echo "Cleaning kernel build..."
	@rm -f *.o
	@rm -f $(KERNEL_BINARY)

help:
	@echo "Kernel Build Targets:"
	@echo "  all     - Build kernel binary"
	@echo "  test    - Run kernel tests"
	@echo "  run     - Run in QEMU"
	@echo "  debug   - Debug in QEMU+GDB"
	@echo "  install - Install kernel"
	@echo "  clean   - Clean build artifacts"
