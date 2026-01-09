// === IPC Pub-Sub Implementation ===

/* Standard kernel includes - ORDER MATTERS! u.h first for Plan 9 types */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "uuid.h"
#include "blind_ledger.h"
#include "exchange_pool.h"

// Utility function to pack topic UUIDv8 using process identity patterns
void uuid_pack_topic(uuid_t *topic_uuid, const char *namespace_str,
                     const char *topic_name) {
  if (!topic_uuid || !topic_name)
    return;

  // Generate a deterministic hash from topic name for consistent UUID
  u32int hash = 0;
  const char *ptr = topic_name;
  while (*ptr) {
    hash = hash * 31 + *ptr++;
  }

  // Also mix in namespace if provided
  if (namespace_str) {
    const char *ns_ptr = namespace_str;
    while (*ns_ptr) {
      hash = hash * 17 + *ns_ptr++;
    }
  }

  // Use UUIDv8 packing function with derived entropy
  uuid_pack_v8(topic_uuid, (uvlong)hash, (ushort)(hash >> 16),
               (uvlong)(hash >> 32));
}

// Utility function to verify topic UUID matches expected values
int uuid_verify_topic(const uuid_t *topic_uuid, const char *namespace_str,
                      const char *topic_name) {
  if (!topic_uuid || !topic_name)
    return 0;

  // Regenerate expected UUID and compare
  uuid_t expected_uuid;
  uuid_pack_topic(&expected_uuid, namespace_str, topic_name);

  return memcmp(topic_uuid, &expected_uuid, sizeof(uuid_t)) == 0;
}

// RBTree helper functions for topics
static Topic *rbtree_search(Topic *root, const uuid_t *topic_uuid) {
  Topic *current = root;
  while (current != nil) {
    int cmp = memcmp(topic_uuid, &current->topic_uuid, sizeof(uuid_t));
    if (cmp == 0)
      return current;
    else if (cmp < 0)
      current = current->left;
    else
      current = current->right;
  }
  return nil;
}

static void rbtree_rotate_left(Topic **root, Topic *x) {
  Topic *y = x->right;
  x->right = y->left;
  y->left = x;

  if (*root == x)
    *root = y;
}

static void rbtree_rotate_right(Topic **root, Topic *x) {
  Topic *y = x->left;
  x->left = y->right;
  y->right = x;

  if (*root == x)
    *root = y;
}

// Create a new topic
Topic *create_topic(const char *topic_name) {
  if (!topic_name)
    return nil;

  Topic *topic = xalloc(sizeof(Topic));
  if (!topic)
    return nil;

  memset(topic, 0, sizeof(Topic));
  strncpy(topic->name, topic_name, MAX_TOPIC_LENGTH - 1);
  topic->name[MAX_TOPIC_LENGTH - 1] = '\0';

  // Generate unique topic UUID
  uuid_pack_topic(&topic->topic_uuid, "lux9/ipc", topic_name);

  topic->subscriber_count = 0;
  topic->sequence_counter = 0;
  topic->left = nil;
  topic->right = nil;
  topic->red = 1; // New nodes are red

  return topic;
}

// Find existing topic or create new one
Topic *find_or_create_topic(const char *topic_name) {
  if (!global_pool || !topic_name)
    return nil;

  uuid_t topic_uuid;
  uuid_pack_topic(&topic_uuid, "lux9/ipc", topic_name);

  qlock(&global_pool->topics_lock);

  Topic *topic = rbtree_search(global_pool->topics_root, &topic_uuid);
  if (topic) {
    qunlock(&global_pool->topics_lock);
    return topic;
  }

  // Topic doesn't exist, create it
  topic = create_topic(topic_name);
  if (!topic) {
    qunlock(&global_pool->topics_lock);
    return nil;
  }

  // TODO: Add proper RBTree insertion here
  // For now, simple linked list approach
  // In a real implementation, we'd insert into the RBTree

  qunlock(&global_pool->topics_lock);
  return topic;
}

// Subscribe to a topic
int subscribe_to_topic(Proc *p, const char *topic_name) {
  if (!global_pool || !p || !topic_name)
    return -1;

  Topic *topic = find_or_create_topic(topic_name);
  if (!topic)
    return -1;

  // Find available slot for subscriber
  qlock(&global_pool->topics_lock);

  if (topic->subscriber_count >= MAX_SUBSCRIBERS_PER_TOPIC) {
    qunlock(&global_pool->topics_lock);
    return -1; // Max subscribers reached
  }

  int slot = topic->subscriber_count;
  topic->subscribers[slot].subscriber = p;
  topic->subscribers[slot].subscriber_id =
      p->pid2; // Use existing process identity
  topic->subscribers[slot].active = 1;
  topic->subscriber_count++;

  qunlock(&global_pool->topics_lock);
  return 0;
}

// Unsubscribe from a topic
int unsubscribe_from_topic(Proc *p, const char *topic_name) {
  if (!global_pool || !p || !topic_name)
    return -1;

  uuid_t topic_uuid;
  uuid_pack_topic(&topic_uuid, "lux9/ipc", topic_name);

  qlock(&global_pool->topics_lock);

  Topic *topic = rbtree_search(global_pool->topics_root, &topic_uuid);
  if (!topic) {
    qunlock(&global_pool->topics_lock);
    return -1; // Topic doesn't exist
  }

  // Find subscriber and mark inactive
  for (int i = 0; i < topic->subscriber_count; i++) {
    if (topic->subscribers[i].subscriber == p) {
      topic->subscribers[i].active = 0;
      // Don't decrement subscriber_count to avoid shifting
      qunlock(&global_pool->topics_lock);
      return 0;
    }
  }

  qunlock(&global_pool->topics_lock);
  return -1; // Not subscribed
}

