#include <lux.h>

int exchange_prepare(uintptr vaddr, ExchangeCapability *out_cap) {
  return sys_exchange_prepare(vaddr, out_cap);
}

int exchange_prepare_range(uintptr vaddr, ulong len,
                           ExchangeCapability *handles) {
  return sys_exchange_prepare_range(vaddr, len, handles);
}

int exchange_accept(const ExchangeCapability *cap, uintptr dest_vaddr,
                    int prot) {
  return sys_exchange_accept(cap, dest_vaddr, prot);
}

int exchange_cancel(const ExchangeCapability *cap) {
  return sys_exchange_cancel(cap);
}

int exchange_transfer(int from_pid, int to_pid,
                      const ExchangeCapability *cap, uintptr dest_vaddr) {
  return sys_exchange_transfer(from_pid, to_pid, cap, dest_vaddr);
}

int exchange_pool_init(ExchangePagePool *pool, void *base, ulong len) {
  uintptr addr;

  if (!pool || !base || len == 0)
    return -1;
  addr = (uintptr)base;
  if ((addr & (EXCHANGE_PAGE_SIZE - 1)) != 0)
    return -1;
  if ((len & (EXCHANGE_PAGE_SIZE - 1)) != 0)
    return -1;

  pool->base = (uchar *)base;
  pool->len = len;
  pool->npages = len / EXCHANGE_PAGE_SIZE;
  return 0;
}

ulong exchange_pool_npages(const ExchangePagePool *pool) {
  if (!pool)
    return 0;
  return pool->npages;
}

void *exchange_pool_page(const ExchangePagePool *pool, ulong index) {
  if (!pool || index >= pool->npages)
    return nil;
  return pool->base + index * EXCHANGE_PAGE_SIZE;
}

int exchange_pool_index(const ExchangePagePool *pool, uintptr addr,
                        ulong *out_index) {
  uintptr base;
  uintptr offset;

  if (!pool || !pool->base || !out_index)
    return -1;

  base = (uintptr)pool->base;
  if (addr < base || addr >= base + pool->len)
    return -1;

  offset = addr - base;
  if ((offset & (EXCHANGE_PAGE_SIZE - 1)) != 0)
    return -1;

  *out_index = offset / EXCHANGE_PAGE_SIZE;
  return 0;
}

int exchange_pool_prepare(const ExchangePagePool *pool, ulong index,
                          ExchangeCapability *out_cap) {
  void *page;

  page = exchange_pool_page(pool, index);
  if (!page)
    return -1;
  return exchange_prepare((uintptr)page, out_cap);
}

int exchange_pool_prepare_addr(const ExchangePagePool *pool, uintptr addr,
                               ExchangeCapability *out_cap) {
  ulong index;

  if (exchange_pool_index(pool, addr, &index) < 0)
    return -1;
  return exchange_pool_prepare(pool, index, out_cap);
}

int exchange_pool_accept(const ExchangePagePool *pool, ulong index,
                         const ExchangeCapability *cap, int prot) {
  void *page;

  page = exchange_pool_page(pool, index);
  if (!page)
    return -1;
  return exchange_accept(cap, (uintptr)page, prot);
}

int exchange_pool_accept_addr(const ExchangePagePool *pool, uintptr addr,
                              const ExchangeCapability *cap, int prot) {
  ulong index;

  if (exchange_pool_index(pool, addr, &index) < 0)
    return -1;
  return exchange_pool_accept(pool, index, cap, prot);
}
