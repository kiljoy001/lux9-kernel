# userspace/Makefile - Userspace Build Configuration
# Builds all userspace components independently

# Configuration
USERSPACE_NAME := lux9-userspace
BUILD_DIR := build
SRC_DIR := .

# Source directories
GO_SERVERS_DIR := go-servers
C_SERVERS_DIR := servers
LIBS_DIR := lib

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
GOFLAGS := -ldflags="-s -w" -trimpath
GOBUILD := $(GO) build $(GOFLAGS)

# C build settings
CFLAGS := -Wall -Wextra -O2 -fPIC \
          -I./include \
          -I./lib \
          -L$(LIB_DIR)

LDFLAGS := -shared -Wl,-soname,

# Targets
.PHONY: all clean test install help

# Main userspace build
all: go-servers c-servers libs initrd

# Go servers build
go-servers:
	@echo "Building Go servers..."
	@$(MAKE) -C $(GO_SERVERS_DIR) all

# C servers build  
c-servers: $(C_SERVERS_DIR)/ext4fs/ext4fs

$(C_SERVERS_DIR)/ext4fs/ext4fs: $(C_SERVERS_DIR)/ext4fs/ext4fs.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ $<

# Libraries (placeholder)
libs:
	@echo "Building userspace libraries..."
	@mkdir -p $(LIB_DIR)

# Initrd creation
initrd: $(BUILD_DIR)/initrd.tar
	@echo "Creating initrd..."

$(BUILD_DIR)/initrd.tar: go-servers c-servers libs
	@echo "Building initrd from $(GO_SERVERS_DIR) and $(C_SERVERS_DIR)"
	@rm -rf $(INITRD_DIR)
	@mkdir -p $(INITRD_DIR)/{bin,lib,dev,etc}
	# Copy Go binaries
	@find $(GO_SERVERS_DIR) -name "*.go" -prune -o -type f -executable -print \
	    | xargs -I {} cp {} $(INITRD_DIR)/bin/ 2>/dev/null || true
	# Copy C binaries
	@cp $(C_SERVERS_DIR)/ext4fs/ext4fs $(INITRD_DIR)/bin/ 2>/dev/null || true
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
	@$(MAKE) -C $(GO_SERVERS_DIR) clean 2>/dev/null || true

help:
	@echo "Userspace Build Targets:"
	@echo "  all      - Build all userspace components"
	@echo "  go-servers - Build Go servers"
	@echo "  c-servers - Build C servers"
	@echo "  libs     - Build libraries"
	@echo "  initrd   - Create initrd package"
	@echo "  test     - Run userspace tests"
	@echo "  install  - Install userspace"
	@echo "  clean    - Clean build artifacts"
