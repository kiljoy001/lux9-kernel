/*
 * Resurrection Server - Layer 2 Service Monitor
 *
 * Monitors and restarts crashed Layer 2 services:
 * - WASM server (manages /wasm/)
 * - HAL server (manages /dev/hal/)
 * - File server (manages /)
 * - Capability server (manages /srv/cap/)
 *
 * Runs as native C at Layer 2 (userspace, not kernel).
 * Started by init as the first Layer 2 service.
 *
 * Architecture:
 * - Registers critical services from config
 * - Starts all services as child processes
 * - Monitors via waitpid() for crashes
 * - Auto-restarts crashed services
 * - Enforces restart limits (max 5 restarts per minute)
 * - Provides 9P control interface at /srv/resurrection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_SERVICES 32
#define MAX_RESTARTS_PER_MINUTE 5
#define RESTART_WINDOW_SECONDS 60

/* Service state machine */
typedef enum {
    SRV_STOPPED,    /* Service not running */
    SRV_STARTING,   /* Service starting up */
    SRV_RUNNING,    /* Service healthy */
    SRV_CRASHED,    /* Service crashed, will restart */
    SRV_FAILED      /* Too many restarts, giving up */
} ServiceState;

/* Tracked service */
typedef struct Service {
    char name[64];              /* "wasm_server", "hal_server" */
    char exec_path[256];        /* "/boot/wasm_server" */
    pid_t pid;                  /* Current process ID (0 = not running) */

    /* Restart tracking */
    int restarts;               /* Total restart count */
    time_t last_restart;        /* Last restart timestamp */
    int restarts_in_window;     /* Restarts in last minute */

    /* State */
    ServiceState state;

    /* Configuration */
    int auto_restart;           /* Auto-restart on crash? */
    int critical;               /* Critical service (log more) */
} Service;

/* Global state */
typedef struct {
    Service *services[MAX_SERVICES];
    int num_services;
    int running;                /* Server running flag */
} ResurrectionServer;

static ResurrectionServer server;

/* Forward declarations */
static int register_service(const char *name, const char *exec_path, int critical);
static int unregister_service(const char *name);
static Service *find_service(const char *name);
static void start_service(Service *svc);
static void stop_service(Service *svc);
static void restart_service(Service *svc);
static void monitor_services(void);
static void process_control_command(const char *cmd);
static void cleanup_and_exit(int sig);

/* Find a service by name */
static Service *find_service(const char *name) {
    for (int i = 0; i < server.num_services; i++) {
        if (strcmp(server.services[i]->name, name) == 0) {
            return server.services[i];
        }
    }
    return NULL;
}

/* Register a new service for monitoring */
static int register_service(const char *name, const char *exec_path, int critical) {
    if (server.num_services >= MAX_SERVICES) {
        fprintf(stderr, "RESURRECTION: Max services reached, can't register %s\n", name);
        return -1;
    }

    /* Check if service already exists */
    if (find_service(name)) {
        fprintf(stderr, "RESURRECTION: Service %s already registered\n", name);
        return -1;
    }

    Service *svc = malloc(sizeof(Service));
    if (!svc) {
        fprintf(stderr, "RESURRECTION: Failed to allocate service %s\n", name);
        return -1;
    }

    memset(svc, 0, sizeof(Service));
    strncpy(svc->name, name, sizeof(svc->name) - 1);
    strncpy(svc->exec_path, exec_path, sizeof(svc->exec_path) - 1);
    svc->state = SRV_STOPPED;
    svc->auto_restart = 1;  /* Auto-restart enabled by default */
    svc->critical = critical;

    server.services[server.num_services++] = svc;

    printf("RESURRECTION: Registered service: %s (%s)%s\n",
           name, exec_path, critical ? " [CRITICAL]" : "");

    return 0;
}

/* Unregister a service (must be stopped first) */
static int unregister_service(const char *name) {
    for (int i = 0; i < server.num_services; i++) {
        Service *svc = server.services[i];

        if (strcmp(svc->name, name) == 0) {
            /* Cannot unregister running service */
            if (svc->state == SRV_RUNNING || svc->state == SRV_STARTING) {
                fprintf(stderr, "RESURRECTION: Cannot unregister running service %s (stop it first)\n", name);
                return -1;
            }

            printf("RESURRECTION: Unregistering service: %s\n", name);

            /* Free service structure */
            free(svc);

            /* Shift remaining services */
            for (int j = i; j < server.num_services - 1; j++) {
                server.services[j] = server.services[j + 1];
            }
            server.num_services--;

            return 0;
        }
    }

    fprintf(stderr, "RESURRECTION: Service %s not found\n", name);
    return -1;
}

