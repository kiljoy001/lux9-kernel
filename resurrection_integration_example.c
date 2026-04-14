/* resurrection_integration_example.c - WASM Fileserver Integration with Resurrection Server
 *
 * This demonstrates how the simplified WASM fileserver integrates with the
 * existing resurrection server for monitoring and lifecycle management.
 *
 * Shows the clean integration:
 * 1. Service registration with resurrection server
 * 2. Process monitoring and health checks
 * 3. Automatic restart on crash
 * 4. Binary verification for security
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/* Resurrection server 9P interface functions */
extern int srv_register_service(const char *name, const char *exec_path, 
                               const char *hash, int critical);
extern int srv_create_service_entry(const char *name, int pid);
extern int srv_configure_service(const char *name, const char *config);
extern int srv_start_service(const char *name);
extern int srv_stop_service(const char *name);
extern int srv_restart_service(const char *name);

/* Health monitoring functions */
extern int srv_get_service_status(const char *name);
extern int srv_monitor_service(const char *name, int (*check_fn)(void), 
                             int interval_sec);

/* WASM fileserver process management */
static pid_t wasm_fileserver_pid = 0;
static int service_registered = 0;
static const char *service_name = "wasm-fileserver";
static const char *exec_path = "/boot/wasm_fileserver";
static const char *wasm_binary_hash = "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456";

/* Health check function for the WASM fileserver */
static int wasm_fileserver_health_check(void) {
    if (wasm_fileserver_pid == 0) {
        return 0; /* Not running */
    }
    
    /* Check if process is still alive */
    int status;
    pid_t result = waitpid(wasm_fileserver_pid, &status, WNOHANG);
    
    if (result == wasm_fileserver_pid) {
        /* Process has exited */
        printf("[RESURRECTION] WASM fileserver (PID %d) has exited\n", wasm_fileserver_pid);
        wasm_fileserver_pid = 0;
        return 0;
    } else if (result == 0) {
        /* Process is still running */
        /* Additional health checks could include:
         * - Checking health file existence
         * - 9P ping to ensure responsiveness
         * - Memory/CPU usage checks
         */
        
        /* Simple file-based health check */
        char health_file[256];
        snprintf(health_file, sizeof(health_file),
                "/tmp/wasm_fileserver_health_%d", wasm_fileserver_pid);
        
        FILE *f = fopen(health_file, "r");
        if (f) {
            char line[256];
            int modules = 0;
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "Modules: ", 9) == 0) {
                    sscanf(line + 9, "%d", &modules);
                    break;
                }
            }
            fclose(f);
            
            printf("[RESURRECTION] WASM fileserver health OK (PID %d, %d modules)\n", 
                   wasm_fileserver_pid, modules);
            return 1;
        } else {
            printf("[RESURRECTION] WASM fileserver health file missing (PID %d)\n", 
                   wasm_fileserver_pid);
            return 0;
        }
    } else {
        /* waitpid error */
        printf("[RESURRECTION] waitpid error for WASM fileserver: %s\n", strerror(errno));
        wasm_fileserver_pid = 0;
        return 0;
    }
}

/* Start WASM fileserver process */
static int start_wasm_fileserver(void) {
    if (wasm_fileserver_pid != 0) {
        printf("[RESURRECTION] WASM fileserver already running (PID %d)\n", wasm_fileserver_pid);
        return 0;
    }
    
    printf("[RESURRECTION] Starting WASM fileserver...\n");
    
    /* Fork and exec the WASM fileserver */
    pid_t pid = fork();
    
    if (pid < 0) {
        printf("[RESURRECTION] Failed to fork WASM fileserver: %s\n", strerror(errno));
        return -1;
    }
    
    if (pid == 0) {
        /* Child process - execute WASM fileserver */
        printf("[RESURRECTION] Child process (PID %d) executing WASM fileserver\n", getpid());
        
        /* Execute the WASM fileserver binary */
        execl(exec_path, "wasm_fileserver", NULL);
        
        /* If execl returns, it failed */
        printf("[RESURRECTION] Failed to exec WASM fileserver: %s\n", strerror(errno));
        exit(1);
    } else {
        /* Parent process - record PID and monitor */
        wasm_fileserver_pid = pid;
        printf("[RESURRECTION] WASM fileserver started (PID %d)\n", pid);
        
        /* Verify the binary hash before marking as healthy */
        /* This would be done by the resurrection server's binary verification */
        printf("[RESURRECTION] Verifying binary hash...\n");
        /* hash_verification_result = verify_binary_hash(exec_path, wasm_binary_hash) */
        
        return 0;
    }
}

