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
	if(memcmp(hdr->magic, "ustar", 5) != 0)
		return 0;

	/* Check if name is not empty */
	if(hdr->name[0] == '\0')
		return 0;

	return 1;
}

/* Initialize initrd from memory */
void
initrd_init(void *addr, usize len)
{
	struct tar_header *hdr;
	struct initrd_file *file, *last = nil;
	usize offset = 0;
	usize size;

	initrd_base = addr;
	initrd_size = len;

	print("initrd: loading from %p, size %lld bytes\n", addr, len);

	/* Parse TAR archive */
	while(offset < len) {
		hdr = (struct tar_header*)((u8int*)addr + offset);

		/* Check for end of archive (all zeros) */
		if(hdr->name[0] == '\0')
			break;

		/* Validate header */
		if(!is_valid_tar(hdr)) {
			print("initrd: invalid TAR header at offset %lld\n", offset);
			break;
		}

		/* Get file size */
		size = parse_octal(hdr->size, sizeof(hdr->size));

		/* Only handle regular files */
		if(hdr->typeflag == '0' || hdr->typeflag == '\0') {
			/* Allocate file entry */
			file = malloc(sizeof(struct initrd_file));
			if(file == nil) {
				print("initrd: out of memory\n");
				return;
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

			print("initrd: found %s (%lld bytes)\n", file->name, size);
		}

		/* Move to next file (512-byte aligned) */
		offset += 512;  /* Header */
		offset += (size + 511) & ~511;  /* Data (rounded up to 512) */
	}

	print("initrd: loaded successfully\n");
}

/* Register initrd files with devroot - call AFTER chandevreset() */
void
initrd_register(void)
{
	struct initrd_file *f;

	for(f = initrd_root; f != nil; f = f->next) {
		print("initrd: registering %s with devroot\n", f->name);
		addbootfile(f->name, f->data, f->size);
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
		print("  %s (%lld bytes)\n", f->name, f->size);
	}
}
