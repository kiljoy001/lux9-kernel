#include <lux.h>

void
main(int argc, char *argv[])
{
	int fd, n;
	char buf[32];
	
	print("init: starting...\n");

	/* Bind devices */
	if(bind("#c", "/dev", MREPL) < 0)
		print("init: bind #c /dev failed\n");
	if(bind("#t", "/dev", MAFTER) < 0)
		print("init: bind #t /dev failed\n");
	if(bind("#p", "/proc", MREPL) < 0)
		print("init: bind #p /proc failed\n");
	if(bind("#Ϯ", "/dev", MAFTER) < 0)
		print("init: bind #Ϯ (TPM) /dev failed\n");

	print("init: namespace built\n");

	/* Verify TPM availability */
	fd = open("/dev/tpm/random", OREAD);
	if(fd < 0){
		print("init: could not open /dev/tpm/random\n");
	} else {
		n = read(fd, buf, sizeof(buf));
		if(n > 0){
			print("init: TPM read %d random bytes successfully\n", n);
		} else {
			print("init: TPM read failed\n");
		}
		close(fd);
	}

	print("init: boot verification complete. looping.\n");

	for(;;)
		;  /* Just loop, no sleep syscall for now */
}

