# Makefile for Resurrection Server (Phase 1)
# Native C userspace service monitor running at Layer 2

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDFLAGS = -static

TARGET = resurrection
SRC = resurrection.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Built resurrection server binary: $@"
	@ls -lh $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Cleaned resurrection build artifacts"

install: $(TARGET)
	cp $(TARGET) ../../initrd/boot/
	@echo "Installed $(TARGET) to initrd/boot/"

.PHONY: all clean install
