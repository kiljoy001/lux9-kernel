/* Test program to verify syscall bridge works */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    printf("Testing Lux9 syscall bridge...\n");
    
    /* Test fork() */
    printf("Calling fork()...\n");
    int pid = fork();
    if (pid < 0) {
        printf("fork() failed: %d\n", pid);
        return 1;
    }
    
    if (pid == 0) {
        /* Child process */
        printf("Child process: PID=%d\n", getpid());
        
        /* Try exec */
        char *argv[] = { "/bin/hello", NULL };
        printf("Child: calling exec()...\n");
        int ret = exec("/bin/hello", argv);
        printf("exec() returned: %d (should not reach here)\n", ret);
        
        /* If exec fails, exit */
        exit(1);
    } else {
        /* Parent process */
        printf("Parent process: forked child PID=%d\n", pid);
        
        /* Wait for child */
        int status;
        int ret = wait(&status);
        printf("Parent: wait() returned %d, status=%d\n", ret, status);
    }
    
    printf("Test completed successfully!\n");
    return 0;
}