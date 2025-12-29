/* init.c - Minimal init using pure 9P syscalls
 *
 * This init uses lib9p_syscall to communicate with the kernel
 * entirely via 9P messages on the exchange page.
 * No traditional syscall ABI - pure message passing.
 */

#include "lib9p.h"

/* Entry point - called by _start in start.S */
void main(void) {
  int fd;

  /* Open console device via 9P */
  fd = p9_open("#c/cons", OWRITE);
  if (fd < 0) {
    p9_exit("open cons failed");
  }

  /* Write hello message via 9P */
  p9_write(fd, "Hello from pure 9P init!\n", 25);

  /* Close and exit */
  p9_close(fd);
  p9_exit(0);
}
