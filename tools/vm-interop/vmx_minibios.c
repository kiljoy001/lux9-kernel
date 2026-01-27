/*
 * Minimal BIOS for VMX to boot from ISO images
 * Stolen from QEMU/SeaBIOS (LGPL)
 * Implements just enough to boot El Torito CDs
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#define CDROM_SECTOR_SIZE 2048
#define DISK_SECTOR_SIZE 512
#define BOOT_SEGMENT 0x07C0

/* El Torito structures from SeaBIOS */
struct eltorito_validation {
    u8int  header_id;      // 01
    u8int  platform_id;    // 00 = x86
    u16int reserved;
    char   id_string[24];
    u16int checksum;
    u8int  key55;          // 0x55
    u8int  keyAA;          // 0xAA
};

struct eltorito_default_entry {
    u8int  bootable;       // 0x88 = bootable
    u8int  media_type;     // 0 = no emulation
    u16int load_segment;
    u8int  system_type;
    u8int  unused;
    u16int sector_count;
    u32int lba;
    u8int  reserved[20];
};

typedef struct ISOBoot {
    int fd;
    char *isofile;
    u32int boot_catalog_lba;
    u32int boot_image_lba;
    u16int boot_image_sectors;
    u16int load_segment;
    u8int  media_type;
} ISOBoot;

static int
read_cd_sector(int fd, u32int lba, void *buf)
{
    vlong off = (vlong)lba * CDROM_SECTOR_SIZE;
    
    if(seek(fd, off, 0) != off){
        werrstr("seek to LBA %ud failed", lba);
        return -1;
    }
    
    if(readn(fd, buf, CDROM_SECTOR_SIZE) != CDROM_SECTOR_SIZE){
        werrstr("read sector at LBA %ud failed", lba);
        return -1;
    }
    
    return 0;
}

/*
 * Find and parse El Torito boot information
 * Based on SeaBIOS cdrom_boot()
 */
static int
parse_eltorito(ISOBoot *iso)
{
    u8int buffer[CDROM_SECTOR_SIZE];
    struct eltorito_validation *valid;
    struct eltorito_default_entry *boot;
    
    /* Read Boot Record Volume Descriptor at LBA 0x11 */
    if(read_cd_sector(iso->fd, 0x11, buffer) < 0)
        return -1;
    
    /* Validate El Torito signature */
    if(buffer[0] != 0)
        return -1;
    if(memcmp(&buffer[1], "CD001\001EL TORITO SPECIFICATION", 32) != 0){
        werrstr("not an El Torito bootable CD");
        return -1;
    }
    
    /* Get boot catalog address */
    iso->boot_catalog_lba = *(u32int*)&buffer[0x47];
    
    /* Read boot catalog */
    if(read_cd_sector(iso->fd, iso->boot_catalog_lba, buffer) < 0)
        return -1;
    
    /* Validate boot catalog */
    valid = (struct eltorito_validation*)buffer;
    if(valid->header_id != 0x01){
        werrstr("invalid boot catalog header");
        return -1;
    }
    if(valid->platform_id != 0x00){
        werrstr("boot catalog not for x86 platform");  
        return -1;
    }
    if(valid->key55 != 0x55 || valid->keyAA != 0xAA){
        werrstr("invalid boot catalog signature");
        return -1;
    }
    
    /* Parse default boot entry */
    boot = (struct eltorito_default_entry*)&buffer[0x20];
    if(boot->bootable != 0x88){
        werrstr("CD is not bootable");
        return -1;
    }
    
    iso->media_type = boot->media_type;
    iso->load_segment = boot->load_segment;
    if(iso->load_segment == 0)
        iso->load_segment = BOOT_SEGMENT;
    
    iso->boot_image_sectors = boot->sector_count;
    iso->boot_image_lba = boot->lba;
    
    print("El Torito: boot image at LBA %ud, %d sectors, load at %04x:0000\n",
          iso->boot_image_lba, iso->boot_image_sectors, iso->load_segment);
    
    return 0;
}

/*
 * Load boot image into memory
 * This would be loaded at real mode segment:offset in real x86
 * For VMX, we need to extract and load as kernel
 */
