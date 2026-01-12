# Makefile for Lux9 Rump Server

TARGET = rump_server
CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld

# Rump Build Directory
RUMPBASE = ../../buildrump.sh
RUMPLIBDIR = $(RUMPBASE)/rump/lib
RUMPINCDIR = $(RUMPBASE)/rump/include

# Lux9 Userspace Flags
CFLAGS = -ffreestanding -fno-builtin -nostdlib -mno-red-zone -fno-pic -fno-pie \
         -I$(RUMPINCDIR) -I. \
         -Wall -Wno-unused-parameter

LDFLAGS = -nostdlib -static -z max-page-size=0x1000

# Rump Libraries to Link (Order matters!)
# librumpfs_ffs librumpfs_cd9660 etc can be added
LIBS = -L$(RUMPLIBDIR) \
       -lrumpvfs \
       -lrump \
       -lrumpuser \
       -lrump \
       -lrumpuser 
       # We might need to cycle refs or use start-group/end-group

# Use --start-group to handle circular deps between rump and rumpuser/vfs
LIBS_GROUP = -L$(RUMPLIBDIR) --start-group -lrump -lrumpvfs -lrumpfs_ffs -lrumpdev -lrumpdev_disk -lrumpdev_dm --end-group

OBJ = start.o exit.o rumpuser_lux9.o rump_server.o
EXIT_OBJ = ../lib9p_syscall/exit.o

all: $(EXIT_OBJ) $(TARGET)

start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

$(EXIT_OBJ):
	$(MAKE) -C ../lib9p_syscall exit.o

exit.o: $(EXIT_OBJ)
	cp $(EXIT_OBJ) exit.o

rumpuser_lux9.o: rumpuser_lux9.c
	$(CC) $(CFLAGS) -c rumpuser_lux9.c -o rumpuser_lux9.o

rump_server.o: rump_server.c
	$(CC) $(CFLAGS) -c rump_server.c -o rump_server.o

$(TARGET): $(OBJ) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(OBJ) $(LIBS_GROUP) -o $@

clean:
	rm -f *.o $(TARGET)
