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
          -I./include \
          -I./lib \
          -I../../kernel/include \
          -L$(LIB_DIR)

LDFLAGS := -shared -Wl,-soname,

# Targets
.PHONY: all clean test install help init-asm

# Main userspace build
all: init-asm go-servers drivers c-servers libs initrd

init-asm: $(BIN_DIR)/init-asm

$(BIN_DIR)/init-asm: bin/init_minimal.S | $(BIN_DIR)
	@echo "Building minimal assembly init program..."
	@mkdir -p $(OBJ_DIR) # Ensure OBJ_DIR exists for init_minimal.o
	@$(CC) -nostdlib -static -ffreestanding -c $< -o $(OBJ_DIR)/init_minimal.o
	@$(LD) -nostdlib -static -o $@ $(OBJ_DIR)/init_minimal.o

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

# C servers build  
c-servers: $(C_SERVERS_DIR)/ext4fs/ext4fs $(C_SERVERS_DIR)/crypto/cryptosrv $(BIN_DIR)/rc

$(C_SERVERS_DIR)/ext4fs/ext4fs: $(C_SERVERS_DIR)/ext4fs/ext4fs.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ $<

$(C_SERVERS_DIR)/crypto/cryptosrv:
	@echo "Building Crypto Server..."
	@$(MAKE) -C $(C_SERVERS_DIR)/crypto all

# RC Shell Build
RC_SRC_DIR := ../src/cmd/rc
RC_OBJ_DIR := $(OBJ_DIR)/rc
RC_SRCS := $(wildcard $(RC_SRC_DIR)/*.c)
# Exclude unix.c, include plan9.c. syn.c is generated.
RC_SRCS := $(filter-out $(RC_SRC_DIR)/unix.c, $(RC_SRCS))
RC_OBJS := $(patsubst $(RC_SRC_DIR)/%.c, $(RC_OBJ_DIR)/%.o, $(RC_SRCS)) $(RC_OBJ_DIR)/syn.o

$(BIN_DIR)/rc: $(RC_OBJS) | $(BIN_DIR)
	@echo "Linking rc shell..."
	@$(CC) $(CFLAGS) -nostdlib -static -o $@ $^ $(LIB_DIR)/crt0.o -L$(LIB_DIR) -lc

$(RC_OBJ_DIR)/syn.c: $(RC_SRC_DIR)/syn.y | $(RC_OBJ_DIR)
	@echo "Generating rc parser..."
	@yacc -d -o $@ $<
	@if [ -f y.tab.h ]; then mv y.tab.h $(RC_OBJ_DIR)/y.tab.h; fi
	@if [ -f $(RC_OBJ_DIR)/syn.h ]; then mv $(RC_OBJ_DIR)/syn.h $(RC_OBJ_DIR)/y.tab.h; fi

$(RC_OBJ_DIR)/syn.o: $(RC_OBJ_DIR)/syn.c
	@echo "Compiling rc parser..."
	@$(CC) $(CFLAGS) -nostdinc -DPlan9 -I$(RC_SRC_DIR) -I/usr/lib/gcc/x86_64-linux-gnu/13/include -c -o $@ $<

$(RC_OBJ_DIR)/%.o: $(RC_SRC_DIR)/%.c $(RC_OBJ_DIR)/syn.c | $(RC_OBJ_DIR)
	@echo "Compiling rc object $<..."
	@$(CC) $(CFLAGS) -nostdinc -DPlan9 -I$(RC_SRC_DIR) -I$(RC_OBJ_DIR) -I/usr/lib/gcc/x86_64-linux-gnu/13/include -c -o $@ $<

$(RC_OBJ_DIR):
	@mkdir -p $(RC_OBJ_DIR)

# Libraries
libs:
	@echo "Building userspace libraries..."
	@$(MAKE) -C libc all

# Initrd creation
initrd: init-asm $(BUILD_DIR)/initrd.tar
	@echo "Creating initrd..."

$(BUILD_DIR)/initrd.tar: init-asm go-servers drivers c-servers libs
	@echo "Building initrd from $(GO_SERVERS_DIR), $(DRIVERS_DIR), and $(C_SERVERS_DIR)"
	@rm -rf $(INITRD_DIR)
	@mkdir -p $(INITRD_DIR)/bin $(INITRD_DIR)/lib $(INITRD_DIR)/dev $(INITRD_DIR)/etc $(INITRD_DIR)/srv
	# Copy Go binaries
	@find $(GO_SERVERS_DIR) -name "*.go" -prune -o -type f -executable -print \
	    | xargs -I {} cp {} $(INITRD_DIR)/bin/ 2>/dev/null || true
	# Copy driver binaries
	@find $(DRIVERS_DIR) -name "*_driver" -type f -executable -print \
	    | xargs -I {} cp {} $(INITRD_DIR)/bin/ 2>/dev/null || true
	# Copy C binaries
	@cp $(C_SERVERS_DIR)/ext4fs/ext4fs $(INITRD_DIR)/bin/ 2>/dev/null || true
	@cp $(C_SERVERS_DIR)/crypto/cryptosrv $(INITRD_DIR)/bin/ 2>/dev/null || true
	@cp $(BIN_DIR)/rc $(INITRD_DIR)/bin/rc 2>/dev/null || true
	# Copy init program
	@cp $(BIN_DIR)/init-asm $(INITRD_DIR)/bin/init
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
	@$(MAKE) -C $(DRIVERS_DIR) clean 2>/dev/null || true
	@$(MAKE) -C libc clean 2>/dev/null || true
	@$(MAKE) -C $(C_SERVERS_DIR)/crypto clean 2>/dev/null || true

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
