/*
 * Test /dev/irq - Wait for timer interrupt
 *
 * Timer interrupt is typically IRQ 0 (i8253 PIT)
 * This should fire regularly (~18.2 Hz on PC)
 */

#include <u.h>
#include <libc.h>

void
main(void)
{
	int ctlfd, irqfd;
	char buf[64];
	int n, i;

	print("test_devirq: Testing /dev/irq interrupt delivery\n");

	// Check if /dev/irq/ctl exists
	ctlfd = open("/dev/irq/ctl", OWRITE);
	if(ctlfd < 0) {
		print("FAIL: Cannot open /dev/irq/ctl: %r\n");
		exits("open ctl");
	}
	print("OK: Opened /dev/irq/ctl\n");

	// Register for timer interrupt (IRQ 0)
	n = fprint(ctlfd, "register 0 test_devirq");
	if(n < 0) {
		print("FAIL: Cannot register for IRQ 0: %r\n");
		close(ctlfd);
		exits("register");
	}
	print("OK: Registered for IRQ 0 (timer)\n");
	close(ctlfd);

	// Open IRQ 0 file
	irqfd = open("/dev/irq/0", OREAD);
	if(irqfd < 0) {
		print("FAIL: Cannot open /dev/irq/0: %r\n");
		exits("open irq");
	}
	print("OK: Opened /dev/irq/0\n");

	// Wait for 5 timer interrupts
	print("Waiting for 5 timer interrupts...\n");
	for(i = 0; i < 5; i++) {
		n = read(irqfd, buf, sizeof(buf)-1);
		if(n < 0) {
			print("FAIL: Cannot read IRQ: %r\n");
			close(irqfd);
			exits("read");
		}
		buf[n] = '\0';
		print("  [%d] Got interrupt: %s", i+1, buf);
	}

	close(irqfd);

	// Check status
	ctlfd = open("/dev/irq/ctl", OREAD);
	if(ctlfd >= 0) {
		print("\nIRQ Status:\n");
		while((n = read(ctlfd, buf, sizeof(buf)-1)) > 0) {
			buf[n] = '\0';
			print("  %s", buf);
		}
		close(ctlfd);
	}

	print("\nSUCCESS: /dev/irq works!\n");
	exits(nil);
}
