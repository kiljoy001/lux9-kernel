# Makefile for all Go servers and tools
.PHONY: build clean test all memfs exchange

build: memfs exchange test-client

all: build

# Build memfs server
memfs:
	cd memfs && go build -o memfs

# Build exchange test programs
exchange:
	cd exchange && go build -o exchange_test
	cd exchange && go build -o exchange_9p_test

# Build test client
test-client:
	go build -o test-client test-client.go

# Build everything including test client
build-all: memfs exchange test-client

# Run tests
test: build
	./test_memfs.sh

# Install to initrd
install: build
	cp memfs/memfs ../../initrd/bin/
	cp exchange/exchange_test ../../initrd/bin/
	cp exchange/exchange_9p_test ../../initrd/bin/

# Clean build artifacts
clean:
	rm -f memfs/memfs
	rm -f exchange/exchange_test exchange/exchange_9p_test
	rm -f test-client
	rm -f *.log