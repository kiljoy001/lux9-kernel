/*
 * Simple IPC Pub-Sub Test Program
 * Demonstrates the new exchange pool IPC functionality
 */

#include "inc/lux.h"

int main() {
    // Test 1: Subscribe to a topic
    int result = sys_exchange_subscribe("test/topic");
    if (result < 0) {
        return 1; // Subscription failed
    }
    
    // Test 2: Publish a message
    char message[] = "Hello, IPC World!";
    ExchangeCapability *cap = sys_exchange_publish("test/topic", message, sizeof(message));
    if (cap == 0) {
        return 2; // Publish failed
    }
    
    // Test 3: Receive a notification
    Notification *notif = sys_exchange_receive();
    if (notif == 0) {
        return 3; // No notification received
    }
    
    // Test 4: Unsubscribe from topic
    result = sys_exchange_unsubscribe("test/topic");
    if (result < 0) {
        return 4; // Unsubscription failed
    }
    
    return 0; // Success
}