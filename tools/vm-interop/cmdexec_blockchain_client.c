/*
 * cmdexec_blockchain_client - VM connects OUT to blockchain
 * Since the VM is behind NAT, it must initiate the connection
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <ip.h>
#include <json.h>

#define BLOCKCHAIN_HOST "10.0.2.2"  /* Host from VM's perspective */
#define BLOCKCHAIN_PORT "9333"       /* DEADBEEF blockchain port */

typedef struct {
    int fd;
    char *blockchain_addr;
} BlockchainConn;

/* Connect to blockchain node (VM initiates connection) */
BlockchainConn*
connect_to_blockchain(void)
{
    BlockchainConn *conn;
    int fd;

    print("🔗 Connecting to blockchain at %s:%s...\n", BLOCKCHAIN_HOST, BLOCKCHAIN_PORT);

    fd = dial(netmkaddr(BLOCKCHAIN_HOST, "tcp", BLOCKCHAIN_PORT), nil, nil, nil);
    if(fd < 0){
        print("❌ Failed to connect to blockchain: %r\n");
        return nil;
    }

    conn = malloc(sizeof(BlockchainConn));
    conn->fd = fd;
    conn->blockchain_addr = smprint("%s:%s", BLOCKCHAIN_HOST, BLOCKCHAIN_PORT);

    print("✅ Connected to blockchain!\n");
    return conn;
}

/* Send VM status to blockchain */
void
send_vm_status(BlockchainConn *conn, char *status)
{
    char *msg;

    msg = smprint("{\"type\":\"vm_status\",\"status\":\"%s\",\"timestamp\":%ld}\n",
                  status, time(0));

    if(write(conn->fd, msg, strlen(msg)) < 0){
        print("Failed to send status: %r\n");
    }

    free(msg);
}

/* Poll blockchain for commands */
char*
poll_blockchain_commands(BlockchainConn *conn)
{
    char buf[8192];
    int n;

    /* Send poll request */
    char *poll = "{\"type\":\"poll_commands\",\"vm_id\":\"9front-vm-001\"}\n";
    if(write(conn->fd, poll, strlen(poll)) < 0){
        return nil;
    }

    /* Read response */
    n = read(conn->fd, buf, sizeof(buf)-1);
    if(n > 0){
        buf[n] = '\0';
        return strdup(buf);
    }

    return nil;
}

/* Execute command from blockchain */
char*
execute_blockchain_command(char *cmd)
{
    Waitmsg *w;
    int p[2];
    char *output;
    int n, total, capacity;

    print("📦 Executing blockchain command: %s\n", cmd);

    if(pipe(p) < 0)
        return strdup("pipe failed");

    switch(rfork(RFPROC|RFFDG|RFREND|RFNOTEG)){
    case -1:
        close(p[0]);
        close(p[1]);
        return strdup("fork failed");

    case 0:  /* child */
        close(p[0]);
        dup(p[1], 1);
        dup(p[1], 2);
        close(p[1]);

        execl("/bin/rc", "rc", "-c", cmd, nil);
        exits("exec failed");

    default:  /* parent */
        close(p[1]);

        capacity = 4096;
        output = malloc(capacity);
        total = 0;

        while((n = read(p[0], output + total, capacity - total - 1)) > 0){
            total += n;
            if(total >= capacity - 1){
                capacity *= 2;
                output = realloc(output, capacity);
            }
        }
        output[total] = '\0';
        close(p[0]);

        w = wait();
        if(w != nil)
            free(w);

        return output;
    }
}

/* Main blockchain client loop */
void
blockchain_client_loop(void*)
{
    BlockchainConn *conn;
    char *cmd, *output, *response;

    for(;;){
        /* Connect to blockchain */
        conn = connect_to_blockchain();
        if(conn == nil){
            print("Retrying in 10 seconds...\n");
            sleep(10000);
            continue;
        }

        /* Send initial handshake */
        send_vm_status(conn, "online");

        /* Poll for commands */
        while(conn->fd >= 0){
            cmd = poll_blockchain_commands(conn);
            if(cmd != nil){
                /* Parse JSON to extract command */
                /* TODO: Proper JSON parsing */
                if(strstr(cmd, "\"command\":\"") != nil){
                    char *start = strstr(cmd, "\"command\":\"") + 11;
                    char *end = strchr(start, '"');
                    if(end != nil){
                        *end = '\0';

                        /* Execute command */
                        output = execute_blockchain_command(start);

                        /* Send result back */
                        response = smprint("{\"type\":\"command_result\",\"output\":\"%s\"}\n",
                                         output);
                        write(conn->fd, response, strlen(response));

                        free(response);
                        free(output);
                    }
                }
                free(cmd);
            }

            sleep(5000);  /* Poll every 5 seconds */
        }

        print("Connection lost, reconnecting...\n");
        close(conn->fd);
        free(conn);
    }
}

void
threadmain(int argc, char *argv[])
{
    ARGBEGIN{
    default:
        fprint(2, "usage: %s\n", argv0);
        threadexits("usage");
    }ARGEND

    print("🚀 VM Blockchain Client Starting\n");
    print("Will connect to blockchain at %s:%s\n", BLOCKCHAIN_HOST, BLOCKCHAIN_PORT);

    /* Start blockchain client thread */
    threadcreate(blockchain_client_loop, nil, 32*1024);

    /* Keep main thread alive */
    for(;;)
        sleep(60000);
}