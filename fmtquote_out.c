/* Frama-C Missing Types - types excluded by #ifndef __FRAMAC__ in Plan 9 headers */
/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* Plan 9 universal header */
/* Add static_assert support for compile-time checks */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;
/* Plan 9 fixed-width types */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
typedef u32int Rune; /* UTF-8 code point */
typedef struct {
  u8int data[16];
} uuid_t;
/* USED macro to suppress unused warnings */
/* Compile-time type size assertions */
_Static_assert(sizeof(ulong) == sizeof(void *), "ulong must match pointer size");
_Static_assert(sizeof(uintptr) == sizeof(void *),
              "uintptr must match pointer size");
_Static_assert(sizeof(usize) == sizeof(void *), "usize must match pointer size");
_Static_assert(sizeof(ssize) == sizeof(void *), "ssize must match pointer size");
/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/*
 *	ISO C99 Standard: 7.13 Nonlocal jumps	<setjmp.h>
 */
/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* These are defined by the user (or the compiler)
   to specify the desired environment:
   __STRICT_ANSI__	ISO Standard C.
   _ISOC99_SOURCE	Extensions to ISO C89 from ISO C99.
   _ISOC11_SOURCE	Extensions to ISO C99 from ISO C11.
   _ISOC2X_SOURCE	Extensions to ISO C99 from ISO C2X.
   __STDC_WANT_LIB_EXT2__
			Extensions to ISO C99 from TR 27431-2:2010.
   __STDC_WANT_IEC_60559_BFP_EXT__
			Extensions to ISO C11 from TS 18661-1:2014.
   __STDC_WANT_IEC_60559_FUNCS_EXT__
			Extensions to ISO C11 from TS 18661-4:2015.
   __STDC_WANT_IEC_60559_TYPES_EXT__
			Extensions to ISO C11 from TS 18661-3:2015.
   __STDC_WANT_IEC_60559_EXT__
			ISO C2X interfaces defined only in Annex F.
   _POSIX_SOURCE	IEEE Std 1003.1.
   _POSIX_C_SOURCE	If ==1, like _POSIX_SOURCE; if >=2 add IEEE Std 1003.2;
			if >=199309L, add IEEE Std 1003.1b-1993;
			if >=199506L, add IEEE Std 1003.1c-1995;
			if >=200112L, all of IEEE 1003.1-2004
			if >=200809L, all of IEEE 1003.1-2008
   _XOPEN_SOURCE	Includes POSIX and XPG things.  Set to 500 if
			Single Unix conformance is wanted, to 600 for the
			sixth revision, to 700 for the seventh revision.
   _XOPEN_SOURCE_EXTENDED XPG things and X/Open Unix extensions.
   _LARGEFILE_SOURCE	Some more functions for correct standard I/O.
   _LARGEFILE64_SOURCE	Additional functionality from LFS for large files.
   _FILE_OFFSET_BITS=N	Select default filesystem interface.
   _ATFILE_SOURCE	Additional *at interfaces.
   _DYNAMIC_STACK_SIZE_SOURCE Select correct (but non compile-time constant)
			MINSIGSTKSZ, SIGSTKSZ and PTHREAD_STACK_MIN.
   _GNU_SOURCE		All of the above, plus GNU extensions.
   _DEFAULT_SOURCE	The default set of features (taking precedence over
			__STRICT_ANSI__).
   _FORTIFY_SOURCE	Add security hardening to many library functions.
			Set to 1, 2 or 3; 3 performs stricter checks than 2, which
			performs stricter checks than 1.
   _REENTRANT, _THREAD_SAFE
			Obsolete; equivalent to _POSIX_C_SOURCE=199506L.
   The `-ansi' switch to the GNU C compiler, and standards conformance
   options such as `-std=c99', define __STRICT_ANSI__.  If none of
   these are defined, or if _DEFAULT_SOURCE is defined, the default is
   to have _POSIX_SOURCE set to one and _POSIX_C_SOURCE set to
   200809L, as well as enabling miscellaneous functions from BSD and
   SVID.  If more than one of these are defined, they accumulate.  For
   example __STRICT_ANSI__, _POSIX_SOURCE and _POSIX_C_SOURCE together
   give you ISO C, 1003.1, and 1003.2, but nothing else.
   These are defined by this file and are used by the
   header files to decide what to declare or define:
   __GLIBC_USE (F)	Define things from feature set F.  This is defined
			to 1 or 0; the subsequent macros are either defined
			or undefined, and those tests should be moved to
			__GLIBC_USE.
   __USE_ISOC11		Define ISO C11 things.
   __USE_ISOC99		Define ISO C99 things.
   __USE_ISOC95		Define ISO C90 AMD1 (C95) things.
   __USE_ISOCXX11	Define ISO C++11 things.
   __USE_POSIX		Define IEEE Std 1003.1 things.
   __USE_POSIX2		Define IEEE Std 1003.2 things.
   __USE_POSIX199309	Define IEEE Std 1003.1, and .1b things.
   __USE_POSIX199506	Define IEEE Std 1003.1, .1b, .1c and .1i things.
   __USE_XOPEN		Define XPG things.
   __USE_XOPEN_EXTENDED	Define X/Open Unix things.
   __USE_UNIX98		Define Single Unix V2 things.
   __USE_XOPEN2K        Define XPG6 things.
   __USE_XOPEN2KXSI     Define XPG6 XSI things.
   __USE_XOPEN2K8       Define XPG7 things.
   __USE_XOPEN2K8XSI    Define XPG7 XSI things.
   __USE_LARGEFILE	Define correct standard I/O things.
   __USE_LARGEFILE64	Define LFS things with separate names.
   __USE_FILE_OFFSET64	Define 64bit interface as default.
   __USE_MISC		Define things from 4.3BSD or System V Unix.
   __USE_ATFILE		Define *at interfaces and AT_* constants for them.
   __USE_DYNAMIC_STACK_SIZE Define correct (but non compile-time constant)
			MINSIGSTKSZ, SIGSTKSZ and PTHREAD_STACK_MIN.
   __USE_GNU		Define GNU extensions.
   __USE_FORTIFY_LEVEL	Additional security measures used, according to level.
   The macros `__GNU_LIBRARY__', `__GLIBC__', and `__GLIBC_MINOR__' are
   defined by this file unconditionally.  `__GNU_LIBRARY__' is provided
   only for compatibility.  All new code should use the other symbols
   to test for features.
   All macros listed above as possibly being defined by this file are
   explicitly undefined if they are not explicitly defined.
   Feature-test macros that are not defined by the user or compiler
   but are implied by the other feature-test macros defined (or by the
   lack of any definitions) are defined by the file.
   ISO C feature test macros depend on the definition of the macro
   when an affected header is included, not when the first system
   header is included, and so they are handled in
   <bits/libc-header-start.h>, which does not have a multiple include
   guard.  Feature test macros that can be handled from the first
   system header included are handled here.  */
/* Undefine everything, so we get a clean slate.  */
/* Suppress kernel-name space pollution unless user expressedly asks
   for it.  */
/* Convenience macro to test the version of gcc.
   Use like this:
   #if __GNUC_PREREQ (2,8)
   ... code requiring gcc 2.8 or later ...
   #endif
   Note: only works for GCC 2.0 and later, because __GNUC_MINOR__ was
   added in 2.0.  */
/* Similarly for clang.  Features added to GCC after version 4.2 may
   or may not also be available in clang, and clang's definitions of
   __GNUC(_MINOR)__ are fixed at 4 and 2 respectively.  Not all such
   features can be queried via __has_extension/__has_feature.  */
/* Whether to use feature set F.  */
/* _BSD_SOURCE and _SVID_SOURCE are deprecated aliases for
   _DEFAULT_SOURCE.  If _DEFAULT_SOURCE is present we do not
   issue a warning; the expectation is that the source is being
   transitioned to use the new macro.  */
/* If _GNU_SOURCE was defined by the user, turn on all the other features.  */
/* If nothing (other than _GNU_SOURCE and _DEFAULT_SOURCE) is defined,
   define _DEFAULT_SOURCE.  */
/* This is to enable the ISO C2X extension.  */
/* This is to enable the ISO C11 extension.  */
/* This is to enable the ISO C99 extension.  */
/* This is to enable the ISO C90 Amendment 1:1995 extension.  */
/* If none of the ANSI/POSIX macros are defined, or if _DEFAULT_SOURCE
   is defined, use POSIX.1-2008 (or another version depending on
   _XOPEN_SOURCE).  */
/* Some C libraries once required _REENTRANT and/or _THREAD_SAFE to be
   defined in all multithreaded code.  GNU libc has not required this
   for many years.  We now treat them as compatibility synonyms for
   _POSIX_C_SOURCE=199506L, which is the earliest level of POSIX with
   comprehensive support for multithreaded code.  Using them never
   lowers the selected level of POSIX conformance, only raises it.  */
/* Features part to handle 64-bit time_t support.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* We need to know the word size in order to check the time size.  */
/* Determine the wordsize from the preprocessor defines.  */
/* Both x86-64 and x32 use the 64-bit system call interface.  */
/* Bit size of the time_t type at glibc build time, x86-64 and x32 case.
   Copyright (C) 2018-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* Determine the wordsize from the preprocessor defines.  */
/* Both x86-64 and x32 use the 64-bit system call interface.  */
/* For others, time size is word size.  */
/* The function 'gets' existed in C89, but is impossible to use
   safely.  It has been removed from ISO C11 and ISO C++14.  Note: for
   compatibility with various implementations of <cstdio>, this test
   must consider only the value of __cplusplus when compiling C++.  */
/* GNU formerly extended the scanf functions with modified format
   specifiers %as, %aS, and %a[...] that allocate a buffer for the
   input using malloc.  This extension conflicts with ISO C99, which
   defines %a as a standalone format specifier that reads a floating-
   point number; moreover, POSIX.1-2008 provides the same feature
   using the modifier letter 'm' instead (%ms, %mS, %m[...]).
   We now follow C99 unless GNU extensions are active and the compiler
   is specifically in C89 or C++98 mode (strict or not).  For
   instance, with GCC, -std=gnu11 will have C99-compliant scanf with
   or without -D_GNU_SOURCE, but -std=c89 -D_GNU_SOURCE will have the
   old extension.  */
/* ISO C2X added support for a 0b or 0B prefix on binary constants as
   inputs to strtol-family functions (base 0 or 2).  This macro is
   used to condition redirection in headers to allow that redirection
   to be disabled when building those functions, despite _GNU_SOURCE
   being defined.  */
/* Get definitions of __STDC_* predefined macros, if the compiler has
   not preincluded this header automatically.  */
/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This macro indicates that the installed library is the GNU C Library.
   For historic reasons the value now is 6 and this will stay from now
   on.  The use of this variable is deprecated.  Use __GLIBC__ and
   __GLIBC_MINOR__ now (see below) when you want to test for a specific
   GNU C library version and use the values in <gnu/lib-names.h> to get
   the sonames of the shared libraries.  */
/* Major and minor version number of the GNU C library package.  Use
   these macros to test for features in specific releases.  */
/* This is here only because every header file already includes this one.  */
/* Copyright (C) 1992-2024 Free Software Foundation, Inc.
   Copyright The GNU Toolchain Authors.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* We are almost always included from features.h. */
/* The GNU libc does not support any K&R compilers or the traditional mode
   of ISO C compilers anymore.  Check for some of the combinations not
   supported anymore.  */
/* Some user header file might have defined this before.  */
/* Compilers that lack __has_attribute may object to
       #if defined __has_attribute && __has_attribute (...)
   even though they do not need to evaluate the right-hand side of the &&.
   Similarly for __has_builtin, etc.  */
/* All functions, except those with callbacks or those that
   synchronize memory, are leaf functions.  */
/* GCC can always grok prototypes.  For C++ programs we add throw()
   to help it optimize the function calls.  But this only works with
   gcc 2.8.x and egcs.  For gcc 3.4 and up we even mark C functions
   as non-throwing using a function attribute since programs can use
   the -fexceptions options for C code as well.  */
/* These two macros are not used in glibc anymore.  They are kept here
   only because some other projects expect the macros to be defined.  */
/* For these things, GCC behaves the ANSI way normally,
   and the non-ANSI way under -traditional.  */
/* This is not a typedef so `const __ptr_t' does the right thing.  */
/* C++ needs to know that types and declarations are C, not C++.  */
/* Fortify support.  */
/* Use __builtin_dynamic_object_size at _FORTIFY_SOURCE=3 when available.  */
/* Support for flexible arrays.
   Headers that should use flexible arrays only if they're "real"
   (e.g. only if they won't affect sizeof()) should test
   #if __glibc_c99_flexarr_available.  */
