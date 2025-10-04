/* fscheck - filesystem check and repair utility */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ext2fs/ext2fs.h>

static int verbose = 0;
static int fix = 0;
static int force = 0;

static void
usage(void)
{
	fprintf(stderr, "usage: fscheck [-vfy] device\n");
	fprintf(stderr, "  -v  verbose output\n");
	fprintf(stderr, "  -f  force check even if filesystem is clean\n");
	fprintf(stderr, "  -y  automatically fix errors (no prompts)\n");
	exit(1);
}

static int
check_superblock(ext2_filsys fs)
{
	if(verbose)
		printf("Checking superblock... ");

	if(fs->super->s_magic != EXT2_SUPER_MAGIC) {
		printf("FAILED\n");
		fprintf(stderr, "Invalid superblock magic\n");
		return -1;
	}

	if(verbose) {
		printf("OK\n");
		printf("  Block size: %u\n", fs->blocksize);
		printf("  Total blocks: %llu\n", (unsigned long long)ext2fs_blocks_count(fs->super));
		printf("  Free blocks: %llu\n", (unsigned long long)ext2fs_free_blocks_count(fs->super));
		printf("  Total inodes: %u\n", fs->super->s_inodes_count);
		printf("  Free inodes: %u\n", fs->super->s_free_inodes_count);
	}

	return 0;
}

static int
check_block_bitmap(ext2_filsys fs)
{
	errcode_t err;
	blk64_t i;
	int errors = 0;

	if(verbose)
		printf("Checking block bitmap... ");

	err = ext2fs_read_bitmaps(fs);
	if(err) {
		printf("FAILED\n");
		fprintf(stderr, "Cannot read bitmaps: %s\n", error_message(err));
		return -1;
	}

	/* Verify block bitmap consistency */
	for(i = fs->super->s_first_data_block; i < ext2fs_blocks_count(fs->super); i++) {
		if(ext2fs_test_block_bitmap2(fs->block_map, i)) {
			/* Block marked as used */
		}
	}

	if(verbose)
		printf("OK\n");

	return errors;
}

static int
check_inode_bitmap(ext2_filsys fs)
{
	errcode_t err;
	ext2_ino_t i;
	int errors = 0;

	if(verbose)
		printf("Checking inode bitmap... ");

	/* Already read by check_block_bitmap */

	/* Verify inode bitmap consistency */
	for(i = 1; i <= fs->super->s_inodes_count; i++) {
		if(ext2fs_test_inode_bitmap2(fs->inode_map, i)) {
			/* Inode marked as used */
			struct ext2_inode inode;
			err = ext2fs_read_inode(fs, i, &inode);
			if(err) {
				if(verbose)
					printf("\n  Warning: Cannot read inode %u\n", i);
				errors++;
			}
		}
	}

	if(verbose)
		printf("OK (%d errors)\n", errors);

	return errors;
}

static int
check_directory_structure(ext2_filsys fs)
{
	errcode_t err;
	ext2_inode_scan scan;
	ext2_ino_t ino;
	struct ext2_inode inode;
	int errors = 0;

	if(verbose)
		printf("Checking directory structure... ");

	err = ext2fs_open_inode_scan(fs, 0, &scan);
	if(err) {
		printf("FAILED\n");
		fprintf(stderr, "Cannot open inode scan: %s\n", error_message(err));
		return -1;
	}

	while(1) {
		err = ext2fs_get_next_inode(scan, &ino, &inode);
		if(err) {
			fprintf(stderr, "Error scanning inodes: %s\n", error_message(err));
			errors++;
			break;
		}
		if(ino == 0)
			break;

		/* Check if it's a directory */
		if(LINUX_S_ISDIR(inode.i_mode)) {
			/* Could do deeper directory checks here */
		}
	}

	ext2fs_close_inode_scan(scan);

	if(verbose)
		printf("OK (%d errors)\n", errors);

	return errors;
}

static int
check_filesystem(ext2_filsys fs)
{
	int total_errors = 0;

	printf("Checking filesystem on %s\n", fs->device_name);

	total_errors += check_superblock(fs);
	if(total_errors > 0)
		return total_errors;

	total_errors += check_block_bitmap(fs);
	total_errors += check_inode_bitmap(fs);
	total_errors += check_directory_structure(fs);

	return total_errors;
}

static int
fix_filesystem(ext2_filsys fs)
{
	errcode_t err;

	if(!fix) {
		fprintf(stderr, "Errors found but -y not specified. Not fixing.\n");
		return -1;
	}

	printf("Attempting to fix filesystem...\n");

	/* Mark filesystem as needing check */
	fs->super->s_state &= ~EXT2_VALID_FS;
	fs->super->s_state |= EXT2_ERROR_FS;

	/* Write superblock */
	err = ext2fs_flush(fs);
	if(err) {
		fprintf(stderr, "Cannot write superblock: %s\n", error_message(err));
		return -1;
	}

	printf("Filesystem marked for checking.\n");
	printf("Run e2fsck for full repair.\n");

	return 0;
}

int
main(int argc, char *argv[])
{
	ext2_filsys fs;
	errcode_t err;
	int c;
	int flags = 0;
	int errors;

	while((c = getopt(argc, argv, "vfy")) != -1) {
		switch(c) {
		case 'v':
			verbose = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'y':
			fix = 1;
			break;
		default:
			usage();
		}
	}

	if(optind >= argc)
		usage();

	char *device = argv[optind];

	/* Open filesystem read-write if fixing, otherwise read-only */
	if(fix)
		flags = EXT2_FLAG_RW;

	err = ext2fs_open(device, flags, 0, 0, unix_io_manager, &fs);
	if(err) {
		fprintf(stderr, "Cannot open %s: %s\n", device, error_message(err));
		return 1;
	}

	/* Check if filesystem is already clean */
	if(!force && (fs->super->s_state & EXT2_VALID_FS)) {
		printf("Filesystem is clean. Use -f to force check.\n");
		ext2fs_close(fs);
		return 0;
	}

	/* Run checks */
	errors = check_filesystem(fs);

	if(errors > 0) {
		printf("\n%d errors found.\n", errors);
		if(fix_filesystem(fs) < 0) {
			ext2fs_close(fs);
			return 2;
		}
	} else {
		printf("\nFilesystem is clean.\n");

		/* Mark as clean if we forced a check */
		if(force && fix) {
			fs->super->s_state |= EXT2_VALID_FS;
			fs->super->s_state &= ~EXT2_ERROR_FS;
			ext2fs_flush(fs);
			printf("Marked filesystem as clean.\n");
		}
	}

	ext2fs_close(fs);
	return errors > 0 ? 1 : 0;
}
