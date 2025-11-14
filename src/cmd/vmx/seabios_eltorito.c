/*
 * SeaBIOS El Torito CD boot code adapted for 9front VMX
 * Original Copyright (C) 2008,2009 Kevin O'Connor <kevin@koconnor.net>
 * Original Copyright (C) 2002 MandrakeSoft S.A.
 * Distributed under GNU LGPLv3 license
 * 
 * Adapted for Plan 9 VMX - just copy and integrate!
 */

#include <u.h>
#include <libc.h>

#define CDROM_SECTOR_SIZE 2048
#define DISK_SECTOR_SIZE 512

/* SeaBIOS's exact El Torito structures */
struct eltorito_s {
    u8int media;
    u8int emulated_drive;
    u32int ilba;
    u16int sector_count;
    u16int load_segment;
    u16int buffer_segment;
    
    /* Geometry */
    struct {
        u16int sptcyl;  /* sectors per track/cylinder */
        u16int cyllow;
        u8int heads;
    } chs;
    
    u8int controller_index;
    u8int device_spec;
    u32int size;
};

/* Disk operation structure - simplified for Plan 9 */
struct disk_op_s {
    int fd;         /* file descriptor */
    u32int lba;     /* logical block address */
    int count;      /* sector count */
    void *buf;      /* buffer */
};

/* Read CD sectors - direct copy of SeaBIOS logic */
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
 * SeaBIOS cdrom_boot() - EXACT copy of algorithm
 * Returns: 0 on success, error code on failure
 */
