/* initrd.c - Initial ramdisk support */
#include "u.h"
#include <lib.h>
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "initrd.h"

struct initrd_file *initrd_root = nil;
void *initrd_base = nil;
usize initrd_size = 0;
uintptr initrd_physaddr = 0;

/* Parse octal number from TAR header */
static usize
parse_octal(const char *str, int len)
{
	usize val = 0;
	int i;

	for(i = 0; i < len && str[i]; i++) {
		if(str[i] >= '0' && str[i] <= '7')
			val = (val << 3) | (str[i] - '0');
	}

	return val;
}

/* Check if TAR header is valid */
static int
is_valid_tar(struct tar_header *hdr)
{
	/* Check magic */
	if(memcmp(hdr->magic, "ustar", 5) != 0) {
		print("initrd: invalid magic\n");
		return 0;
	}
	
	/* Check version */
	if(hdr->version[0] != '0' && hdr->version[0] != ' ') {
		print("initrd: invalid version\n");
		return 0;
	}

	/* Check if name is not empty */
	if(hdr->name[0] == '\0') {
		print("initrd: empty name\n");
		return 0;
	}
	
	/* Check if size field looks reasonable */
	/* Basic check: should be all octal digits or spaces */
	int i;
	for(i = 0; i < 12 && hdr->size[i] != '\0'; i++) {
		if(hdr->size[i] != ' ' && (hdr->size[i] < '0' || hdr->size[i] > '7')) {
			print("initrd: invalid size field\n");
			return 0;
		}
	}

	return 1;
}

/* Initialize initrd from memory */
extern void uartputs(char*, int);

static void
printhex(char *label, uvlong value)
{
    static char hex[] = "0123456789abcdef";
    char buf[2 + sizeof(uvlong) * 2 + 2];
    char *p = buf;

    *p++ = '0';
    *p++ = 'x';
    for(int i = (int)(sizeof(uvlong) * 2 - 1); i >= 0; i--)
        *p++ = hex[(value >> (i * 4)) & 0xF];
    *p++ = '\n';
    *p = 0;

    if(label != nil)
        uartputs(label, strlen(label));
    uartputs(buf, p - buf);
}

void
initrd_init(void *addr, usize len)
{
	struct tar_header *hdr;
	struct initrd_file *file, *last = nil;
	usize offset = 0;
	usize size;

	initrd_base = addr;
	initrd_size = len;
	printhex("initrd addr ", (uvlong)(uintptr)addr);
	printhex("initrd size ", (uvlong)len);

	print("initrd: loading entries\n");
	printhex("initrd start offset ", 0);
	printhex("initrd end offset ", (uvlong)len);

	/* Parse TAR archive */
	int entry_count = 0;
	const int MAX_ENTRIES = 1000;
	
	while(offset < len && entry_count < MAX_ENTRIES) {
		hdr = (struct tar_header*)((u8int*)addr + offset);
		printhex("header @ ", (uvlong)(uintptr)hdr);
		printhex("offset ", (uvlong)offset);
		printhex("entry_count ", (uvlong)entry_count);
		
		/* Basic safety check */
		if((u8int*)hdr < (u8int*)addr || (u8int*)hdr >= (u8int*)addr + len) {
			print("initrd: invalid header pointer\n");
			break;
		}
		
		uvlong word0;
		memmove(&word0, hdr, sizeof word0);
		printhex("hdr word0 ", word0);

		/* Check for end of archive (all zeros) */
		if(hdr->name[0] == '\0')
			break;

		/* Validate header */
		if(!is_valid_tar(hdr)) {
			print("initrd: invalid TAR header\n");
			break;
		}

		/* Get file size */
		size = parse_octal(hdr->size, sizeof(hdr->size));
		printhex("size parsed ", (uvlong)size);
		printhex("malloc request ", (uvlong)sizeof(struct initrd_file));

		/* Only handle regular files */
		if(hdr->typeflag == '0' || hdr->typeflag == '\0') {
			/* Allocate file entry */
			file = malloc(sizeof(struct initrd_file));
			printhex("malloc returned ", (uvlong)(uintptr)file);
			if(file == nil) {
				print("initrd: out of memory, continuing anyway\n");
				/* Continue to next entry rather than stopping */
				goto skip_entry;
			}

			/* Copy name */
			memmove(file->name, hdr->name, sizeof(hdr->name));
			file->name[sizeof(hdr->name)] = '\0';

			/* Strip leading ./ if present */
			char *name = file->name;
			if(name[0] == '.' && name[1] == '/')
				name += 2;
			memmove(file->name, name, strlen(name) + 1);

			/* Set data pointer and size */
			file->data = (u8int*)addr + offset + 512;
			file->size = size;
			file->next = nil;

			/* Add to list */
			if(initrd_root == nil)
				initrd_root = file;
			else
				last->next = file;
			last = file;

			print("initrd: found file\n");
		}

	skip_entry:
		/* Move to next file (512-byte aligned) */
		offset += 512;  /* Header */
		if(size > 0) {
			usize aligned_size = (size + 511) & ~511;  /* Data (rounded up to 512) */
			if(offset + aligned_size < offset) {  /* Check for overflow */
				print("initrd: size overflow detected\n");
				break;
			}
			offset += aligned_size;
		}
		entry_count++;
		print("initrd: entry processed\n");
	}
	
	if(entry_count >= MAX_ENTRIES) {
		print("initrd: maximum entry count reached\n");
	}

	print("initrd: loaded successfully\n");
}

/* Register initrd files with devroot - call AFTER chandevreset() */
void
initrd_register(void)
{
	struct initrd_file *f;
	char *name;

	for(f = initrd_root; f != nil; f = f->next) {
		name = f->name;
		/* Strip "bin/" prefix if present - files go into /boot/ directly */
		if(strncmp(name, "bin/", 4) == 0)
			name += 4;
		print("initrd: registering '%s' as '/boot/%s'\n", f->name, name);
		addbootfile(name, f->data, f->size);
	}
}

/* Find file in initrd */
void*
initrd_find(const char *path)
{
	struct initrd_file *f;

	/* Strip leading / if present */
	if(path[0] == '/')
		path++;

	for(f = initrd_root; f != nil; f = f->next) {
		if(strcmp(f->name, path) == 0)
			return f->data;
	}

	return nil;
}

/* Get file size */
usize
initrd_filesize(const char *path)
{
	struct initrd_file *f;

	/* Strip leading / if present */
	if(path[0] == '/')
		path++;

	for(f = initrd_root; f != nil; f = f->next) {
		if(strcmp(f->name, path) == 0)
			return f->size;
	}

	return 0;
}

/* Read from file */
int
initrd_read(const char *path, void *buf, usize offset, usize len)
{
	struct initrd_file *f;

	/* Strip leading / if present */
	if(path[0] == '/')
		path++;

	for(f = initrd_root; f != nil; f = f->next) {
		if(strcmp(f->name, path) == 0) {
			if(offset >= f->size)
				return 0;

			if(offset + len > f->size)
				len = f->size - offset;

			memmove(buf, (u8int*)f->data + offset, len);
			return len;
		}
	}

	return -1;
}

/* List all files in initrd */
void
initrd_list(void)
{
	struct initrd_file *f;

	print("initrd: file list:\n");
	for(f = initrd_root; f != nil; f = f->next) {
		print("  file entry\n");
	}
}
