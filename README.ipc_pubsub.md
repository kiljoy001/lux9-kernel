# Lux9 IPC Pub-Sub System

This document describes the new IPC (Inter-Process Communication) pub-sub system implemented in the Lux9 kernel using the exchange pool.

## Overview

The IPC system extends the existing exchange pool functionality to provide a publish-subscribe messaging mechanism between processes. It leverages the proven UUIDv8-based security model and integrates seamlessly with the existing kernel architecture.

## Key Features

### 1. UUIDv8-Based Security
- Uses existing process identity system (pid2)
- Topic UUIDs generated from deterministic hashing
- Capability-based access control for published messages

### 2. Pub-Sub Architecture
- **Topics**: Named channels for message distribution
- **Publishers**: Processes that send messages to topics
- **Subscribers**: Processes that receive messages from topics
- **Notifications**: Asynchronous message delivery mechanism

### 3. Memory-Efficient Design
- Uses existing 4KB page allocation system
- Single-message and multi-chunk message support
- Zero-copy message delivery where possible

## API Functions

### Kernel-Level Functions

#### Topic Management
- `create_topic(const char *topic_name)` - Create a new topic
- `find_or_create_topic(const char *topic_name)` - Find existing or create new topic
- `subscribe_to_topic(Proc *p, const char *topic_name)` - Subscribe process to topic
- `unsubscribe_from_topic(Proc *p, const char *topic_name)` - Unsubscribe process from topic

#### Message Publishing
- `publish_message(Proc *p, const char *topic_name, void *data, ulong len)` - Publish single message
- `publish_message_chunk_start(Proc *p, const char *topic_name, uuid_t *msg_id, ulong total_len)` - Start multi-chunk message
- `publish_message_chunk(uuid_t *msg_id, u16int chunk_num, void *data, ulong len)` - Publish message chunk

#### Notification System
- `notify_subscribers(Topic *topic, UserCapability *published_page)` - Notify all subscribers
- `dequeue_notification(Proc *p)` - Retrieve next notification for process

### Userspace Syscalls

#### Exchange Pool IPC Syscalls
- `EXCHANGE_PUBLISH` (69) - Publish message to topic
- `EXCHANGE_SUBSCRIBE` (70) - Subscribe to topic  
- `EXCHANGE_UNSUBSCRIBE` (71) - Unsubscribe from topic
- `EXCHANGE_RECEIVE` (72) - Receive notification

#### Liblux Wrapper Functions
```c
ExchangeCapability* sys_exchange_publish(char *topic, void *data, ulong len);
int sys_exchange_subscribe(char *topic);
int sys_exchange_unsubscribe(char *topic);
Notification* sys_exchange_receive(void);
```

## Data Structures

### Topic
```c
typedef struct Topic {
    uuid_t topic_uuid;                  // Topic UUIDv8 identifier
    char name[MAX_TOPIC_LENGTH];        // Human-readable topic name
    Subscription subscribers[MAX_SUBSCRIBERS_PER_TOPIC]; // Registered subscribers
    int subscriber_count;               // Number of active subscribers
    u32int sequence_counter;            // For generating sequence numbers
    struct Topic *left;                 // RBTree left child
    struct Topic *right;                // RBTree right child
    int red;                            // RBTree color
} Topic;
```

### Notification
```c
typedef struct Notification {
    uuid_t message_id;                  // Message UUID
    uuid_t topic_uuid;                  // Topic UUID
    ExchangeCapability *capability;     // Published page capability
    int delivered_count;                // How many subscribers received it
    int ack_count;                      // How many acknowledged receipt
} Notification;
```

## Usage Examples

### Publisher Example
```c
#include "lux.h"

int main() {
    // Publish a message to a topic
    char message[] = "Hello, World!";
    ExchangeCapability *cap = sys_exchange_publish("chat/messages", message, sizeof(message));
    if (cap != 0) {
        // Message published successfully
        return 0;
    }
    return -1; // Failed to publish
}
```

### Subscriber Example
```c
#include "lux.h"

int main() {
    // Subscribe to a topic
    if (sys_exchange_subscribe("chat/messages") < 0) {
        return -1; // Failed to subscribe
    }
    
    // Receive notifications
    while (1) {
        Notification *notif = sys_exchange_receive();
        if (notif != 0) {
            // Process the received message
            // ... handle message ...
        }
    }
    
    // Unsubscribe when done
    sys_exchange_unsubscribe("chat/messages");
    return 0;
}
```

## Implementation Details

### UUID Generation
Topics use deterministic UUIDv8 generation based on topic names:
```c
void uuid_pack_topic(uuid_t *topic_uuid, const char *namespace_str, const char *topic_name) {
    // Generate hash from topic name for consistent UUID
    u32int hash = 0;
    const char *ptr = topic_name;
    while (*ptr) {
        hash = hash * 31 + *ptr++;
    }
    // Mix in namespace
    uuid_pack_v8(topic_uuid, (uvlong)hash, (ushort)(hash >> 16), (uvlong)(hash >> 32));
}
```

### Security Model
- All communication uses existing capability-based security
- Process identities verified using pid2 system
- Message integrity protected by BLAKE2b hashes

### Performance Considerations
- O(1) topic lookup using RBTree structure
- Lock-free notification queues where possible
- Batch notification delivery for efficiency
- Memory pages reused from existing exchange pool

## Integration Points

### With Existing Systems
- Leverages existing exchange pool infrastructure
- Uses proven borrow checker for memory safety
- Integrates with existing UUIDv8 process identity system
- Compatible with existing 9P router and syscall infrastructure

### Scalability Features
- Configurable limits (MAX_SUBSCRIBERS_PER_TOPIC, etc.)
- Automatic cleanup of inactive subscriptions
- Efficient memory usage with shared page pool
- Horizontal scaling through namespace partitioning

## Future Enhancements

### Planned Features
1. **Quality of Service**: Priority-based message delivery
2. **Persistence**: Durable message storage options
3. **Flow Control**: Backpressure mechanisms for high-volume topics
4. **Encryption**: End-to-end message encryption support
5. **Routing**: Advanced message routing patterns

### Performance Improvements
1. **Batch Processing**: Multi-message atomic operations
2. **Compression**: Transparent message compression
3. **Caching**: Frequently accessed topic optimization
4. **Sharding**: Distributed topic management

This IPC system provides a robust, secure foundation for inter-process communication in Lux9 while maintaining the efficiency and security principles of the underlying kernel architecture.