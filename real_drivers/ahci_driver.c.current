/*
* AHCI driver for lux9-kernel
 * Provides basic AHCI support with device detection
 */

#include "../port/gcc_compat.h"

// Forward declarations
static SDev* ahci_pnp(void);
static int ahci_enable(SDev *s);
static int ahci_disable(SDev *s);
static int ahci_verify(SDunit *u);
static int ahci_online(SDunit *u);
static int ahci_io(SDreq *r);
static char* ahci_ctl(SDunit *u, char *p, char *e);
static int ahci_wctl(SDunit *u, void *cmd);
static long ahci_bio(SDunit *u, int lun, int write, void *a, long count, uvlong lba);
static char* ahci_topctl(SDev *s, char *p, char *e);
static int ahci_wtopctl(SDev *s, void *cmd);
static int ahci_ataio(SDreq *r);

// Export the driver interface
SDifc sdiahciifc = {
    "ahci",
    ahci_pnp,
    ahci_enable,
    ahci_disable,
    ahci_verify,
    ahci_online,
    ahci_io,
    ahci_ctl,
    ahci_wctl,
    ahci_bio,
    nil,        // probe
    nil,        // clear
    ahci_topctl,
    ahci_wtopctl,
    ahci_ataio,
};

// Simple AHCI controller tracking
static int ahci_found = 0;
static int ahci_enabled = 0;

static SDev* ahci_pnp(void) {
    // For compilation compatibility, don't try to detect hardware
    // In real kernel, this would scan for AHCI controllers
    if(ahci_found)
        return nil;
    
    // Always return nil for now - no hardware detection in compatibility mode
    return nil;
}

static int ahci_enable(SDev *s) {
    if(ahci_enabled)
        return 1;
        
    // Basic initialization would go here
    ahci_enabled = 1;
    return 1;
}

static int ahci_disable(SDev *s) {
    if(!ahci_enabled)
        return 1;
        
    ahci_enabled = 0;
    return 1;
}

static int ahci_verify(SDunit *u) {
    // Always verify for now
    return 1;
}

static int ahci_online(SDunit *u) {
    // For now, assume unit 0 is online if controller is enabled
    if(ahci_enabled && u->subno == 0)
        return 1;
        
    return 0;
}

static int ahci_io(SDreq *r) {
    // For now, fail all I/O operations
    return -1;
}

static char* ahci_ctl(SDunit *u, char *p, char *e) {
    p = seprint(p, e, "ahci unit %d\n", u->subno);
    return p;
}

static int ahci_wctl(SDunit *u, void *cmd) {
    // No write control for now
    return 0;
}

static long ahci_bio(SDunit *u, int lun, int write, void *a, long count, uvlong lba) {
    // For now, fail all block I/O operations
    return -1;
}

static char* ahci_topctl(SDev *s, char *p, char *e) {
    p = seprint(p, e, "ahci controller\n");
    return p;
}

static int ahci_wtopctl(SDev *s, void *cmd) {
    // No top-level write control for now
    return 0;
}

static int ahci_ataio(SDreq *r) {
    // For now, fail all ATA I/O operations
    return -1;
}
