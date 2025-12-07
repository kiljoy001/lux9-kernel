	.file	"test_syscalls.c"
	.text
.Ltext0:
	.file 0 "/home/scott/Repo/lux9-kernel" "test_syscalls.c"
	.p2align 4
	.globl	tsemacquire
	.type	tsemacquire, @function
tsemacquire:
.LVL0:
.LFB50:
	.file 1 "test_syscalls.c"
	.loc 1 4 49 view -0
	.cfi_startproc
	.loc 1 4 49 is_stmt 0 view .LVU1
	endbr64
	.loc 1 4 51 is_stmt 1 view .LVU2
	.loc 1 4 61 is_stmt 0 view .LVU3
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE50:
	.size	tsemacquire, .-tsemacquire
	.p2align 4
	.globl	sys_nsec
	.type	sys_nsec, @function
sys_nsec:
.LFB51:
	.loc 1 5 23 is_stmt 1 view -0
	.cfi_startproc
	endbr64
	.loc 1 5 25 view .LVU5
	.loc 1 5 35 is_stmt 0 view .LVU6
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE51:
	.size	sys_nsec, .-sys_nsec
.Letext0:
	.file 2 "/usr/include/x86_64-linux-gnu/sys/types.h"
	.file 3 "/home/scott/Repo/lux9-kernel/../plan9port/include/u.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x111
	.value	0x5
	.byte	0x1
	.byte	0x8
	.long	.Ldebug_abbrev0
	.uleb128 0x4
	.long	.LASF18
	.byte	0x1d
	.long	.LASF0
	.long	.LASF1
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.long	.Ldebug_line0
	.uleb128 0x1
	.byte	0x1
	.byte	0x8
	.long	.LASF2
	.uleb128 0x1
	.byte	0x2
	.byte	0x7
	.long	.LASF3
	.uleb128 0x1
	.byte	0x4
	.byte	0x7
	.long	.LASF4
	.uleb128 0x1
	.byte	0x8
	.byte	0x7
	.long	.LASF5
	.uleb128 0x1
	.byte	0x1
	.byte	0x6
	.long	.LASF6
	.uleb128 0x1
	.byte	0x2
	.byte	0x5
	.long	.LASF7
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x1
	.byte	0x8
	.byte	0x5
	.long	.LASF8
	.uleb128 0x6
	.byte	0x8
	.uleb128 0x1
	.byte	0x1
	.byte	0x6
	.long	.LASF9
	.uleb128 0x1
	.byte	0x8
	.byte	0x5
	.long	.LASF10
	.uleb128 0x2
	.long	.LASF15
	.byte	0x2
	.byte	0x94
	.byte	0x1b
	.long	0x43
	.uleb128 0x1
	.byte	0x8
	.byte	0x7
	.long	.LASF11
	.uleb128 0x1
	.byte	0x10
	.byte	0x4
	.long	.LASF12
	.uleb128 0x1
	.byte	0x4
	.byte	0x4
	.long	.LASF13
	.uleb128 0x1
	.byte	0x8
	.byte	0x4
	.long	.LASF14
	.uleb128 0x2
	.long	.LASF16
	.byte	0x3
	.byte	0x8f
	.byte	0x1c
	.long	0x82
	.uleb128 0x7
	.long	.LASF19
	.byte	0x1
	.byte	0x5
	.byte	0x8
	.long	0x9e
	.quad	.LFB51
	.quad	.LFE51-.LFB51
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x8
	.long	.LASF20
	.byte	0x1
	.byte	0x4
	.byte	0x6
	.long	0x5f
	.quad	.LFB50
	.quad	.LFE50-.LFB50
	.uleb128 0x1
	.byte	0x9c
	.long	0x10e
	.uleb128 0x3
	.string	"s"
	.byte	0x18
	.long	0x66
	.uleb128 0x1
	.byte	0x55
	.uleb128 0x9
	.long	.LASF17
	.byte	0x1
	.byte	0x4
	.byte	0x21
	.long	0x10e
	.uleb128 0x1
	.byte	0x54
	.uleb128 0x3
	.string	"ms"
	.byte	0x2d
	.long	0x76
	.uleb128 0x1
	.byte	0x51
	.byte	0
	.uleb128 0xa
	.byte	0x8
	.long	0x5f
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0x21
	.sleb128 4
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x1f
	.uleb128 0x1b
	.uleb128 0x1f
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x8
	.byte	0
	.value	0
	.value	0
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF10:
	.string	"long long int"
.LASF4:
	.string	"unsigned int"
.LASF5:
	.string	"long unsigned int"
.LASF11:
	.string	"long long unsigned int"
.LASF17:
	.string	"addr"
.LASF2:
	.string	"unsigned char"
.LASF9:
	.string	"char"
.LASF16:
	.string	"uvlong"
.LASF8:
	.string	"long int"
.LASF14:
	.string	"double"
.LASF15:
	.string	"ulong"
.LASF3:
	.string	"short unsigned int"
.LASF6:
	.string	"signed char"
.LASF12:
	.string	"long double"
.LASF13:
	.string	"float"
.LASF20:
	.string	"tsemacquire"
.LASF19:
	.string	"sys_nsec"
.LASF7:
	.string	"short int"
.LASF18:
	.string	"GNU C11 13.3.0 -mtune=generic -march=x86-64 -ggdb -O2 -std=gnu11 -fno-omit-frame-pointer -fsigned-char -fno-common -fasynchronous-unwind-tables -fstack-protector-strong -fstack-clash-protection -fcf-protection"
	.section	.debug_line_str,"MS",@progbits,1
.LASF0:
	.string	"test_syscalls.c"
.LASF1:
	.string	"/home/scott/Repo/lux9-kernel"
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
