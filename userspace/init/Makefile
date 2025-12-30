# Makefile for init - First userspace process (PID 1)
# Spawns and monitors resurrection server as first Layer 2 service

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDFLAGS = -static

TARGET = init
SRC = init.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Built init binary: $@"
	@ls -lh $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Cleaned init build artifacts"

install: $(TARGET)
	cp $(TARGET) ../../initrd/boot/
	@echo "Installed $(TARGET) to initrd/boot/"

.PHONY: all clean install
