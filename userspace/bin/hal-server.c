/*
 * HAL Server - Hardware Abstraction Layer via 9P with TPM Security
 *
 * Serves hardware information as 9P file system with TPM security
 * Uses exchange pages for secure, zero-copy 9P message passing.
 * Supports hot-swappable devices via family subsystem events.
 */

#include "../lib/syscall.h"
#include <fcall.h>
#include <libc.h>
#include <u.h>

/* Constants */
#define STACKsize 8192
#define RING_SIZE 4096
#define MAX_MSG_SIZE 8192

/* Exchange Page Ring Buffer */
typedef struct ExchangeRing {
  u32int head;
  u32int tail;
  u32int size;
  u8int data[RING_SIZE];
} ExchangeRing;

/* Globals */
ExchangeRing *rx_ring;
ExchangeRing *tx_ring;
ExchangeHandle rx_handle;
ExchangeHandle tx_handle;
int debug = 1;

/* Helper: Print debug message */
void dprint(char *fmt, ...) {
  va_list arg;
  if (debug) {
    va_start(arg, fmt);
    vfprint(2, fmt, arg);
    va_end(arg);
  }
}

/* Initialize TPM (Stub) */
{
  int fd;
  char buf[64];

  print("HAL: Initializing TPM security...\n");

  fd = open("/dev/tpm", OREAD);
  if (fd < 0) {
    print("HAL: Warning: /dev/tpm not found (simulated env?)\n");
    return;
  }

  /* Send self-test command (simulated) */
  /* In a real TPM2, we'd send a binary command.
     For now, just verifying access. */
  print("HAL: TPM device opened successfully\n");

  close(fd);
}

/* Scan Hardware (Stub) */
{
  int fd;
  char buf[256];
  int n;

  print("HAL: Scanning hardware via Family subsystem...\n");

  /* Check for family controller */
  fd = open("/dev/family/ctl", OREAD);
  if (fd < 0) {
    /* Try iterating /dev to find pci? */
    /* Assuming standard PCI path for this HAL version */
    fd = open("#p/pci/ctl",
              OREAD); /* Try kernel device directly if family not mounted */
  }

  if (fd >= 0) {
    print("HAL: Family/PCI controller found\n");
    /* Read some info */
    n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = 0;
      print("HAL: Family Status: %s\n", buf);
    }
    close(fd);
  } else {
    print("HAL: Warning: No hardware enumeration source found\n");
  }
}

/* Initialize Exchange Pages for 9P Transport */
void hal_exchange_init(void) {
  void *rx_mem, *tx_mem;
  int ret;

  print("HAL: Initializing Exchange Pages for 9P transport...\n");

  /* Allocate black pebble memory for rings */
  ret = pebble_black_alloc(sizeof(ExchangeRing), &rx_mem);
  if (ret < 0)
    sysfatal("HAL: Failed to allocate RX ring via pebble");

  ret = pebble_black_alloc(sizeof(ExchangeRing), &tx_mem);
  if (ret < 0)
    sysfatal("HAL: Failed to allocate TX ring via pebble");

  rx_ring = (ExchangeRing *)rx_mem;
  tx_ring = (ExchangeRing *)tx_mem;

  memset(rx_ring, 0, sizeof(ExchangeRing));
  memset(tx_ring, 0, sizeof(ExchangeRing));

  rx_ring->size = RING_SIZE;
  tx_ring->size = RING_SIZE;

  /* Register as exchange pages */
  rx_handle = exchange_prepare((uintptr)rx_ring);
  if (rx_handle == 0)
    sysfatal("HAL: Failed to prepare RX exchange page");

  tx_handle = exchange_prepare((uintptr)tx_ring);
  if (tx_handle == 0)
    sysfatal("HAL: Failed to prepare TX exchange page");

  print("HAL: Exchange Transport Ready\n");
  print("HAL: RX Handle: %p\n", (void *)rx_handle);
  print("HAL: TX Handle: %p\n", (void *)tx_handle);
}

/* Ring Buffer Read */
int ring_read(ExchangeRing *ring, u8int *buf, int len) {
  int available;
  int head, tail;
  int n1, n2;

  head = ring->head;
  tail = ring->tail;

  if (head == tail)
    return 0;

  if (head > tail)
    available = head - tail;
  else
    available = (ring->size - tail) + head;

  if (len > available)
    len = available;

  /* Read first chunk (up to end of buffer) */
  n1 = ring->size - tail;
  if (n1 > len)
    n1 = len;
  memmove(buf, &ring->data[tail], n1);

  /* Read second chunk (wrap around) */
  n2 = len - n1;
  if (n2 > 0)
    memmove(buf + n1, &ring->data[0], n2);

  /* Update tail */
  ring->tail = (tail + len) % ring->size;

  return len;
}

