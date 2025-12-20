/* rumpuser_config.h.  Generated from rumpuser_config.h.in by configure.  */
/* rumpuser_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the `aligned_alloc' function. */
#define HAVE_ALIGNED_ALLOC 1

/* Define to 1 if you have the `arc4random_buf' function. */
#define HAVE_ARC4RANDOM_BUF 1

/* Define to 1 if you have the `chflags' function. */
/* #undef HAVE_CHFLAGS */

/* Define to 1 if the system has the type `clockid_t'. */
#define HAVE_CLOCKID_T 1

/* clock_gettime */
#define HAVE_CLOCK_GETTIME 1

/* clock_nanosleep */
#define HAVE_CLOCK_NANOSLEEP 1

/* dlinfo */
#define HAVE_DLINFO 1

/* Define to 1 if you have the `fsync_range' function. */
/* #undef HAVE_FSYNC_RANGE */

/* Define to 1 if you have the `getenv_r' function. */
/* #undef HAVE_GETENV_R */

/* Define to 1 if you have the `getprogname' function. */
/* #undef HAVE_GETPROGNAME */

/* Define to 1 if you have the `getsubopt' function. */
#define HAVE_GETSUBOPT 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if ioctl()'s cmd arg is int */
/* #undef HAVE_IOCTL_CMD_INT */

/* Define to 1 if you have the `kqueue' function. */
/* #undef HAVE_KQUEUE */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the `memalign' function. */
#define HAVE_MEMALIGN 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <paths.h> header file. */
#define HAVE_PATHS_H 1

/* Define to 1 if you have the `posix_memalign' function. */
#define HAVE_POSIX_MEMALIGN 1

/* Define to 1 if you have the `preadv' function. */
#define HAVE_PREADV 1

/* Define to 1 if you have 2-arg pthread_setname_np() */
#define HAVE_PTHREAD_SETNAME2 1

/* Define to 1 if you have 3-arg pthread_setname_np() */
/* #undef HAVE_PTHREAD_SETNAME3 */

/* Define to 1 if you have the `pwritev' function. */
#define HAVE_PWRITEV 1

/* Define to 1 if the system has the type `register_t'. */
#define HAVE_REGISTER_T 1

/* Define to 1 if you have the `setprogname' function. */
/* #undef HAVE_SETPROGNAME */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strsuftoll' function. */
/* #undef HAVE_STRSUFTOLL */

/* Define to 1 if `sin_len' is a member of `struct sockaddr_in'. */
/* #undef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */

/* Define to 1 if you have the <sys/atomic.h> header file. */
/* #undef HAVE_SYS_ATOMIC_H */

/* Define to 1 if you have the <sys/cdefs.h> header file. */
#define HAVE_SYS_CDEFS_H 1

/* Define to 1 if you have the <sys/disklabel.h> header file. */
/* #undef HAVE_SYS_DISKLABEL_H */

/* Define to 1 if you have the <sys/disk.h> header file. */
/* #undef HAVE_SYS_DISK_H */

/* Define to 1 if you have the <sys/dkio.h> header file. */
/* #undef HAVE_SYS_DKIO_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `utimensat' function. */
#define HAVE_UTIMENSAT 1

/* Define to 1 if you have the `__quotactl' function. */
/* #undef HAVE___QUOTACTL */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://github.com/rumpkernel/"

/* Define to the full name of this package. */
#define PACKAGE_NAME "rumpuser-posix"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "rumpuser-posix 999"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "rumpuser-posix"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "999"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
