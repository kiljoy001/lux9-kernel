/* clang-format off */
/* u.h is included via command line -include */
#include "kernel.h"

/* Local Plan 9 Syscall ABI fix */
typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type*)((list)++))

#define syscall_vainit(list, start) ((list) = (syscall_va_list)(start))

#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>
#include "exchange.h"
#include "exchange_pool.h"

/* STUB FUNCTIONS - Enabled */

static Proc *exchange_pid_to_proc(int pid) {
  int slot;

  if (pid <= 0)
    return nil;
  slot = procindex((ulong)pid);
  if (slot < 0)
    return nil;
  return proctab(slot);
}

/*
 * sys_exchange_prepare - Prepare a page for exchange
 *
 * Usage: exchange_prepare(vaddr) -> UserCapability
 *
 * Removes page from current process and prepares it for transfer.
 * Returns a capability that can be passed to exchange_accept.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_prepare(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uintptr vaddr;
  UserCapability *out_cap;
  UserCapability cap;
  BlindLedgerError err;

  vaddr = SYSCALL_ARG(list, uintptr);
  out_cap = SYSCALL_ARG(list, UserCapability *);

  if(out_cap == nil)
    error("exchange_prepare: null output capability");

  validaddr((uintptr)out_cap, sizeof(UserCapability), 1);

  /* Call kernel exchange_prepare function */
  err = exchange_prepare(vaddr, &cap);

  /* Convert BlindLedgerError to syscall return code */
  switch(err) {
  case BLIND_LEDGER_OK:
    memmove(out_cap, &cap, sizeof(UserCapability));
    return 0;
  case BLIND_LEDGER_EINVAL:
    error("exchange_prepare: invalid address");
    break;
  case BLIND_LEDGER_EPERM:
    error("exchange_prepare: permission denied");
    break;
  case BLIND_LEDGER_ENOMEM:
    error("exchange_prepare: out of memory");
    break;
  default:
    error("exchange_prepare: unknown error");
    break;
  }
  
  return -1;
}

/*
 * sys_exchange_accept - Accept an exchanged page
 *
 * Usage: exchange_accept(cap, dest_vaddr, prot)
 *
 * Accepts a page capability and maps it into the current process.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_accept(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  UserCapability *cap;
  uintptr dest_vaddr;
  int prot;
  int err;
  
  cap = SYSCALL_ARG(list, UserCapability *);
  dest_vaddr = SYSCALL_ARG(list, uintptr);
  prot = SYSCALL_ARG(list, int);
  
  /* Validate capability pointer */
  validaddr((uintptr)cap, sizeof(UserCapability), 0);
  
  /* Call kernel exchange_accept function */
  err = exchange_accept(cap, dest_vaddr, prot);
  
  /* Convert exchange error to syscall return */
  switch(err) {
  case EXCHANGE_OK:
    return 0;
  case EXCHANGE_EINVAL:
    error("exchange_accept: invalid capability or address");
    break;
  default:
    error("exchange_accept: unknown error");
    break;
  }
  
  return -1;
}

/*
 * sys_exchange_cancel - Cancel an exchange preparation
 *
 * Usage: exchange_cancel(cap)
 *
 * Cancels a prepared exchange and returns the page to original owner.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_cancel(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  UserCapability *cap;
  int err;
  
  cap = SYSCALL_ARG(list, UserCapability *);
  
  /* Validate capability pointer */
  validaddr((uintptr)cap, sizeof(UserCapability), 0);
  
  /* Call kernel exchange_cancel function */
  err = exchange_cancel(cap);
  
  /* Convert exchange error to syscall return */
  switch(err) {
  case EXCHANGE_OK:
    return 0;
  case EXCHANGE_EINVAL:
    error("exchange_cancel: invalid capability");
    break;
  default:
    error("exchange_cancel: unknown error");
    break;
  }
  
  return -1;
}

/*
 * sys_exchange_transfer - Transfer a prepared page to another process
 *
 * Usage: exchange_transfer(from_pid, to_pid, cap, dest_vaddr)
 *
 * The source pid must match the current process unless the caller is a kernel
 * process.
 */
