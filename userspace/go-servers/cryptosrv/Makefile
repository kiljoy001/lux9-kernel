# Makefile for cryptosrv - Go-based crypto server using SIP framework

TARGET := cryptosrv
SRC := main.go cryptofs.go blake2b.go ed25519.go

# Go build configuration
GOOS := plan9
GOARCH := amd64
GO := go
GOFLAGS :=

# Module path
MODULE := lux9/servers

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "Building $(TARGET) for Plan 9..."
	GOOS=$(GOOS) GOARCH=$(GOARCH) $(GO) build $(GOFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

test:
	$(GO) test -v ./...

fmt:
	$(GO) fmt ./...

vet:
	$(GO) vet ./...

.PHONY: all clean test fmt vet