/* Stop WASM fileserver process */
static int stop_wasm_fileserver(void) {
    if (wasm_fileserver_pid == 0) {
        printf("[RESURRECTION] WASM fileserver not running\n");
        return 0;
    }
    
    printf("[RESURRECTION] Stopping WASM fileserver (PID %d)...\n", wasm_fileserver_pid);
    
    /* Send SIGTERM for graceful shutdown */
    if (kill(wasm_fileserver_pid, SIGTERM) < 0) {
        printf("[RESURRECTION] Failed to send SIGTERM: %s\n", strerror(errno));
        return -1;
    }
    
    /* Wait for graceful shutdown */
    int status;
    pid_t result = waitpid(wasm_fileserver_pid, &status, 0);
    
    if (result == wasm_fileserver_pid) {
        printf("[RESURRECTION] WASM fileserver stopped gracefully\n");
    } else {
        printf("[RESURRECTION] WASM fileserver did not stop gracefully, forcing...\n");
        kill(wasm_fileserver_pid, SIGKILL);
        waitpid(wasm_fileserver_pid, &status, 0);
    }
    
    wasm_fileserver_pid = 0;
    return 0;
}

/* Restart WASM fileserver with rate limiting */
static int restart_wasm_fileserver(void) {
    static time_t last_restart = 0;
    static int restarts_in_window = 0;
    
    time_t now = time(NULL);
    
    /* Rate limiting: max 5 restarts per minute */
    if (now - last_restart > 60) {
        restarts_in_window = 0;
    }
    
    if (restarts_in_window >= 5) {
        printf("[RESURRECTION] Too many restarts (%d), marking service as failed\n", 
               restarts_in_window);
        return -1;
    }
    
    restarts_in_window++;
    last_restart = now;
    
    printf("[RESURRECTION] Restarting WASM fileserver (attempt %d)\n", restarts_in_window);
    
    stop_wasm_fileserver();
    return start_wasm_fileserver();
}

/* Register WASM fileserver with resurrection server */
static int register_wasm_fileserver_service(void) {
    if (service_registered) {
        printf("[RESURRECTION] WASM fileserver already registered\n");
        return 0;
    }
    
    printf("[RESURRECTION] Registering WASM fileserver service...\n");
    
    /* Register service with resurrection server */
    /* This uses the existing 9P /srv interface */
    
    /* Method 1: Via 9P Tcreate on /srv */
    /* srv_create_service_entry(service_name, 0); */
    
    /* Method 2: Via resurrection server API */
    int result = srv_register_service(service_name, exec_path, wasm_binary_hash, 1);
    
    if (result < 0) {
        printf("[RESURRECTION] Failed to register service: %s\n", strerror(errno));
        return -1;
    }
    
    service_registered = 1;
    printf("[RESURRECTION] WASM fileserver service registered\n");
    
    /* Configure service properties */
    srv_configure_service(service_name, "auto_restart=1");
    srv_configure_service(service_name, "critical=1");
    srv_configure_service(service_name, "health_check_interval=10");
    
    return 0;
}

