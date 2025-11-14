#pragma once

#ifndef __ASSEMBLER__
#include "../include/ureg.h"
#endif

#define UREG_RAX	0x00
#define UREG_RBX	0x08
#define UREG_RCX	0x10
#define UREG_RDX	0x18
#define UREG_RSI	0x20
#define UREG_RDI	0x28
#define UREG_RBP	0x30
#define UREG_R8	0x38
#define UREG_R9	0x40
#define UREG_R10	0x48
#define UREG_R11	0x50
#define UREG_R12	0x58
#define UREG_R13	0x60
#define UREG_R14	0x68
#define UREG_R15	0x70
#define UREG_DS	0x78
#define UREG_ES	0x7A
#define UREG_FS	0x7C
#define UREG_GS	0x7E
#define UREG_TYPE	0x80
#define UREG_ERR	0x88
#define UREG_RIP	0x90
#define UREG_CS	0x98
#define UREG_EFL	0xA0
#define UREG_RSP	0xA8
#define UREG_SS	0xB0
#define UREG_SIZE	0xB8
