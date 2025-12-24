##########################################################################
# Frama-C Plan 9 Plugin Makefile
##########################################################################

# Plugin name
PLUGIN_NAME = Plan9

# Plugin sources
PLUGIN_CMO = plan9_plugin

# Use Frama-C's plugin infrastructure
ifndef FRAMAC_SHARE
  FRAMAC_SHARE := $(shell frama-c -print-share-path)
endif

ifndef FRAMAC_LIBDIR
  FRAMAC_LIBDIR := $(shell frama-c -print-libpath)
endif

# Include Frama-C's plugin makefile
include $(FRAMAC_SHARE)/Makefile.dynamic

# Plugin metadata
PLUGIN_DESCRIPTION = "Enables formal verification of Plan 9 C code"
PLUGIN_VERSION = 1.0

# Build flags
OCAMLC_FLAGS = -w +a-4-9-41-42-44-48-50
OCAMLOPT_FLAGS = $(OCAMLC_FLAGS)

# Installation
install::
	@echo "Installing Plan9 plugin to $(FRAMAC_LIBDIR)"
	@mkdir -p $(FRAMAC_LIBDIR)/plugins
	@cp -f Plan9.cm* $(FRAMAC_LIBDIR)/plugins/ 2>/dev/null || true

# Testing
test: all
	@echo "Testing Plan9 plugin..."
	@frama-c -load-module ./Plan9.cmxs -plan9 -plan9-verbose -print test_plan9.c

.PHONY: install test

# Clean
clean::
	rm -f *.cm* *.o *.a *.so *.cmxs .depend

# Help
help:
	@echo "Frama-C Plan9 Plugin Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the plugin"
	@echo "  install   - Install plugin to Frama-C"
	@echo "  test      - Test the plugin"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make all"
	@echo "  make install"
	@echo "  frama-c -load-module Plan9.cmxs -plan9 yourfile.c"
