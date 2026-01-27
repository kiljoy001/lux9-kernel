#include <u.h>
#include <libc.h>

void
main(void)
{
	int fd;
	char tmpfile[256];
	
	print("Testing temp file creation...\n");
	
	/* Try different temp file approaches */
	snprint(tmpfile, sizeof(tmpfile), "/tmp/vmx_test.%d", getpid());
	print("Trying to create: %s\n", tmpfile);
	
	remove(tmpfile);
	
	fd = create(tmpfile, OWRITE, 0666);
	if(fd < 0){
		print("create failed: %r\n");
		exits("create failed");
	}
	print("create succeeded, fd=%d\n", fd);
	
	if(write(fd, "test", 4) != 4){
		print("write failed: %r\n");
		close(fd);
		exits("write failed");
	}
	print("write succeeded\n");
	close(fd);
	
	/* Try to open for reading */
	fd = open(tmpfile, OREAD);
	if(fd < 0){
		print("open for read failed: %r\n");
		exits("open failed");
	}
	print("open for read succeeded, fd=%d\n", fd);
	close(fd);
	
	print("All operations succeeded!\n");
	remove(tmpfile);
}