/*
 * Alpine Linux kernel extraction from ISO9660
 * For VMX direct kernel boot
 */

#include <u.h>
#include <libc.h>

#define CDROM_SECTOR_SIZE 2048

/* ISO9660 directory entry */
struct iso_dir_entry {
    u8int length;
    u8int ext_length;
    u32int lba_le;
    u32int lba_be;
    u32int size_le;
    u32int size_be;
    u8int date[7];
    u8int flags;
    u8int file_unit_size;
    u8int interleave_gap;
    u16int vol_seq_le;
    u16int vol_seq_be;
    u8int name_length;
    /* filename follows */
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

    /* Get root directory entry from PVD */
    struct iso_dir_entry *root = (struct iso_dir_entry*)&buffer[156];
    u32int root_lba = root->lba_le;

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
        struct iso_dir_entry *entry = (struct iso_dir_entry*)ptr;
        if(entry->length == 0) break;

        /* Check if this is BOOT directory */
        if(entry->name_length == 4 &&
           memcmp(ptr + sizeof(struct iso_dir_entry), "BOOT", 4) == 0 &&
           (entry->flags & 2)){ /* directory flag */

            print("Alpine: Found BOOT dir at LBA %ud\n", entry->lba_le);

            /* Read boot directory */
            dop.lba = entry->lba_le;
            dop.count = 1;
            ret = process_op(&dop);
            if(ret){
                close(dop.fd);
                return 5;
            }

            /* Search for vmlinuz-virt in boot directory */
            u8int *bootptr = buffer;
            while(bootptr < buffer + CDROM_SECTOR_SIZE){
                struct iso_dir_entry *bootentry = (struct iso_dir_entry*)bootptr;
                if(bootentry->length == 0) break;

                /* Look for vmlinuz-virt (12 chars) */
                if(bootentry->name_length >= 12){
                    char *name = (char*)(bootptr + sizeof(struct iso_dir_entry));
                    if(memcmp(name, "VMLINUZ-VIRT", 12) == 0){
                        print("Alpine: Found kernel at LBA %ud, size %ud\n",
                              bootentry->lba_le, bootentry->size_le);

                        /* Allocate and read kernel */
                        *kernelsize = bootentry->size_le;
                        *kernelimg = malloc(*kernelsize);
                        if(*kernelimg == nil){
                            close(dop.fd);
                            return 6;
                        }

                        /* Read kernel in chunks */
                        u32int sectors = (*kernelsize + CDROM_SECTOR_SIZE - 1) / CDROM_SECTOR_SIZE;
                        u8int *kptr = *kernelimg;
                        dop.lba = bootentry->lba_le;

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
                bootptr += bootentry->length;
            }
            break;
        }
        ptr += entry->length;
    }

    close(dop.fd);
    return 8; /* Kernel not found */
}