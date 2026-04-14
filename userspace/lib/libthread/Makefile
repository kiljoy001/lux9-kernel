CC = gcc
AR = ar
CFLAGS = -Iinc -I../liblux/inc -ffreestanding -O2 -Wall -c

SRCS = src/thread.c src/channel.c
OBJS = $(SRCS:.c=.o)

all: libthread.a

libthread.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) libthread.a
