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
# Might need others like System.Collections.dll if used
INITRD_TAR := $(BUILD_DIR)/initrd.tar

.PHONY: all clean initrd build-init

all: initrd

clean:
	rm -rf $(BUILD_DIR)
	cd $(INIT_SRC_DIR) && dotnet clean

$(INIT_DLL):
	cd $(INIT_SRC_DIR) && dotnet build -c Debug

# Build the initrd tarball
# Structure:
#   boot/init  <- The F# init binary (renamed from init.dll)
#   boot/System.Runtime.dll
initrd: $(INIT_DLL)
	@mkdir -p $(INIT_BOOT_DIR)
	@cp $(INIT_DLL) $(INIT_BOOT_DIR)/init
	@cp $(BCL_DLL) $(INIT_BOOT_DIR)/
	@cp $(FSHARP_CORE_DLL) $(INIT_BOOT_DIR)/
	@if [ -f $(INIT_OUT_DIR)/System.Collections.dll ]; then cp $(INIT_OUT_DIR)/System.Collections.dll $(INIT_BOOT_DIR)/; fi
	@echo "Creating initrd.tar from $(INIT_BUILD_DIR)..."
	@cd $(INIT_BUILD_DIR) && tar -cvf ../initrd.tar *
