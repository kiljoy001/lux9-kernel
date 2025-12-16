#include "sip.h"
#include <string.h>
#include <stdio.h>

static uint32_t get_fid() {
    static _Atomic uint32_t f = 1;
    return atomic_fetch_add(&f, 1);
}

int sip_spawn(char *path) {
    Fcall tx, rx;
    uint32_t fid_root = get_fid();
    uint32_t fid_ctl = get_fid();
    char buf[256];
    
    /* 1. Attach to /proc/self */
    tx.type = Tattach;
    tx.fid = fid_root;
    tx.afid = NOFID;
    tx.uname = "user";
    tx.aname = "/proc/self";
    if (sip_transact(&tx, &rx) < 0 || rx.type == Rerror) return -1;
    
    /* 2. Walk to ctl */
    tx.type = Twalk;
    tx.fid = fid_root;
    tx.newfid = fid_ctl;
    tx.nwname = 1;
    tx.wname[0] = "ctl";
    if (sip_transact(&tx, &rx) < 0 || rx.type == Rerror) return -1;
    
    /* 3. Open ctl */
    tx.type = Topen;
    tx.fid = fid_ctl;
    tx.mode = OWRITE;
    if (sip_transact(&tx, &rx) < 0 || rx.type == Rerror) return -1;
    
    /* 4. Write command */
    snprintf(buf, sizeof(buf), "spawn %s", path);
    tx.type = Twrite;
    tx.fid = fid_ctl;
    tx.offset = 0;
    tx.count = strlen(buf);
    tx.data = buf;
    if (sip_transact(&tx, &rx) < 0 || rx.type == Rerror) return -1;
    
    /* 5. Clunk */
    tx.type = Tclunk; tx.fid = fid_ctl; sip_transact(&tx, &rx);
    tx.type = Tclunk; tx.fid = fid_root; sip_transact(&tx, &rx);
    
    return 0;
}

int sip_exits(char *status) {
    /* Similar flow but write "exits status" */
    Fcall tx, rx;
    uint32_t fid_root = get_fid();
    uint32_t fid_ctl = get_fid();
    char buf[256];
    
    tx.type = Tattach; tx.fid = fid_root; tx.afid = NOFID; tx.uname = "user"; tx.aname = "/proc/self";
    if (sip_transact(&tx, &rx) < 0) return -1;
    
    tx.type = Twalk; tx.fid = fid_root; tx.newfid = fid_ctl; tx.nwname = 1; tx.wname[0] = "ctl";
    if (sip_transact(&tx, &rx) < 0) return -1;
    
    tx.type = Topen; tx.fid = fid_ctl; tx.mode = OWRITE;
    sip_transact(&tx, &rx);
    
    snprintf(buf, sizeof(buf), "exits %s", status ? status : "");
    tx.type = Twrite; tx.fid = fid_ctl; tx.offset = 0; tx.count = strlen(buf); tx.data = buf;
    sip_transact(&tx, &rx);
    
    /* Should not return if successful */
    return 0;
}
