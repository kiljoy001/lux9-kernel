/*
 * init - First userspace process (Layer 2 bootstrap)
 *
 * Responsibilities:
 * 1. Mount root filesystem (if needed)
 * 2. Start resurrection server as first Layer 2 service
 * 3. Wait for resurrection server to initialize
 * 4. Resurrection server then manages all other Layer 2 services
 *
 * Architecture:
 * - init runs as PID 1 in userspace (Layer 2)
 * - Spawns resurrection server via fork()/exec()
 * - Resurrection server becomes the service monitor
 * - All other Layer 2 services are children of resurrection server
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define RESURRECTION_PATH "/boot/resurrection"

/* Spawn resurrection server */
static pid_t spawn_resurrection(void) {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "INIT: Failed to fork resurrection server: %s\n", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* Child process - exec resurrection server */
        char *argv[] = { "resurrection", NULL };
        char *envp[] = { NULL };

        printf("INIT: Executing %s...\n", RESURRECTION_PATH);
        execve(RESURRECTION_PATH, argv, envp);

        /* If we get here, exec failed */
        fprintf(stderr, "INIT: Failed to exec %s: %s\n", RESURRECTION_PATH, strerror(errno));
        exit(1);
    }

    /* Parent process */
    printf("INIT: Spawned resurrection server (pid %d)\n", pid);
    return pid;
}

/* Wait for resurrection server to be ready */
static int wait_for_resurrection(void) {
    /* TODO: Check for /srv/resurrection control interface */
    /* For now, just sleep briefly */
    printf("INIT: Waiting for resurrection server to initialize...\n");
    sleep(1);
    printf("INIT: Resurrection server should be ready\n");
    return 0;
}

/* Monitor resurrection server */
static void monitor_resurrection(pid_t resurrection_pid) {
    printf("INIT: Monitoring resurrection server (pid %d)\n", resurrection_pid);

    while (1) {
        int status;
        pid_t result = waitpid(resurrection_pid, &status, WNOHANG);

        if (result == resurrection_pid) {
            /* Resurrection server exited */
            if (WIFEXITED(status)) {
                fprintf(stderr, "INIT: CRITICAL: Resurrection server exited with code %d\n",
                        WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "INIT: CRITICAL: Resurrection server killed by signal %d\n",
                        WTERMSIG(status));
            }

            fprintf(stderr, "INIT: System cannot continue without resurrection server\n");
            fprintf(stderr, "INIT: Attempting to restart resurrection server...\n");

            /* Try to restart resurrection server */
            resurrection_pid = spawn_resurrection();
            if (resurrection_pid < 0) {
                fprintf(stderr, "INIT: FATAL: Cannot restart resurrection server\n");
                fprintf(stderr, "INIT: System halted\n");
                exit(1);
            }

            wait_for_resurrection();
        }

        /* Sleep briefly before next check */
        sleep(1);

        /* Reap any other zombie children */
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
}

int main(void) {
    printf("=== init (PID 1) Starting (Layer 2 Bootstrap) ===\n");

    /* TODO: Mount root filesystem if needed */
    /* For now, assuming kernel has already set up initrd */

    /* Start resurrection server (first Layer 2 service) */
    pid_t resurrection_pid = spawn_resurrection();
    if (resurrection_pid < 0) {
        fprintf(stderr, "INIT: FATAL: Cannot start resurrection server\n");
        fprintf(stderr, "INIT: System cannot continue\n");
        exit(1);
    }

    /* Wait for resurrection server to initialize */
    if (wait_for_resurrection() < 0) {
        fprintf(stderr, "INIT: WARNING: Resurrection server may not be ready\n");
    }

    printf("INIT: Resurrection server initialized\n");
    printf("INIT: Entering monitoring mode\n");
    printf("INIT: Resurrection server will manage all other Layer 2 services\n");

    /* Monitor resurrection server forever */
    monitor_resurrection(resurrection_pid);

    /* Should never reach here */
    fprintf(stderr, "INIT: FATAL: Exited monitoring loop\n");
    return 1;
}
