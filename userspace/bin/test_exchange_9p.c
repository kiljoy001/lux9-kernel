/* Test program for exchange device using 9P library */
#include "libc.h"
#include "fcall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* 9P message size */
#define MSGSIZE 8192

/* Forward declarations for 9P functions */
extern uint convM2S(uchar*, uint, Fcall*);
extern uint convS2M(Fcall*, uchar*, uint);
extern uint sizeS2M(Fcall*);
extern uint convM2D(uchar*, uint, Dir*, char*);
extern uint convD2M(Dir*, uchar*, uint);

/* Simple 9P client functions */
static int exchange_fd = -1;
static ushort tag = 1;

static int
send_9p_message(Fcall *tx, Fcall *rx)
{
	uchar buf[MSGSIZE];
	uint n;
	
	n = convS2M(tx, buf, MSGSIZE);
	if(n == 0) {
		printf("Failed to convert message to 9P format\n");
		return -1;
	}
	
	if(write(exchange_fd, buf, n) != n) {
		printf("Failed to send 9P message\n");
		return -1;
	}
	
	/* Read response */
	if(read(exchange_fd, buf, MSGSIZE) <= 0) {
		printf("Failed to read 9P response\n");
		return -1;
	}
	
	if(convM2S(buf, MSGSIZE, rx) == 0) {
		printf("Failed to parse 9P response\n");
		return -1;
	}
	
	return 0;
}

static int
open_exchange_device(void)
{
	exchange_fd = open("#X/exchange", O_RDWR);
	if(exchange_fd < 0) {
		printf("Failed to open exchange device\n");
		return -1;
	}
	
	printf("Opened exchange device successfully\n");
	return 0;
}

static int
read_exchange_status(void)
{
	char buf[1024];
	int n;
	
	/* Simple read test */
	n = read(exchange_fd, buf, sizeof(buf)-1);
	if(n > 0) {
		buf[n] = 0;
		printf("Exchange status:\n%s\n", buf);
		return 0;
	}
	
	printf("Failed to read exchange status\n");
	return -1;
}

static int
test_exchange_prepare(uintptr_t vaddr)
{
	char cmd[256];
	int n;
	
	/* Send prepare command */
	snprintf(cmd, sizeof(cmd), "prepare 0x%lx\n", vaddr);
	n = write(exchange_fd, cmd, strlen(cmd));
	if(n != strlen(cmd)) {
		printf("Failed to send prepare command\n");
		return -1;
	}
	
	printf("Sent prepare command for vaddr 0x%lx\n", vaddr);
	return 0;
}

int
main(void)
{
	printf("Exchange device test program\n");
	printf("============================\n");
	
	/* Open the exchange device */
	if(open_exchange_device() < 0) {
		return 1;
	}
	
	/* Read current status */
	if(read_exchange_status() < 0) {
		close(exchange_fd);
		return 1;
	}
	
	/* Test prepare command with a dummy address */
	/* In a real test, we would allocate a page and prepare it */
	if(test_exchange_prepare(0x10000000) < 0) {
		printf("Prepare test failed\n");
	} else {
		printf("Prepare test completed\n");
	}
	
	/* Read status again to see changes */
	read_exchange_status();
	
	close(exchange_fd);
	printf("Test completed\n");
	return 0;
}