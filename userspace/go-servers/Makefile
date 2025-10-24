# Makefile for exchange test program

.PHONY: build clean

build:
	cd exchange && go build -o exchange_test
	cd exchange && go build -o exchange_9p_test

clean:
	rm -f exchange/exchange_test exchange/exchange_9p_test

install: build
	cp exchange/exchange_test ../../initrd/bin/
	cp exchange/exchange_9p_test ../../initrd/bin/