/* Start a service */
static void start_service(Service *svc) {
    if (svc->state == SRV_RUNNING) {
        fprintf(stderr, "RESURRECTION: %s already running (pid %d)\n",
                svc->name, svc->pid);
        return;
    }

    printf("RESURRECTION: Starting %s...\n", svc->name);
    svc->state = SRV_STARTING;

    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "RESURRECTION: Failed to fork for %s\n", svc->name);
        svc->state = SRV_FAILED;
        return;
    }

    if (pid == 0) {
        /* Child process - exec the service */
        char *argv[] = {svc->exec_path, NULL};
        char *envp[] = {NULL};

        execve(svc->exec_path, argv, envp);

        /* If we get here, execve failed */
        fprintf(stderr, "RESURRECTION: Failed to exec %s: %m\n", svc->exec_path);
        exit(1);
    }

    /* Parent process */
    svc->pid = pid;
    svc->state = SRV_RUNNING;

    printf("RESURRECTION: Started %s (pid %d)\n", svc->name, pid);
}

/* Restart a crashed service */
static void restart_service(Service *svc) {
    time_t now = time(NULL);

    /* Check restart window */
    if (now - svc->last_restart < RESTART_WINDOW_SECONDS) {
        svc->restarts_in_window++;
    } else {
        /* Window expired, reset counter */
        svc->restarts_in_window = 1;
        svc->last_restart = now;
    }

    /* Check restart limit */
    if (svc->restarts_in_window > MAX_RESTARTS_PER_MINUTE) {
        fprintf(stderr, "RESURRECTION: %s exceeded restart limit (%d restarts in %d seconds)\n",
                svc->name, svc->restarts_in_window, RESTART_WINDOW_SECONDS);
        fprintf(stderr, "RESURRECTION: Marking %s as FAILED (not restarting)\n", svc->name);
        svc->state = SRV_FAILED;
        return;
    }

    svc->restarts++;
    svc->last_restart = now;

    printf("RESURRECTION: Restarting %s (restart #%d)\n",
           svc->name, svc->restarts);

    start_service(svc);
}

/* Stop a running service */
static void stop_service(Service *svc) {
    if (svc->state != SRV_RUNNING && svc->state != SRV_STARTING) {
        printf("RESURRECTION: %s not running (state=%d)\n", svc->name, svc->state);
        return;
    }

    if (svc->pid == 0) {
        fprintf(stderr, "RESURRECTION: %s has no pid, marking as stopped\n", svc->name);
        svc->state = SRV_STOPPED;
        return;
    }

    printf("RESURRECTION: Stopping %s (pid %d)...\n", svc->name, svc->pid);

    /* Send SIGTERM for graceful shutdown */
    if (kill(svc->pid, SIGTERM) < 0) {
        fprintf(stderr, "RESURRECTION: Failed to send SIGTERM to %s: %m\n", svc->name);
        svc->state = SRV_STOPPED;
        svc->pid = 0;
        return;
    }

    /* Wait up to 2 seconds for graceful exit */
    for (int i = 0; i < 20; i++) {
        int status;
        pid_t result = waitpid(svc->pid, &status, WNOHANG);

        if (result == svc->pid) {
            /* Process exited */
            printf("RESURRECTION: %s stopped gracefully\n", svc->name);
            svc->state = SRV_STOPPED;
            svc->pid = 0;
            return;
        }

        usleep(100000);  /* 100ms */
    }

    /* Still running after 2 seconds, force kill */
    fprintf(stderr, "RESURRECTION: %s did not stop gracefully, sending SIGKILL\n", svc->name);
    kill(svc->pid, SIGKILL);

    /* Wait for forced exit */
    waitpid(svc->pid, NULL, 0);

    printf("RESURRECTION: %s force killed\n", svc->name);
    svc->state = SRV_STOPPED;
    svc->pid = 0;
}