/* __asm__ ("xyz") is used throughout the headers to rename functions
   at the assembly language level.  This is wrapped by the __REDIRECT
   macro, in order to support compilers that can do this some other
   way.  When compilers don't support asm-names at all, we have to do
   preprocessor tricks instead (which don't have exactly the right
   semantics, but it's the best we can do).
   Example:
   int __REDIRECT(setpgrp, (__pid_t pid, __pid_t pgrp), setpgid); */
/*
#elif __SOME_OTHER_COMPILER__
# define __REDIRECT(name, proto, alias) name proto; 	_Pragma("let " #name " = " #alias)
)
*/
/* GCC and clang have various useful declarations that can be made with
   the '__attribute__' syntax.  All of the ways we use this do fine if
   they are omitted for compilers that don't understand it.  */
/* At some point during the gcc 2.96 development the `malloc' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
/* Tell the compiler which arguments to an allocation function
   indicate the size of the allocation.  */
/* Tell the compiler which argument to an allocation function
   indicates the alignment of the allocation.  */
/* At some point during the gcc 2.96 development the `pure' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
/* This declaration tells the compiler that the value is constant.  */
/* At some point during the gcc 3.1 development the `used' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
/* Since version 3.2, gcc allows marking deprecated functions.  */
/* Since version 4.5, gcc also allows one to specify the message printed
   when a deprecated function is used.  clang claims to be gcc 4.2, but
   may also support this feature.  */
