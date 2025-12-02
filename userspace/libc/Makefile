CC = gcc
AR = ar
# Get GCC internal headers
GCC_INC := $(shell $(CC) -print-file-name=include)

CFLAGS = -nostdlib -fno-builtin -m64 -O2 -I../include -I../lib -I../../kernel/libc9 -I../../kernel/include -I$(GCC_INC)
ASFLAGS = -m64

LIBC_DIR = ../../kernel/libc9
SYSCALL_DIR = ../lib

# Sources
# Use all kernel libc sources except kernel-specific ones that we replace
KERNEL_LIBC_SRCS = $(wildcard $(LIBC_DIR)/*.c)
# We replace errstr.c locally. We keep pool.c but provide stubs.
LIBC_SRCS = $(filter-out $(LIBC_DIR)/errstr.c, $(KERNEL_LIBC_SRCS))

SYSCALL_SRCS = $(SYSCALL_DIR)/syscall.c
LOCAL_SRCS = malloc.c errstr.c stub.c print.c

# Objects (flattened)
LIBC_OBJS = $(notdir $(LIBC_SRCS:.c=.o))
SYSCALL_OBJS = syscall.o
LOCAL_OBJS = malloc.o syscall_amd64.o errstr.o stub.o print.o

ALL_OBJS = $(LIBC_OBJS) $(SYSCALL_OBJS) $(LOCAL_OBJS)

vpath %.c $(LIBC_DIR) $(SYSCALL_DIR) .
vpath %.S $(SYSCALL_DIR) .

all: ../lib/libc.a ../lib/crt0.o

../lib/libc.a: $(ALL_OBJS)
	$(AR) rcs $@ $(ALL_OBJS)

../lib/crt0.o: ../lib/crt0.S
	$(CC) $(ASFLAGS) -c $< -o $@

syscall_amd64.o: syscall_amd64.S
	$(CC) $(ASFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o ../lib/libc.a ../lib/crt0.o