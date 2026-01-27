# WASI Rump POSIX 9P Bridge

This defines the request/response format for `/srv/rump/posix/*` virtual files
served by `userspace/rump/rump_server.c`. The kernel WASI shim uses these files
to delegate POSIX-style operations to NetBSD rump.

## Mount Point

The service is expected to be mounted at `/srv/rump`. The WASI shim opens
specific files under `/srv/rump/posix/`.

## Virtual Files

Current virtual files:
- `/srv/rump/posix/fdstat`
- `/srv/rump/posix/pathstat`
- `/srv/rump/posix/pread`
- `/srv/rump/posix/pwrite`
- `/srv/rump/posix/readdir`
- `/srv/rump/posix/fd_sync` (stub)
- `/srv/rump/posix/fd_tell` (stub)
- `/srv/rump/posix/fd_set_size` (stub)
- `/srv/rump/posix/fd_set_times` (stub)
- `/srv/rump/posix/path_set_times` (stub)
- `/srv/rump/posix/path_create_directory` (stub)
- `/srv/rump/posix/path_remove_directory` (stub)
- `/srv/rump/posix/path_unlink_file` (stub)
- `/srv/rump/posix/path_rename` (stub)
- `/srv/rump/posix/path_symlink` (stub)
- `/srv/rump/posix/path_readlink` (stub)
- `/srv/rump/posix/poll_oneoff` (stub)
- `/srv/rump/posix/sock_accept` (stub)
- `/srv/rump/posix/sock_recv` (stub)
- `/srv/rump/posix/sock_send` (stub)
- `/srv/rump/posix/sock_shutdown` (stub)

## Common Encoding

All requests are little-endian. Responses start with a 32-bit errno (0 on
success). Non-zero errno values are NetBSD errno codes.

## fdstat

Request:
```
u32 op        (POSIX_FDSTAT_GET = 1)
u32 fd
```

Response:
```
u32 errno
struct rump_stat (NetBSD layout)
```

## pathstat

Request:
```
u32 op        (POSIX_PATHSTAT_GET = 1)
u32 path_len
u8  path[path_len]
```

Response:
```
u32 errno
struct rump_stat
```

## pread

Request:
```
u32 op        (POSIX_PREAD = 1)
u32 fd
u64 offset
u32 len
```

Response:
```
u32 errno
u32 count
u8  data[count]
```

## pwrite

Request:
```
u32 op        (POSIX_PWRITE = 1)
u32 fd
u64 offset
u32 len
u8  data[len]
```

Response:
```
u32 errno
u32 count
```

## readdir

Request:
```
u32 op        (POSIX_READDIR = 1)
u32 fd
u64 offset
u32 len
```

Response:
```
u32 errno
u32 count
u8  data[count]   (NetBSD getdents payload)
```

## poll_oneoff (stub)

Request:
```
u32 op        (POSIX_POLL_ONEOFF = 1)
u32 nsubscriptions
u8  subscriptions[nsubscriptions * 48]
```

Response:
```
u32 errno
u32 nevents
u8  events[nevents * 32]
```

## sock_accept (stub)

Request:
```
u32 op        (POSIX_SOCK_ACCEPT = 1)
u32 fd
u32 flags
```

Response:
```
u32 errno
u32 newfd
```

## sock_recv (stub)

Request:
```
u32 op        (POSIX_SOCK_RECV = 1)
u32 fd
u32 flags
u32 len
```

Response:
```
u32 errno
u32 ro_flags
u32 count
u8  data[count]
```

## sock_send (stub)

Request:
```
u32 op        (POSIX_SOCK_SEND = 1)
u32 fd
u32 flags
u32 len
u8  data[len]
```

Response:
```
u32 errno
u32 count
```

## sock_shutdown (stub)

Request:
```
u32 op        (POSIX_SOCK_SHUTDOWN = 1)
u32 fd
u32 how
```

Response:
```
u32 errno
```

## path_create_directory (stub)

Request:
```
u32 op        (POSIX_PATH_CREATE_DIRECTORY = 1)
u32 path_len
u8  path[path_len]
```

Response:
```
u32 errno
```

## path_remove_directory (stub)

Request:
```
u32 op        (POSIX_PATH_REMOVE_DIRECTORY = 1)
u32 path_len
u8  path[path_len]
```

Response:
```
u32 errno
```

## path_unlink_file (stub)

Request:
```
u32 op        (POSIX_PATH_UNLINK_FILE = 1)
u32 path_len
u8  path[path_len]
```

Response:
```
u32 errno
```

## path_rename (stub)

Request:
```
u32 op        (POSIX_PATH_RENAME = 1)
u32 old_len
u8  old_path[old_len]
u32 new_len
u8  new_path[new_len]
```

Response:
```
u32 errno
```

## path_symlink (stub)

Request:
```
u32 op        (POSIX_PATH_SYMLINK = 1)
u32 old_len
u8  old_path[old_len]
u32 new_len
u8  new_path[new_len]
```

Response:
```
u32 errno
```

## path_readlink (stub)

Request:
```
u32 op        (POSIX_PATH_READLINK = 1)
u32 path_len
u8  path[path_len]
u32 buf_len
```

Response:
```
u32 errno
u32 count
u8  data[count]
```

## path_set_times (stub)

Request:
```
u32 op        (POSIX_PATH_SET_TIMES = 1)
u32 path_len
u8  path[path_len]
u64 atim
u64 mtim
u32 flags
```

Response:
```
u32 errno
```

## Outstanding

Poll, sockets, and path mutation ops are currently stubbed (ENOSYS) on the
rump side. Implement the actual handlers and return payloads when ready.
