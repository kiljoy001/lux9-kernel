# Makefile for Lux9 Rump Server

TARGET = rump_server
CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld

# Rump Build Directory
RUMPBASE = ../../buildrump.sh
RUMPLIBDIR = $(RUMPBASE)/rump/lib
RUMPINCDIR = $(RUMPBASE)/rump/include

# Lux9 Library
LIBLUX_DIR = ../lib/liblux
LIBLUX = $(LIBLUX_DIR)/liblux.a

# Lux9 Userspace Flags
CFLAGS = -ffreestanding -fno-builtin -nostdlib -mno-red-zone -fno-pic -fno-pie \
         -I$(RUMPINCDIR) -I. -I$(LIBLUX_DIR)/inc \
         -Wall -Wno-unused-parameter

LDFLAGS = -nostdlib -static -z max-page-size=0x1000 --allow-multiple-definition

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
# WORKING CONFIGURATION - Deferred: USB (needs DMA layer), null/union FS (need layer support)
# Includes: Common FS, full TCP/IP, Bluetooth, crypto, devices
LIBS_GROUP = -L$(RUMPLIBDIR) \
       --whole-archive --start-group \
       -lrump -lrumpvfs \
       -lrumpfs_ffs -lrumpfs_ext2fs -lrumpfs_tmpfs -lrumpfs_msdos \
       -lrumpfs_cd9660 -lrumpfs_nfs -lrumpfs_fdesc -lrumpfs_kernfs \
       -lrumpdev -lrumpdev_disk -lrumpdev_dm -lrumpdev_bpf \
       -lrumpdev_scsipi -lrumpdev_cgd -lrumpdev_vnd -lrumpdev_md \
       -lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_netinet6 \
       -lrumpnet_local -lrumpnet_netbt \
       -lrumpkern_crypto -lrumpkern_z \
       --end-group --no-whole-archive

OBJ = start.o exit.o rumpuser_lux9.o rump_server.o
EXIT_OBJ = ../lib9p_syscall/exit.o

all: $(LIBLUX) $(EXIT_OBJ) $(TARGET)

$(LIBLUX):
	$(MAKE) -C $(LIBLUX_DIR)

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

$(TARGET): $(OBJ) $(LIBLUX) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(OBJ) $(LIBS_GROUP) $(LIBLUX) -o $@

clean:
	rm -f *.o $(TARGET)
