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
NSD_DIR := ns/nsd
PROCD_DIR := procd
ENVD_DIR := envd
SYMBOLICD_DIR := symbolicd
BENCH_DIR := bench

KERNEL = ../kernel
LIBLUX = lib/liblux

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fno-pic -fno-pie -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I$(LIBLUX)/inc

.PHONY: all clean initrd sign liblux FORCE

all: initrd sign

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(C_INIT_DIR) clean
	$(MAKE) -C $(RESURRECTION_DIR) clean
	$(MAKE) -C $(RUMP_DIR) clean
	$(MAKE) -C $(TURBOCID_DIR) clean
	$(MAKE) -C $(WASM_EXTENSIVE_DIR) clean
	$(MAKE) -C $(SRVCTL_DIR) clean
	$(MAKE) -C $(HAL_DIR) clean
	$(MAKE) -C $(NSD_DIR) clean
	$(MAKE) -C $(PROCD_DIR) clean
	$(MAKE) -C $(ENVD_DIR) clean
	$(MAKE) -C $(SYMBOLICD_DIR) clean
	$(MAKE) -C $(BENCH_DIR) clean
	$(MAKE) -C $(LIBLUX) clean

# Build liblux.
# FORCE ensures top-level make always reconciles subdir dependencies.
$(LIBLUX)/liblux.a: FORCE
	$(MAKE) -C $(LIBLUX)

FORCE:

# Build the C init binary
$(C_INIT_DIR)/init: $(LIBLUX)/liblux.a $(C_INIT_DIR)/init.c $(C_INIT_DIR)/start.S $(C_INIT_DIR)/exit.c
	$(MAKE) -C $(C_INIT_DIR)