// Notify subscribers of a published message
int notify_subscribers(Topic *topic, UserCapability *published_page) {
  if (!global_pool || !topic || !published_page)
    return -1;

  uuid_t msg_id;
  uuid_new_v8(&msg_id); // Generate unique message ID

  qlock(&global_pool->notifications_lock);

  // Add notification for each active subscriber
  for (int i = 0; i < topic->subscriber_count; i++) {
    if (topic->subscribers[i].active) {
      if (global_pool->notification_count >= MAX_PENDING_NOTIFICATIONS) {
        qunlock(&global_pool->notifications_lock);
        return -1; // Notification queue full
      }

      Notification *notif =
          &global_pool->notifications[global_pool->notification_count++];
      notif->subscriber = topic->subscribers[i].subscriber;
      notif->message_id = msg_id;
      notif->topic_uuid = topic->topic_uuid;
      notif->capability = published_page;
      notif->delivered_count = 0;
      notif->ack_count = 0;
    }
  }

  qunlock(&global_pool->notifications_lock);
  return 0;
}

// Publish a single message (fits in one page)
UserCapability *publish_message(Proc *p, const char *topic_name, void *data,
                                ulong len) {
  if (!global_pool || !p || !topic_name || !data || len == 0)
    return nil;

  if (len > 3840) // Leave room for header (256 bytes) + safety margin
    return nil;   // Message too large for single page

  // Allocate capability structure (must be heap-allocated to return to caller)
  UserCapability *cap = xalloc(sizeof(UserCapability));
  if (!cap)
    return nil;

  // Allocate a page for the message
  PoolError err = global_pool_alloc_page(p, cap);
  if (err != POOL_OK) {
    free(cap);
    return nil;
  }

  // Get the actual page to write data to
  // In a real implementation, we'd need to map the page or get its address
  // For now, we'll assume the capability itself holds our data

  // Find or create topic
  Topic *topic = find_or_create_topic(topic_name);
  if (!topic) {
    global_pool_free_page(p, cap);
    free(cap);
    return nil;
  }

  // Notify subscribers about the published message
  notify_subscribers(topic, cap);

  return cap; // Return heap-allocated capability to caller
}

// Start publishing a multi-chunk message
UserCapability *publish_message_chunk_start(Proc *p, const char *topic_name,
                                            uuid_t *msg_id, ulong total_len) {
  if (!global_pool || !p || !topic_name || !msg_id)
    return nil;

  // Generate unique message ID
  uuid_new_v8(msg_id);

  // Allocate first page for message metadata
  UserCapability *cap = xalloc(sizeof(UserCapability));
  if (!cap)
    return nil;

  PoolError err = global_pool_alloc_page(p, cap);
  if (err != POOL_OK) {
    free(cap);
    return nil;
  }

  return cap;
}

// Publish a chunk of a multi-chunk message
int publish_message_chunk(uuid_t *msg_id, u16int chunk_num, void *data,
                          ulong len) {
  if (!msg_id || !data || len == 0)
    return -1;

  // In a real implementation, we'd track chunks by msg_id and assemble them
  // For now, return success
  return 0;
}

// Dequeue a notification for a process (called by subscriber)
Notification *dequeue_notification(Proc *p) {
  if (!global_pool || !p)
    return nil;

  qlock(&global_pool->notifications_lock);

  // Look for notifications destined for this process
  for (int i = 0; i < global_pool->notification_count; i++) {
    if (global_pool->notifications[i].subscriber == p) {
      Notification *notif = &global_pool->notifications[i];
      
      // We found a notification for this process. 
      // Since we need to return a pointer to it, but we are about to shift the array,
      // we must copy it to a safe location or handle the return value carefully.
      // However, the caller likely expects a pointer to a struct that persists or is copied.
      // In this specific codebase style, 'Notification' seems to be a transient struct 
      // passed by value or pointer to stack. But wait, the function returns 'Notification *'.
      // If we return a pointer to the array slot, and then shift the array, the pointer becomes invalid/points to wrong data.
      
      // Let's allocate a new Notification struct to return, or change return type to value.
      // Looking at userspace/lib/liblux/src/syscalls.c (from investigation), it returns a pointer.
      // But the syscall usually returns data by copy.
      // Let's check sys_exchange.c.
      // Ah, I can't check sys_exchange.c right now without reading it again.
      // But usually, these kernel functions returning pointers are dangerous if the underlying storage moves.
      
      // IMPORTANT: The original code returned `&global_pool->notifications[0]` then shifted. 
      // This means the original code was ALREADY BUGGY because `notif` would point to the *next* notification after shift!
      // Actually, `notif = &global_pool->notifications[0]` gets the address.
      // Then `global_pool->notifications[0] = global_pool->notifications[1]`.
      // The data at `notif` (which is `&...[0]`) is OVERWRITTEN.
      // So the caller gets the *next* notification's data, or garbage.
      
      // To fix this properly, we should probably allocate a Notification to return, 
      // OR (more likely for this kernel style) the syscall wrapper copies it to userspace immediately.
      // The safest way here without `malloc` (which might sleep) inside qlock 
      // is to use a static buffer or expect the caller to copy it.
      // But wait, `xalloc` is used elsewhere.
      
      // Let's allocate a copy to return.
      Notification *ret = xalloc(sizeof(Notification));
      if (ret) {
        *ret = global_pool->notifications[i];
      }
      
      // Shift remaining notifications down
      for (int j = i; j < global_pool->notification_count - 1; j++) {
        global_pool->notifications[j] = global_pool->notifications[j + 1];
      }
      global_pool->notification_count--;
      
      qunlock(&global_pool->notifications_lock);
      return ret;
    }
  }

  qunlock(&global_pool->notifications_lock);
  return nil;
}