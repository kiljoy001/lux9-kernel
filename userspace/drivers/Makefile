# Userspace Drivers Makefile

# Check if Go is available
GO_AVAILABLE := $(shell command -v go >/dev/null 2>&1 && echo YES || echo NO)

# Go compiler
GO := go

# Build flags
GOFLAGS := -v

# Driver binaries
DRIVERS := ahci_driver ide_driver

# Default target
all: check-go $(DRIVERS)

# Check if Go is available
check-go:
ifeq ($(GO_AVAILABLE),NO)
	@echo "Warning: Go not found. Skipping driver compilation."
	@echo "Please install Go to build userspace drivers."
	@exit 0
endif

# Build AHCI driver
ahci_driver: storage/ahci_main.go storage/ahci.go storage/storage.go
ifeq ($(GO_AVAILABLE),YES)
	$(GO) build $(GOFLAGS) -o $@ storage/ahci_main.go
else
	@echo "Skipping AHCI driver build - Go not available"
endif

# Build IDE driver
ide_driver: storage/ide_main.go storage/ide.go storage/storage.go
ifeq ($(GO_AVAILABLE),YES)
	$(GO) build $(GOFLAGS) -o $@ storage/ide_main.go
else
	@echo "Skipping IDE driver build - Go not available"
endif

# Install drivers to system
install: $(DRIVERS)
	mkdir -p /usr/local/bin
	cp $(DRIVERS) /usr/local/bin/

# Clean build artifacts
clean:
	rm -f $(DRIVERS)

# Run AHCI driver
run-ahci: ahci_driver
	./ahci_driver

# Run IDE driver
run-ide: ide_driver
	./ide_driver

.PHONY: all check-go install clean run-ahci run-ide
