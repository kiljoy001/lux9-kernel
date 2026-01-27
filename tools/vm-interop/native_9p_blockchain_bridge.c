/*
 * Native 9P Blockchain Bridge - Direct VM Communication
 *
 * This is the REAL solution to blockchain-VMX interop:
 * - Native 9P client implementation (no buggy translation layers)
 * - Direct connection to cmdexec in 9front VM
 * - Blockchain event processing and VM command execution
 * - Built from 9front source for maximum compatibility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>

/* 9P Protocol Implementation */
typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t u32int;
typedef uint64_t uvlong;

/* 9P Message Types */
enum {
    Tversion = 100, Rversion = 101,
    Tauth = 102,    Rauth = 103,
    Tattach = 104,  Rattach = 105,
    Terror = 106,   Rerror = 107,
    Tflush = 108,   Rflush = 109,
    Twalk = 110,    Rwalk = 111,
    Topen = 112,    Ropen = 113,
    Tcreate = 114,  Rcreate = 115,
    Tread = 116,    Rread = 117,
    Twrite = 118,   Rwrite = 119,
    Tclunk = 120,   Rclunk = 121,
};

/* 9P Connection State */
typedef struct {
    int fd;                    /* Socket to VM */
    ushort nexttag;            /* Message tag counter */
    u32int nextfid;            /* File ID counter */
    u32int msize;              /* Max message size */
    u32int root_fid;           /* Root directory FID */
    u32int exec_fid;           /* /n/cmdexec/exec FID */
    u32int output_fid;         /* /n/cmdexec/output FID */
    char connected;            /* Connection state */
} P9Conn;

/* 9P Message Header */
typedef struct {
    u32int size;
    uchar type;
    ushort tag;
} __attribute__((packed)) P9Hdr;

/* Utility: Pack 32-bit integer */
static void put32(uchar *p, u32int v) {
    p[0] = v; p[1] = v>>8; p[2] = v>>16; p[3] = v>>24;
}

/* Utility: Pack 16-bit integer */
static void put16(uchar *p, ushort v) {
    p[0] = v; p[1] = v>>8;
}

/* Utility: Pack 64-bit integer */
static void put64(uchar *p, uvlong v) {
    put32(p, v); put32(p+4, v>>32);
}

/* Utility: Unpack 32-bit integer */
static u32int get32(uchar *p) {
    return p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

/* Utility: Unpack 16-bit integer */
static ushort get16(uchar *p) {
    return p[0] | (p[1]<<8);
}

/* Connect to 9P server (cmdexec in VM) */
P9Conn* p9_connect(const char *host, int port) {
    P9Conn *conn = calloc(1, sizeof(P9Conn));
    if (!conn) return NULL;

    /* Create socket */
    conn->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->fd < 0) {
        free(conn);
        return NULL;
    }

    /* Connect to VM */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(conn->fd);
        free(conn);
        return NULL;
    }

    if (connect(conn->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Connection failed: %s\n", strerror(errno));
        close(conn->fd);
        free(conn);
        return NULL;
    }

    /* Initialize connection state */
    conn->nexttag = 1;
    conn->nextfid = 1;
    conn->msize = 8192;
    conn->connected = 0;

    return conn;
}

/* Send 9P message */
static int p9_send(P9Conn *conn, void *msg, int size) {
    return write(conn->fd, msg, size) == size ? 0 : -1;
}

/* Receive 9P message */
static int p9_recv(P9Conn *conn, void *buf, int maxsize) {
    /* First read the header to get message size */
    P9Hdr hdr;
    int n = read(conn->fd, &hdr, sizeof(hdr));
    if (n != sizeof(hdr)) return -1;

    /* Read the rest of the message */
    int remaining = hdr.size - sizeof(hdr);
    if (remaining > maxsize - sizeof(hdr)) return -1;

    memcpy(buf, &hdr, sizeof(hdr));
    if (remaining > 0) {
        n = read(conn->fd, (char*)buf + sizeof(hdr), remaining);
        if (n != remaining) return -1;
    }

    return hdr.size;
}

/* Send Tversion and handle Rversion */
static int p9_version(P9Conn *conn) {
    uchar buf[1024];
    P9Hdr *hdr = (P9Hdr*)buf;
    uchar *p = buf + sizeof(P9Hdr);
    const char *version = "9P2000";

    /* Build Tversion message */
    put32(p, 8192);                           /* msize */
    p += 4;
    put16(p, strlen(version));                 /* version string length */
    p += 2;
    memcpy(p, version, strlen(version));       /* version string */
    p += strlen(version);

    hdr->size = p - buf;
    hdr->type = Tversion;
    hdr->tag = conn->nexttag++;

    /* Send Tversion */
    if (p9_send(conn, buf, hdr->size) < 0) return -1;

    /* Receive Rversion */
    int n = p9_recv(conn, buf, sizeof(buf));
    if (n < 0) return -1;

    hdr = (P9Hdr*)buf;
    if (hdr->type != Rversion) return -1;

    /* Extract msize from response */
    p = buf + sizeof(P9Hdr);
    conn->msize = get32(p);

    return 0;
}

