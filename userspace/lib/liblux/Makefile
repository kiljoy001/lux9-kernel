CC = gcc
AR = ar
CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -I./inc -I../../../kernel/include -I../../../kernel/port

C_SRCS = src/syscalls.c src/msgord.c src/pebble.c src/string.c \
         src/convM2S.c src/convS2M.c
ASM_SRCS = src/syscall_asm.S

OBJS = $(C_SRCS:.c=.o) $(ASM_SRCS:.S=.o)

all: liblux.a

liblux.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) liblux.a