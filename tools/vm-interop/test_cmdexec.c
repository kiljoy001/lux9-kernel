/*
 * Test program for cmdexec
 */

#include <u.h>
#include <libc.h>

void
main(void)
{
	int fd;
	char *cmds[] = {
		"echo 'Test 1: Basic echo'",
		"ls /tmp | head -5",
		"pwd",
		"echo 'Test 4: Multiple words'",
		"false",  /* Should show error status */
		"echo 'Test 6: After error'"
	};
	int i;
	
	print("Testing cmdexec...\n");
	
	/* Write test commands */
	fd = open("/n/interop/command.txt", OWRITE|OTRUNC);
	if(fd < 0){
		fprint(2, "cannot open command.txt: %r\n");
		exits("open");
	}
	
	for(i = 0; i < nelem(cmds); i++){
		fprint(fd, "%s\n", cmds[i]);
		print("Wrote command %d: %s\n", i+1, cmds[i]);
	}
	close(fd);
	
	/* Wait for processing */
	print("Waiting 3 seconds for commands to be processed...\n");
	sleep(3000);
	
	/* Check output */
	fd = open("/n/interop/output.txt", OREAD);
	if(fd < 0){
		fprint(2, "cannot open output.txt: %r\n");
		exits("open");
	}
	
	/* Seek to near end to see recent output */
	seek(fd, -2000, 2);
	
	char buf[2048];
	int n = read(fd, buf, sizeof(buf)-1);
	if(n > 0){
		buf[n] = '\0';
		print("\nRecent output:\n%s\n", buf);
	}
	close(fd);
	
	print("\nTest complete. Check output.txt for full results.\n");
	exits(nil);
}