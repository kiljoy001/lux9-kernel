# Makefile for Fake Server (Resurrection Test)

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

TARGET = fakeserver
SRC = fakeserver.c
OBJ = $(SRC:.c=.o)

START_OBJ = ../lib9p_syscall/start.o

all: $(TARGET)

$(TARGET): $(START_OBJ) $(OBJ) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(OBJ) -o $@
	@echo "Built fakeserver: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)

install: $(TARGET)
	cp $(TARGET) ../../initrd/boot/
	@echo "Installed $(TARGET) to initrd/boot/"

.PHONY: all clean install
