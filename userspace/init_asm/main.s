// Minimal Init in Go Assembly
// Plan 9 ABI: syscall args on stack at SP+8, SP+16...
// Calling convention: Caller PUSHes args (last to first), then CALL.
// CALL pushes ret addr. So SP->ret, SP+8->arg0.
// Direct SYSCALL instruction: SP->arg0 (if no push) or SP->ret (if push).
// We must match what kernel expects (SP+8). So we push dummy.

// #include "textflag.h" // Removed as go tool asm does not find it easily

// Plan 9 syscall numbers
#define SYS_BIND 2
#define SYS_EXEC 7
#define SYS_OPEN 14
#define SYS_PWRITE 51
#define SYS_EXITS 8

#define OREAD 0
#define OWRITE 1
#define ORDWR 2

#define MREPL 0
#define MBEFORE 1
#define MAFTER 2

// Go ASM flags (from textflag.h)
#define NOSPLIT 4
#define RODATA 8
#define NOPTR 16

TEXT ·_start(SB),NOSPLIT,$0
	// 1. Open #c/cons
	// open("#c/cons", OWRITE)
	PUSHQ	$OWRITE
	LEAQ	cons_path(SB), CX
	PUSHQ	CX
	PUSHQ	$0
	MOVQ	$SYS_OPEN, BP
	SYSCALL
	ADDQ	$24, SP
	
	MOVQ	AX, R14 // fd

	// 2. Write Banner
	// pwrite(fd, msg, len, -1)
	PUSHQ	$-1
	PUSHQ	$16
	LEAQ	msg(SB), CX
	PUSHQ	CX
	PUSHQ	R14
	PUSHQ	$0
	MOVQ	$SYS_PWRITE, BP
	SYSCALL
	ADDQ	$40, SP

	// 3. Bind #c to /dev
	// bind("#c", "/dev", MREPL)
	PUSHQ	$MREPL
	LEAQ	dev_path(SB), CX
	PUSHQ	CX
	LEAQ	cons_root(SB), CX
	PUSHQ	CX
	PUSHQ	$0
	MOVQ	$SYS_BIND, BP
	SYSCALL
	ADDQ	$32, SP

	// 4. Exec /bin/sh
	// exec("/bin/sh", argv)
	// argv = ["/bin/sh", 0]
	// Construct argv on stack
	PUSHQ	$0
	LEAQ	sh_path(SB), CX
	PUSHQ	CX
	MOVQ	SP, DX // argv pointer

	// exec args: path, argv
	PUSHQ	DX // argv
	PUSHQ	CX // path
	PUSHQ	$0
	MOVQ	$SYS_EXEC, BP
	SYSCALL
	ADDQ	$24, SP

	// If exec returns, it failed. Loop.
fail:
	JMP	fail

DATA cons_path+0(SB)/8, $"#c/cons\x00"
GLOBL cons_path(SB), RODATA, $8

DATA msg+0(SB)/16, $"Lux9 Asm Init\n\x00\x00"
GLOBL msg(SB), RODATA, $16

DATA dev_path+0(SB)/8, $"/dev\x00\x00\x00\x00"
GLOBL dev_path(SB), RODATA, $8

DATA cons_root+0(SB)/8, $"#c\x00\x00\x00\x00\x00\x00"
GLOBL cons_root(SB), RODATA, $8

DATA sh_path+0(SB)/8, $"/bin/sh\x00"
GLOBL sh_path(SB), RODATA, $8
