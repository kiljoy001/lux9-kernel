#include <u.h>
#include <libc.h>
#include <fcall.h>

#define PAGE_SIZE 4096

enum {
	PTEVALID = 1,
	PTEWRITE = 2,
	PTEUSER = 4,
};

char src_page[PAGE_SIZE];
char dst_page[PAGE_SIZE];

void
fill_page(char *page, uchar pattern)
{
	int i;
	for(i = 0; i < PAGE_SIZE; i++)
		page[i] = pattern;
}

int
check_pages_equal(char *a, char *b)
{
	int i;
	for(i = 0; i < PAGE_SIZE; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

void
main(void)
{
	int fd;
	char handle_str[64];
	unsigned long long handle;
	long n;
	char cmd[256];
	char resp[512];

	fd = open("#X/exchange", ORDWR);
	if(fd < 0) {
		fprint(2, "Failed to open #X/exchange\n");
		exits("open");
	}

	fill_page(src_page, 0xAB);
	fill_page(dst_page, 0x00);

	// Prepare
	snprint(cmd, sizeof(cmd), "prepare %p\n", src_page);
	n = write(fd, cmd, strlen(cmd));
	if(n != strlen(cmd)) {
		fprint(2, "Failed to write prepare command\n");
		close(fd);
		exits("write");
	}

	// Read handle response
	n = read(fd, resp, sizeof(resp)-1);
	if(n <= 0) {
		fprint(2, "Failed to read handle\n");
		close(fd);
		exits("read");
	}
	resp[n] = '\0';

	// Parse handle
	if(tokenize(resp, &handle_str, 1) < 1) {
		fprint(2, "Failed to parse handle from response\n");
		close(fd);
		exits("parse");
	}
	handle = strtoull(handle_str, nil, 10);

	// Accept
	snprint(cmd, sizeof(cmd), "accept %llu %p %d\n", handle, dst_page, PTEVALID|PTEWRITE|PTEUSER);
	n = write(fd, cmd, strlen(cmd));
	if(n != strlen(cmd)) {
		fprint(2, "Failed to write accept command\n");
		close(fd);
		exits("write");
	}

	// Verify
	if(check_pages_equal(src_page, dst_page)) {
		print("exchange test PASSED\n");
	} else {
		print("exchange test FAILED\n");
	}

	close(fd);
	exits(nil);
}