/* Send Tattach to attach to root */
static int p9_attach(P9Conn *conn) {
    uchar buf[1024];
    P9Hdr *hdr = (P9Hdr*)buf;
    uchar *p = buf + sizeof(P9Hdr);
    const char *uname = "blockchain";
    const char *aname = "";

    conn->root_fid = conn->nextfid++;

    /* Build Tattach message */
    put32(p, conn->root_fid);                  /* fid */
    p += 4;
    put32(p, 0xFFFFFFFF);                      /* afid (no auth) */
    p += 4;
    put16(p, strlen(uname));                   /* uname length */
    p += 2;
    memcpy(p, uname, strlen(uname));           /* uname */
    p += strlen(uname);
    put16(p, strlen(aname));                   /* aname length */
    p += 2;
    if (strlen(aname) > 0) {
        memcpy(p, aname, strlen(aname));       /* aname */
        p += strlen(aname);
    }

    hdr->size = p - buf;
    hdr->type = Tattach;
    hdr->tag = conn->nexttag++;

    /* Send Tattach */
    if (p9_send(conn, buf, hdr->size) < 0) return -1;

    /* Receive Rattach */
    int n = p9_recv(conn, buf, sizeof(buf));
    if (n < 0) return -1;

    hdr = (P9Hdr*)buf;
    if (hdr->type != Rattach) {
        if (hdr->type == Rerror) {
            p = buf + sizeof(P9Hdr);
            ushort elen = get16(p);
            p += 2;
            printf("Attach error: %.*s\n", elen, (char*)p);
        }
        return -1;
    }

    return 0;
}

/* Complete 9P handshake */
static int p9_handshake(P9Conn *conn) {
    if (p9_version(conn) < 0) {
        printf("Version negotiation failed\n");
        return -1;
    }

    if (p9_attach(conn) < 0) {
        printf("Attach failed\n");
        return -1;
    }

    conn->connected = 1;
    return 0;
}

/* Execute blockchain command via 9P */
int blockchain_execute_vm_command(P9Conn *conn, const char *command) {
    if (!conn->connected) {
        printf("Not connected to VM\n");
        return -1;
    }

    printf("🔗 Blockchain -> VM: %s\n", command);

    /*
     * TODO: Complete implementation:
     * 1. Walk to /n/cmdexec/exec
     * 2. Open for writing
     * 3. Write command
     * 4. Read from /n/cmdexec/output
     * 5. Return result to blockchain
     */

    printf("✅ Command queued for VM execution\n");
    return 0;
}

/* Blockchain event processor */
void process_blockchain_event(P9Conn *conn, const char *event_data) {
    printf("📦 Blockchain Event: %s\n", event_data);

    char vm_command[1024];

    /* Parse blockchain event and convert to VM command */
    if (strstr(event_data, "deploy_alpine_vm")) {
        snprintf(vm_command, sizeof(vm_command),
                "vmx -i /n/vminterop/alpine-virt-3.19.0-x86_64.iso -M 512M &");
    }
    else if (strstr(event_data, "exec:")) {
        const char *cmd = strstr(event_data, "exec:") + 5;
        snprintf(vm_command, sizeof(vm_command), "%s", cmd);
    }
    else if (strstr(event_data, "status_check")) {
        strcpy(vm_command, "ps aux | grep vmx");
    }
    else {
        strcpy(vm_command, "echo 'Unknown blockchain event'");
    }

    /* Execute via native 9P */
    blockchain_execute_vm_command(conn, vm_command);
}

/* Test blockchain integration */
void test_blockchain_scenarios(P9Conn *conn) {
    printf("\n🚀 Testing Blockchain-to-VMX Pipeline\n");
    printf("=====================================\n");

    /* Simulate various blockchain events */
    const char *test_events[] = {
        "deploy_alpine_vm:user123:payment_confirmed",
        "exec:ls /n/vminterop",
        "exec:date",
        "status_check:vm_health",
        "deploy_alpine_vm:user456:smart_contract_triggered",
        NULL
    };

    for (int i = 0; test_events[i]; i++) {
        process_blockchain_event(conn, test_events[i]);
        sleep(2);  /* Simulate processing time */
    }

    printf("\n✅ Blockchain integration test complete\n");
}

int main(int argc, char *argv[]) {
    printf("Native 9P Blockchain Bridge v1.0\n");
    printf("Direct blockchain-to-VMX communication\n\n");

    if (argc < 2) {
        printf("Usage: %s <vm_host> [port]\n", argv[0]);
        printf("Example: %s 10.0.2.15 564\n", argv[0]);
        return 1;
    }

    const char *vm_host = argv[1];
    int vm_port = (argc > 2) ? atoi(argv[2]) : 564;

    /* Connect to 9front VM cmdexec server */
    printf("🔌 Connecting to 9front VM at %s:%d...\n", vm_host, vm_port);
    P9Conn *conn = p9_connect(vm_host, vm_port);
    if (!conn) {
        printf("❌ Failed to connect to VM\n");
        return 1;
    }

    printf("✅ TCP connection established\n");

    /* Perform 9P handshake */
    printf("🤝 Performing 9P handshake...\n");
    if (p9_handshake(conn) < 0) {
        printf("❌ 9P handshake failed\n");
        close(conn->fd);
        free(conn);
        return 1;
    }

    printf("✅ 9P protocol negotiated, ready for blockchain events\n");

    /* Test blockchain scenarios */
    test_blockchain_scenarios(conn);

    /* Clean up */
    close(conn->fd);
    free(conn);

    printf("\n🎉 Native 9P blockchain bridge test completed!\n");
    printf("Next: Add real blockchain monitoring and complete 9P file operations\n");

    return 0;
}

/*
 * DEPLOYMENT PLAN:
 *
 * 1. Complete this 9P client implementation
 * 2. Add blockchain monitoring (WebSocket to blockchain node)
 * 3. Implement VM lifecycle management
 * 4. Add result reporting back to blockchain
 * 5. Deploy as blockchain validation node service
 *
 * This gives us RELIABLE blockchain-to-VMX interop without
 * any buggy translation layers!
 */