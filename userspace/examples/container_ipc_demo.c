/*
 * Container IPC Demo - Two WASM containers talking via exchange pages
 *
 * Shows:
 * - Container A: Database server (exports /srv/db)
 * - Container B: Web server (mounts /srv/db, makes RPCs)
 * - Zero-copy IPC via exchange pages
 * - MSGORD ordering
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include "../lib/liblux/inc/exchange_pool.h"

/* Simplified WASM container runtime (userspace) */
typedef struct {
    char *name;              /* Container name */
    int srv_fd;              /* /srv file descriptor */
    void *page_pool;         /* Exchange page pool */
    int num_pages;           /* Pool size */
    ExchangePagePool pool;   /* Page pool view */
    void *wasm_runtime;      /* WASM3 runtime */
} Container;

/* Message format for container IPC */
typedef struct {
    uint msg_id;             /* Message ID (from MSGORD) */
    uint type;               /* Message type (RPC, response, etc.) */
    uint len;                /* Payload length */
    char data[];             /* Payload */
} ContainerMsg;

/* Database query message */
typedef struct {
    char sql[256];
    int limit;
} DbQuery;

/* Database result message */
typedef struct {
    int rows;
    int cols;
    char results[3840];      /* 4096 - header = 3840 bytes */
} DbResult;

/*
 * Container A: Database Server
 * Exports /srv/db with RPC endpoint
 */
void db_container_handler(void *msg_page) {
    ContainerMsg *msg = (ContainerMsg *)msg_page;
    DbQuery *query = (DbQuery *)msg->data;

    print("[DB] Received query: %s (limit=%d)\n", query->sql, query->limit);

    /* Process query (normally would call WASM module) */
    /* For demo, just return mock data */

    /* Reuse same exchange page for response (zero-copy!) */
    msg->type = 2; /* Response */

    DbResult *result = (DbResult *)msg->data;
    result->rows = 42;
    result->cols = 3;
    snprint(result->results, sizeof(result->results),
            "user1,alice@example.com,active\n"
            "user2,bob@example.com,active\n"
            "...(40 more rows)\n");

    msg->len = sizeof(DbResult);

    print("[DB] Sending response: %d rows\n", result->rows);
}

int db_container_main(void) {
    Container db;
    int fd;
    uintptr page_addr;

    /* Initialize container */
    db.name = "database";
    db.num_pages = 16;

    /* Allocate exchange page pool */
    db.page_pool = sbrk(db.num_pages * 4096);
    if (db.page_pool == (void *)-1) {
        fprint(2, "db: failed to allocate page pool\n");
        return -1;
    }

    print("[DB] Allocated %d exchange pages at %p\n",
          db.num_pages, db.page_pool);
    if (exchange_pool_init(&db.pool, db.page_pool,
                           db.num_pages * EXCHANGE_PAGE_SIZE) < 0) {
        fprint(2, "db: failed to initialize exchange pool\n");
        return -1;
    }

    /* Export service to /srv */
    fd = create("/srv/db", OWRITE, 0666);
    if (fd < 0) {
        fprint(2, "db: failed to create /srv/db\n");
        return -1;
    }
    db.srv_fd = fd;

    print("[DB] Exported /srv/db\n");

    /* Main loop: process incoming RPCs */
    for (;;) {
        /* Wait for RPC on exchange page */
        /* In real implementation, would use MSGORD to get next message */
        page_addr = (uintptr)exchange_pool_page(&db.pool, 0);

        /* Process message */
        db_container_handler((void *)page_addr);

        /* Return response via same page */
        /* Caller will read response from exchange page */

        sleep(1000); /* Demo: slow down for visibility */
    }

    return 0;
}

/*
 * Container B: Web Server
 * Mounts /srv/db and makes RPC calls
 */
