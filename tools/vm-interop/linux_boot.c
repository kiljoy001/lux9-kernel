/*
 * Linux boot protocol implementation for VMX
 * Based on Linux Documentation/x86/boot.txt
 */

#include <u.h>
#include <libc.h>

/* Linux boot_params structure offsets */
#define BP_SETUP_SECTS    0x1F1
#define BP_ROOT_FLAGS     0x1F2
#define BP_SYSSIZE        0x1F4
#define BP_RAM_SIZE       0x1F8
#define BP_VID_MODE       0x1FA
#define BP_ROOT_DEV       0x1FC
#define BP_BOOT_FLAG      0x1FE
#define BP_JUMP           0x200
#define BP_HEADER         0x202
#define BP_VERSION        0x206
#define BP_REALMODE_SWTCH 0x208
#define BP_TYPE_OF_LOADER 0x210
#define BP_LOADFLAGS      0x211
#define BP_SETUP_MOVE_SIZE 0x212
#define BP_CODE32_START   0x214
#define BP_RAMDISK_IMAGE  0x218
#define BP_RAMDISK_SIZE   0x21C
#define BP_HEAP_END_PTR   0x224
#define BP_CMD_LINE_PTR   0x228

/* Boot signature */
#define BOOT_SIGNATURE    0xAA55

/* Loadflags bits */
#define LOADED_HIGH       0x01
#define KEEP_SEGMENTS     0x40
#define CAN_USE_HEAP      0x80

/*
 * Check if this is a Linux kernel
 * Returns boot protocol version or 0 if not Linux
 */
int
is_linux_kernel(void *kernel, long size)
{
    u8int *k = kernel;
    u16int sig;

    if(size < 0x202 + 4)
        return 0;

    /* Check for boot signature at 0x1FE */
    sig = k[BP_BOOT_FLAG] | (k[BP_BOOT_FLAG+1] << 8);
    if(sig != BOOT_SIGNATURE)
        return 0;

    /* Check for "HdrS" magic at 0x202 */
    if(memcmp(&k[BP_HEADER], "HdrS", 4) != 0)
        return 0;

    /* Return boot protocol version */
    return k[BP_VERSION] | (k[BP_VERSION+1] << 8);
}

/*
 * Setup Linux boot parameters
 * kernel: pointer to loaded kernel image
 * size: size of kernel image
 * cmdline: kernel command line
 * initrd: pointer to initrd (can be nil)
 * initrd_size: size of initrd
 * Returns: entry point address
 */
uintptr
setup_linux_boot(void *kernel, long size, char *cmdline, void *initrd, long initrd_size)
{
    u8int *k = kernel;
    u8int setup_sects;
    u32int code32_start;
    u16int version;

    version = is_linux_kernel(kernel, size);
    if(version == 0){
        werrstr("not a Linux kernel");
        return 0;
    }

    print("Linux: boot protocol version %x.%02x\n", version>>8, version&0xFF);

    /* Get setup sectors count */
    setup_sects = k[BP_SETUP_SECTS];
    if(setup_sects == 0)
        setup_sects = 4;  /* Ancient kernels */

    /* Get 32-bit entry point */
    code32_start = *(u32int*)&k[BP_CODE32_START];
    if(code32_start == 0)
        code32_start = 0x100000;  /* Default protected mode load address */

    print("Linux: setup_sects=%d, code32_start=0x%ux\n", setup_sects, code32_start);

    /* Set boot parameters */
    k[BP_TYPE_OF_LOADER] = 0xFF;  /* Unknown loader */

    /* Set heap end pointer for setup code */
    if(version >= 0x0201){
        *(u16int*)&k[BP_HEAP_END_PTR] = 0xFE00;
        k[BP_LOADFLAGS] |= CAN_USE_HEAP;
    }

    /* Set video mode (normal 80x25) */
    *(u16int*)&k[BP_VID_MODE] = 0xFFFF;

    /* Set command line if provided */
    if(cmdline != nil && version >= 0x0202){
        /* Command line goes at 0x10000 */
        u32int cmdline_addr = 0x10000;
        *(u32int*)&k[BP_CMD_LINE_PTR] = cmdline_addr;
        /* VMX will need to copy cmdline to guest memory at cmdline_addr */
    }

    /* Set initrd if provided */
    if(initrd != nil && initrd_size > 0 && version >= 0x0200){
        /* Load initrd high in memory (32MB) */
        u32int initrd_addr = 0x2000000;
        *(u32int*)&k[BP_RAMDISK_IMAGE] = initrd_addr;
        *(u32int*)&k[BP_RAMDISK_SIZE] = initrd_size;
        /* VMX will need to copy initrd to guest memory at initrd_addr */
    }

    /* Return the protected mode entry point */
    return code32_start;
}

/*
 * Get the size of Linux kernel setup code
 */
long
get_linux_setup_size(void *kernel)
{
    u8int *k = kernel;
    u8int setup_sects;

    setup_sects = k[BP_SETUP_SECTS];
    if(setup_sects == 0)
        setup_sects = 4;

    /* Setup size = (setup_sects + 1) * 512 */
    return (setup_sects + 1) * 512;
}

/*
 * Get pointer to Linux protected mode kernel
 * (the part that loads at 0x100000)
 */
void*
get_linux_kernel32(void *kernel)
{
    long setup_size = get_linux_setup_size(kernel);
    return (u8int*)kernel + setup_size;
}

/*
 * Get size of Linux protected mode kernel
 */
long
get_linux_kernel32_size(void *kernel, long total_size)
{
    long setup_size = get_linux_setup_size(kernel);
    return total_size - setup_size;
}