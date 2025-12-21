# Lux9 Userspace Build System

# Directories
BUILD_DIR := build
INIT_BUILD_DIR := $(BUILD_DIR)/init
INIT_BOOT_DIR := $(INIT_BUILD_DIR)/boot
INIT_SRC_DIR := init9p
INIT_OUT_DIR := $(INIT_SRC_DIR)/bin/Debug/net9.0

# Files
INIT_DLL := $(INIT_OUT_DIR)/init.dll
BCL_DLL := ../kernel/clr/bcl/src/System.Runtime/bin/Debug/net9.0/System.Runtime.dll
FSHARP_CORE_DLL := $(INIT_OUT_DIR)/FSharp.Core.dll
SHELL_SRC_DIR := shell
SHELL_OUT_DIR := $(SHELL_SRC_DIR)/bin/Debug/net9.0
SHELL_DLL := $(SHELL_OUT_DIR)/shell.dll

.PHONY: all clean initrd build-init build-shell

all: initrd

clean:
	rm -rf $(BUILD_DIR)
	cd $(INIT_SRC_DIR) && dotnet clean
	cd $(SHELL_SRC_DIR) && dotnet clean
	cd $(RAMFS_SRC_DIR) && dotnet clean

$(INIT_DLL):
	cd $(INIT_SRC_DIR) && dotnet build -c Debug

$(SHELL_DLL):
	cd $(SHELL_SRC_DIR) && dotnet build -c Debug

RAMFS_SRC_DIR := ramfs
RAMFS_OUT_DIR := $(RAMFS_SRC_DIR)/bin/Debug/net9.0
RAMFS_DLL := $(RAMFS_OUT_DIR)/Ramfs.dll

$(RAMFS_DLL):
	cd $(RAMFS_SRC_DIR) && dotnet build -c Debug

# Build the initrd tarball
# Structure:
#   boot/init  <- The F# init binary
#   bin/shell  <- The F# shell
#   boot/System.Runtime.dll
initrd: $(INIT_DLL) $(SHELL_DLL) $(RAMFS_DLL)
	@mkdir -p $(INIT_BOOT_DIR)
	@mkdir -p $(INIT_BUILD_DIR)/bin
	@cp $(INIT_DLL) $(INIT_BOOT_DIR)/init
	@cp $(SHELL_DLL) $(INIT_BUILD_DIR)/bin/shell
	@cp $(RAMFS_DLL) $(INIT_BUILD_DIR)/bin/ramfs
	@cp $(BCL_DLL) $(INIT_BOOT_DIR)/
	@cp $(FSHARP_CORE_DLL) $(INIT_BOOT_DIR)/
	@# Copy deps to bin as well so shell finds them?
	@# Actually, kernel loader shares assemblies, but for now duplicate
	@cp $(BCL_DLL) $(INIT_BUILD_DIR)/bin/
	@cp $(FSHARP_CORE_DLL) $(INIT_BUILD_DIR)/bin/
	@if [ -f $(INIT_OUT_DIR)/System.Collections.dll ]; then \
		cp $(INIT_OUT_DIR)/System.Collections.dll $(INIT_BOOT_DIR)/; \
		cp $(INIT_OUT_DIR)/System.Collections.dll $(INIT_BUILD_DIR)/bin/; \
	fi
	@echo "Creating initrd.tar from $(INIT_BUILD_DIR)..."
	@cd $(INIT_BUILD_DIR) && tar -cvf ../initrd.tar *