uintptr sys_exchange_transfer(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int from_pid;
  int to_pid;
  UserCapability *cap;
  uintptr dest_vaddr;
  Proc *from;
  Proc *to;
  int err;

  from_pid = SYSCALL_ARG(list, int);
  to_pid = SYSCALL_ARG(list, int);
  cap = SYSCALL_ARG(list, UserCapability *);
  dest_vaddr = SYSCALL_ARG(list, uintptr);

  if (up == nil)
    error("exchange_transfer: no current process");
  if (cap == nil)
    error("exchange_transfer: null capability");
  if ((dest_vaddr & (BY2PG - 1)) != 0)
    error("exchange_transfer: destination not page-aligned");

  validaddr((uintptr)cap, sizeof(UserCapability), 0);

  from = up;
  if (up->kp == 0 && from_pid != (int)up->pid)
    error("exchange_transfer: source pid mismatch");
  if (up->kp != 0)
    from = exchange_pid_to_proc(from_pid);
  if (from == nil)
    error("exchange_transfer: invalid source process");

  to = exchange_pid_to_proc(to_pid);
  if (to == nil)
    error("exchange_transfer: invalid target process");

  err = exchange_transfer(from, to, cap, dest_vaddr);
  switch(err) {
  case EXCHANGE_OK:
    return 0;
  case EXCHANGE_ENOTOWNER:
    error("exchange_transfer: not owner");
    break;
  case EXCHANGE_EBORROWED:
    error("exchange_transfer: page is borrowed");
    break;
  default:
    error("exchange_transfer: failed");
    break;
  }

  return -1;
}

/*
 * sys_exchange_prepare_range - Prepare multiple pages for exchange
 *
 * Usage: exchange_prepare_range(vaddr, len, handles) -> count
 *
 * Prepares a range of pages for exchange.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_prepare_range(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uintptr vaddr;
  ulong len;
  UserCapability *handles;
  int count = 0;
  ulong offset;
  
  vaddr = SYSCALL_ARG(list, uintptr);
  len = SYSCALL_ARG(list, ulong);
  handles = SYSCALL_ARG(list, UserCapability *);
  
  /* Validate parameters */
  if((vaddr & (BY2PG-1)) != 0)
    error("exchange_prepare_range: address not page-aligned");
  if(len == 0)
    error("exchange_prepare_range: zero length");
  if(handles == nil)
    error("exchange_prepare_range: null handles pointer");
    
  /* Validate handles pointer for the maximum possible range */
  validaddr((uintptr)handles, (len/BY2PG) * sizeof(UserCapability), 1);
  
  /* Prepare each page in the range */
  for(offset = 0; offset < len; offset += BY2PG) {
    uintptr page_vaddr = vaddr + offset;
    BlindLedgerError err;
    
    /* Call kernel exchange_prepare function */
    err = exchange_prepare(page_vaddr, &handles[count]);
    
    if(err != BLIND_LEDGER_OK) {
      /* On error, cancel any previously prepared pages */
        /*@ loop invariant 0 <= i <= count;
    @ loop assigns i;
    @ loop variant count - i;
    @*/
  for(int i = 0; i < count; i++) {
        exchange_cancel(&handles[i]);
      }
      
      switch(err) {
      case BLIND_LEDGER_EINVAL:
        error("exchange_prepare_range: invalid address");
        break;
      case BLIND_LEDGER_EPERM:
        error("exchange_prepare_range: permission denied");
        break;
      case BLIND_LEDGER_ENOMEM:
        error("exchange_prepare_range: out of memory");
        break;
      default:
        error("exchange_prepare_range: unknown error");
        break;
      }
    }
    
    count++;
  }
  
  return count;
}

/*
 * sys_exchange_alloc - Allocate a page from the global exchange pool
 *
 * Usage: exchange_alloc() -> UserCapability*
 *
 * Allocates a page from the global exchange pool and returns a capability.
 */
