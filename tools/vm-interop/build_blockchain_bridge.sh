#!/bin/bash
# Build Native 9P Blockchain Bridge
# This script extracts 9front's 9P client code and builds a native Linux client

set -e

echo "Building Blockchain-to-VMX 9P Bridge..."

# Create build directory
mkdir -p build_9p_bridge
cd build_9p_bridge

# Extract essential 9P client code from 9front
echo "Extracting 9front 9P client libraries..."

# Copy 9P protocol headers
cp ../9front/sys/include/fcall.h .
cp ../9front/sys/include/9p.h .

# Extract core 9P client functions
# These provide the actual 9P message handling we need
cp ../9front/sys/src/libc/9sys/read9pmsg.c .

# Look for 9P client utilities
find ../9front -name "*9p*" -name "*.c" | grep -E "(client|con)" | head -5 | while read file; do
    basename=$(basename "$file")
    echo "Extracting: $basename"
    cp "$file" "./9p_$basename"
done

echo "Creating standalone 9P client library..."

# Create a minimal 9P client implementation
cat > minimal_9p_client.c << 'EOF'
/*
 * Minimal 9P Client Implementation
 * Extracted and adapted from 9front for Linux compilation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

/* Minimal 9P types for Linux */
typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t u32int;

/* 9P message types */
enum {
    Tversion = 100,
    Rversion,
    Tauth = 102,
    Rauth,
    Tattach = 104,
    Rattach,
    Terror = 106,
    Rerror,
    Tflush = 108,
    Rflush,
    Twalk = 110,
    Rwalk,
    Topen = 112,
    Ropen,
    Tcreate = 114,
    Rcreate,
    Tread = 116,
    Rread,
    Twrite = 118,
    Rwrite,
    Tclunk = 120,
    Rclunk,
};

/* Qid structure */
typedef struct {
    uchar type;
    u32int vers;
    uint64_t path;
} Qid;

/* Basic 9P message structure */
typedef struct {
    u32int size;
    uchar type;
    ushort tag;
    /* Variable data follows */
} P9Hdr;

/* 9P Connection */
typedef struct {
    int fd;
    ushort nexttag;
    u32int nextfid;
    u32int msize;
} P9Conn;

/* Connect to 9P server */
P9Conn* p9_dial(const char* address, int port) {
    P9Conn *conn = calloc(1, sizeof(P9Conn));
    if (!conn) return NULL;

    conn->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->fd < 0) {
        free(conn);
        return NULL;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &addr.sin_addr);

    if (connect(conn->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(conn->fd);
        free(conn);
        return NULL;
    }

    conn->nexttag = 1;
    conn->nextfid = 1;
    conn->msize = 8192;

    return conn;
}

/* Send 9P message */
int p9_send(P9Conn *conn, void *msg, int size) {
    return write(conn->fd, msg, size) == size ? 0 : -1;
}

/* Receive 9P message */
int p9_recv(P9Conn *conn, void *buf, int maxsize) {
    return read(conn->fd, buf, maxsize);
}

/* Build and send Twrite message */
int p9_write_file(P9Conn *conn, u32int fid, const char *data, int len) {
    /* Build Twrite message */
    char buf[8192];
    P9Hdr *hdr = (P9Hdr*)buf;

    hdr->size = sizeof(P9Hdr) + sizeof(u32int) + sizeof(uint64_t) + sizeof(u32int) + len;
    hdr->type = Twrite;
    hdr->tag = conn->nexttag++;

    /* Add fid, offset, count, data */
    u32int *p = (u32int*)(hdr + 1);
    *p++ = fid;                    /* fid */
    *(uint64_t*)p = 0;             /* offset */
    p = (u32int*)((char*)p + 8);
    *p++ = len;                    /* count */
    memcpy(p, data, len);          /* data */

    return p9_send(conn, buf, hdr->size);
}

/* Close connection */
void p9_close(P9Conn *conn) {
    if (conn) {
        close(conn->fd);
        free(conn);
    }
}

EOF

echo "Creating blockchain integration test..."

cat > test_blockchain_bridge.c << 'EOF'
/*
 * Test: Blockchain to VMX via Native 9P
 * Direct connection to cmdexec in 9front VM
 */

#include "minimal_9p_client.c"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <vm_ip>\n", argv[0]);
        return 1;
    }

    printf("Connecting to 9front VM at %s...\n", argv[1]);

    /* Connect to cmdexec 9P server in VM */
    P9Conn *conn = p9_dial(argv[1], 564); /* Standard 9P port */
    if (!conn) {
        printf("Failed to connect to VM\n");
        return 1;
    }

    printf("Connected! Testing blockchain command execution...\n");

    /* Simulate blockchain-triggered VM commands */
    const char* blockchain_commands[] = {
        "echo 'Blockchain event: deploy VM'",
        "vmx -i /n/vminterop/alpine-virt-3.19.0-x86_64.iso -M 512M",
        "echo 'VM deployment complete'",
        NULL
    };

    for (int i = 0; blockchain_commands[i]; i++) {
        printf("Executing: %s\n", blockchain_commands[i]);

        /* TODO: Complete 9P protocol handshake and file operations */
        /* For now, this demonstrates the connection concept */

        sleep(1);
    }

    p9_close(conn);
    printf("Blockchain-VMX bridge test complete\n");
    return 0;
}
EOF

echo "Building native 9P blockchain bridge..."

# Compile the test
gcc -o test_blockchain_bridge test_blockchain_bridge.c -std=c99

echo "Built: test_blockchain_bridge"
echo ""
echo "USAGE:"
echo "  ./test_blockchain_bridge <vm_ip_address>"
echo ""
echo "This creates a native 9P client that connects directly to your"
echo "cmdexec server in the 9front VM, bypassing all buggy translation layers."
echo ""
echo "Next steps:"
echo "1. Complete the 9P protocol implementation"
echo "2. Add real blockchain monitoring"
echo "3. Implement VMX lifecycle management"
echo "4. Test full pipeline: Blockchain -> 9P -> cmdexec -> VMX"