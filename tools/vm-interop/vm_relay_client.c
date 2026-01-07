/*
 * vm_relay_client - VM connects to command relay on host
 * Gets commands from user via the relay server
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>

#define RELAY_HOST "10.0.2.2"  /* Host from VM's perspective */
#define RELAY_PORT "8888"      /* Command relay port */

typedef struct {
    int fd;
    Biobuf *bin;
} RelayConn;

/* Connect to relay server on host */
RelayConn*
connect_to_relay(void)
{
    RelayConn *conn;
    int fd;

    print("🔗 Connecting to command relay at %s:%s...\n", RELAY_HOST, RELAY_PORT);

    fd = dial(netmkaddr(RELAY_HOST, "tcp", RELAY_PORT), nil, nil, nil);
    if(fd < 0){
        print("❌ Failed to connect: %r\n");
        return nil;
    }

    conn = malloc(sizeof(RelayConn));
    conn->fd = fd;
    conn->bin = Bfdopen(fd, OREAD);

    print("✅ Connected to command relay!\n");
    return conn;
}

/* Execute command and return output */
char*
execute_command(char *cmd)
{
    Waitmsg *w;
    int p[2];
    char *output;
    int n, total, capacity;

    print("📦 Executing: %s\n", cmd);

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

        capacity = 8192;
        output = malloc(capacity);
        total = 0;

        while((n = read(p[0], output + total, capacity - total - 1)) > 0){
            total += n;
            if(total >= capacity - 1){
                capacity *= 2;
                if(capacity > 65536) capacity = 65536;
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

/* Parse simple JSON to extract command */
char*
extract_command(char *json)
{
    char *start, *end;

    /* Look for "cmd": "..." */
    start = strstr(json, "\"cmd\"");
    if(start == nil)
        return nil;

    start = strchr(start, ':');
    if(start == nil)
        return nil;

    /* Skip whitespace and quote */
    start++;
    while(*start == ' ' || *start == '\t' || *start == '"')
        start++;

    /* Find end quote */
    end = start;
    while(*end && *end != '"')
        end++;

    if(end > start){
        *end = '\0';
        return strdup(start);
    }

    return nil;
}

/* Main relay client loop */
void
relay_client_loop(void*)
{
    RelayConn *conn;
    char *line, *cmd, *output, *response;

    for(;;){
        /* Connect to relay */
        conn = connect_to_relay();
        if(conn == nil){
            print("Retrying in 5 seconds...\n");
            sleep(5000);
            continue;
        }

        /* Read commands from relay */
        while((line = Brdstr(conn->bin, '\n', 1)) != nil){
            print("📨 Received: %s\n", line);

            /* Check message type */
            if(strstr(line, "\"type\":\"command\"") != nil){
                /* Extract and execute command */
                cmd = extract_command(line);
                if(cmd != nil){
                    output = execute_command(cmd);

                    /* Send result back */
                    response = smprint("{\"status\":\"ok\",\"output\":\"%s\"}\n",
                                     output);
                    write(conn->fd, response, strlen(response));

                    free(response);
                    free(output);
                    free(cmd);
                }
            }
            else if(strstr(line, "\"type\":\"ping\"") != nil){
                /* Respond to ping */
                write(conn->fd, "{\"type\":\"pong\"}\n", 16);
            }

            free(line);
        }

        print("Connection lost, reconnecting...\n");
        Bterm(conn->bin);
        close(conn->fd);
        free(conn);
        sleep(2000);
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

    print("🚀 VM Relay Client Starting\n");
    print("Will connect to relay at %s:%s\n", RELAY_HOST, RELAY_PORT);
    print("Waiting for commands from host...\n\n");

    /* Start relay client thread */
    threadcreate(relay_client_loop, nil, 32*1024);

    /* Keep main thread alive */
    for(;;)
        sleep(60000);
}