/*
 * Test program for exchange device
 * Uses 9P interface to test page exchange operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Simple 9P client for testing exchange operations */

static int exchange_fd = -1;

static int
exchange_open(void)
{
	exchange_fd = open("/dev/exchange", O_RDWR);
	if(exchange_fd < 0) {
		printf("Failed to open exchange device: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

static void
exchange_close(void)
{
	if(exchange_fd >= 0) {
		close(exchange_fd);
		exchange_fd = -1;
	}
}

static int
exchange_prepare(uintptr_t vaddr)
{
	char cmd[64];
	int ret;
	
	snprintf(cmd, sizeof(cmd), "prepare 0x%lx\n", vaddr);
	ret = write(exchange_fd, cmd, strlen(cmd));
	if(ret < 0) {
		printf("exchange_prepare failed: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

static int
exchange_accept(uintptr_t handle, uintptr_t dest_vaddr, int prot)
{
	char cmd[128];
	int ret;
	
	snprintf(cmd, sizeof(cmd), "accept 0x%lx 0x%lx %d\n", handle, dest_vaddr, prot);
	ret = write(exchange_fd, cmd, strlen(cmd));
	if(ret < 0) {
		printf("exchange_accept failed: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

static int
exchange_cancel(uintptr_t handle)
{
	char cmd[64];
	int ret;
	
	snprintf(cmd, sizeof(cmd), "cancel 0x%lx\n", handle);
	ret = write(exchange_fd, cmd, strlen(cmd));
	if(ret < 0) {
		printf("exchange_cancel failed: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

static int
exchange_stat(void)
{
	char buf[1024];
	int ret;
	
	/* Read exchange status */
	lseek(exchange_fd, 0, SEEK_SET);
	ret = read(exchange_fd, buf, sizeof(buf)-1);
	if(ret < 0) {
		printf("exchange_stat read failed: %s\n", strerror(errno));
		return -1;
	}
	
	buf[ret] = 0;
	printf("Exchange status:\n%s\n", buf);
	return 0;
}

int
main(void)
{
	uintptr_t *page1, *page2;
	
	printf("Exchange Device Test Program\n");
	printf("============================\n");
	
	/* Open exchange device */
	if(exchange_open() < 0) {
		return 1;
	}
	
	/* Show initial status */
	printf("\nInitial exchange status:\n");
	exchange_stat();
	
	/* Allocate two pages */
	page1 = (uintptr_t*)sbrk(4096);
	if(page1 == (void*)-1) {
		printf("Failed to allocate first page\n");
		exchange_close();
		return 1;
	}
	
	page2 = (uintptr_t*)sbrk(4096);
	if(page2 == (void*)-1) {
		printf("Failed to allocate second page\n");
		exchange_close();
		return 1;
	}
	
	/* Initialize first page with test data */
	page1[0] = 0xDEADBEEF;
	page1[511] = 0xCAFEBABE;
	
	printf("\nPage 1 data: 0x%lx at end: 0x%lx\n", page1[0], page1[511]);
	
	/* Try to prepare a page for exchange */
	printf("\nPreparing page at %p for exchange...\n", page1);
	if(exchange_prepare((uintptr_t)page1) < 0) {
		printf("exchange_prepare failed\n");
		exchange_close();
		return 1;
	}
	
	printf("Page prepared successfully\n");
	
	/* Show status after prepare */
	printf("\nExchange status after prepare:\n");
	exchange_stat();
	
	/* Cancel the exchange */
	printf("\nCancelling exchange...\n");
	if(exchange_cancel((uintptr_t)page1) < 0) {
		printf("exchange_cancel failed\n");
		exchange_close();
		return 1;
	}
	
	printf("Exchange cancelled successfully\n");
	
	/* Show final status */
	printf("\nFinal exchange status:\n");
	exchange_stat();
	
	/* Close exchange device */
	exchange_close();
	
	printf("\nAll tests completed successfully!\n");
	return 0;
}