/* Monitor WASM fileserver continuously */
static void monitor_wasm_fileserver(void) {
    printf("[RESURRECTION] Starting WASM fileserver monitoring...\n");
    
    int consecutive_failures = 0;
    const int max_consecutive_failures = 3;
    
    while (1) {
        int is_healthy = wasm_fileserver_health_check();
        
        if (is_healthy) {
            consecutive_failures = 0;
        } else {
            consecutive_failures++;
            
            if (consecutive_failures >= max_consecutive_failures) {
                printf("[RESURRECTION] WASM fileserver unhealthy for %d checks, restarting...\n",
                       consecutive_failures);
                
                if (restart_wasm_fileserver() < 0) {
                    printf("[RESURRECTION] Failed to restart WASM fileserver\n");
                    break;
                }
                
                consecutive_failures = 0;
            }
        }
        
        /* Sleep before next health check */
        sleep(10);
    }
}

/* Demonstrate service lifecycle */
static void demonstrate_lifecycle(void) {
    printf("\n=== WASM Fileserver Lifecycle Demonstration ===\n");
    
    /* 1. Register service */
    printf("\n1. Registering service with resurrection server...\n");
    register_wasm_fileserver_service();
    
    /* 2. Start service */
    printf("\n2. Starting WASM fileserver...\n");
    start_wasm_fileserver();
    
    /* 3. Monitor for a bit */
    printf("\n3. Monitoring service (30 seconds)...\n");
    for (int i = 0; i < 3; i++) {
        sleep(10);
        wasm_fileserver_health_check();
    }
    
    /* 4. Simulate crash and recovery */
    printf("\n4. Simulating service crash...\n");
    if (wasm_fileserver_pid != 0) {
        kill(wasm_fileserver_pid, SIGKILL);
        wasm_fileserver_pid = 0;
    }
    
    printf("\n5. Resurrection server detects crash and restarts...\n");
    sleep(2);
    start_wasm_fileserver();
    
    /* 6. Monitor again */
    printf("\n6. Monitoring restarted service...\n");
    for (int i = 0; i < 3; i++) {
        sleep(10);
        wasm_fileserver_health_check();
    }
    
    /* 7. Stop service */
    printf("\n7. Stopping service...\n");
    stop_wasm_fileserver();
    
    printf("\n=== Lifecycle Demonstration Complete ===\n");
}

/* Configuration management */
static void configure_service(void) {
    printf("\n=== WASM Fileserver Configuration ===\n");
    
    /* Set various configuration parameters */
    srv_configure_service(service_name, "exec=/boot/wasm_fileserver");
    srv_configure_service(service_name, "max_memory=64M");
    srv_configure_service(service_name, "max_cpu_time=30s");
    srv_configure_service(service_name, "health_check_interval=5");
    srv_configure_service(service_name, "restart_policy=always");
    srv_configure_service(service_name, "priority=high");
    
    printf("Service configured successfully\n");
}

/* Main demonstration */
int main(int argc, char *argv[]) {
    printf("=== WASM Fileserver Resurrection Server Integration ===\n");
    printf("Demonstrating simplified architecture with resurrection monitoring\n\n");
    
    /* Show configuration */
    configure_service();
    
    /* Demonstrate full lifecycle */
    demonstrate_lifecycle();
    
    /* Or start continuous monitoring */
    if (argc > 1 && strcmp(argv[1], "--monitor") == 0) {
        printf("\nStarting continuous monitoring mode...\n");
        monitor_wasm_fileserver();
    }
    
    printf("\nIntegration demonstration complete\n");
    return 0;
}

/* 
 * Integration Summary:
 * 
 * 1. Service Registration:
 *    - Register with resurrection server via /srv interface
 *    - Provide binary path and hash for verification
 *    - Set configuration (auto-restart, health checks, etc.)
 * 
 * 2. Process Management:
 *    - Resurrection server forks and monitors WASM fileserver
 *    - Binary verification ensures only trusted versions run
 *    - Process isolation prevents security issues
 * 
 * 3. Health Monitoring:
 *    - Regular health checks (file-based or 9P ping)
 *    - Automatic restart on crash with rate limiting
 *    - Status reporting via /srv interface
 * 
 * 4. Reliability:
 *    - Automatic recovery from crashes
 *    - Binary verification prevents compromised restarts
 *    - Resource limits and isolation
 * 
 * 5. Management:
 *    - Standard 9P /srv interface for control
 *    - Configuration via file writes
 *    - Status monitoring and logging
 */