int
seabios_cdrom_boot(char *isofile, struct eltorito_s *CDEmu, void **bootimg, long *bootsize)
{
    struct disk_op_s dop;
    int ret;
    u8int buffer[CDROM_SECTOR_SIZE];
    
    /* Open ISO file */
    dop.fd = open(isofile, OREAD);
    if(dop.fd < 0)
        return 1;
    
    /* Read the Boot Record Volume Descriptor */
    memset(&dop, 0, sizeof(dop));
    dop.fd = open(isofile, OREAD);
    dop.lba = 0x11;  /* Sector 17 - El Torito location */
    dop.count = 1;
    dop.buf = buffer;
    ret = process_op(&dop);
    if(ret)
        return 3;
    
    /* Validity checks - EXACT from SeaBIOS */
    if(buffer[0])
        return 4;
    if(strcmp((char*)&buffer[1], "CD001\001EL TORITO SPECIFICATION") != 0)
        return 5;
    
    /* ok, now we calculate the Boot catalog address */
    u32int lba = *(u32int*)&buffer[0x47];
    
    /* And we read the Boot Catalog */
    dop.lba = lba;
    dop.count = 1;
    ret = process_op(&dop);
    if(ret)
        return 7;
    
    /* Validation entry - EXACT checks from SeaBIOS */
    if(buffer[0x00] != 0x01)
        return 8;   /* Header */
    if(buffer[0x01] != 0x00)
        return 9;   /* Platform */
    if(buffer[0x1E] != 0x55)
        return 10;  /* key 1 */
    if(buffer[0x1F] != 0xAA)
        return 10;  /* key 2 */
    
    /* Initial/Default Entry */
    if(buffer[0x20] != 0x88)
        return 11;  /* Bootable */
    
    /* Fill in el-torito cdrom emulation fields */
    u8int media = buffer[0x21];
    CDEmu->media = media;
    
    u16int boot_segment = *(u16int*)&buffer[0x22];
    if(!boot_segment)
        boot_segment = 0x07C0;
    CDEmu->load_segment = boot_segment;
    CDEmu->buffer_segment = 0x0000;
    
    u16int nbsectors = *(u16int*)&buffer[0x26];
    CDEmu->sector_count = nbsectors;
    
    lba = *(u32int*)&buffer[0x28];
    CDEmu->ilba = lba;
    
    /* Allocate memory for boot image */
    int imgsize = ((nbsectors + 3) / 4) * CDROM_SECTOR_SIZE;
    *bootimg = malloc(imgsize);
    if(*bootimg == nil){
        close(dop.fd);
        return 12;
    }
    
    /* And we read the image in memory - EXACT from SeaBIOS */
    nbsectors = (nbsectors + 3) / 4;  /* DIV_ROUND_UP */
    dop.lba = lba;
    dop.buf = *bootimg;
    u8int *bufptr = *bootimg;
    
    while(nbsectors){
        int count = nbsectors;
        if(count > 64*1024/CDROM_SECTOR_SIZE)
            count = 64*1024/CDROM_SECTOR_SIZE;
        dop.count = count;
        dop.buf = bufptr;
        ret = process_op(&dop);
        if(ret){
            free(*bootimg);
            close(dop.fd);
            return 12;
        }
        nbsectors -= count;
        dop.lba += count;
        bufptr += count * CDROM_SECTOR_SIZE;
    }
    
    *bootsize = imgsize;
    
    if(media == 0){
        /* No emulation requested - return success */
        CDEmu->emulated_drive = 0xE0;  /* EXTSTART_CD */
        close(dop.fd);
        return 0;
    }
    
    /* Emulation of a floppy/harddisk requested */
    if(media < 4){
        /* Floppy emulation */
        CDEmu->emulated_drive = 0x00;
        
        switch(media){
        case 0x01:  /* 1.2M floppy */
            CDEmu->chs.sptcyl = 15;
            CDEmu->chs.cyllow = 79;
            CDEmu->chs.heads = 1;
            break;
        case 0x02:  /* 1.44M floppy */
            CDEmu->chs.sptcyl = 18;
            CDEmu->chs.cyllow = 79;
            CDEmu->chs.heads = 1;
            break;
        case 0x03:  /* 2.88M floppy */
            CDEmu->chs.sptcyl = 36;
            CDEmu->chs.cyllow = 79;
            CDEmu->chs.heads = 1;
            break;
        }
    } else {
        /* Harddrive emulation */
        CDEmu->emulated_drive = 0x80;
        
        /* Peak at partition table to get chs */
        struct mbr_s {
            u8int code[446];
            struct {
                u8int status;
                u8int first[3];
                u8int type;
                u8int last[3];
                u32int lba;
                u32int sectors;
            } partitions[4];
            u16int signature;
        } *mbr = *bootimg;
        
        /* Copy CHS from first partition */
        CDEmu->chs.heads = mbr->partitions[0].last[0];
        CDEmu->chs.sptcyl = mbr->partitions[0].last[1] & 0x3f;
        CDEmu->chs.cyllow = mbr->partitions[0].last[2] | 
                           ((mbr->partitions[0].last[1] & 0xc0) << 2);
    }
    
    print("SeaBIOS: cdemu media=%d, drive=%02x, lba=%ud, sectors=%d\n", 
          media, CDEmu->emulated_drive, CDEmu->ilba, CDEmu->sector_count);
    
    close(dop.fd);
    return 0;
}

/*
 * Check if ISO is bootable - from SeaBIOS cdrom_media_info()
 */
int
seabios_is_bootable(char *isofile)
{
    struct disk_op_s dop;
    u8int buffer[CDROM_SECTOR_SIZE];
    int ret;
    
    dop.fd = open(isofile, OREAD);
    if(dop.fd < 0)
        return 0;
    
    /* Read the Boot Record Volume Descriptor */
    dop.lba = 0x11;
    dop.count = 1;
    dop.buf = buffer;
    ret = process_op(&dop);
    if(ret){
        close(dop.fd);
        return 0;
    }
    
    /* Is it bootable? */
    if(buffer[0]){
        close(dop.fd);
        return 0;
    }
    if(strcmp((char*)&buffer[1], "CD001\001EL TORITO SPECIFICATION") != 0){
        close(dop.fd);
        return 0;
    }
    
    close(dop.fd);
    return 1;  /* Yes, it's bootable */
}