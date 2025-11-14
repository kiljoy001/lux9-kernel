/* init - first userspace process */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Syscall stubs - will be implemented in libc later */
extern int fork(void);
extern int exec(const char *path, char *const argv[]);
extern int mount(const char *path, int server_pid, const char *proto);
extern int open(const char *path, int flags);
extern void exit(int status);
extern int wait(int *status);
extern void sleep_ms(int ms);

static char cmdline[256];

static void
panic(const char *msg)
{
	printf("PANIC: %s\n", msg);
	for(;;);
}

static char*
get_kernel_param(const char *key)
{
	/* Parse kernel command line for key=value */
	/* For now, hardcoded - TODO: syscall to get cmdline */
	static char value[64];

	/* Example cmdline: "root=hd0:0 verbose" */
	/* Hardcode for now */
	if(strcmp(key, "root") == 0) {
		strcpy(value, "hd0:0");
		return value;
	}

	return NULL;
}

static int
start_server(const char *path, const char *arg)
{
	int pid;

	printf("init: starting %s", path);
	if(arg)
		printf(" %s", arg);
	printf("\n");

	pid = fork();
	if(pid < 0) {
		printf("init: fork failed\n");
		return -1;
	}

	if(pid == 0) {
		/* Child */
		char *argv[3];
		argv[0] = (char*)path;
		argv[1] = (char*)arg;
		argv[2] = NULL;

		exec(path, argv);
		panic("exec failed");
	}

	/* Parent */
	return pid;
}

static void
emergency_shell(void)
{
	static char *shell_argv[] = { "/bin/sh", NULL };

	printf("\n");
	printf("========================================\n");
	printf("   EMERGENCY SHELL - System Recovery\n");
	printf("========================================\n");
	printf("\n");
	printf("Something went wrong during boot.\n");
	printf("You are now in a minimal shell.\n");
	printf("\n");

	exec("/bin/sh", shell_argv);
	panic("cannot exec emergency shell");
}

int
main(int argc, char *argv[])
{
	char *rootdev;
	int fs_pid;
	int fd;
	char *init_args[] = { "/sbin/init", NULL };
	char *shell_args[] = { "/bin/sh", NULL };

	(void)argc;
	(void)argv;

	printf("\n");
	printf("=== Lux9 Init ===\n");
	printf("First userspace process starting...\n");
	printf("\n");

	/* Step 1: Determine root device */
	rootdev = get_kernel_param("root");
	if(!rootdev) {
		printf("init: no root= parameter, using default\n");
		rootdev = "hd0:0";
	}

	printf("init: root device is %s\n", rootdev);

	/* Step 2: Start filesystem server */
	fs_pid = start_server("/bin/ext4fs", rootdev);
	if(fs_pid < 0) {
		printf("init: failed to start ext4fs\n");
		emergency_shell();
	}

	/* Give server time to initialize */
	printf("init: waiting for ext4fs to initialize...\n");
	sleep_ms(200);

	/* Step 3: Mount root filesystem */
	printf("init: mounting root at /\n");
	if(mount("/", fs_pid, "9P2000") < 0) {
		printf("init: mount failed\n");
		emergency_shell();
	}

	/* Step 4: Verify mount worked */
	printf("init: verifying root filesystem...\n");
	fd = open("/etc/fstab", 0);  /* O_RDONLY */
	if(fd < 0) {
		printf("init: cannot access /etc/fstab\n");
		printf("init: root filesystem may not be ready\n");
		emergency_shell();
	}
	/* Close would go here */

	printf("init: root filesystem mounted successfully\n");

	/* Step 5: Start other essential servers */
	printf("init: starting system servers...\n");

	/* TODO: devfs, procfs, etc. */

	/* Step 6: Run exchange tests */
	printf("init: running simple test...\n");
	start_server("/bin/simple_test", NULL);
	
	/* Uncomment these lines to run the exchange tests instead
	printf("init: running original exchange test...\n");
	start_server("/bin/exchange_test", NULL);
	
	printf("init: running 9P exchange test...\n");
	start_server("/bin/exchange_9p_test", NULL);
	*/

	/* Step 7: Execute real init or shell */
	printf("init: attempting to exec /sbin/init...\n");
	exec("/sbin/init", init_args);

	printf("init: /sbin/init not found, trying /bin/sh...\n");
	exec("/bin/sh", shell_args);

	/* If we get here, nothing worked */
	panic("init: no shell available");

	return 0;
}
