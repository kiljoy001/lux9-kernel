/* exchange_pool.h - Global exchange pool for page allocation */

#ifndef _EXCHANGE_POOL_H_
#define _EXCHANGE_POOL_H_

/* Error codes */
typedef enum {
  POOL_OK = 0,
  POOL_EINVAL,
  POOL_ENOMEM,
  POOL_EPERM,
} PoolError;

/* Pool configuration */
#define POOL_SIZE 512 /* Fixed size for now */
#define MIN_PAGES_PER_PROCESS 1
#define MAX_PAGES_PER_PROCESS 32

/* IPC Pub-Sub configuration */
#define MAX_TOPIC_LENGTH 64
#define MAX_SUBSCRIBERS_PER_TOPIC 16
#define MAX_PENDING_NOTIFICATIONS 256

/* Forward declarations */
struct Proc;

/* Subscription - tracks who is subscribed to a topic */
typedef struct Subscription {
  struct Proc *subscriber;
  uuid_t subscriber_id;
  int active;
} Subscription;

/* Topic - named channel for message distribution */
typedef struct Topic {
  uuid_t topic_uuid;           /* Topic UUIDv8 identifier */
  char name[MAX_TOPIC_LENGTH]; /* Human-readable topic name */
  Subscription
      subscribers[MAX_SUBSCRIBERS_PER_TOPIC]; /* Registered subscribers */
  int subscriber_count;                       /* Number of active subscribers */
  u32int sequence_counter; /* For generating sequence numbers */
  struct Topic *left;      /* RBTree left child */
  struct Topic *right;     /* RBTree right child */
  int red;                 /* RBTree color */
} Topic;

/* Notification - tracks a pending message to be delivered */
typedef struct Notification {
  uuid_t message_id;          /* Message UUID */
  uuid_t topic_uuid;          /* Topic UUID */
  UserCapability *capability; /* Published page capability */
  int delivered_count;        /* How many subscribers received it */
  int ack_count;              /* How many acknowledged receipt */
} Notification;

/* Per-process allocation tracking */
typedef struct ProcAllocation {
  struct Proc *proc;
  UserCapability pages[MAX_PAGES_PER_PROCESS];
  uint num_pages;
  float syscall_rate;
  uint target_pages;
  uint syscall_count;
  uvlong last_measurement;
  struct ProcAllocation *next;
} ProcAllocation;

/* Global exchange pool structure */
typedef struct GlobalExchangePool {
  /* Fixed-size arrays (will be converted to dynamic later) */
  UserCapability pages[POOL_SIZE];
  uint free_list[POOL_SIZE];
  uint free_count;

  /* Process allocation tracking */
  ProcAllocation *proc_allocs;

  /* IPC Pub-Sub */
  Topic *topics_root;
  Notification notifications[MAX_PENDING_NOTIFICATIONS];
  uint notification_count;

  /* Locks */
  QLock pool_lock;
  QLock topics_lock;
  QLock notifications_lock;
} GlobalExchangePool;

/* Pool lifecycle */
void exchange_pool_init(uint pool_size);
void exchange_pool_shutdown(void);

/* Global page allocation (called by devexchange.c per-channel pool) */
PoolError global_pool_alloc_page(struct Proc *p, UserCapability *out);
PoolError global_pool_free_page(struct Proc *p, const UserCapability *cap);

/* Process tracking */
ProcAllocation *get_proc_allocation(struct Proc *p);
void exchange_cleanup_process(struct Proc *p);

/* Demand measurement */
void measure_process_demand(ProcAllocation *pa);
void compute_target_allocations(GlobalExchangePool *pool);

/* Global pool instance (extern for use by IPC functions) */
extern GlobalExchangePool *global_pool;

/* IPC Pub-Sub Functions */
void uuid_pack_topic(uuid_t *topic_uuid, const char *namespace_str,
                     const char *topic_name);
int uuid_verify_topic(const uuid_t *topic_uuid, const char *namespace_str,
                      const char *topic_name);
Topic *create_topic(const char *topic_name);
Topic *find_or_create_topic(const char *topic_name);
int subscribe_to_topic(struct Proc *p, const char *topic_name);
int unsubscribe_from_topic(struct Proc *p, const char *topic_name);
int notify_subscribers(Topic *topic, UserCapability *published_page);
UserCapability *publish_message(struct Proc *p, const char *topic_name,
                                void *data, ulong len);
UserCapability *publish_message_chunk_start(struct Proc *p,
                                            const char *topic_name,
                                            uuid_t *msg_id, ulong total_len);
int publish_message_chunk(uuid_t *msg_id, u16int chunk_num, void *data,
                          ulong len);
Notification *dequeue_notification(struct Proc *p);

#endif /* _EXCHANGE_POOL_H_ */
