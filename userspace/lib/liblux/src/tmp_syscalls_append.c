int sys_sleep(long ms) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyssleep;
  tx.tag = 1;
  tx.count = (u32int)ms;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return 0;
}

/* Exchange Pool Syscalls */

ExchangeCapability* sys_exchange_alloc(void) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_ALLOC;
  tx.scount = 0;
  tx.sdata = 0;

  if (lux_call(&tx, &rx) < 0)
    return nil;
    
  if (rx.retval == 0)
    return nil;
    
  return (ExchangeCapability*)rx.retval;
}

int sys_exchange_free(ExchangeCapability *cap) {
  uchar buf[sizeof(uintptr)];
  uchar *p = buf;
  
  // Pack the capability pointer
  uintptr cap_addr = (uintptr)cap;
  pack32(p, (uint)(cap_addr & 0xFFFFFFFF));
  if (sizeof(uintptr) > 4) {
    pack32(p + 4, (uint)(cap_addr >> 32));
  }
  
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_FREE;
  tx.sdata = buf;
  tx.scount = sizeof(uintptr);

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return 0;
}