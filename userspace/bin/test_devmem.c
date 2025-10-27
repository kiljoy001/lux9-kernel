/*
 * Test /dev/mem - Read VGA text buffer
 *
 * VGA text mode buffer is at physical address 0xB8000
 * This is a safe MMIO region to test with.
 */

#include <u.h>
#include <libc.h>

void
main(void)
{
	int fd;
	uchar buf[160];  // One line of VGA text (80 chars * 2 bytes)
	int n;

	print("test_devmem: Testing /dev/mem access\n");

	// Open /dev/mem
	fd = open("/dev/mem", OREAD);
	if(fd < 0) {
		print("FAIL: Cannot open /dev/mem: %r\n");
		exits("open");
	}
	print("OK: Opened /dev/mem\n");

	// Seek to VGA text buffer (0xB8000)
	if(seek(fd, 0xB8000, 0) < 0) {
		print("FAIL: Cannot seek to 0xB8000: %r\n");
		close(fd);
		exits("seek");
	}
	print("OK: Seeked to 0xB8000 (VGA text buffer)\n");

	// Read first line of screen
	n = read(fd, buf, sizeof(buf));
	if(n < 0) {
		print("FAIL: Cannot read from VGA buffer: %r\n");
		close(fd);
		exits("read");
	}
	print("OK: Read %d bytes from VGA buffer\n", n);

	// Print first few characters (ASCII only, skip attributes)
	print("VGA buffer contents (first 20 chars): ");
	for(int i = 0; i < 40 && i < n; i += 2) {
		char c = buf[i];
		if(c >= 32 && c < 127)
			print("%c", c);
		else
			print(".");
	}
	print("\n");

	close(fd);
	print("SUCCESS: /dev/mem works!\n");
	exits(nil);
}
