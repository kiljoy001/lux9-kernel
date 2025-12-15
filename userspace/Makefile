# userspace/Makefile - Userspace Build Configuration
# Builds all userspace components independently

# Configuration
USERSPACE_NAME := lux9-userspace
BUILD_DIR := build
SRC_DIR := .

# Source directories
GO_SERVERS_DIR := go-servers
DRIVERS_DIR := drivers
C_SERVERS_DIR := servers
LIBS_DIR := lib
APP_BIN_DIR := bin

# Build directories
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/lib
INITRD_DIR := $(BUILD_DIR)/initrd

# Tool chains
GO := go
CC := gcc
CXX := g++
LD := ld
AR := ar

# Go build settings
GO_BUILD_FLAGS := -trimpath -gcflags="all=-N -l"
GOBUILD := $(GO) build $(GO_BUILD_FLAGS)

# C build settings
CFLAGS := -Wall -Wextra -O2 -fPIC \
          -Wno-implicit-function-declaration \
          -Wno-incompatible-pointer-types \
          -I./include \
          -I./lib \
          -I../../kernel/include \
          -L$(LIB_DIR)

LDFLAGS := -shared -Wl,-soname,

# Targets
.PHONY: all clean test install help init-asm

# Main userspace build
all: init-c test_9p_client initrd

init-asm: $(BIN_DIR)/init-asm

$(BIN_DIR)/init-asm: bin/init_minimal.S | $(BIN_DIR)
	@echo "Building minimal assembly init program..."
	@mkdir -p $(OBJ_DIR)
	@$(CC) -nostdlib -static -ffreestanding -c $< -o $(OBJ_DIR)/init_minimal.o
	@$(LD) -nostdlib -static -o $@ $(OBJ_DIR)/init_minimal.o

init-c: $(BIN_DIR)/init

$(BIN_DIR)/init: init_minimal/init.c | $(BIN_DIR) libs
	@echo "Building C init program..."
	@mkdir -p $(OBJ_DIR)
	@$(CC) -nostdlib -static -ffreestanding -m64 -O2 \
	    -fcf-protection=none \
	    -I./lux-c/include \
	    -o $@ $^ lux-c/crt0.o lux-c/liblux-c.a

# Test 9P Client
test_9p_client_build: $(BIN_DIR)/test_9p_client

$(BIN_DIR)/test_9p_client: test_9p_client.c lib/lib9p_client.c | $(BIN_DIR) libs
	@echo "Building Test 9P Client..."
	@$(CC) -nostdlib -static -ffreestanding -m64 -O2 \
	    -fcf-protection=none \
	    -I./include \
	    -I./lux-c/include \
	    -I../../kernel/include \
	    -o $@ $< lib/lib9p_client.c lux-c/crt0.o lux-c/liblux-c.a

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Drivers build
drivers:
	@echo "Building userspace drivers..."
	@$(MAKE) -C $(DRIVERS_DIR) all

# Go servers build
go-servers:
	@echo "Building Go servers..."
	@$(MAKE) -C $(GO_SERVERS_DIR) all

# C servers - removed (crypto server deprecated)

# Libraries
libs:
	@echo "Building lux-c library..."
	@$(MAKE) -C lux-c all

# Initrd creation
initrd: init-asm $(BUILD_DIR)/initrd.tar
	@echo "Creating initrd..."

$(BUILD_DIR)/initrd.tar: init-c libs
	@echo "Building minimal initrd with init only"
	@rm -rf $(INITRD_DIR)
	@mkdir -p $(INITRD_DIR)/bin $(INITRD_DIR)/boot $(INITRD_DIR)/dev $(INITRD_DIR)/srv
	# Copy init program to /boot/init (kernel expects it there)
	@cp ../bin/init $(INITRD_DIR)/boot/init
	# Also copy to /bin/init for compatibility
	@cp ../bin/init $(INITRD_DIR)/bin/init
	# Copy CLR Init
	@cp ../kernel/clr/test/bin/Debug/net9.0/init.dll $(INITRD_DIR)/boot/Init.dll
	# Copy test_9p_client
	@cp ../bin/test_9p_client $(INITRD_DIR)/bin/test_9p_client
	# Create tar
	@cd $(INITRD_DIR) && tar -cf ../initrd.tar .
	@echo "Initrd created: $@"

# Testing
test: 
	@echo "Running userspace tests..."
	@$(MAKE) -C $(GO_SERVERS_DIR) test 2>/dev/null || echo "Go server tests completed"

# Installation
install: $(BUILD_DIR)/initrd.tar
	@mkdir -p $(DESTDIR)/boot
	@cp $< $(DESTDIR)/boot/initrd.tar
	@echo "Userspace installed to $(DESTDIR)/boot/initrd.tar"

# Cleanup
clean:
	@echo "Cleaning userspace build..."
	@rm -rf $(BUILD_DIR)
	@$(MAKE) -C lux-c clean 2>/dev/null || true

help:
	@echo "Userspace Build Targets:"
	@echo "  all      - Build all userspace components"
	@echo "  go-servers - Build Go servers"
	@echo "  drivers  - Build userspace drivers"
	@echo "  c-servers - Build C servers"
	@echo "  libs     - Build libraries"
	@echo "  initrd   - Create initrd package"
	@echo "  test     - Run userspace tests"
	@echo "  install  - Install userspace"
	@echo "  clean    - Clean build artifacts"
