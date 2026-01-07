# Makefile for Resurrection Server
# Layer 2 service monitor using liblux syscall library

KERNEL = ../../kernel
LIBLUX = ../lib/liblux

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I$(LIBLUX)/inc

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

TARGET = resurrection
SRC = resurrection.c
OBJ = $(SRC:.c=.o) blind_cap.o monocypher.o

# Use same start.S as init
START_OBJ = ../lib9p_syscall/start.o

all: $(START_OBJ) $(TARGET)

$(TARGET): $(START_OBJ) $(OBJ) $(LIBLUX)/liblux.a ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(OBJ) $(LIBLUX)/liblux.a -o $@
	@echo "Built resurrection server: $@"
	@ls -lh $@

$(START_OBJ):
	$(MAKE) -C ../lib9p_syscall start.o

# Update clean rule to include crypto objects? (OBJ handles it)

blind_cap.o: $(KERNEL)/crypto/blind_cap.c
	$(CC) $(CFLAGS) -c -o $@ $<

monocypher.o: $(KERNEL)/crypto/monocypher.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Cleaned resurrection build"

install: $(TARGET)
	cp $(TARGET) ../../initrd/boot/
	@echo "Installed $(TARGET) to initrd/boot/"

.PHONY: all clean install
