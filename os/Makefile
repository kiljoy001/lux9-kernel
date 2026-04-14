# Legacy wrapper for the canonical root OS build.

ROOT_DIR := ..
ROOT_MAKEFILE := GNUmakefile
ISO := ../lux9.iso

.PHONY: all clean test run debug help

all:
	@echo "Legacy wrapper: delegating OS build to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) iso

clean:
	@echo "Legacy wrapper: delegating clean to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) clean

test: all
	@echo "Running legacy OS smoke test against $(ISO)..."
	@timeout 30s qemu-system-x86_64 \
		-cdrom $(ISO) -boot d \
		-m 2G -smp 4 \
		-nographic -serial mon:stdio \
		-display none \
		-no-reboot || echo "Test completed"

run: all
	@echo "Running legacy OS wrapper against $(ISO)..."
	@qemu-system-x86_64 \
		-cdrom $(ISO) -boot d \
		-m 2G -smp 4 \
		-chardev socket,id=chrtpm,path=../swtpm/swtpm-sock \
		-tpmdev emulator,id=tpm0,chardev=chrtpm \
		-device tpm-tis,tpmdev=tpm0 \
		-nographic -serial stdio \
		-display none

debug: all
	@echo "Debugging legacy OS wrapper against $(ISO)..."
	@qemu-system-x86_64 \
		-cdrom $(ISO) -boot d \
		-m 2G -smp 4 \
		-nographic -serial stdio \
		-display none \
		-s -S

help:
	@echo "Legacy OS wrapper"
	@echo "Use the repo root commands instead:"
	@echo "  make clean"
	@echo "  make"
	@echo "  make iso"
