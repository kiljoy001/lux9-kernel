#ifndef _LUX_BOOTSTRAP_H_
#define _LUX_BOOTSTRAP_H_

/*
 * Bootstrap-only namespace hooks.
 *
 * These are private bootstrap hooks for the early namespace owners.
 * Root publication uses the kernel's routed-root registry directly.
 * The raw bind/mount helpers still hit the legacy mount machinery and should
 * only remain in bootstrap and migration code.
 */
int sys_nsroot_publish_raw(int fd, char *path, char *aname);
int sys_nsroot_unpublish_raw(char *path);
int sys_mount_raw(int fd, int afd, char *old, int flags, char *aname);
int sys_bind_raw(char *old, char *newname, int flags);
int sys_unmount_raw(char *name, char *old);

#endif