/* At some point during the gcc 2.8 development the `format_arg' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.
   If several `format_arg' attributes are given for the same function, in
   gcc-3.0 and older, all but the last one are ignored.  In newer gccs,
   all designated arguments are considered.  */
/* At some point during the gcc 2.97 development the `strfmon' format
   attribute for functions was introduced.  We don't want to use it
   unconditionally (although this would be possible) since it
   generates warnings.  */
/* The nonnull function attribute marks pointer parameters that
   must not be NULL.  This has the name __nonnull in glibc,
   and __attribute_nonnull__ in files shared with Gnulib to avoid
   collision with a different __nonnull in DragonFlyBSD 5.9.  */
/* The returns_nonnull function attribute marks the return type of the function
   as always being non-null.  */
/* If fortification mode, we warn about unused results of certain
   function calls which can lead to problems.  */
/* Forces a function to be always inlined.  */
/* The Linux kernel defines __always_inline in stddef.h (283d7573), and
   it conflicts with this definition.  Therefore undefine it first to
   allow either header to be included first.  */
/* Associate error messages with the source location of the call site rather
   than with the source location inside the function.  */
/* GCC 4.3 and above with -std=c99 or -std=gnu99 implements ISO C99
   inline semantics, unless -fgnu89-inline is used.  Using __GNUC_STDC_INLINE__
   or __GNUC_GNU_INLINE is not a good enough check for gcc because gcc versions
   older than 4.3 may define these macros and still not guarantee GNU inlining
   semantics.
   clang++ identifies itself as gcc-4.2, but has support for GNU inlining
   semantics, that can be checked for by using the __GNUC_STDC_INLINE_ and
   __GNUC_GNU_INLINE__ macro definitions.  */
/* GCC 4.3 and above allow passing all anonymous arguments of an
   __extern_always_inline function to some other vararg function.  */
/* It is possible to compile containing GCC extensions even if GCC is
   run in pedantic mode if the uses are carefully marked using the
   `__extension__' keyword.  But this is not generally available before
   version 2.8.  */
/* __restrict is known in EGCS 1.2 and above, and in clang.
   It works also in C++ mode (outside of arrays), but only when spelled
   as '__restrict', not 'restrict'.  */
/* ISO C99 also allows to declare arrays as non-overlapping.  The syntax is
     array_name[restrict]
   GCC 3.1 and clang support this.
   This syntax is not usable in C++ mode.  */
/* Describes a char array whose address can safely be passed as the first
   argument to strncpy and strncat, as the char array is not necessarily
   a NUL-terminated string.  */
/* Undefine (also defined in libc-symbols.h).  */
/* Copies attributes from the declaration or type referenced by
   the argument.  */
/* Gnulib avoids including these, as they don't work on non-glibc or
   older glibc platforms.  */
/* Determine the wordsize from the preprocessor defines.  */
/* Both x86-64 and x32 use the 64-bit system call interface.  */
/* Properties of long double type.  ldbl-96 version.
   Copyright (C) 2016-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License  published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* long double is distinct from double, so there is nothing to
   define here.  */
/* __glibc_macro_warning (MESSAGE) issues warning MESSAGE.  This is
   intended for use in preprocessor macros.
   Note: MESSAGE must be a _single_ string; concatenation of string
   literals is not supported.  */
/* Generic selection (ISO C11) is a C-only feature, available in GCC
   since version 4.9.  Previous versions do not provide generic
   selection, even though they might set __STDC_VERSION__ to 201112L,
   when in -std=c11 mode.  Thus, we must check for !defined __GNUC__
   when testing __STDC_VERSION__ for generic selection support.
   On the other hand, Clang also defines __GNUC__, so a clang-specific
   check is required to enable the use of generic selection.  */
/* Designates a 1-based positional argument ref-index of pointer type
   that can be used to access size-index elements of the pointed-to
   array according to access mode, or at least one element when
   size-index is not provided:
     access (access-mode, <ref-index> [, <size-index>])  */
/* For _FORTIFY_SOURCE == 3 we use __builtin_dynamic_object_size, which may
   use the access attribute to get object sizes from function definition
   arguments, so we can't use them on functions we fortify.  Drop the object
   size hints for such functions.  */
/* Designates dealloc as a function to call to deallocate objects
   allocated by the declared function.  */
/* Specify that a function such as setjmp or vfork may return
   twice.  */
/* If we don't have __REDIRECT, prototypes will be missing if
   __USE_FILE_OFFSET64 but not __USE_LARGEFILE[64]. */
/* Decide whether we can define 'extern inline' functions in headers.  */
/* This is here only because every header file already includes this one.
   Get the definitions of all the appropriate `__stub_FUNCTION' symbols.
   <gnu/stubs.h> contains `#define __stub_FUNCTION' when FUNCTION is a stub
   that will always return failure (and set errno to ENOSYS).  */
/* This file is automatically generated.
   This file selects the right generated file of `__stub_FUNCTION' macros
   based on the architecture being compiled for.  */
/* This file is automatically generated.
   It defines a symbol `__stub_FUNCTION' for each function
   in the C library which is a stub, meaning it will fail
   every time called, usually setting errno to ENOSYS.  */
/* Copyright (C) 2001-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* Define the machine-dependent type `jmp_buf'.  x86-64 version.  */
/* Determine the wordsize from the preprocessor defines.  */
/* Both x86-64 and x32 use the 64-bit system call interface.  */
typedef long int __jmp_buf[8];
/* Define struct __jmp_buf_tag.
   Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* Copyright (C) 2001-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* Define the machine-dependent type `jmp_buf'.  x86-64 version.  */
typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
/* Calling environment, plus possibly a saved signal mask.  */
struct __jmp_buf_tag
  {
    /* NOTE: The machine-dependent definitions of `__sigsetjmp'
       assume that a `jmp_buf' begins with a `__jmp_buf' and that
       `__mask_was_saved' follows it.  Do not move these members
       or add others before it.  */
    __jmp_buf __jmpbuf; /* Calling environment.  */
    int __mask_was_saved; /* Saved the signal mask?  */
    __sigset_t __saved_mask; /* Saved signal mask.  */
  };
