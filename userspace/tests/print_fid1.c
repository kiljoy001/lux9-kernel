#include <lux.h>

int main(int argc, char *argv[]) {
    char *msg = "=== PRINT FID 1 TEST ===\n";
    sys_write(1, msg, 25);
    
    char *msg2 = "If you see this, userspace can print to fid 1.\n";
    sys_write(1, msg2, 47);
    
    return 0;
}