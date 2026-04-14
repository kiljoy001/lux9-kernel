#ifndef _LIBLUX_EXCHANGE_POOL_H_
#define _LIBLUX_EXCHANGE_POOL_H_

#ifndef _LIBLUX_H_
#ifndef _U_H_
#include <u.h>
#endif
#endif

#ifndef LUX_EXCHANGE_TYPES_DEFINED
#define LUX_EXCHANGE_TYPES_DEFINED
typedef struct {
  uchar uuid[16];
  uchar hash[32];
  ulong size;
  uint type;
  uint perms;
} ExchangeCapability;

typedef struct {
  uchar message_id[16];
  uchar topic_uuid[16];
  ExchangeCapability *capability;
  int delivered_count;
  int ack_count;
} Notification;
#endif

#ifndef EXCHANGE_PAGE_SIZE
#define EXCHANGE_PAGE_SIZE 0x1000UL
#endif

typedef struct ExchangePagePool {
  uchar *base;
  ulong len;
  ulong npages;
} ExchangePagePool;

int sys_exchange_prepare(uintptr vaddr, ExchangeCapability *out_cap);
int sys_exchange_prepare_range(uintptr vaddr, ulong len,
                               ExchangeCapability *handles);
int sys_exchange_accept(const ExchangeCapability *cap, uintptr dest_vaddr,
                        int prot);
int sys_exchange_cancel(const ExchangeCapability *cap);
int sys_exchange_transfer(int from_pid, int to_pid,
                          const ExchangeCapability *cap, uintptr dest_vaddr);
ExchangeCapability *sys_exchange_alloc(void);
int sys_exchange_free(ExchangeCapability *cap);
ExchangeCapability *sys_exchange_publish(char *topic, void *data, ulong len);
int sys_exchange_subscribe(char *topic);
int sys_exchange_unsubscribe(char *topic);
Notification *sys_exchange_receive(void);

int exchange_prepare(uintptr vaddr, ExchangeCapability *out_cap);
int exchange_prepare_range(uintptr vaddr, ulong len,
                           ExchangeCapability *handles);
int exchange_accept(const ExchangeCapability *cap, uintptr dest_vaddr,
                    int prot);
int exchange_cancel(const ExchangeCapability *cap);
int exchange_transfer(int from_pid, int to_pid,
                      const ExchangeCapability *cap, uintptr dest_vaddr);
int exchange_pool_init(ExchangePagePool *pool, void *base, ulong len);
ulong exchange_pool_npages(const ExchangePagePool *pool);
void *exchange_pool_page(const ExchangePagePool *pool, ulong index);
int exchange_pool_index(const ExchangePagePool *pool, uintptr addr,
                        ulong *out_index);
int exchange_pool_prepare(const ExchangePagePool *pool, ulong index,
                          ExchangeCapability *out_cap);
int exchange_pool_prepare_addr(const ExchangePagePool *pool, uintptr addr,
                               ExchangeCapability *out_cap);
int exchange_pool_accept(const ExchangePagePool *pool, ulong index,
                         const ExchangeCapability *cap, int prot);
int exchange_pool_accept_addr(const ExchangePagePool *pool, uintptr addr,
                              const ExchangeCapability *cap, int prot);

#endif