typedef struct __jmp_buf_tag jmp_buf[1];
/* Store the calling environment in ENV, also saving the signal mask.
   Return 0.  */
extern int setjmp (jmp_buf __env) __attribute__ ((__nothrow__));
/* Store the calling environment in ENV, also saving the
   signal mask if SAVEMASK is nonzero.  Return 0.
   This is the internal name for `sigsetjmp'.  */
extern int __sigsetjmp (struct __jmp_buf_tag __env[1], int __savemask) __attribute__ ((__nothrow__));
/* Store the calling environment in ENV, not saving the signal mask.
   Return 0.  */
extern int _setjmp (struct __jmp_buf_tag __env[1]) __attribute__ ((__nothrow__));
/* Do not save the signal mask.  This is equivalent to the `_setjmp'
   BSD function.  */
/* Jump to the environment saved in ENV, making the
   `setjmp' call there return VAL, or 1 if VAL is 0.  */
extern void longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));
/* Same.  Usually `_longjmp' is used with `_setjmp', which does not save
   the signal mask.  But it is how ENV was saved that determines whether
   `longjmp' restores the mask; `_longjmp' is just an alias.  */
extern void _longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));
/* Use the same type for `jmp_buf' and `sigjmp_buf'.
   The `__mask_was_saved' flag determines whether
   or not `longjmp' will restore the signal mask.  */
typedef struct __jmp_buf_tag sigjmp_buf[1];
/* Store the calling environment in ENV, also saving the
   signal mask if SAVEMASK is nonzero.  Return 0.  */
/* Jump to the environment saved in ENV, making the
   sigsetjmp call there return VAL, or 1 if VAL is 0.
   Restore the signal mask if that sigsetjmp call saved it.
   This is just an alias `longjmp'.  */
extern void siglongjmp (sigjmp_buf __env, int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));
/* Define helper functions to catch unsafe code.  */
/* Plan 9 universal header */
/* Copyright (C) 1989-2023 Free Software Foundation, Inc.
This file is part of GCC.
GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.
GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.
You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */
/*
 * ISO C Standard:  7.15  Variable arguments  <stdarg.h>
 */
/* Define __gnuc_va_list.  */
typedef __builtin_va_list __gnuc_va_list;
/* Define the standard macros for the user,
   if this invocation was from the user program.  */
/* Define va_list, if desired, from __gnuc_va_list. */
/* We deliberately do not define va_list when called from
   stdio.h, because ANSI C says that stdio.h is not supposed to define
   va_list.  stdio.h needs to have access to that data type, 
   but must not use that name.  It should use the name __gnuc_va_list,
   which is safe because it is reserved for the implementation.  */
/* The macro _VA_LIST_ is the same thing used by this file in Ultrix.
   But on BSD NET2 we must not test or define or undef it.
   (Note that the comments in NET 2's ansi.h
   are incorrect for _VA_LIST_--see stdio.h!)  */
/* The macro _VA_LIST_DEFINED is used in Windows NT 3.5  */
/* The macro _VA_LIST is used in SCO Unix 3.2.  */
/* The macro _VA_LIST_T_H is used in the Bull dpx2  */
/* The macro __va_list__ is used by BeOS.  */
typedef __gnuc_va_list va_list;
/*
 * mem routines
 */
extern void *memccpy(void *, const void *, int, usize);
extern void *memset(void *, int, usize);
extern int memcmp(const void *, const void *, usize);
extern void *memcpy(void *, const void *, usize);
extern void *memmove(void *, const void *, usize);
extern void *memchr(const void *, int, usize);
/*
 * string routines
 */
extern char *strcat(char *, char *);
extern char *strchr(char *, int);
extern int strcmp(char *, char *);
extern char *strcpy(char *, char *);
extern char *strecpy(char *, char *, char *);
extern char *strdup(char *);
extern char *strncat(char *, char *, long);
extern char *strncpy(char *, char *, long);
extern int strncmp(char *, char *, long);
extern char *strpbrk(char *, char *);
extern char *strrchr(char *, int);
extern char *strtok(char *, char *);
extern long strlen(char *);
extern long strspn(char *, char *);
extern long strcspn(char *, char *);
extern char *strstr(char *, char *);
extern int cistrncmp(char *, char *, int);
extern int cistrcmp(char *, char *);
extern char *cistrstr(char *, char *);
extern int tokenize(char *, char **, int);
enum {
  UTFmax = 4, /* maximum bytes per rune */
  Runesync = 0x80, /* cannot represent part of a UTF sequence (<) */
  Runeself = 0x80, /* rune and UTF sequences are the same (<) */
  Runeerror = 0xFFFD, /* decoding error in UTF */
  Runemax = 0x10FFFF, /* 21 bit rune */
  Runemask = 0x1FFFFF, /* bits used by runes (see grep) */
};
/*
 * rune routines
 */
extern int runetochar(char *, Rune *);
extern int chartorune(Rune *, char *);
extern int runelen(long);
extern int runenlen(Rune *, int);
extern int fullrune(char *, int);
extern int utflen(char *);
extern int utfnlen(char *, long);
extern char *utfrune(char *, long);
extern char *utfrrune(char *, long);
extern char *utfutf(char *, char *);
extern char *utfecpy(char *, char *, char *);
extern Rune *runestrcat(Rune *, Rune *);
extern Rune *runestrchr(Rune *, Rune);
extern int runestrcmp(Rune *, Rune *);
extern Rune *runestrcpy(Rune *, Rune *);
extern Rune *runestrncpy(Rune *, Rune *, long);
extern Rune *runestrecpy(Rune *, Rune *, Rune *);
extern Rune *runestrdup(Rune *);
extern Rune *runestrncat(Rune *, Rune *, long);
extern int runestrncmp(Rune *, Rune *, long);
extern Rune *runestrrchr(Rune *, Rune);
extern long runestrlen(Rune *);
extern Rune *runestrstr(Rune *, Rune *);
enum { Maxnormctx = 1 + 30 };
typedef struct Norm Norm;
struct Norm {
  long (*getrune)(void *);
  void *ctx;
  struct {
    Rune *e;
    Rune a[Maxnormctx];
  } ibuf, obuf;
  int compose;
};
extern void norminit(Norm *, int, void *, long (*getrune)(void *));
extern long normpull(Norm *, Rune *, long, int);
extern long runecomp(Rune *, long, Rune *, long);
extern long runedecomp(Rune *, long, Rune *, long);
extern long utfcomp(char *, long, char *, long);
extern long utfdecomp(char *, long, char *, long);
extern Rune *runewbreak(Rune *);
extern char *utfwbreak(char *);
extern Rune *runegbreak(Rune *);
extern char *utfgbreak(char *);
extern Rune tolowerrune(Rune);
extern Rune totitlerune(Rune);
extern Rune toupperrune(Rune);
extern int isalpharune(Rune);
extern int islowerrune(Rune);
extern int isspacerune(Rune);
extern int istitlerune(Rune);
extern int isupperrune(Rune);
extern int isdigitrune(Rune);
/*
 * malloc
 */
