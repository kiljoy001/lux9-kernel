# Makefile - Lux9 Build System Orchestrator
# Coordinates kernel, userspace, and OS builds

# Configuration
CONFIG_DIR := config
KERNEL_DIR := kernel
USERSPACE_DIR := userspace
DRIVERS_DIR := userspace/drivers
OS_DIR := os
SCRIPTS_DIR := scripts
BUILD_DIR := build
OUTPUT_DIR := output

# Build targets
.PHONY: iso all kernel userspace drivers os clean test dev ci help

# Default target
all: os

# Main ISO build target - RECOMMENDED COMMAND
iso: kernel userspace drivers os
	@echo "Complete ISO build finished: os/lux9.iso"
	@ls -lh os/lux9.iso

# Individual component builds
kernel:
	@echo "Building kernel..."
	@$(MAKE) -C $(KERNEL_DIR) all

userspace:
	@echo "Building userspace..."
	@$(MAKE) -C $(USERSPACE_DIR) all

drivers:
	@echo "Building userspace drivers..."
	@$(MAKE) -C $(DRIVERS_DIR) all

# Combined OS build
os: kernel userspace drivers
	@echo "Integrating into complete OS..."
	@$(MAKE) -C $(OS_DIR) all

# Development workflow
dev: clean kernel userspace
	@echo "Development build complete"
	@$(MAKE) test

# CI/CD build
ci: kernel userspace test-integration
	@echo "CI build complete"

# Testing
test: test-kernel test-userspace test-integration

test-kernel:
	@$(MAKE) -C $(KERNEL_DIR) test

test-userspace:
	@$(MAKE) -C $(USERSPACE_DIR) test

test-integration:
	@$(SCRIPTS_DIR)/test.sh integration

# Cleanup
clean:
	@$(MAKE) -C $(KERNEL_DIR) clean
	@$(MAKE) -C $(USERSPACE_DIR) clean
	@$(MAKE) -C $(OS_DIR) clean
	@rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

help:
	@echo "Lux9 Build System Targets:"
	@echo "  iso           - Build complete bootable ISO (RECOMMENDED)"
	@echo "  all           - Build complete OS (alias for iso)"
	@echo "  kernel        - Build kernel only"
	@echo "  userspace     - Build userspace only"
	@echo "  drivers       - Build userspace drivers only"
	@echo "  os            - Create bootable ISO from components"
	@echo "  clean         - Clean all builds"
	@echo "  dev           - Development build (fast)"
	@echo "  test          - Run all tests"
