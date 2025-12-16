/* Minimal init for Lux9 - First userspace process */
#include <u.h>
#include <libc.h>

int
main(void)
{
	int i;
	char *argv_rc[] = {"rc", nil};
	char *argv_sh[] = {"sh", nil};

	/* Print startup banner */
	fprint(2, "\n");
	fprint(2, "=== Lux9 Minimal Init ===\n");
	fprint(2, "First userspace process starting...\n");
	fprint(2, "\n");

	/* Close extra file descriptors */
	for(i = 3; i < 30; i++)
		close(i);

	/* Try to exec the shell */
	fprint(2, "init: starting /bin/rc\n");
	exec("/bin/rc", argv_rc);

	/* If exec fails, print error and try another shell */
	fprint(2, "init: exec /bin/rc failed: %r\n");
	fprint(2, "init: trying /bin/sh\n");
	exec("/bin/sh", argv_sh);

	fprint(2, "init: exec /bin/sh failed: %r\n");
	fprint(2, "init: no shell available, halting\n");

	/* Loop forever */
	for(;;)
		sleep(1000);

	return 0;
}