extern void *malloc(ulong);
extern void *mallocz(ulong, int);
extern void free(void *);
extern ulong msize(void *);
extern void *mallocalign(ulong, ulong, long, ulong);
extern void *calloc(ulong, ulong);
extern void *realloc(void *, ulong);
extern void setmalloctag(void *, uintptr);
extern void setrealloctag(void *, uintptr);
extern uintptr getmalloctag(void *);
extern uintptr getrealloctag(void *);
extern void *malloctopoolblock(void *);
/*
 * print routines
 */
typedef struct Fmt Fmt;
struct Fmt {
  uchar runes; /* output buffer is runes or chars? */
  void *start; /* of buffer */
  void *to; /* current place in the buffer */
  void *stop; /* end of the buffer; overwritten if flush fails */
  int (*flush)(Fmt *); /* called when to == stop */
  void *farg; /* to make flush a closure */
  int nfmt; /* num chars formatted so far */
  va_list args; /* args passed to dofmt */
  int r; /* % format Rune */
  int width; /* width of format */
  int prec; /* precision of format */
  ulong flags;
};
enum {
  FmtWidth = 1,
  FmtLeft = FmtWidth << 1,
  FmtPrec = FmtLeft << 1,
  FmtSharp = FmtPrec << 1,
  FmtSpace = FmtSharp << 1,
  FmtSign = FmtSpace << 1,
  FmtZero = FmtSign << 1,
  FmtUnsigned = FmtZero << 1,
  FmtShort = FmtUnsigned << 1,
  FmtLong = FmtShort << 1,
  FmtVLong = FmtLong << 1,
  FmtComma = FmtVLong << 1,
  FmtByte = FmtComma << 1,
  FmtFlag = FmtByte << 1
};
extern int print(char *, ...);
extern char *seprint(char *, char *, char *, ...);
extern char *vseprint(char *, char *, char *, va_list);
extern int snprint(char *, int, char *, ...);
extern int vsnprint(char *, int, char *, va_list);
extern char *smprint(char *, ...);
extern char *vsmprint(char *, va_list);
extern int sprint(char *, char *, ...);
extern int fprint(int, char *, ...);
extern int vfprint(int, char *, va_list);
extern int runesprint(Rune *, char *, ...);
extern int runesnprint(Rune *, int, char *, ...);
extern int runevsnprint(Rune *, int, char *, va_list);
extern Rune *runeseprint(Rune *, Rune *, char *, ...);
extern Rune *runevseprint(Rune *, Rune *, char *, va_list);
extern Rune *runesmprint(char *, ...);
extern Rune *runevsmprint(char *, va_list);
extern int fmtfdinit(Fmt *, int, char *, int);
extern int fmtfdflush(Fmt *);
extern int fmtstrinit(Fmt *);
extern char *fmtstrflush(Fmt *);
extern int runefmtstrinit(Fmt *);
extern Rune *runefmtstrflush(Fmt *);
extern int fmtinstall(int, int (*)(Fmt *));
extern int dofmt(Fmt *, char *);
extern int dorfmt(Fmt *, Rune *);
extern int fmtprint(Fmt *, char *, ...);
extern int fmtvprint(Fmt *, char *, va_list);
extern int fmtrune(Fmt *, int);
extern int fmtstrcpy(Fmt *, char *);
extern int fmtrunestrcpy(Fmt *, Rune *);
/*
 * error string for %r
 * supplied on per os basis, not part of fmt library
 */
extern int errfmt(Fmt *f);
/*
 * quoted strings
 */
extern char *unquotestrdup(char *);
extern Rune *unquoterunestrdup(Rune *);
extern char *quotestrdup(char *);
extern Rune *quoterunestrdup(Rune *);
extern int quotestrfmt(Fmt *);
extern int quoterunestrfmt(Fmt *);
extern void quotefmtinstall(void);
extern int (*doquote)(int);
extern int needsrcquote(int);
/*
 * random number
 */
extern void srand(long);
extern int rand(void);
extern int nrand(int);
extern long lrand(void);
extern long lnrand(long);
extern double frand(void);
extern ulong truerand(void); /* uses /dev/random */
extern ulong ntruerand(ulong); /* uses /dev/random */
/*
 * math
 */
extern ulong getfcr(void);
extern void setfsr(ulong);
extern ulong getfsr(void);
extern void setfcr(ulong);
extern double NaN(void);
extern double Inf(int);
extern int isNaN(double);
extern int isInf(double, int);
extern ulong umuldiv(ulong, ulong, ulong);
extern long muldiv(long, long, long);
extern double pow(double, double);
extern double atan2(double, double);
extern double fabs(double);
extern double atan(double);
extern double log(double);
extern double log10(double);
extern double exp(double);
extern double floor(double);
extern double ceil(double);
extern double hypot(double, double);
extern double sin(double);
extern double cos(double);
extern double tan(double);
extern double asin(double);
extern double acos(double);
extern double sinh(double);
extern double cosh(double);
extern double tanh(double);
extern double sqrt(double);
extern double fmod(double, double);
/*
 * Time-of-day
 */
typedef struct Tzone Tzone;
typedef struct Tm {
  int nsec; /* nseconds (range 0...1e9) */
  int sec; /* seconds (range 0..60) */
  int min; /* minutes (0..59) */
  int hour; /* hours (0..23) */
  int mday; /* day of the month (1..31) */
  int mon; /* month of the year (0..11) */
  int year; /* year A.D. */
  int wday; /* day of week (0..6, Sunday = 0) */
  int yday; /* day of year (0..365) */
  char zone[16]; /* time zone name */
  int tzoff; /* time zone delta from GMT */
  Tzone *tz; /* time zone associated with this date */
} Tm;
typedef struct Tmfmt {
  char *fmt;
  Tm *tm;
} Tmfmt;
extern Tzone *tzload(char *name);
extern Tm *tmnow(Tm *, Tzone *);
extern Tm *tmtime(Tm *, vlong, Tzone *);
extern Tm *tmtimens(Tm *, vlong, int, Tzone *);
extern Tm *tmparse(Tm *, char *, char *, Tzone *, char **ep);
extern vlong tmnorm(Tm *);
extern Tmfmt tmfmt(Tm *, char *);
extern void tmfmtinstall(void);
extern Tm *gmtime(long);
extern Tm *localtime(long);
extern char *asctime(Tm *);
extern char *ctime(long);
extern double cputime(void);
extern long times(long *);
extern long tm2sec(Tm *);
extern vlong nsec(void);
extern void (*cycles)(uvlong *); /* 64-bit value of the cycle counter if there
                                    is one, 0 if there isn't */
