#include "lib/lib9p_client.h"
#include <u.h>

int main(void) {
  char buf[128];
  long n;
  int fid;

  print("Starting 9P Client Test...\n");

  /* Initialize 9P exchange page */
  p9_init();

  /* Test 1: Console Write via /dev/cons */
  print("Test 1: Writing to /dev/cons via 9P...\n");
  fid = p9_attach("/dev/cons");
  if (fid < 0) {
    print("FAIL: attach /dev/cons error: %s\n", p9_errstr());
    return 1;
  }

  if (p9_open(fid, OWRITE) < 0) {
    print("FAIL: open /dev/cons error: %s\n", p9_errstr());
    return 1;
  }

  if (p9_write(fid, "Hello via 9P!\n", 14, 0) < 0) {
    print("FAIL: write /dev/cons error: %s\n", p9_errstr());
    return 1;
  }
  p9_clunk(fid);
  print("PASS: /dev/cons write\n");

  /* Test 2: Pipe Creation and Stats */
  print("Test 2: Exploring /dev/pipe...\n");
  fid = p9_attach("/dev/pipe");
  if (fid < 0) {
    print("FAIL: attach /dev/pipe error: %s\n", p9_errstr());
    return 1;
  }
  /* Stat the pipe root */
  Dir d;
  if (p9_stat(fid, &d) < 0) {
    print("FAIL: stat /dev/pipe error: %s\n", p9_errstr());
    return 1;
  }
  print("PASS: /dev/pipe stat name=%s type=%d dev=%d\n", d.name, d.type, d.dev);
  p9_clunk(fid);

  /* Test 3: /fd/1 Access (Self FD) */
  print("Test 3: Accessing /fd/1...\n");
  /* We need to walk to /fd first? Or attach /fd? */
  /* Handlers are routed by path? */
  /* p9_attach expects a path. p9_dispatch routes checks. */
  /* If we attach to "/fd", we get the fd root. */
  fid = p9_attach("/fd");
  if (fid < 0) {
    print("FAIL: attach /fd error: %s\n", p9_errstr());
  } else {
    /* Walk to "1" */
    int newfid = p9_allocfid();
    if (p9_walk(fid, "1", newfid) < 0) {
      print("FAIL: walk /fd/1 error: %s\n", p9_errstr());
    } else {
      /* Open it? */
      if (p9_open(newfid, OWRITE) < 0) {
        print("FAIL: open /fd/1 error: %s\n", p9_errstr());
      } else {
        if (p9_write(newfid, "Write to valid fd 1\n", 20, 0) < 0) {
          print("FAIL: write /fd/1 error: %s\n", p9_errstr());
        } else {
          print("PASS: /fd/1 write success\n");
        }
      }
      p9_clunk(newfid);
    }
    p9_clunk(fid);
  }

  print("9P Client Test Completed Successfully.\n");
  return 0;
}