static void*
load_boot_image(ISOBoot *iso, long *size)
{
    void *bootimg;
    int nsectors, nbytes;
    u32int lba;
    int i;
    
    /* Calculate size */
    nsectors = iso->boot_image_sectors;
    if(nsectors == 0)
        nsectors = 4;  /* Default for floppy emulation */
    
    nbytes = nsectors * DISK_SECTOR_SIZE;
    
    /* Allocate buffer */
    bootimg = malloc(nbytes);
    if(bootimg == nil){
        werrstr("cannot allocate %d bytes for boot image", nbytes);
        return nil;
    }
    
    /* Read boot image sectors */
    lba = iso->boot_image_lba;
    for(i = 0; i < (nsectors + 3) / 4; i++){
        u8int sector[CDROM_SECTOR_SIZE];
        int copy;
        
        if(read_cd_sector(iso->fd, lba + i, sector) < 0){
            free(bootimg);
            return nil;
        }
        
        /* Copy up to 4 disk sectors from this CD sector */
        copy = nsectors - i * 4;
        if(copy > 4)
            copy = 4;
        
        memmove((u8int*)bootimg + i * CDROM_SECTOR_SIZE, 
                sector, copy * DISK_SECTOR_SIZE);
    }
    
    *size = nbytes;
    return bootimg;
}

/*
 * Extract Linux kernel from boot image
 * Boot image might be:
 * - Raw boot sector (512 bytes)
 * - ISOLINUX/SYSLINUX boot loader
 * - GRUB boot loader
 * - Linux kernel directly
 */
static void*
extract_kernel_from_bootimg(void *bootimg, long bootsize, ISOBoot *iso, long *kernsize)
{
    u8int *img = bootimg;
    
    /* Check for Linux kernel signature */
    if(bootsize >= 0x202 && img[0x1FE] == 0x55 && img[0x1FF] == 0xAA){
        /* Looks like boot sector */
        if(memcmp(&img[0x202], "HdrS", 4) == 0){
            /* Linux kernel header found */
            print("Found Linux kernel in boot image\n");
            void *kernel = malloc(bootsize);
            memmove(kernel, bootimg, bootsize);
            *kernsize = bootsize;
            return kernel;
        }
    }
    
    /* Check for ISOLINUX */
    if(memmem(bootimg, bootsize, "ISOLINUX", 8) != nil){
        print("Found ISOLINUX boot loader\n");
        /* Would need to parse isolinux.cfg and load actual kernel */
        /* For now, try to find kernel on ISO... */
        return find_kernel_on_iso(iso, kernsize);
    }
    
    /* Check for GRUB */
    if(memmem(bootimg, bootsize, "GRUB", 4) != nil){
        print("Found GRUB boot loader\n");
        /* Would need to parse grub.cfg and load actual kernel */
        return find_kernel_on_iso(iso, kernsize);
    }
    
    /* Unknown format - return boot image as-is */
    print("Unknown boot image format, using as-is\n");
    void *kernel = malloc(bootsize);
    memmove(kernel, bootimg, bootsize);
    *kernsize = bootsize;
    return kernel;
}

/*
 * Search for Linux kernel on ISO filesystem
 * Common locations: /boot/vmlinuz, /isolinux/vmlinuz, /casper/vmlinuz
 */
static void*
find_kernel_on_iso(ISOBoot *iso, long *kernsize)
{
    /* This would require ISO9660 filesystem parsing */
    /* For now, return nil to indicate we need the user to extract */
    werrstr("automatic kernel extraction not yet implemented");
    return nil;
}

/*
 * Main entry point - boot from ISO file
 */
void*
vmx_boot_iso(char *isofile, long *kernsize)
{
    ISOBoot iso;
    void *bootimg, *kernel;
    long bootsize;
    
    memset(&iso, 0, sizeof(iso));
    iso.isofile = isofile;
    
    /* Open ISO */
    iso.fd = open(isofile, OREAD);
    if(iso.fd < 0){
        werrstr("cannot open ISO: %r");
        return nil;
    }
    
    /* Parse El Torito boot info */
    if(parse_eltorito(&iso) < 0){
        close(iso.fd);
        return nil;
    }
    
    /* Load boot image */
    bootimg = load_boot_image(&iso, &bootsize);
    if(bootimg == nil){
        close(iso.fd);
        return nil;
    }
    
    /* Extract kernel from boot image */
    kernel = extract_kernel_from_bootimg(bootimg, bootsize, &iso, kernsize);
    
    free(bootimg);
    close(iso.fd);
    
    return kernel;
}

/*
 * Helper function - search memory for pattern
 */
static void*
memmem(void *haystack, size_t hlen, char *needle, size_t nlen)
{
    u8int *h = haystack;
    size_t i;
    
    if(nlen > hlen)
        return nil;
    
    for(i = 0; i <= hlen - nlen; i++){
        if(memcmp(h + i, needle, nlen) == 0)
            return h + i;
    }
    
    return nil;
}