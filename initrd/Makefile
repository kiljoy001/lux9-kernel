# Legacy wrapper for the canonical userspace initrd build.

ROOT_DIR := ..
ROOT_MAKEFILE := GNUmakefile

.PHONY: all clean help

all:
	@echo "Legacy wrapper: delegating initrd build to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) userspace-all

clean:
	@echo "Legacy wrapper: delegating userspace clean"
	@$(MAKE) -C ../userspace clean

help:
	@echo "Legacy initrd wrapper"
	@echo "Use the repo root commands instead:"
	@echo "  make clean"
	@echo "  make"
	@echo "  make iso"
