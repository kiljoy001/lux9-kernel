# Resurrection 9P Server Implementation Guide

This document describes the 9P server implementation in `userspace/resurrection/resurrection.c` as a reference for developing additional 9P servers.

## Overview

Resurrection hosts `/srv` as a 9P filesystem namespace. Services register by creating entries, and clients authenticate using cryptographic capability tokens.

## Architecture

```
┌─────────────────┐
│     main()      │ ─── Creates pipe, mounts to /srv, forks worker
└────────┬────────┘
         │ rfork(RFPROC | RFMEM)
         v
┌─────────────────┐
│   srv_loop()    │ ─── Reads/writes 9P messages on pipe fd
└────────┬────────┘
         │
         v
┌─────────────────┐
│ srv_dispatch()  │ ─── Routes by message type
└────────┬────────┘
         │
    ┌────┴────┬────────┬────────┬────────┐
    v         v        v        v        v
Tattach   Twalk   Tcreate   Topen   Tauth ...
```

## Key Data Structures

### SrvEntry (Service Registration)
```c
typedef struct SrvEntry {
  char name[SRV_NAME_LEN];     /* Service name */
  uchar service_hash[32];      /* Blake2b(name) for token binding */
  u32int owner_pid;            /* PID that registered */
  u64int registered_epoch;     /* Registration time */
  int active;                  /* In use */
  Qid qid;                     /* 9P qid */
} SrvEntry;
```

### SrvFid (File Descriptor State)
```c
typedef struct SrvFid {
  u32int fid;          /* 9P fid */
  int type;            /* FID_FREE, FID_ROOT, FID_ENTRY, FID_AUTH */
  int srv_idx;         /* Index into srv_entries[] */
  int authenticated;   /* Has valid token */
  Qid qid;             /* Current qid */
} SrvFid;
```

## 9P Message Handlers

| Handler | Purpose |
|---------|---------|
| `srv_handle_attach()` | Client attaches, allocates root fid |
| `srv_handle_walk()` | Navigate to service entry by name |
| `srv_handle_create()` | Register new service in /srv |
| `srv_handle_open()` | Open file, optionally verify inline token |
| `srv_handle_write()` | Authenticate with CapToken or write data |
| `srv_handle_auth()` | Blind-sign capability requests |
| `srv_handle_clunk()` | Release fid |

## Response Builders

```c
u32int srv_build_error(buf, tag, "message");
u32int srv_build_rattach(buf, tag, &qid);
u32int srv_build_rwalk(buf, tag, nwqid, qids);
u32int srv_build_ropen(buf, tag, &qid, iounit);
u32int srv_build_rcreate(buf, tag, &qid, iounit);
u32int srv_build_rauth(buf, tag, &aqid, sig, siglen);
u32int srv_build_rwrite(buf, tag, count);
u32int srv_build_rclunk(buf, tag);
```

## Server Initialization Pattern

```c
int main(void) {
  /* 1. Initialize exchange page */
  exchange = (volatile uchar *)lux_exchange_page();
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  /* 2. Generate keypair for signing */
  crypto_eddsa_key_pair(privkey, pubkey);

  /* 3. Create pipe */
  int p[2];
  do_pipe(p);

  /* 4. Mount pipe to namespace */
  do_mount(p[1], -1, "/srv", MREPL | MCREATE, "");
  do_close(p[1]);

  /* 5. Fork worker */
  if (do_rfork(RFPROC | RFMEM) == 0) {
    srv_loop(p[0]);  /* Child: handle 9P */
    return 0;
  }

  /* Parent: continue with other tasks */
  do_close(p[0]);
}
```

## Capability Token Flow

1. **Client requests token**: `Tauth` with `CapBlindRequest`
2. **Server blind-signs**: `cap_blind_sign(&resp, &req, privkey)`
3. **Client unblinds**: `cap_unblind(token, &resp, &req)`
4. **Client authenticates**: `Twrite` with `CapToken`
5. **Server verifies**: `cap_token_verify(tok, service_hash, pubkey, epoch)`

## Syscall Wrappers

The server uses freestanding syscall wrappers:

```c
int do_pipe(int fd[2]);              /* SYS_PIPE: create pipe */
int do_mount(fd, afd, path, flags, aname);  /* SYS_MOUNT */
int do_rfork(int flags);             /* SYS_RFORK: fork process */
int do_read(int fd, char *buf, int n);
int do_write(int fd, void *buf, int n);
vlong nsec(void);                    /* SYS_NSEC: get time */
```

## Creating a New 9P Server

1. **Copy resurrection.c** as template
2. **Define entry structures** for your namespace objects
3. **Implement handlers** for supported operations
4. **Mount to target path** (e.g., `/net`, `/dev/custom`)
5. **Add to userspace/Makefile**

### Minimal Handler Template

```c
static u32int srv_handle_custom(uchar *req, uchar *resp) {
  uint pos = 7;  /* Skip size[4] + type[1] + tag[2] */
  u32int fid = get_u32(req + pos); pos += 4;
  unsigned short tag = get_u16(req + 5);

  SrvFid *f = srv_lookup_fid(fid);
  if (!f)
    return srv_build_error(resp, tag, "unknown fid");

  /* Process request... */

  return srv_build_rcustom(resp, tag, ...);
}
```

## Build Integration

Add to `userspace/resurrection/Makefile`:
```makefile
OBJ = $(SRC:.c=.o) blind_cap.o monocypher.o

blind_cap.o: $(KERNEL)/crypto/blind_cap.c
	$(CC) $(CFLAGS) -c -o $@ $<

monocypher.o: $(KERNEL)/crypto/monocypher.c
	$(CC) $(CFLAGS) -c -o $@ $<
```

## Thread Safety

Use spinlocks when handlers modify shared state:
```c
static int srv_lock = 0;

lock(&srv_lock);
result = srv_dispatch(rx, tx);
unlock(&srv_lock);
```

## References

- [userspace/resurrection/resurrection.c](file:///home/scott/Repo/lux9-kernel/userspace/resurrection/resurrection.c)
- [kernel/include/capability.h](file:///home/scott/Repo/lux9-kernel/kernel/include/capability.h)
- [kernel/crypto/blind_cap.c](file:///home/scott/Repo/lux9-kernel/kernel/crypto/blind_cap.c)