/*
 * one-of-a-kind
 */
enum {
  PNPROC = 1,
  PNGROUP = 2,
};
extern void _assert(char *);
extern int abs(int);
extern int atexit(void (*)(void));
extern void atexitdont(void (*)(void));
extern int atnotify(int (*)(void *, char *), int);
extern double atof(char *);
extern int atoi(char *);
extern long atol(char *);
extern vlong atoll(char *);
extern double charstod(int (*)(void *), void *);
extern char *cleanname(char *);
extern int decrypt(void *, void *, int);
extern int encrypt(void *, void *, int);
extern int dec64(uchar *, int, char *, int);
extern int enc64(char *, int, uchar *, int);
extern int dec64x(uchar *, int, char *, int, int (*)(int));
extern int enc64x(char *, int, uchar *, int, int (*)(int));
extern int dec32(uchar *, int, char *, int);
extern int enc32(char *, int, uchar *, int);
extern int dec32x(uchar *, int, char *, int, int (*)(int));
extern int enc32x(char *, int, uchar *, int, int (*)(int));
extern int dec16(uchar *, int, char *, int);
extern int enc16(char *, int, uchar *, int);
extern int dec64chr(int);
extern int enc64chr(int);
extern int dec32chr(int);
extern int enc32chr(int);
extern int dec16chr(int);
extern int enc16chr(int);
extern int encodefmt(Fmt *);
extern _Noreturn void exits(char *);
extern double frexp(double, int *);
extern uintptr getcallerpc(void *);
extern char *getenv(char *);
extern int getfields(char *, char **, int, int, char *);
extern int gettokens(char *, char **, int, char *);
extern char *getuser(void);
extern char *getwd(char *, int);
extern int iounit(int);
extern long labs(long);
extern double ldexp(double, int);
extern _Noreturn void longjmp(jmp_buf, int);
extern char *mktemp(char *);
extern double modf(double, double *);
extern int netcrypt(void *, void *);
extern void notejmp(void *, jmp_buf, int);
extern void perror(char *);
extern int postnote(int, int, char *);
extern double pow10(int);
extern int putenv(char *, char *);
extern void qsort(void *, usize, usize, int (*)(void *, void *));
extern double strtod(char *, char **);
extern long strtol(char *, char **, int);
extern ulong strtoul(char *, char **, int);
extern vlong strtoll(char *, char **, int);
extern uvlong strtoull(char *, char **, int);
extern _Noreturn void sysfatal(char *, ...);
extern long time(long *);
extern int tolower(int);
extern int toupper(int);
/*
 *  profiling
 */
enum {
  Profoff, /* No profiling */
  Profuser, /* Measure user time only (default) */
  Profkernel, /* Measure user + kernel time */
  Proftime, /* Measure total time */
  Profsample, /* Use clock interrupt to sample (default when there is no cycle
                 counter) */
}; /* what */
extern void prof(void (*fn)(void *), void *arg, int entries, int what);
/*
 *  synchronization
 */
typedef struct Lock {
  int val;
} Lock;
extern int _tas(int *);
extern void lock(Lock *);
extern void unlock(Lock *);
extern int canlock(Lock *);
typedef struct QLp QLp;
struct QLp {
  int inuse;
  int state;
  QLp *next;
};
typedef struct QLock {
  Lock lock;
  int locked;
  QLp *head;
  QLp *tail;
} QLock;
extern void qlock(QLock *);
extern void qunlock(QLock *);
extern int canqlock(QLock *);
extern void
_qlockinit(void *(*)(void *, void *)); /* called only by the thread library */
typedef struct RWLock {
  Lock lock;
  int readers; /* number of readers */
  int writer; /* number of writers */
  QLp *head; /* list of waiting processes */
  QLp *tail;
} RWLock;
extern void rlock(RWLock *);
extern void runlock(RWLock *);
extern int canrlock(RWLock *);
extern void wlock(RWLock *);
extern void wunlock(RWLock *);
extern int canwlock(RWLock *);
typedef struct Rendez {
  QLock *l;
  QLp *head;
  QLp *tail;
} Rendez;
extern void rsleep(Rendez *); /* unlocks r->l, sleeps, locks r->l again */
extern int rwakeup(Rendez *);
extern int rwakeupall(Rendez *);
extern void **privalloc(void);
extern void procsetname(char *, ...);
/*
 * atomic operations
 */
typedef struct Along {
  long v;
} Along;
typedef struct Avlong {
  vlong v;
} Avlong;
typedef struct Aptr {
  void *v;
} Aptr;
extern long agetl(Along *);
extern vlong agetv(Avlong *);
extern void *agetp(Aptr *);
extern long aswapl(Along *, long);
extern vlong aswapv(Avlong *, vlong);
extern void *aswapp(Aptr *, void *);
extern long aincl(Along *, long);
extern vlong aincv(Avlong *, vlong);
extern int acasl(Along *, long, long);
extern int acasv(Avlong *, vlong, vlong);
extern int acasp(Aptr *, void *, void *);
extern void coherence(void);
/*
 *  network dialing
 */
extern int accept(int, char *);
extern int announce(char *, char *);
extern int dial(char *, char *, char *, int *);
extern void setnetmtpt(char *, int, char *);
extern int hangup(int);
extern int listen(char *, char *);
extern char *netmkaddr(char *, char *, char *);
extern char *netmkaddrbuf(char *, char *, char *, char *, int);
extern int reject(int, char *, char *);
/*
 *  encryption
 */
extern int pushtls(int, char *, char *, int, char *, char *);
/*
 *  network services
 */
typedef struct NetConnInfo NetConnInfo;
struct NetConnInfo {
  char *dir; /* connection directory */
  char *root; /* network root */
  char *spec; /* binding spec */
  char *lsys; /* local system */
  char *lserv; /* local service */
  char *rsys; /* remote system */
  char *rserv; /* remote service */
  char *laddr; /* local address */
  char *raddr; /* remote address */
};
extern NetConnInfo *getnetconninfo(char *, int);
extern void freenetconninfo(NetConnInfo *);
extern int idn2utf(char *, char *, int);
extern int utf2idn(char *, char *, int);
/*
 * system calls
 *
 */
