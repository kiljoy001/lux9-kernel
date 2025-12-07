#include "textflag.h"
// Strings and tables
DATA main·cons_path+0(SB)/8, $"#c/cons"
GLOBL main·cons_path(SB), RODATA, $8

DATA main·tls_prefix+0(SB)/6, $"TLS=0x"
GLOBL main·tls_prefix(SB), RODATA, $6
DATA main·ax_prefix+0(SB)/6, $" AX=0x"
GLOBL main·ax_prefix(SB), RODATA, $6
DATA main·newline+0(SB)/1, $"\n"
GLOBL main·newline(SB), RODATA, $1

DATA main·hexchars+0(SB)/16, $"0123456789abcdef"
GLOBL main·hexchars(SB), RODATA, $16

GLOBL main·outbuf(SB), NOPTR, $64

// write_hex(dest=DI, value=SI)
TEXT main·write_hex(SB), NOSPLIT, $0
    MOVQ    $60, BX
hex_loop:
    MOVQ    SI, AX
    MOVB    BL, CL
    SHRQ    CL, AX
    ANDQ    $0xf, AX
    LEAQ    main·hexchars(SB), R9
    MOVB    (R9)(AX*1), CL
    MOVB    CL, (DI)
    INCQ    DI
    SUBQ    $4, BX
    JGE     hex_loop
    RET

TEXT main·_start(SB), NOSPLIT, $0
    // Save Tos pointer from AX
    MOVQ    AX, R14

    // Get TLS base
    MOVQ    TLS, R8

    LEAQ    main·outbuf(SB), DI

    // copy "TLS=0x"
    LEAQ    main·tls_prefix(SB), SI
    MOVL    $6, CX
copy_tls_prefix:
    MOVB    (SI), AL
    MOVB    AL, (DI)
    INCQ    SI
    INCQ    DI
    DECQ    CX
    JNZ     copy_tls_prefix

    // write TLS hex
    MOVQ    R8, SI
    CALL    main·write_hex(SB)

    // copy " AX=0x"
    LEAQ    main·ax_prefix(SB), SI
    MOVL    $6, CX
copy_ax_prefix:
    MOVB    (SI), AL
    MOVB    AL, (DI)
    INCQ    SI
    INCQ    DI
    DECQ    CX
    JNZ     copy_ax_prefix

    // write Tos (entry AX saved in R14)
    MOVQ    R14, SI
    CALL    main·write_hex(SB)

    // newline
    MOVB    main·newline(SB), AL
    MOVB    AL, (DI)
    INCQ    DI

    // compute message length
    LEAQ    main·outbuf(SB), R13
    MOVQ    DI, AX
    SUBQ    R13, AX      // RAX = length
    MOVQ    AX, R15      // save length

    // open("#c/cons", OWRITE=1, perm=0)
    SUBQ    $32, SP
    MOVQ    $0, 0(SP)         // dummy return
    LEAQ    main·cons_path(SB), R10
    MOVQ    R10, 8(SP)
    MOVQ    $1, 16(SP)
    MOVQ    $0, 24(SP)
    MOVQ    $14, BP
    SYSCALL
    MOVQ    AX, R11           // fd
    ADDQ    $32, SP

    // pwrite(fd, outbuf, len, -1)
    SUBQ    $40, SP
    MOVQ    $0, 0(SP)         // dummy return
    MOVQ    R11, 8(SP)
    LEAQ    main·outbuf(SB), R10
    MOVQ    R10, 16(SP)
    MOVQ    R15, 24(SP)
    MOVQ    $-1, 32(SP)
    MOVQ    $51, BP
    SYSCALL
    ADDQ    $40, SP

    // exits("")
    SUBQ    $16, SP
    MOVQ    $0, 0(SP)
    MOVQ    $0, 8(SP)
    MOVQ    $8, BP
    SYSCALL