/* Process control commands */
static void process_control_command(const char *cmd) __attribute__((unused));
static void process_control_command(const char *cmd) {
    char command[64];
    char arg1[256];
    char arg2[256];
    int critical = 0;

    /* Parse command */
    int nargs = sscanf(cmd, "%63s %255s %255s %d", command, arg1, arg2, &critical);

    if (nargs < 1) {
        fprintf(stderr, "RESURRECTION: Empty command\n");
        return;
    }

    /* Handle commands */
    if (strcmp(command, "register") == 0) {
        if (nargs < 3) {
            fprintf(stderr, "RESURRECTION: Usage: register <name> <path> [critical]\n");
            return;
        }

        int result = register_service(arg1, arg2, critical);
        if (result == 0) {
            printf("RESURRECTION: Successfully registered %s\n", arg1);
        } else {
            fprintf(stderr, "RESURRECTION: Failed to register %s\n", arg1);
        }
    }
    else if (strcmp(command, "unregister") == 0) {
        if (nargs < 2) {
            fprintf(stderr, "RESURRECTION: Usage: unregister <name>\n");
            return;
        }

        int result = unregister_service(arg1);
        if (result == 0) {
            printf("RESURRECTION: Successfully unregistered %s\n", arg1);
        } else {
            fprintf(stderr, "RESURRECTION: Failed to unregister %s\n", arg1);
        }
    }
    else if (strcmp(command, "start") == 0) {
        if (nargs < 2) {
            fprintf(stderr, "RESURRECTION: Usage: start <name>\n");
            return;
        }

        Service *svc = find_service(arg1);
        if (!svc) {
            fprintf(stderr, "RESURRECTION: Service %s not found\n", arg1);
            return;
        }

        start_service(svc);
    }
    else if (strcmp(command, "stop") == 0) {
        if (nargs < 2) {
            fprintf(stderr, "RESURRECTION: Usage: stop <name>\n");
            return;
        }

        Service *svc = find_service(arg1);
        if (!svc) {
            fprintf(stderr, "RESURRECTION: Service %s not found\n", arg1);
            return;
        }

        stop_service(svc);
    }
    else if (strcmp(command, "restart") == 0) {
        if (nargs < 2) {
            fprintf(stderr, "RESURRECTION: Usage: restart <name>\n");
            return;
        }

        Service *svc = find_service(arg1);
        if (!svc) {
            fprintf(stderr, "RESURRECTION: Service %s not found\n", arg1);
            return;
        }

        stop_service(svc);
        usleep(500000);  /* Wait 500ms between stop and start */
        start_service(svc);
    }
    else if (strcmp(command, "status") == 0) {
        /* Status for specific service or all services */
        if (nargs >= 2) {
            Service *svc = find_service(arg1);
            if (!svc) {
                fprintf(stderr, "RESURRECTION: Service %s not found\n", arg1);
                return;
            }

            const char *state_str[] = {
                "STOPPED", "STARTING", "RUNNING", "CRASHED", "FAILED"
            };

            printf("Service: %s\n", svc->name);
            printf("  Path: %s\n", svc->exec_path);
            printf("  State: %s\n", state_str[svc->state]);
            printf("  PID: %d\n", svc->pid);
            printf("  Restarts: %d (total), %d (in window)\n",
                   svc->restarts, svc->restarts_in_window);
            printf("  Auto-restart: %s\n", svc->auto_restart ? "enabled" : "disabled");
            printf("  Critical: %s\n", svc->critical ? "yes" : "no");
        } else {
            /* Show all services */
            printf("RESURRECTION: Registered services (%d/%d):\n",
                   server.num_services, MAX_SERVICES);

            for (int i = 0; i < server.num_services; i++) {
                Service *svc = server.services[i];
                const char *state_str[] = {
                    "STOPPED", "STARTING", "RUNNING", "CRASHED", "FAILED"
                };

                printf("  [%d] %s (%s) - %s%s\n",
                       i, svc->name, state_str[svc->state],
                       svc->critical ? "CRITICAL " : "",
                       svc->pid > 0 ? "" : "(no pid)");
            }
        }
    }
    else {
        fprintf(stderr, "RESURRECTION: Unknown command: %s\n", command);
        fprintf(stderr, "RESURRECTION: Available commands:\n");
        fprintf(stderr, "  register <name> <path> [critical] - Register new service\n");
        fprintf(stderr, "  unregister <name>                 - Unregister service\n");
        fprintf(stderr, "  start <name>                      - Start service\n");
        fprintf(stderr, "  stop <name>                       - Stop service\n");
        fprintf(stderr, "  restart <name>                    - Restart service\n");
        fprintf(stderr, "  status [name]                     - Show service status\n");
    }
}

