# Lux9 Userspace Build System
# Pure C userspace with automatic init database signing

BUILD_DIR := build
INIT_BUILD_DIR := $(BUILD_DIR)/init
INIT_BOOT_DIR := $(INIT_BUILD_DIR)/boot
C_INIT_DIR := lib9p_syscall
RESURRECTION_DIR := resurrection
RUMP_DIR := rump
INIT_DB := ../scripts/init_db.py
TURBOCID_DIR := turbocid
WASM_EXTENSIVE_DIR := wasm_extensive_test
SRVCTL_DIR := srvctl
HAL_DIR := hal

KERNEL = ../kernel
LIBLUX = lib/liblux

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I$(LIBLUX)/inc

.PHONY: all clean initrd sign liblux

all: initrd sign

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(C_INIT_DIR) clean
	$(MAKE) -C $(RESURRECTION_DIR) clean
	$(MAKE) -C $(RUMP_DIR) clean
	$(MAKE) -C $(TURBOCID_DIR) clean
	$(MAKE) -C $(WASM_EXTENSIVE_DIR) clean
	$(MAKE) -C $(SRVCTL_DIR) clean
	$(MAKE) -C $(LIBLUX) clean

# Build liblux
$(LIBLUX)/liblux.a:
	$(MAKE) -C $(LIBLUX)

# Build the C init binary
$(C_INIT_DIR)/init: $(LIBLUX)/liblux.a $(C_INIT_DIR)/init.c $(C_INIT_DIR)/start.S $(C_INIT_DIR)/exit.c
	$(MAKE) -C $(C_INIT_DIR)

# Build the resurrection server
$(RESURRECTION_DIR)/resurrection: $(LIBLUX)/liblux.a
	$(MAKE) -C $(RESURRECTION_DIR)

# Build the rump server
$(RUMP_DIR)/rump_server: $(LIBLUX)/liblux.a
	$(MAKE) -C $(RUMP_DIR)

# Build turbocid server
$(TURBOCID_DIR)/turbocid: $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(TURBOCID_DIR)

# Build sophia server
$(TURBOCID_DIR)/sophia: $(LIBLUX)/liblux.a $(TURBOCID_DIR)/sophia.c $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(TURBOCID_DIR)

# Build extensive wasm test
$(WASM_EXTENSIVE_DIR)/wasm_test: $(LIBLUX)/liblux.a
	$(MAKE) -C $(WASM_EXTENSIVE_DIR)

# Build srvctl helper
$(SRVCTL_DIR)/srvctl: $(LIBLUX)/liblux.a
	$(MAKE) -C $(SRVCTL_DIR)

# Build HAL server
$(HAL_DIR)/hal: $(LIBLUX)/liblux.a $(HAL_DIR)/hal.c $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(HAL_DIR)

# Build the exec_wasm test
$(BUILD_DIR)/exec_wasm: tests/exec_wasm.c $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ tests/exec_wasm.c $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o -L$(LIBLUX) -llux

# Build the initrd tarball
# Structure:
#   boot/init         <- Init process (PID 1)
#   boot/resurrection <- Resurrection server (started by init)
#   boot/rump_server  <- Rump server (started by resurrection)
#   boot/turbocid     <- Semantic FS (started by resurrection)
#   boot/services.conf <- Resurrection config
#   boot/wasm_test     <- Extensive WASM test runner
#   boot/extensive_test.wasm <- Test module
initrd: $(C_INIT_DIR)/init $(RESURRECTION_DIR)/resurrection $(RUMP_DIR)/rump_server $(TURBOCID_DIR)/turbocid $(TURBOCID_DIR)/sophia $(WASM_EXTENSIVE_DIR)/wasm_test $(SRVCTL_DIR)/srvctl $(HAL_DIR)/hal $(BUILD_DIR)/exec_wasm
	@mkdir -p $(INIT_BOOT_DIR)
	@cp $(C_INIT_DIR)/init $(INIT_BOOT_DIR)/init
	@cp $(RESURRECTION_DIR)/resurrection $(INIT_BOOT_DIR)/resurrection
	@cp $(RUMP_DIR)/rump_server $(INIT_BOOT_DIR)/rump_server
	@cp $(SRVCTL_DIR)/srvctl $(INIT_BOOT_DIR)/srvctl
	@cp $(HAL_DIR)/hal $(INIT_BOOT_DIR)/hal
	@cp $(TURBOCID_DIR)/sophia $(INIT_BOOT_DIR)/sophia
	@cp $(WASM_EXTENSIVE_DIR)/wasm_test $(INIT_BOOT_DIR)/wasm_test
	@cp $(WASM_EXTENSIVE_DIR)/extensive_test.wasm $(INIT_BOOT_DIR)/extensive_test.wasm
	@cp hello.wasm $(INIT_BOOT_DIR)/hello.wasm
	@cp $(BUILD_DIR)/exec_wasm $(INIT_BOOT_DIR)/exec_wasm
	@rm -rf $(INIT_BUILD_DIR)/srv/ahci0
	@mkdir -p $(INIT_BUILD_DIR)/srv/ahci0
	@echo "Creating initrd.tar from $(INIT_BUILD_DIR)..."
	@cd $(INIT_BUILD_DIR) && tar -cvf ../initrd.tar *
	@ls -lh $(BUILD_DIR)/initrd.tar

