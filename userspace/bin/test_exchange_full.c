/* Comprehensive test program for exchange device */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

int
main(void)
{
	int fd, fd2;
	char *page1, *page2;
	char cmd[256];
	int n;
	char buf[1024];
	
	printf("=== Exchange Device Comprehensive Test ===\n");
	
	/* Open the exchange device */
	fd = open("#X/exchange", O_RDWR);
	if(fd < 0) {
		printf("Failed to open exchange device #X/exchange\n");
		return 1;
	}
	
	printf("✓ Opened exchange device\n");
	
	/* Allocate two pages of memory */
	page1 = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	page2 = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if(page1 == MAP_FAILED || page2 == MAP_FAILED) {
		printf("Failed to allocate pages\n");
		close(fd);
		return 1;
	}
	
	printf("✓ Allocated two pages at %p and %p\n", page1, page2);
	
	/* Initialize first page with test data */
	strcpy(page1, "Hello, Exchange System!");
	strcpy(page1 + PAGE_SIZE - 20, "End of page data");
	
	printf("✓ Initialized page1 with test data\n");
	printf("  Page1 start: %.30s\n", page1);
	printf("  Page1 end: %.20s\n", page1 + PAGE_SIZE - 20);
	
	/* Test 1: Read exchange status */
	n = read(fd, buf, sizeof(buf)-1);
	if(n > 0) {
		buf[n] = 0;
		printf("✓ Exchange status read (%d bytes):\n%s\n", n, buf);
	}
	
	/* Test 2: Prepare a page for exchange */
	snprintf(cmd, sizeof(cmd), "prepare %p", page1);
	n = write(fd, cmd, strlen(cmd));
	if(n < 0) {
		printf("✗ Failed to prepare page: %s\n", strerror(errno));
		munmap(page1, PAGE_SIZE);
		munmap(page2, PAGE_SIZE);
		close(fd);
		return 1;
	}
	
	printf("✓ Prepared page1 for exchange\n");
	
	/* Test 3: Read exchange status after prepare */
	lseek(fd, 0, SEEK_SET);  /* Reset file position */
	n = read(fd, buf, sizeof(buf)-1);
	if(n > 0) {
		buf[n] = 0;
		printf("✓ Exchange status after prepare:\n%s\n", buf);
	}
	
	/* Test 4: Accept the page at new location */
	snprintf(cmd, sizeof(cmd), "accept %p %p 7", page1, page2);  /* 7 = PTEVALID|PTEUSER|PTEWRITE */
	n = write(fd, cmd, strlen(cmd));
	if(n < 0) {
		printf("✗ Failed to accept page: %s\n", strerror(errno));
		/* Try to cancel the prepare */
		snprintf(cmd, sizeof(cmd), "cancel %p", page1);
		write(fd, cmd, strlen(cmd));
		munmap(page1, PAGE_SIZE);
		munmap(page2, PAGE_SIZE);
		close(fd);
		return 1;
	}
	
	printf("✓ Accepted page at new location\n");
	
	/* Test 5: Verify data was transferred */
	printf("Verifying data transfer...\n");
	printf("  Page2 start: %.30s\n", page2);
	printf("  Page2 end: %.20s\n", page2 + PAGE_SIZE - 20);
	
	if(strcmp(page2, "Hello, Exchange System!") == 0 &&
	   strcmp(page2 + PAGE_SIZE - 20, "End of page data") == 0) {
		printf("✓ Data transfer verified successfully\n");
	} else {
		printf("✗ Data transfer verification failed\n");
	}
	
	/* Test 6: Read exchange status after accept */
	lseek(fd, 0, SEEK_SET);  /* Reset file position */
	n = read(fd, buf, sizeof(buf)-1);
	if(n > 0) {
		buf[n] = 0;
		printf("✓ Exchange status after accept:\n%s\n", buf);
	}
	
	/* Cleanup */
	munmap(page1, PAGE_SIZE);
	munmap(page2, PAGE_SIZE);
	close(fd);
	
	printf("=== Test completed ===\n");
	return 0;
}