/*@
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_alloc(void *a) {
  UserCapability *cap;
  Proc *p = up; // Current process
  
  if(p == nil)
    error("exchange_alloc: no current process");
    
  cap = xalloc(sizeof(UserCapability));
  if(cap == nil)
    error("exchange_alloc: out of memory");
    
  if(global_pool_alloc_page(p, cap) != POOL_OK) {
    free(cap);
    error("exchange_alloc: pool allocation failed");
  }
  
  return (uintptr)cap;
}

/*
 * sys_exchange_free - Return a page to the global exchange pool
 *
 * Usage: exchange_free(cap)
 *
 * Returns a previously allocated page to the global exchange pool.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_free(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  UserCapability *cap;
  Proc *p = up; // Current process
  
  if (list_void != nil) {
    cap = SYSCALL_ARG(list, UserCapability *);
  } else {
    // Alternative signature for direct syscall with one argument
    error("exchange_free: invalid syscall args");
  }
  
  if(p == nil)
    error("exchange_free: no current process");
    
  if(cap == nil)
    error("exchange_free: null capability");
    
  /* Validate capability pointer */
  validaddr((uintptr)cap, sizeof(UserCapability), 0);
  
  if(global_pool_free_page(p, cap) != POOL_OK) {
    error("exchange_free: pool free failed");
  }
  
  free(cap);
  return 0;
}
/*
 * sys_exchange_publish - Publish a message to a topic
 *
 * Usage: exchange_publish(topic_name, data, len) -> UserCapability*
 *
 * Publishes a message to a named topic and returns a capability for the published page.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_publish(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *topic_name;
  void *data;
  ulong len;
  Proc *p = up; // Current process
  
  topic_name = SYSCALL_ARG(list, char *);
  data = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, ulong);
  
  if(p == nil)
    error("exchange_publish: no current process");
    
  if(topic_name == nil)
    error("exchange_publish: null topic name");
    
  if(data == nil && len > 0)
    error("exchange_publish: null data with non-zero length");
    
  // Validate topic name pointer
  validaddr((uintptr)topic_name, 1, 0); // At least 1 byte
  
  // Validate data pointer if provided
  if(data != nil && len > 0) {
    validaddr((uintptr)data, len, 0);
  }
  
  // Call the actual implementation
  UserCapability *cap = publish_message(p, topic_name, data, len);
  if(cap == nil)
    error("exchange_publish: failed to publish message");
  return (uintptr)cap;
}

/*
 * sys_exchange_subscribe - Subscribe to a topic
 *
 * Usage: exchange_subscribe(topic_name) -> int
 *
 * Subscribes the current process to receive messages from a named topic.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_subscribe(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *topic_name;
  Proc *p = up; // Current process
  
  topic_name = SYSCALL_ARG(list, char *);
  
  if(p == nil)
    error("exchange_subscribe: no current process");
    
  if(topic_name == nil)
    error("exchange_subscribe: null topic name");
    
  // Validate topic name pointer
  validaddr((uintptr)topic_name, 1, 0); // At least 1 byte
  
  // Call the actual implementation
  int result = subscribe_to_topic(p, topic_name);
  if(result < 0)
    error("exchange_subscribe: failed to subscribe");
  return 0;
}

/*
 * sys_exchange_unsubscribe - Unsubscribe from a topic
 *
 * Usage: exchange_unsubscribe(topic_name) -> int
 *
 * Unsubscribes the current process from a named topic.
 */
/*@
  @ requires list_void == \null || \valid(list_void);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_unsubscribe(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *topic_name;
  Proc *p = up; // Current process
  
  topic_name = SYSCALL_ARG(list, char *);
  
  if(p == nil)
    error("exchange_unsubscribe: no current process");
    
  if(topic_name == nil)
    error("exchange_unsubscribe: null topic name");
    
  // Validate topic name pointer
  validaddr((uintptr)topic_name, 1, 0); // At least 1 byte
  
  // Call the actual implementation
  int result = unsubscribe_from_topic(p, topic_name);
  if(result < 0)
    error("exchange_unsubscribe: failed to unsubscribe");
  return 0;
}

/*
 * sys_exchange_receive - Receive a notification from subscribed topics
 *
 * Usage: exchange_receive() -> Notification*
 *
 * Receives the next notification for messages published to subscribed topics.
 */
/*@
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
uintptr sys_exchange_receive(void *a) {
  Proc *p = up; // Current process
  
  if(p == nil)
    error("exchange_receive: no current process");

  // Call the actual implementation
  USED(a);
  Notification *notif = dequeue_notification(p);
  return (uintptr)notif;  // Returns nil if no notifications
}

/* Syscall table wrappers (names without underscores) */
/*@
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
uintptr sysexchangepublish(void *a) {
  return sys_exchange_publish(a);
}

/*@
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
uintptr sysexchangesubscribe(void *a) {
  return sys_exchange_subscribe(a);
}

/*@
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
uintptr sysexchangeunsubscribe(void *a) {
  return sys_exchange_unsubscribe(a);
}

/*@
  @ requires a == \null || \valid(a);
  @ assigns \nothing;
  @*/
uintptr sysexchangereceive(void *a) {
  return sys_exchange_receive(a);
}

/* End of syscall wrappers */