int web_container_main(void) {
    Container web;
    int db_fd;
    uintptr send_page, recv_page;
    ExchangeCapability exchange_cap;
    ContainerMsg *msg;
    DbQuery *query;
    DbResult *result;

    /* Initialize container */
    web.name = "webserver";
    web.num_pages = 32;

    /* Allocate exchange page pool */
    web.page_pool = sbrk(web.num_pages * 4096);
    if (web.page_pool == (void *)-1) {
        fprint(2, "web: failed to allocate page pool\n");
        return -1;
    }

    print("[WEB] Allocated %d exchange pages at %p\n",
          web.num_pages, web.page_pool);
    if (exchange_pool_init(&web.pool, web.page_pool,
                           web.num_pages * EXCHANGE_PAGE_SIZE) < 0) {
        fprint(2, "web: failed to initialize exchange pool\n");
        return -1;
    }

    /* Mount database container */
    db_fd = open("/srv/db", ORDWR);
    if (db_fd < 0) {
        fprint(2, "web: failed to open /srv/db (is database running?)\n");
        return -1;
    }

    if (mount(db_fd, -1, "/n/db", MREPL, "") < 0) {
        fprint(2, "web: failed to mount /n/db\n");
        return -1;
    }

    print("[WEB] Mounted /n/db\n");

    /* Simulate HTTP request that needs database */
    print("[WEB] Simulating GET /api/users...\n");

    /* Allocate exchange page for request */
    send_page = (uintptr)exchange_pool_page(&web.pool, 0);
    msg = (ContainerMsg *)send_page;

    /* Build query message */
    msg->msg_id = 1;  /* Would come from MSGORD */
    msg->type = 1;    /* RPC request */
    msg->len = sizeof(DbQuery);

    query = (DbQuery *)msg->data;
    snprint(query->sql, sizeof(query->sql), "SELECT * FROM users");
    query->limit = 50;

    print("[WEB] Sending query via exchange page %p\n", (void *)send_page);

    /* Prepare exchange page for transfer */
    if (exchange_pool_prepare(&web.pool, 0, &exchange_cap) < 0) {
        fprint(2, "web: failed to prepare exchange page\n");
        return -1;
    }

    /* Write to /n/db/rpc (this transfers the exchange page) */
    int rpc_fd = open("/n/db/rpc", ORDWR);
    if (rpc_fd < 0) {
        fprint(2, "web: failed to open /n/db/rpc\n");
        return -1;
    }

    /* Send capability handle (not the data - zero copy!) */
    if (write(rpc_fd, &exchange_cap, sizeof(exchange_cap)) < 0) {
        fprint(2, "web: failed to send RPC\n");
        return -1;
    }

    print("[WEB] Exchange page transferred to database (zero-copy!)\n");

    /* Read response (database server reuses same page) */
    if (read(rpc_fd, &exchange_cap, sizeof(exchange_cap)) < 0) {
        fprint(2, "web: failed to receive response\n");
        return -1;
    }

    /* Accept response page back */
    recv_page = (uintptr)exchange_pool_page(&web.pool, 1);
    if (exchange_pool_accept(&web.pool, 1, &exchange_cap, 0) < 0) {
        fprint(2, "web: failed to accept response page\n");
        return -1;
    }

    print("[WEB] Response received via exchange page %p\n", (void *)recv_page);

    /* Process response */
    msg = (ContainerMsg *)recv_page;
    result = (DbResult *)msg->data;

    print("[WEB] Query returned %d rows:\n%s\n",
          result->rows, result->results);

    /* Format HTTP response (would normally call WASM module) */
    print("[WEB] HTTP 200 OK - returned %d users\n", result->rows);

    close(rpc_fd);
    close(db_fd);

    return 0;
}

/*
 * Demo launcher - runs both containers
 */
void main(int argc, char *argv[]) {
    int pid;

    print("=== Container IPC Demo ===\n");
    print("Starting database container...\n");

    /* Fork database container */
    pid = fork();
    if (pid == 0) {
        /* Child: run database */
        db_container_main();
        exits(nil);
    }

    /* Parent: wait for database to start */
    sleep(500);

    print("Starting web server container...\n");

    /* Run web server in parent */
    web_container_main();

    print("=== Demo Complete ===\n");

    /* Kill database container */
    postnote(PNPROC, pid, "kill");

    exits(nil);
}
