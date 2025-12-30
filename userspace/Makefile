# Lux9 Userspace Build System
# Pure C userspace with automatic init database signing

BUILD_DIR := build
INIT_BUILD_DIR := $(BUILD_DIR)/init
INIT_BOOT_DIR := $(INIT_BUILD_DIR)/boot
C_INIT_DIR := lib9p_syscall
RESURRECTION_DIR := resurrection
INIT_DB := ../scripts/init_db.py

.PHONY: all clean initrd sign

all: initrd sign

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(C_INIT_DIR) clean
	$(MAKE) -C $(RESURRECTION_DIR) clean

# Build the C init binary
$(C_INIT_DIR)/init:
	$(MAKE) -C $(C_INIT_DIR)

# Build the resurrection server
$(RESURRECTION_DIR)/resurrection:
	$(MAKE) -C $(RESURRECTION_DIR)

# Build the initrd tarball
# Structure:
#   boot/init         <- Init process (PID 1)
#   boot/resurrection <- Resurrection server (started by init)
initrd: $(C_INIT_DIR)/init $(RESURRECTION_DIR)/resurrection
	@mkdir -p $(INIT_BOOT_DIR)
	@cp $(C_INIT_DIR)/init $(INIT_BOOT_DIR)/init
	@cp $(RESURRECTION_DIR)/resurrection $(INIT_BOOT_DIR)/resurrection
	@echo "Creating initrd.tar from $(INIT_BUILD_DIR)..."
	@cd $(INIT_BUILD_DIR) && tar -cvf ../initrd.tar *
	@ls -lh $(BUILD_DIR)/initrd.tar

# Register and sign binaries using init database
sign: $(C_INIT_DIR)/init $(RESURRECTION_DIR)/resurrection
	@echo "Registering binaries with init database..."
	@python3 $(INIT_DB) init 2>/dev/null || true
	@python3 $(INIT_DB) register $(C_INIT_DIR)/init "C init (4KB exchange)" 2>/dev/null || \
		echo "  init already registered"
	@python3 $(INIT_DB) register $(RESURRECTION_DIR)/resurrection "Resurrection server" 2>/dev/null || \
		echo "  resurrection already registered"
	@echo "Signing binaries (requires LUX_SIGNING_KEY or TPM)..."
	@if [ -n "$$LUX_SIGNING_KEY" ]; then \
		INIT_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(C_INIT_DIR)/init" | head -1 | awk '{print $$1}'); \
		RES_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(RESURRECTION_DIR)/resurrection" | head -1 | awk '{print $$1}'); \
		[ -n "$$INIT_ID" ] && python3 $(INIT_DB) sign $$INIT_ID 2>/dev/null || true; \
		[ -n "$$RES_ID" ] && python3 $(INIT_DB) sign $$RES_ID 2>/dev/null || true; \
		echo "✅ Binaries signed"; \
	else \
		echo "⚠️  LUX_SIGNING_KEY not set - binaries will be unsigned"; \
		echo "   Set LUX_SIGNING_KEY=<64-char-hex> to enable signing"; \
	fi
	@echo "Verifying binaries..."
	@python3 $(INIT_DB) verify $(C_INIT_DIR)/init || true
	@python3 $(INIT_DB) verify $(RESURRECTION_DIR)/resurrection || true