/* Monitor services for crashes */
static void monitor_services(void) {
    printf("RESURRECTION: Entering service monitoring loop\n");

    while (server.running) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid > 0) {
            /* A child process exited */
            Service *svc = NULL;

            /* Find which service crashed */
            for (int i = 0; i < server.num_services; i++) {
                if (server.services[i]->pid == pid) {
                    svc = server.services[i];
                    break;
                }
            }

            if (!svc) {
                fprintf(stderr, "RESURRECTION: Unknown pid %d exited\n", pid);
                continue;
            }

            /* Log crash */
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                fprintf(stderr, "RESURRECTION: %s (pid %d) exited with code %d\n",
                        svc->name, pid, exit_code);
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                fprintf(stderr, "RESURRECTION: %s (pid %d) killed by signal %d\n",
                        svc->name, pid, sig);
            }

            svc->state = SRV_CRASHED;
            svc->pid = 0;

            /* Auto-restart if enabled */
            if (svc->auto_restart) {
                restart_service(svc);
            } else {
                printf("RESURRECTION: %s auto-restart disabled, not restarting\n",
                       svc->name);
            }
        }

        /* Sleep briefly to avoid busy loop */
        usleep(100000);  /* 100ms */
    }

    printf("RESURRECTION: Exiting monitoring loop\n");
}

/* Cleanup and exit handler */
static void cleanup_and_exit(int sig) {
    printf("RESURRECTION: Received signal %d, shutting down\n", sig);

    server.running = 0;

    /* Kill all monitored services */
    for (int i = 0; i < server.num_services; i++) {
        Service *svc = server.services[i];
        if (svc->pid > 0) {
            printf("RESURRECTION: Stopping %s (pid %d)\n", svc->name, svc->pid);
            kill(svc->pid, SIGTERM);
        }
    }

    /* Wait briefly for graceful shutdown */
    sleep(1);

    /* Force kill any remaining */
    for (int i = 0; i < server.num_services; i++) {
        Service *svc = server.services[i];
        if (svc->pid > 0) {
            kill(svc->pid, SIGKILL);
        }
    }

    exit(0);
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    printf("=== Resurrection Server Starting (Layer 2) ===\n");

    /* Initialize server state */
    memset(&server, 0, sizeof(server));
    server.running = 1;

    /* Register signal handlers */
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);

    /* Register critical Layer 2 services */
    /* Note: These will be WASM modules once Phase 0 Layer 2 is complete */
    printf("RESURRECTION: Registering Layer 2 services...\n");

    /* TODO: Replace with actual WASM modules when Phase 0 Layer 2 is done */
    /* For now, register placeholder paths */
    register_service("wasm_server", "/boot/wasm_server.wasm", 1);
    register_service("hal_server", "/boot/hal_server.wasm", 1);
    register_service("file_server", "/boot/file_server.wasm", 0);
    register_service("capability_server", "/boot/cap_server.wasm", 0);

    /* Start all registered services */
    printf("RESURRECTION: Starting all services...\n");
    for (int i = 0; i < server.num_services; i++) {
        start_service(server.services[i]);
    }

    /* TODO: Mount 9P server at /srv/resurrection for control interface */
    /* This will allow:
     * - echo "register wasm_server /boot/wasm_server.wasm" > /srv/resurrection/ctl
     * - echo "start wasm_server" > /srv/resurrection/ctl
     * - echo "stop wasm_server" > /srv/resurrection/ctl
     * - cat /srv/resurrection/status
     */

    /* Enter service monitoring loop */
    monitor_services();

    /* Cleanup */
    printf("RESURRECTION: Cleaning up...\n");
    for (int i = 0; i < server.num_services; i++) {
        free(server.services[i]);
    }

    printf("=== Resurrection Server Stopped ===\n");
    return 0;
}
