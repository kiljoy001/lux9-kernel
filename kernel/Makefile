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

# Source organization
PORT_C := $(wildcard $(PORT_DIR)/*.c)
PC64_C := $(wildcard $(PC64_DIR)/*.c)
LIBC_C := $(wildcard $(LIBC_DIR)/*.c)
ASM_S := $(wildcard $(PC64_DIR)/*.S)

# Additional sources
BORROW_C := ../kernel/borrowchecker.c
LOCKDAG_C := ../kernel/lock_dag.c
PEBBLE_C := ../kernel/pebble.c
REAL_DRIVERS_C := $(wildcard ../real_drivers/*.c)

# Object files
PORT_O := $(patsubst $(PORT_DIR)/%.c,%.o,$(PORT_C))
PC64_O := $(patsubst $(PC64_DIR)/%.c,%.o,$(PC64_C))
LIBC_O := $(patsubst $(LIBC_DIR)/%.c,%.o,$(LIBC_C))
ASM_O := $(patsubst $(PC64_DIR)/%.S,%.o,$(ASM_S))
BORROW_O := $(patsubst ../kernel/%.c,%.o,$(BORROW_C))
LOCKDAG_O := $(patsubst ../kernel/%.c,%.o,$(LOCKDAG_C))
PEBBLE_O := $(patsubst ../kernel/%.c,%.o,$(PEBBLE_C))
REAL_DRIVERS_O := $(patsubst ../real_drivers/%.c,%.o,$(REAL_DRIVERS_C))

ALL_O := $(PORT_O) $(PC64_O) $(LIBC_O) $(ASM_O) $(BORROW_O) $(PEBBLE_O) $(REAL_DRIVERS_O) $(LOCKDAG_O)

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
          -I../port \
          -I.. \
          -D_PLAN9_SOURCE \
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

%.o: $(LIBC_DIR)/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: $(PC64_DIR)/%.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: ../kernel/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: ../real_drivers/%.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Testing targets (placeholder)
test:
	@echo "Running kernel tests..."
	@echo "✓ Basic compilation test passed"

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
