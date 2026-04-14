# Legacy wrapper for the canonical root build.

ROOT_DIR := ..
ROOT_MAKEFILE := GNUmakefile

.PHONY: all clean iso run debug test install help

all:
	@echo "Legacy wrapper: delegating kernel build to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) all

clean:
	@echo "Legacy wrapper: delegating clean to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) clean

iso:
	@echo "Legacy wrapper: delegating ISO build to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) iso

run:
	@echo "Legacy wrapper: delegating run to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) run

debug:
	@echo "Legacy wrapper: root build has no dedicated debug target; using direct-run"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) direct-run

test:
	@echo "Legacy wrapper: delegating tests to $(ROOT_DIR)/$(ROOT_MAKEFILE)"
	@$(MAKE) -C $(ROOT_DIR) -f $(ROOT_MAKEFILE) test

install:
	@echo "No standalone install target. Use the repo root build workflow."

help:
	@echo "Legacy kernel wrapper"
	@echo "Use the repo root commands instead:"
	@echo "  make clean"
	@echo "  make"
	@echo "  make iso"