# Register and sign binaries using init database
sign: $(C_INIT_DIR)/init $(RESURRECTION_DIR)/resurrection $(TURBOCID_DIR)/sophia $(BUILD_DIR)/exec_wasm
	@echo "Registering binaries with init database..."
	@python3 $(INIT_DB) init 2>/dev/null || true
	@python3 $(INIT_DB) register $(C_INIT_DIR)/init "C init (4KB exchange)" 2>/dev/null || \
		echo "  init already registered"
	@python3 $(INIT_DB) register $(RESURRECTION_DIR)/resurrection "Resurrection server" 2>/dev/null || \
	@python3 $(INIT_DB) register $(RUMP_DIR)/rump_server "NetBSD Rump Server" 2>/dev/null || \
		echo "  rump_server already registered"
	@python3 $(INIT_DB) register $(TURBOCID_DIR)/sophia "Sophia Semantic FS" 2>/dev/null || \
		echo "  sophia already registered"
	@python3 $(INIT_DB) register $(BUILD_DIR)/exec_wasm "WASM Exec Test" 2>/dev/null || \
		echo "  exec_wasm already registered"
	@echo "Signing binaries (requires LUX_SIGNING_KEY or TPM)..."
	@if [ -n "$$LUX_SIGNING_KEY" ]; then \
		INIT_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(C_INIT_DIR)/init" | head -1 | awk '{print $$1}'); \
		RES_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(RESURRECTION_DIR)/resurrection" | head -1 | awk '{print $$1}'); \
		RUMP_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(RUMP_DIR)/rump_server" | head -1 | awk '{print $$1}'); \
		SOPHIA_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(TURBOCID_DIR)/sophia" | head -1 | awk '{print $$1}'); \
		WASM_TEST_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(BUILD_DIR)/exec_wasm" | head -1 | awk '{print $$1}'); \
		[ -n "$$INIT_ID" ] && python3 $(INIT_DB) sign $$INIT_ID 2>/dev/null || true; \
		[ -n "$$RES_ID" ] && python3 $(INIT_DB) sign $$RES_ID 2>/dev/null || true; \
		[ -n "$$RUMP_ID" ] && python3 $(INIT_DB) sign $$RUMP_ID 2>/dev/null || true; \
		[ -n "$$SOPHIA_ID" ] && python3 $(INIT_DB) sign $$SOPHIA_ID 2>/dev/null || true; \
		[ -n "$$WASM_TEST_ID" ] && python3 $(INIT_DB) sign $$WASM_TEST_ID 2>/dev/null || true; \
		echo "✅ Binaries signed"; \
	else \
		echo "⚠️  LUX_SIGNING_KEY not set - binaries will be unsigned"; \
		echo "   Set LUX_SIGNING_KEY=<64-char-hex> to enable signing"; \
	fi
	@echo "Verifying binaries..."
	@python3 $(INIT_DB) verify $(C_INIT_DIR)/init || true
	@python3 $(INIT_DB) verify $(RESURRECTION_DIR)/resurrection || true
	@python3 $(INIT_DB) verify $(RUMP_DIR)/rump_server || true
	@python3 $(INIT_DB) verify $(TURBOCID_DIR)/sophia || true
	@python3 $(INIT_DB) verify $(BUILD_DIR)/exec_wasm || true
