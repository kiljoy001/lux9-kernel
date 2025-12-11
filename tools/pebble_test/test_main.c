#include "u.h"
#include "dat.h"
#include "fns.h"

extern void pebbleinit(void);
extern void pebbleprocinit(Proc *p);
extern void pebble_selftest(void);
extern void pebble_sip_issue_test(void);
extern void borrowinit(void);
extern void blind_ledger_init(void);
extern void stubs_init(void);
extern int pebble_enabled;

int main(int argc, char **argv) {
    print("Starting Pebble Userspace Test...\n");
    
    stubs_init();
    
    print("Initializing Subsystems...\n");
    borrowinit();
    blind_ledger_init();
    pebbleinit();
    
    up->pebble.drop_budget = 0;
    pebbleprocinit(up);
    
    print("Running pebble_selftest...\n");
    pebble_selftest();
    
    print("Running pebble_sip_issue_test...\n");
    pebble_sip_issue_test();
    
    print("Tests Completed Successfully.\n");
    return 0;
}
