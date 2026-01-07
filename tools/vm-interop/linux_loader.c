/*
 * Linux kernel loader wrapper for VMX
 * Creates a minimal Plan 9 executable header that jumps to Linux
 */

#include <u.h>
#include <libc.h>

/* Plan 9 a.out header */
struct Exec {
    long magic;     /* magic number */
    long text;      /* size of text segment */
    long data;      /* size of initialized data */
    long bss;       /* size of uninitialized data */
    long syms;      /* size of symbol table */
    long entry;     /* entry point */
    long spsz;      /* size of pc/sp offset table */
    long pcsz;      /* size of pc/line number table */
};

#define I_MAGIC     0x00008000    /* i386 executable */

/*
 * Create a Plan 9 executable that loads Linux kernel
 * Returns a Plan 9 executable that VMX can load
 */
int
create_linux_loader(char *outfile, void *kernel32, long kernel32_size, uintptr entry)
{
    struct Exec hdr;
    int fd;

    /* Create Plan 9 header */
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = I_MAGIC;
    hdr.text = kernel32_size;
    hdr.data = 0;
    hdr.bss = 0;
    hdr.syms = 0;
    hdr.entry = entry;  /* Linux entry point at 0x100000 */
    hdr.spsz = 0;
    hdr.pcsz = 0;

    fd = create(outfile, OWRITE, 0755);
    if(fd < 0)
        return -1;

    /* Write Plan 9 header */
    if(write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)){
        close(fd);
        return -1;
    }

    /* Write Linux kernel as text segment */
    if(write(fd, kernel32, kernel32_size) != kernel32_size){
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}