/* Segattch */
/* bits in Qid.type */
/* bits in Dir.mode */
/* rfork */
enum {
  RFNAMEG = (1 << 0),
  RFENVG = (1 << 1),
  RFFDG = (1 << 2),
  RFNOTEG = (1 << 3),
  RFPROC = (1 << 4),
  RFMEM = (1 << 5),
  RFNOWAIT = (1 << 6),
  RFCNAMEG = (1 << 10),
  RFCENVG = (1 << 11),
  RFCFDG = (1 << 12),
  RFREND = (1 << 13),
  RFNOMNT = (1 << 14)
};
typedef struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
} Qid;
typedef struct Dir {
  /* system-modified data */
  ushort type; /* server type */
  uint dev; /* server subtype */
  /* file data */
  Qid qid; /* unique id from server */
  ulong mode; /* permissions */
  ulong atime; /* last read time */
  ulong mtime; /* last write time */
  vlong length; /* file length */
  char *name; /* last element of path */
  char *uid; /* owner name */
  char *gid; /* group name */
  char *muid; /* last modifier name */
} Dir;
typedef struct Waitmsg {
  int pid; /* of loved one */
  ulong time[3]; /* of loved one & descendants */
  char *msg;
} Waitmsg;
typedef struct IOchunk {
  void *addr;
  ulong len;
} IOchunk;
extern _Noreturn void _exits(char *);
extern _Noreturn void abort(void);
extern int access(char *, int);
extern long alarm(ulong);
extern int await(char *, int);
extern int bind(char *, char *, int);
extern int brk(void *);
extern int chdir(char *);
extern int close(int);
extern int create(char *, int, ulong);
extern int dup(int, int);
extern int errstr(char *, uint);
extern int exec(char *, char *[]);
extern int execl(char *, ...);
extern int fork(void);
extern int rfork(int);
extern int fauth(int, char *);
extern int fstat(int, uchar *, int);
extern int fwstat(int, uchar *, int);
extern int fversion(int, int, char *, int);
extern int mount(int, int, char *, int, char *);
extern int unmount(char *, char *);
extern int noted(int);
extern int notify(void (*)(void *, char *));
extern int open(char *, int);
extern int fd2path(int, char *, int);
extern int pipe(int *);
extern long pread(int, void *, long, vlong);
extern long preadv(int, IOchunk *, int, vlong);
extern long pwrite(int, void *, long, vlong);
extern long pwritev(int, IOchunk *, int, vlong);
extern long read(int, void *, long);
extern long readn(int, void *, long);
extern long readv(int, IOchunk *, int);
extern int remove(char *);
extern void *sbrk(usize);
extern long oseek(int, long, int);
extern vlong seek(int, vlong, int);
extern void *segattach(int, char *, void *, ulong);
extern void *segbrk(void *, void *);
extern int segdetach(void *);
extern int segflush(void *, ulong);
extern int segfree(void *, ulong);
extern int semacquire(long *, int);
extern long semrelease(long *, long);
extern int sleep(long);
extern int stat(char *, uchar *, int);
extern int tsemacquire(long *, ulong);
extern Waitmsg *wait(void);
extern int waitpid(void);
extern long write(int, void *, long);
extern long writev(int, IOchunk *, int);
extern int wstat(char *, uchar *, int);
extern void *rendezvous(void *, void *);
extern Dir *dirstat(char *);
extern Dir *dirfstat(int);
extern int dirwstat(char *, Dir *);
extern int dirfwstat(int, Dir *);
extern long dirread(int, Dir **);
extern void nulldir(Dir *);
extern long dirreadall(int, Dir **);
extern int getpid(void);
extern int getppid(void);
extern void rerrstr(char *, uint);
extern char *sysname(void);
extern void werrstr(char *, ...);
extern long ainc(long *);
extern long adec(long *);
extern char *argv0;
/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
extern char end[];
/* Plan 9 universal header */
/*
 * dofmt -- format to a buffer
...
 * the number of characters formatted is returned,
 * or -1 if there was an error.
 * if the buffer is ever filled, flush is called.
 * it should reset the buffer and return whether formatting should continue.
 */
typedef struct Quoteinfo Quoteinfo;
struct Quoteinfo {
  int quoted; /* if set, string must be quoted */
  int nrunesin; /* number of input runes that can be accepted */
  int nbytesin; /* number of input bytes that can be accepted */
  int nrunesout; /* number of runes that will be generated */
  int nbytesout; /* number of bytes that will be generated */
};
void *_fmtflush(Fmt *, void *, int);
void *_fmtdispatch(Fmt *, void *, int);
int _floatfmt(Fmt *, double);
int _fmtpad(Fmt *, int);
int _rfmtpad(Fmt *, int);
int _fmtFdFlush(Fmt *);
int _efgfmt(Fmt *);
int _charfmt(Fmt *);
int _countfmt(Fmt *);
int _flagfmt(Fmt *);
int _percentfmt(Fmt *);
int _ifmt(Fmt *);
int _runefmt(Fmt *);
int _runesfmt(Fmt *);
int _strfmt(Fmt *);
int _badfmt(Fmt *);
int _fmtcpy(Fmt *, void *, int, int);
int _fmtrcpy(Fmt *, void *, int n);
void _fmtlock(void);
void _fmtunlock(void);
/*
 * How many bytes of output UTF will be produced by quoting (if necessary) this string?
 * How many runes? How much of the input will be consumed?
 * The parameter q is filled in by _quotesetup.
 * The string may be UTF or Runes (s or r).
 * Return count does not include NUL.
 * Terminate the scan at the first of:
 *	NUL in input
 *	count exceeded in input
 *	count exceeded on output
 * *ninp is set to number of input bytes accepted.
 * nin may be <0 initially, to avoid checking input by count.
 */
