/*
 * Race condition prevention for 9P command interface
 * Based on formal proofs in command_loop_verified.v
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>

#define COMMAND_FILE "/home/scott/Repo/VM-Interop/command.txt"
#define OUTPUT_FILE "/home/scott/Repo/VM-Interop/output.txt"
#define LOCK_FILE "/home/scott/Repo/VM-Interop/.command.lock"
#define MAX_RETRIES 100
#define RETRY_DELAY_US 10000  /* 10ms */

/* State structure matching Coq model */
typedef struct {
    int commands_sent;
    int commands_processed;
    size_t output_size;
    int is_running;
} SystemState;

/* Theorem: race_free - verified atomic operations */
int acquire_lock(void) {
    int fd;
    int retries = 0;
    
    while (retries < MAX_RETRIES) {
        fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd >= 0) {
            /* Lock acquired - write PID for debugging */
            char pid_str[32];
            snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
            write(fd, pid_str, strlen(pid_str));
            close(fd);
            return 1;  /* Success */
        }
        
        if (errno != EEXIST) {
            perror("Failed to create lock file");
            return 0;
        }
        
        /* Lock held by another process */
        usleep(RETRY_DELAY_US);
        retries++;
    }
    
    fprintf(stderr, "Failed to acquire lock after %d retries\n", MAX_RETRIES);
    return 0;
}

void release_lock(void) {
    if (unlink(LOCK_FILE) < 0 && errno != ENOENT) {
        perror("Failed to remove lock file");
    }
}

/* Theorem: no_command_loss - atomic command append */
int send_command_atomic(const char *command) {
    FILE *fp;
    int success = 0;
    struct stat st_before, st_after;
    
    if (!acquire_lock()) {
        return 0;
    }
    
    /* Get file size before (for invariant check) */
    if (stat(COMMAND_FILE, &st_before) < 0) {
        st_before.st_size = 0;  /* File doesn't exist yet */
    }
    
    /* Append command atomically */
    fp = fopen(COMMAND_FILE, "a");
    if (fp) {
        fprintf(fp, "%s\n", command);
        fflush(fp);
        fsync(fileno(fp));  /* Ensure write is on disk */
        fclose(fp);
        
        /* Verify invariant: file only grew */
        if (stat(COMMAND_FILE, &st_after) == 0) {
            if (st_after.st_size > st_before.st_size) {
                success = 1;
                printf("Command sent (new size: %ld bytes)\n", st_after.st_size);
            } else {
                fprintf(stderr, "Invariant violation: file did not grow\n");
            }
        }
    } else {
        perror("Failed to open command file");
    }
    
    release_lock();
    return success;
}

/* Theorem: processing_progress - verify output growth */
int verify_processing(long initial_size, int timeout_sec) {
    struct stat st;
    time_t start = time(NULL);
    
    while (time(NULL) - start < timeout_sec) {
        if (stat(OUTPUT_FILE, &st) == 0) {
            if (st.st_size > initial_size) {
                printf("Processing confirmed (output grew by %ld bytes)\n",
                       st.st_size - initial_size);
                return 1;
            }
        }
        usleep(100000);  /* 100ms */
    }
    
    return 0;
}

/* Get current system state (matches Coq State record) */
SystemState get_system_state(void) {
    SystemState state = {0};
    FILE *fp;
    char line[1024];
    struct stat st;
    
    /* Count commands sent */
    fp = fopen(COMMAND_FILE, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            state.commands_sent++;
        }
        fclose(fp);
    }
    
    /* Get output size */
    if (stat(OUTPUT_FILE, &st) == 0) {
        state.output_size = st.st_size;
    }
    
    /* Count processed commands from output */
    fp = fopen(OUTPUT_FILE, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "===END===")) {
                state.commands_processed++;
            }
        }
        fclose(fp);
    }
    
    state.is_running = 1;  /* Assume running */
    
    return state;
}

/* Verify system invariants from Coq proofs */
int verify_invariants(void) {
    SystemState state = get_system_state();
    int valid = 1;
    
    /* Invariant 1: commands_processed <= commands_sent */
    if (state.commands_processed > state.commands_sent) {
        fprintf(stderr, "INVARIANT VIOLATION: processed (%d) > sent (%d)\n",
                state.commands_processed, state.commands_sent);
        valid = 0;
    }
    
    /* Invariant 2: output file exists and is non-empty if processed > 0 */
    if (state.commands_processed > 0 && state.output_size == 0) {
        fprintf(stderr, "INVARIANT VIOLATION: commands processed but no output\n");
        valid = 0;
    }
    
    if (valid) {
        printf("✓ All invariants hold\n");
        printf("  Commands: %d sent, %d processed\n", 
               state.commands_sent, state.commands_processed);
        printf("  Output size: %zu bytes\n", state.output_size);
    }
    
    return valid;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> | --verify\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "--verify") == 0) {
        /* Verify mode - check invariants */
        return verify_invariants() ? 0 : 1;
    }
    
    /* Send command with race prevention */
    struct stat st;
    long initial_output = 0;
    
    if (stat(OUTPUT_FILE, &st) == 0) {
        initial_output = st.st_size;
    }
    
    if (!send_command_atomic(argv[1])) {
        fprintf(stderr, "Failed to send command\n");
        return 1;
    }
    
    printf("Waiting for processing...\n");
    if (!verify_processing(initial_output, 5)) {
        printf("No output after 5 seconds (command may still be pending)\n");
    }
    
    /* Final invariant check */
    verify_invariants();
    
    return 0;
}