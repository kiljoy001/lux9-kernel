#include <lux.h>

void
main(int argc, char *argv[])
{
	int fd;

	/* First, we need to open console for stdin/stdout/stderr */
	/* Bind #c (console device) to /dev */
	if(bind("#c", "/dev", MREPL) < 0) {
		/* Can't print error - no console yet! Just loop */
		for(;;);
	}

	/* Open /dev/cons for stdin (fd 0) */
	fd = open("/dev/cons", OREAD);
	if(fd != 0) {
		/* Wrong fd, close and try again or just fail */
		for(;;);
	}

	/* Open /dev/cons for stdout (fd 1) */
	fd = open("/dev/cons", OWRITE);
	if(fd != 1) {
		for(;;);
	}

	/* Open /dev/cons for stderr (fd 2) */
	fd = open("/dev/cons", OWRITE);
	if(fd != 2) {
		for(;;);
	}

	/* Now we can print! */
	print("init: Console opened successfully!\n");
	print("init: Starting CLR bootstrap...\n");

	/* Loop forever for now */
	for(;;)
		;
}

