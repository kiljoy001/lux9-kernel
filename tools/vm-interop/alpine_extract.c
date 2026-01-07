/*
 * Alpine Linux kernel extraction from ISO9660
 * For VMX direct kernel boot
 */

#include <u.h>
#include <libc.h>

#define CDROM_SECTOR_SIZE 2048

/* ISO9660 directory entry - correct structure per spec */
struct iso_dir_entry {
    u8int length;           /* 0: Length of Directory Record */
    u8int ext_attr_length;  /* 1: Extended Attribute Record length */
    u32int lba_le;          /* 2-5: Location of extent (LBA) little-endian */
    u32int lba_be;          /* 6-9: Location of extent (LBA) big-endian */
    u32int size_le;         /* 10-13: Data length (size) little-endian */
    u32int size_be;         /* 14-17: Data length (size) big-endian */
    u8int date[7];          /* 18-24: Recording date and time */
    u8int flags;            /* 25: File flags */
    u8int file_unit_size;   /* 26: File unit size */
    u8int interleave_gap;   /* 27: Interleave gap size */
    u16int vol_seq_le;      /* 28-29: Volume sequence number LE */
    u16int vol_seq_be;      /* 30-31: Volume sequence number BE */
    u8int name_length;      /* 32: Length of file identifier */
    /* 33+: File identifier follows */
};

/* Disk operation structure */
struct disk_op_s {
    int fd;
    u32int lba;
    int count;
    void *buf;
};

static int
process_op(struct disk_op_s *dop)
{
    vlong off = (vlong)dop->lba * CDROM_SECTOR_SIZE;
    int nbytes = dop->count * CDROM_SECTOR_SIZE;

    if(seek(dop->fd, off, 0) != off)
        return -1;

    if(readn(dop->fd, dop->buf, nbytes) != nbytes)
        return -1;

    return 0;
}

/*
 * Extract Alpine Linux kernel from ISO9660 filesystem
 * Returns: 0 on success, error code on failure
 */
int
extract_alpine_kernel(char *isofile, void **kernelimg, long *kernelsize)
{
    struct disk_op_s dop;
    u8int buffer[CDROM_SECTOR_SIZE];
    int ret;

    dop.fd = open(isofile, OREAD);
    if(dop.fd < 0)
        return 1;

    /* Read Primary Volume Descriptor at sector 16 */
    dop.lba = 16;
    dop.count = 1;
    dop.buf = buffer;
    ret = process_op(&dop);
    if(ret){
        close(dop.fd);
        return 2;
    }

    /* Check PVD signature */
    if(buffer[0] != 1 || memcmp(&buffer[1], "CD001", 5) != 0){
        close(dop.fd);
        return 3;
    }

    /* Get root directory entry from PVD - it's at offset 156 */
    /* Read LBA directly from bytes to avoid struct alignment issues */
    u32int root_lba = buffer[156+2] | (buffer[156+3]<<8) | (buffer[156+4]<<16) | (buffer[156+5]<<24);

    print("Alpine: Root dir at LBA %ud\n", root_lba);

    /* Search for boot directory */
    dop.lba = root_lba;
    dop.count = 1;
    ret = process_op(&dop);
    if(ret){
        close(dop.fd);
        return 4;
    }

    /* Simple scan for "BOOT" directory in root */
    u8int *ptr = buffer;
    u8int *end = buffer + CDROM_SECTOR_SIZE;

    while(ptr < end){
        u8int entry_len = ptr[0];
        if(entry_len == 0) break;

        u8int name_len = ptr[32];  /* Name length is at offset 32 */
        u8int flags = ptr[25];     /* Flags at offset 25 */

        /* Print directory entry for debugging */
        if(name_len > 0 && name_len < 32){
            char name[33];
            memcpy(name, ptr + 33, name_len);
            name[name_len] = 0;
            print("Alpine: Found entry '%s' (len=%d, flags=%02x)\n", name, name_len, flags);
        }

        /* Check if this is BOOT directory */
        if(name_len == 4 &&
           memcmp(ptr + 33, "BOOT", 4) == 0 &&
           (flags & 2)){ /* directory flag */

            /* Extract LBA from the directory entry */
            u32int boot_lba = ptr[2] | (ptr[3]<<8) | (ptr[4]<<16) | (ptr[5]<<24);
            print("Alpine: Found BOOT dir at LBA %ud\n", boot_lba);

            /* Read boot directory */
            dop.lba = boot_lba;
            dop.count = 1;
            ret = process_op(&dop);
            if(ret){
                close(dop.fd);
                return 5;
            }

            /* Search for vmlinuz-virt in boot directory */
            u8int *bootptr = buffer;
            while(bootptr < buffer + CDROM_SECTOR_SIZE){
                u8int boot_entry_len = bootptr[0];
                if(boot_entry_len == 0) break;

                u8int boot_name_len = bootptr[32];

                /* Debug: print all entries in boot directory */
                if(boot_name_len > 0 && boot_name_len < 32){
                    char bootname[33];
                    memcpy(bootname, bootptr + 33, boot_name_len);
                    bootname[boot_name_len] = 0;
                    print("Alpine: BOOT entry '%s' (len=%d)\n", bootname, boot_name_len);
                }

                /* Look for vmlinuz-virt - handle ISO9660 version suffix */
                if(boot_name_len >= 7){
                    char *name = (char*)(bootptr + 33);  /* Name at offset 33 */
                    /* Check if name starts with VMLINUZ-VIRT */
                    if(memcmp(name, "VMLINUZ-VIRT", 12) == 0 ||
                       memcmp(name, "VMLINUZ-LTS", 11) == 0 ||
                       memcmp(name, "VMLINUZ", 7) == 0){
                        /* Extract LBA and size from the directory entry */
                        u32int kernel_lba = bootptr[2] | (bootptr[3]<<8) | (bootptr[4]<<16) | (bootptr[5]<<24);
                        u32int kernel_size = bootptr[10] | (bootptr[11]<<8) | (bootptr[12]<<16) | (bootptr[13]<<24);

                        print("Alpine: Found kernel at LBA %ud, size %ud\n", kernel_lba, kernel_size);

                        /* Allocate and read kernel */
                        *kernelsize = kernel_size;
                        *kernelimg = malloc(*kernelsize);
                        if(*kernelimg == nil){
                            close(dop.fd);
                            return 6;
                        }

                        /* Read kernel in chunks */
                        u32int sectors = (*kernelsize + CDROM_SECTOR_SIZE - 1) / CDROM_SECTOR_SIZE;
                        u8int *kptr = *kernelimg;
                        dop.lba = kernel_lba;

                        while(sectors > 0){
                            int count = sectors > 32 ? 32 : sectors;
                            dop.count = count;
                            dop.buf = kptr;
                            ret = process_op(&dop);
                            if(ret){
                                free(*kernelimg);
                                close(dop.fd);
                                return 7;
                            }
                            sectors -= count;
                            dop.lba += count;
                            kptr += count * CDROM_SECTOR_SIZE;
                        }

                        close(dop.fd);
                        return 0; /* Success! */
                    }
                }
                bootptr += boot_entry_len;
            }
            break;
        }
        ptr += entry_len;
    }

    close(dop.fd);
    return 8; /* Kernel not found */
}