/* init - first userspace process */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Exchange page layout (must match kernel/include/9p_router.h) */
#define EXCHANGE_PAGE_ADDR 0x7fffffff0000ULL
#define P9_REQUEST_OFFSET 0x000
#define P9_REQUEST_SIZE 0xF00
#define P9_REPLY_OFFSET 0x1000
#define P9_REPLY_SIZE 0x1000
#define P9_CONTROL_OFFSET 0xF00
#define P9_STATUS_IDLE 0

/* Syscall stubs - will be implemented in libc later */
extern int spawn(const char *path, char *const argv[]); /* New 9P spawn */
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

	/* Example cmdline: "root=hd0:0 init_tests=1" */
	if(strcmp(key, "root") == 0) {
		strcpy(value, "hd0:0");
		return value;
	}
	if(strcmp(key, "init_tests") == 0) {
		const char *env = getenv("INIT_TESTS");
		if(env != NULL && *env != '\0'){
			strncpy(value, env, sizeof(value)-1);
			value[sizeof(value)-1] = '\0';
			return value;
		}
		return NULL;
	}

	return NULL;
}

static int
should_run_tests(void)
{
	const char *flag = get_kernel_param("init_tests");
	if(flag == NULL)
		return 0;
	if(strcmp(flag, "1") == 0 || strcmp(flag, "true") == 0 || strcmp(flag, "on") == 0)
		return 1;
	return 0;
}

/* Secure Ramdisk Test */
static void
test_secure_vault(void)
{
	int ctl, data;
	char buf[128];
	int n;

	printf("init: starting Secure Ramdisk test...\n");

	/* 1. Initialize/Unlock Vault */
	ctl = open("/dev/secureram.ctl", 1); /* O_WRITE */
	if(ctl < 0) {
		printf("init: failed to open vault control: %r\n");
		return;
	}

	printf("init: initializing vault...\n");
	if(write(ctl, "init password123", 16) < 0) {
		printf("init: vault init failed (maybe already init?)\n");
		/* Try unlock */
		if(write(ctl, "unlock password123", 18) < 0) {
			printf("init: vault unlock failed\n");
			close(ctl);
			return;
		}
	}
	close(ctl);

	/* 2. Write Secret Data */
	printf("init: writing to secure vault...\n");
	data = open("/dev/secureram", 1); /* O_WRITE */
	if(data < 0) {
		printf("init: failed to open vault data: %r\n");
		return;
	}
	if(write(data, "Top Secret Payload", 16) < 0) {
		printf("init: vault write failed\n");
		close(data);
		return;
	}
	close(data);

	/* 3. Read Back */
	printf("init: reading from secure vault...\n");
	data = open("/dev/secureram", 0); /* O_READ */
	if(data < 0) {
		printf("init: failed to open vault data for read\n");
		return;
	}
	memset(buf, 0, sizeof(buf));
	n = read(data, buf, sizeof(buf)-1);
	if(n < 0) {
		printf("init: vault read failed\n");
	} else {
		printf("init: vault content: '%s'\n", buf);
		if(strncmp(buf, "Top Secret Payload", 16) == 0)
			printf("init: vault integrity PASS\n");
		else
			printf("init: vault integrity FAIL\n");
	}
	close(data);
}

int
main(int argc, char *argv[])
{
	char *rootdev;
	char *init_args[] = { "/sbin/init", NULL };
	char *shell_args[] = { "/bin/sh", NULL };

	(void)argc;
	(void)argv;

	printf("\n");
	printf("=== Lux9 Init (Secure Mode) ===\n");
	printf("\n");

	/* Step 1: Run Secure Vault Test */
	test_secure_vault();

	/* Step 2: Spawn Shell */
	printf("init: spawning /bin/sh...\n");
	spawn("/bin/sh", shell_args);

	/* Loop forever */
	while(1) {
		sleep_ms(1000);
	}

	return 0;
}