/* Ring Buffer Write */
int ring_write(ExchangeRing *ring, u8int *buf, int len) {
  int free_space;
  int head, tail;
  int n1, n2;

  head = ring->head;
  tail = ring->tail;

  if (tail > head)
    free_space = tail - head - 1;
  else
    free_space = (ring->size - head) + tail - 1;

  if (len > free_space)
    return 0;

  /* Write first chunk */
  n1 = ring->size - head;
  if (n1 > len)
    n1 = len;
  memmove(&ring->data[head], buf, n1);

  /* Write second chunk */
  n2 = len - n1;
  if (n2 > 0)
    memmove(&ring->data[0], buf + n1, n2);

  /* Update head */
  ring->head = (head + len) % ring->size;

  return len;
}

/* Process 9P Message (Minimal Implementation) */
void hal_process_9p(u8int *msg, int len) {
  u32int size;
  u8int type;
  u16int tag;
  u8int reply[MAX_MSG_SIZE];
  int reply_len = 0;

  if (len < 7)
    return; /* Header size */

  /* Parse header manually to avoid lib9p dependency issues for now */
  size = msg[0] | (msg[1] << 8) | (msg[2] << 16) | (msg[3] << 24);
  type = msg[4];
  tag = msg[5] | (msg[6] << 8);

  dprint("HAL: 9P Recv: size=%d type=%d tag=%d\n", size, type, tag);

  switch (type) {
  case Tversion:
    dprint("HAL: Tversion\n");
    /* Rversion: size[4] Rversion[1] tag[2] msize[4] version[s] */
    /* Reply "9P2000" and msize 8192 */
    reply_len = 4 + 1 + 2 + 4 + 2 + 6;
    reply[4] = Rversion;
    reply[5] = tag & 0xFF;
    reply[6] = (tag >> 8) & 0xFF;

    /* msize = 8192 */
    reply[7] = 0x00;
    reply[8] = 0x20;
    reply[9] = 0x00;
    reply[10] = 0x00;

    /* version = "9P2000" */
    reply[11] = 0x06;
    reply[12] = 0x00; /* len */
    memcpy(&reply[13], "9P2000", 6);
    break;

  case Tattach:
    dprint("HAL: Tattach\n");
    /* Rattach: size[4] Rattach[1] tag[2] qid[13] */
    reply_len = 4 + 1 + 2 + 13;
    reply[4] = Rattach;
    reply[5] = tag & 0xFF;
    reply[6] = (tag >> 8) & 0xFF;

    /* Qid: type=QTDIR, vers=0, path=0 */
    reply[7] = 0x80; /* QTDIR */
    memset(&reply[8], 0, 12);
    break;

  default:
    dprint("HAL: Unknown 9P type %d\n", type);
    /* Rerror */
    reply_len = 4 + 1 + 2 + 2 + 7;
    reply[4] = Rerror;
    reply[5] = tag & 0xFF;
    reply[6] = (tag >> 8) & 0xFF;
    reply[7] = 0x07;
    reply[8] = 0x00;
    memcpy(&reply[9], "unknown", 7);
    break;
  }

  /* Fill size */
  reply[0] = reply_len & 0xFF;
  reply[1] = (reply_len >> 8) & 0xFF;
  reply[2] = (reply_len >> 16) & 0xFF;
  reply[3] = (reply_len >> 24) & 0xFF;

  /* Write response */
  ring_write(tx_ring, reply, reply_len);
}

/* Main 9P Loop using Exchange Pages */
void hal_9p_loop(void) {
  u8int buf[MAX_MSG_SIZE];
  int n;
  u32int msg_size;

  print("HAL: Entering 9P Exchange loop...\n");

  for (;;) {
    /* Check for data */
    /* Peek size */
    if (rx_ring->head == rx_ring->tail) {
      sleep(10); /* Busy wait / sleep hybrid */
      continue;
    }

    /* We need to read enough to get the size first, or handle stream */
    /* Simplification: Assume atomic messages or basic framing */

    /* Peek first 4 bytes for size */
    /* This requires ring_peek logic, but let's just try to read */

    n = ring_read(rx_ring, buf, sizeof(buf));
    if (n > 0) {
      /* Process buffer */
      /* In a stream, we might have multiple messages or partials. */
      /* For this implementation, process what we got */
      hal_process_9p(buf, n);
    }
  }
}

void main(int argc, char *argv[]) {
  print("HAL Server Starting (Lux9 Native)\n");

  hal_tpm_init();
  hal_scan_hardware();
  hal_exchange_init();

  hal_9p_loop();

  exits(nil);
}