void
_quotesetup(char *s, Rune *r, int nin, int nout, Quoteinfo *q, int sharp, int runesout)
{
 int w;
 Rune c;
 q->quoted = 0;
 q->nbytesout = 0;
 q->nrunesout = 0;
 q->nbytesin = 0;
 q->nrunesin = 0;
 if(sharp || nin==0 || (s && *s=='\0') || (r && *r=='\0')){
  if(nout < 2)
   return;
  q->quoted = 1;
  q->nbytesout = 2;
  q->nrunesout = 2;
 }
 for(; nin!=0; nin--){
  if(s)
   w = chartorune(&c, s);
  else{
   c = *r;
   w = runelen(c);
  }
  if(c == '\0')
   break;
  if(runesout){
   if(q->nrunesout+1 > nout)
    break;
  }else{
   if(q->nbytesout+w > nout)
    break;
  }
  if((c <= L' ') || (c == L'\'') || (doquote!=((void *)0) && doquote(c))){
   if(!q->quoted){
    if(runesout){
     if(1+q->nrunesout+1+1 > nout) /* no room for quotes */
      break;
    }else{
     if(1+q->nbytesout+w+1 > nout) /* no room for quotes */
      break;
    }
    q->nrunesout += 2; /* include quotes */
    q->nbytesout += 2; /* include quotes */
    q->quoted = 1;
   }
   if(c == '\'') {
    if(runesout){
     if(1+q->nrunesout+1 > nout) /* no room for quotes */
      break;
    }else{
     if(1+q->nbytesout+w > nout) /* no room for quotes */
      break;
    }
    q->nbytesout++;
    q->nrunesout++; /* quotes reproduce as two characters */
   }
  }
  /* advance input */
  if(s)
   s += w;
  else
   r++;
  q->nbytesin += w;
  q->nrunesin++;
  /* advance output */
  q->nbytesout += w;
  q->nrunesout++;
 }
}
static int
qstrfmt(char *sin, Rune *rin, Quoteinfo *q, Fmt *f)
{
 Rune r, *rm, *rme;
 char *t, *s, *m, *me;
 Rune *rt, *rs;
 ulong fl;
 int nc, w;
 m = sin;
 me = m + q->nbytesin;
 rm = rin;
 rme = rm + q->nrunesin;
 w = f->width;
 fl = f->flags;
 if(f->runes){
  if(!(fl & FmtLeft) && _rfmtpad(f, w - q->nrunesout) < 0)
   return -1;
 }else{
  if(!(fl & FmtLeft) && _fmtpad(f, w - q->nbytesout) < 0)
   return -1;
 }
 t = f->to;
 s = f->stop;
 rt = f->to;
 rs = f->stop;
 if(f->runes)
  do { if (rt + 1 > (Rune *)rs) { rt = _fmtflush(f, rt, sizeof(Rune)); if (rt != ((void *)0)) rs = f->stop; else return -1; } *rt++ = '\''; } while (0);
 else
  do { Rune _rune; int _runelen; if (t + UTFmax > (char *)s && t + (_runelen = runelen('\'')) > (char *)s) { t = _fmtflush(f, t, _runelen); if (t != ((void *)0)) s = f->stop; else return -1; } if ('\'' < Runeself) *t++ = '\''; else { _rune = '\''; t += runetochar(t, &_rune); } } while (0);
 for(nc = q->nrunesin; nc > 0; nc--){
  if(sin){
   r = *(uchar*)m;
   if(r < Runeself)
    m++;
   else if((me - m) >= UTFmax || fullrune(m, me-m))
    m += chartorune(&r, m);
   else
    break;
  }else{
   if(rm >= rme)
    break;
   r = *(uchar*)rm++;
  }
  if(f->runes){
   do { if (rt + 1 > (Rune *)rs) { rt = _fmtflush(f, rt, sizeof(Rune)); if (rt != ((void *)0)) rs = f->stop; else return -1; } *rt++ = r; } while (0);
   if(r == '\'')
    do { if (rt + 1 > (Rune *)rs) { rt = _fmtflush(f, rt, sizeof(Rune)); if (rt != ((void *)0)) rs = f->stop; else return -1; } *rt++ = r; } while (0);
  }else{
   do { Rune _rune; int _runelen; if (t + UTFmax > (char *)s && t + (_runelen = runelen(r)) > (char *)s) { t = _fmtflush(f, t, _runelen); if (t != ((void *)0)) s = f->stop; else return -1; } if (r < Runeself) *t++ = r; else { _rune = r; t += runetochar(t, &_rune); } } while (0);
   if(r == '\'')
    do { Rune _rune; int _runelen; if (t + UTFmax > (char *)s && t + (_runelen = runelen(r)) > (char *)s) { t = _fmtflush(f, t, _runelen); if (t != ((void *)0)) s = f->stop; else return -1; } if (r < Runeself) *t++ = r; else { _rune = r; t += runetochar(t, &_rune); } } while (0);
  }
 }
 if(f->runes){
  do { if (rt + 1 > (Rune *)rs) { rt = _fmtflush(f, rt, sizeof(Rune)); if (rt != ((void *)0)) rs = f->stop; else return -1; } *rt++ = '\''; } while (0);
  if (rs) { };
  f->nfmt += rt - (Rune *)f->to;
  f->to = rt;
  if(fl & FmtLeft && _rfmtpad(f, w - q->nrunesout) < 0)
   return -1;
 }else{
  do { Rune _rune; int _runelen; if (t + UTFmax > (char *)s && t + (_runelen = runelen('\'')) > (char *)s) { t = _fmtflush(f, t, _runelen); if (t != ((void *)0)) s = f->stop; else return -1; } if ('\'' < Runeself) *t++ = '\''; else { _rune = '\''; t += runetochar(t, &_rune); } } while (0);
  if (s) { };
  f->nfmt += t - (char *)f->to;
  f->to = t;
  if(fl & FmtLeft && _fmtpad(f, w - q->nbytesout) < 0)
   return -1;
 }
 return 0;
}
int
_quotestrfmt(int runesin, Fmt *f)
{
 int nin, outlen;
 Rune *r;
 char *s;
 Quoteinfo q;
 nin = -1;
 if(f->flags&FmtPrec)
  nin = f->prec;
 if(runesin){
  r = __builtin_va_arg(f->args,Rune *);
  s = ((void *)0);
 }else{
  s = __builtin_va_arg(f->args,char *);
  r = ((void *)0);
 }
 if(!s && !r)
  return _fmtcpy(f, "<nil>", 5, 5);
 if(f->flush)
  outlen = 0x7FFFFFFF; /* if we can flush, no output limit */
 else if(f->runes)
  outlen = (Rune*)f->stop - (Rune*)f->to;
 else
  outlen = (char*)f->stop - (char*)f->to;
 _quotesetup(s, r, nin, outlen, &q, f->flags&FmtSharp, f->runes);
//print("bytes in %d bytes out %d runes in %d runesout %d\n", q.nbytesin, q.nbytesout, q.nrunesin, q.nrunesout);
 if(runesin){
  if(!q.quoted)
   return _fmtrcpy(f, r, q.nrunesin);
  return qstrfmt(((void *)0), r, &q, f);
 }
 if(!q.quoted)
  return _fmtcpy(f, s, q.nrunesin, q.nbytesin);
 return qstrfmt(s, ((void *)0), &q, f);
}
int
quotestrfmt(Fmt *f)
{
 return _quotestrfmt(0, f);
}
int
quoterunestrfmt(Fmt *f)
{
 return _quotestrfmt(1, f);
}
void
quotefmtinstall(void)
{
 fmtinstall('q', quotestrfmt);
 fmtinstall('Q', quoterunestrfmt);
}
int
_needsquotes(char *s, int *quotelenp)
{
 Quoteinfo q;
 _quotesetup(s, ((void *)0), -1, 0x7FFFFFFF, &q, 0, 0);
 *quotelenp = q.nbytesout;
 return q.quoted;
}
int
_runeneedsquotes(Rune *r, int *quotelenp)
{
 Quoteinfo q;
 _quotesetup(((void *)0), r, -1, 0x7FFFFFFF, &q, 0, 0);
 *quotelenp = q.nrunesout;
 return q.quoted;
}