# Build the resurrection server
$(RESURRECTION_DIR)/resurrection: $(LIBLUX)/liblux.a $(RESURRECTION_DIR)/resurrection.c
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
HAL_SRCS := $(wildcard $(HAL_DIR)/*.c) $(wildcard $(HAL_DIR)/*.h) \
            $(wildcard $(HAL_DIR)/*.S)

$(HAL_DIR)/hal: $(LIBLUX)/liblux.a $(HAL_SRCS) $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(HAL_DIR)

# Build nsd (userspace namespace service)
$(NSD_DIR)/nsd: $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(NSD_DIR)

# Build procd (userspace proc manager)
$(PROCD_DIR)/procd: $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(PROCD_DIR)

# Build envd (userspace environment service)
$(ENVD_DIR)/envd: $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(ENVD_DIR)

# Build symbolicd (userspace symbolic reasoning server)
$(SYMBOLICD_DIR)/symbolicd: $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(SYMBOLICD_DIR)

# Build ipc_bench
$(BENCH_DIR)/ipc_bench: $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	$(MAKE) -C $(BENCH_DIR)

# Build the exec_wasm test
$(BUILD_DIR)/exec_wasm: tests/exec_wasm.c $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -no-pie -o $@ tests/exec_wasm.c $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o -L$(LIBLUX) -llux

# Build the wasm smoke module
$(BUILD_DIR)/wasm_smoke.wasm: tests/wasm_smoke.wat
	@mkdir -p $(BUILD_DIR)
	wat2wasm tests/wasm_smoke.wat -o $@

# Build the print_fid1 test
$(BUILD_DIR)/print_fid1: tests/print_fid1.c $(LIBLUX)/liblux.a $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -no-pie -o $@ tests/print_fid1.c $(C_INIT_DIR)/start.o $(C_INIT_DIR)/exit.o -L$(LIBLUX) -llux

# Build the initrd tarball
initrd: $(C_INIT_DIR)/init $(RESURRECTION_DIR)/resurrection $(TURBOCID_DIR)/turbocid $(TURBOCID_DIR)/sophia $(WASM_EXTENSIVE_DIR)/wasm_test $(SRVCTL_DIR)/srvctl $(HAL_DIR)/hal $(NSD_DIR)/nsd $(PROCD_DIR)/procd $(ENVD_DIR)/envd $(SYMBOLICD_DIR)/symbolicd $(BUILD_DIR)/exec_wasm $(BUILD_DIR)/wasm_smoke.wasm $(BUILD_DIR)/print_fid1 $(BENCH_DIR)/ipc_bench
	@mkdir -p $(INIT_BOOT_DIR)
	@cp $(C_INIT_DIR)/init $(INIT_BOOT_DIR)/init
	@cp $(RESURRECTION_DIR)/resurrection $(INIT_BOOT_DIR)/resurrection
	@cp $(SRVCTL_DIR)/srvctl $(INIT_BOOT_DIR)/srvctl
	@cp $(HAL_DIR)/hal $(INIT_BOOT_DIR)/hal
	@cp $(NSD_DIR)/nsd $(INIT_BOOT_DIR)/nsd
	@cp $(PROCD_DIR)/procd $(INIT_BOOT_DIR)/procd
	@cp $(ENVD_DIR)/envd $(INIT_BOOT_DIR)/envd
	@cp $(SYMBOLICD_DIR)/symbolicd $(INIT_BOOT_DIR)/symbolicd
	@cp $(BENCH_DIR)/ipc_bench $(INIT_BOOT_DIR)/ipc_bench
	@cp $(TURBOCID_DIR)/sophia $(INIT_BOOT_DIR)/sophia
	@cp $(WASM_EXTENSIVE_DIR)/wasm_test $(INIT_BOOT_DIR)/wasm_test
	@cp $(WASM_EXTENSIVE_DIR)/extensive_test.wasm $(INIT_BOOT_DIR)/extensive_test.wasm
	@cp hello.wasm $(INIT_BOOT_DIR)/hello.wasm
	@cp $(BUILD_DIR)/exec_wasm $(INIT_BOOT_DIR)/exec_wasm
	@cp $(BUILD_DIR)/wasm_smoke.wasm $(INIT_BOOT_DIR)/wasm_smoke.wasm
	@cp $(BUILD_DIR)/print_fid1 $(INIT_BOOT_DIR)/print_fid1
	@rm -rf $(INIT_BUILD_DIR)/srv/ahci0
	@mkdir -p $(INIT_BUILD_DIR)/srv/ahci0
	@echo "Creating initrd.tar from $(INIT_BUILD_DIR)..."
	@cd $(INIT_BUILD_DIR) && tar -cvf ../initrd.tar *
	@ls -lh $(BUILD_DIR)/initrd.tar

# Register and sign binaries using init database
sign: $(C_INIT_DIR)/init $(RESURRECTION_DIR)/resurrection $(NSD_DIR)/nsd $(PROCD_DIR)/procd $(ENVD_DIR)/envd $(SYMBOLICD_DIR)/symbolicd $(TURBOCID_DIR)/sophia $(BUILD_DIR)/exec_wasm $(BUILD_DIR)/print_fid1 $(BENCH_DIR)/ipc_bench
	@echo "Registering binaries with init database..."
	@python3 $(INIT_DB) init 2>/dev/null || true
	@python3 $(INIT_DB) register $(C_INIT_DIR)/init "C init (4KB exchange)" 2>/dev/null || \
		echo "  init already registered"
	@python3 $(INIT_DB) register $(RESURRECTION_DIR)/resurrection "Resurrection server" 2>/dev/null || \
		echo "  resurrection already registered"
	@python3 $(INIT_DB) register $(NSD_DIR)/nsd "Userspace Namespace Service" 2>/dev/null || \
		echo "  nsd already registered"
	@python3 $(INIT_DB) register $(PROCD_DIR)/procd "Userspace Proc Manager" 2>/dev/null || \
		echo "  procd already registered"
	@python3 $(INIT_DB) register $(ENVD_DIR)/envd "Userspace Environment Service" 2>/dev/null || \
		echo "  envd already registered"
	@python3 $(INIT_DB) register $(SYMBOLICD_DIR)/symbolicd "Symbolic Reasoning Server" 2>/dev/null || \
		echo "  symbolicd already registered"
	@python3 $(INIT_DB) register $(TURBOCID_DIR)/sophia "Sophia Semantic FS" 2>/dev/null || \
		echo "  sophia already registered"
	@python3 $(INIT_DB) register $(BUILD_DIR)/exec_wasm "WASM Exec Test" 2>/dev/null || \
		echo "  exec_wasm already registered"
	@python3 $(INIT_DB) register $(BUILD_DIR)/print_fid1 "Print Fid 1 Test" 2>/dev/null || \
		echo "  print_fid1 already registered"
	@python3 $(INIT_DB) register $(BENCH_DIR)/ipc_bench "IPC Benchmark" 2>/dev/null || \
		echo "  ipc_bench already registered"
	@echo "Signing binaries (requires LUX_SIGNING_KEY or TPM)..."
	@if [ -n "$$LUX_SIGNING_KEY" ]; then \
		INIT_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(C_INIT_DIR)/init" | head -1 | awk '{print $$1}'); \
		RES_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(RESURRECTION_DIR)/resurrection" | head -1 | awk '{print $$1}'); \
		NSD_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(NSD_DIR)/nsd" | head -1 | awk '{print $$1}'); \
		PROCD_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(PROCD_DIR)/procd" | head -1 | awk '{print $$1}'); \
		ENVD_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(ENVD_DIR)/envd" | head -1 | awk '{print $$1}'); \
		SYMBOLICD_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(SYMBOLICD_DIR)/symbolicd" | head -1 | awk '{print $$1}'); \
		SOPHIA_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(TURBOCID_DIR)/sophia" | head -1 | awk '{print $$1}'); \
		WASM_TEST_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(BUILD_DIR)/exec_wasm" | head -1 | awk '{print $$1}'); \
		PRINT_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(BUILD_DIR)/print_fid1" | head -1 | awk '{print $$1}'); \
		BENCH_ID=$$(python3 $(INIT_DB) list 2>/dev/null | grep "$(BENCH_DIR)/ipc_bench" | head -1 | awk '{print $$1}'); \
		[ -n "$$INIT_ID" ] && python3 $(INIT_DB) sign $$INIT_ID 2>/dev/null || true; \
		[ -n "$$RES_ID" ] && python3 $(INIT_DB) sign $$RES_ID 2>/dev/null || true; \
		[ -n "$$NSD_ID" ] && python3 $(INIT_DB) sign $$NSD_ID 2>/dev/null || true; \
		[ -n "$$PROCD_ID" ] && python3 $(INIT_DB) sign $$PROCD_ID 2>/dev/null || true; \
		[ -n "$$ENVD_ID" ] && python3 $(INIT_DB) sign $$ENVD_ID 2>/dev/null || true; \
		[ -n "$$SYMBOLICD_ID" ] && python3 $(INIT_DB) sign $$SYMBOLICD_ID 2>/dev/null || true; \
		[ -n "$$SOPHIA_ID" ] && python3 $(INIT_DB) sign $$SOPHIA_ID 2>/dev/null || true; \
		[ -n "$$WASM_TEST_ID" ] && python3 $(INIT_DB) sign $$WASM_TEST_ID 2>/dev/null || true; \
		[ -n "$$PRINT_ID" ] && python3 $(INIT_DB) sign $$PRINT_ID 2>/dev/null || true; \
		[ -n "$$BENCH_ID" ] && python3 $(INIT_DB) sign $$BENCH_ID 2>/dev/null || true; \
		echo "✅ Binaries signed"; \
	else \
		echo "⚠️  LUX_SIGNING_KEY not set - binaries will be unsigned"; \
		echo "   Set LUX_SIGNING_KEY=<64-char-hex> to enable signing"; \
	fi
	@echo "Verifying binaries..."
	@python3 $(INIT_DB) verify $(C_INIT_DIR)/init || true
	@python3 $(INIT_DB) verify $(RESURRECTION_DIR)/resurrection || true
	@python3 $(INIT_DB) verify $(NSD_DIR)/nsd || true
	@python3 $(INIT_DB) verify $(PROCD_DIR)/procd || true
	@python3 $(INIT_DB) verify $(ENVD_DIR)/envd || true
	@python3 $(INIT_DB) verify $(SYMBOLICD_DIR)/symbolicd || true
	@python3 $(INIT_DB) verify $(TURBOCID_DIR)/sophia || true
	@python3 $(INIT_DB) verify $(BUILD_DIR)/exec_wasm || true
	@python3 $(INIT_DB) verify $(BUILD_DIR)/print_fid1 || true
