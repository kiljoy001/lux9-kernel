/* 9front l.s converted to GAS syntax for x86_64 */
#include "mem.h"
#include "ureg.h"
.code64
.section .data
.align 8
.global touser_target_pc
touser_target_pc:
    .quad 0
.align 8
.global touser_target_sp
touser_target_sp:
    .quad 0
.align 8
.global touser_target_flags
touser_target_flags:
    .quad 0
.align 8
.global touser_attempt_count
touser_attempt_count:
    .quad 0

.section .text

.extern pebble_red_blue_exit
.extern up

/* CPU I/O port access */
.global inb
inb:
    movq %rdi, %rdx
    xorq %rax, %rax
    inb %dx, %al
    ret

.global outb
outb:
    movq %rdi, %rdx
    movq %rsi, %rax
    outb %al, %dx
    ret

.global ins
ins:
    movq %rdi, %rdx
    xorq %rax, %rax
    inw %dx, %ax
    ret

.global outs
outs:
    movq %rdi, %rdx
    movq %rsi, %rax
    outw %ax, %dx
    ret

.global inl
inl:
    movq %rdi, %rdx
    inl %dx, %eax
    ret

.global outl
outl:
    movq %rdi, %rdx
    movq %rsi, %rax
    outl %eax, %dx
    ret

/* Control register access */
.global getcr0
getcr0:
    movq %cr0, %rax
    ret

.global putcr0
putcr0:
    movq %rdi, %cr0
    ret

.global getcr2
getcr2:
    movq %cr2, %rax
    ret

.global putcr2
putcr2:
    movq %rdi, %cr2
    ret

.global getcr3
getcr3:
    movq %cr3, %rax
    ret

.global putcr3
putcr3:
    movq %rdi, %cr3
    ret

.global getcr4
getcr4:
    movq %cr4, %rax
    ret

.global putcr4
putcr4:
    movq %rdi, %cr4
    ret

.global wbinvd
wbinvd:
    wbinvd
    ret

/* CPUID */
.global cpuid
cpuid:
    pushq %rbx

    movl %edi, %eax      /* function in rdi */
    movl %esi, %ecx      /* subfunction in rsi */
    movq %rdx, %r8       /* save output pointer from rdx */

    cpuid

    /* Store results in array pointed to by r8 */
    test %r8, %r8
    jz 1f
    movl %eax, 0(%r8)
    movl %ebx, 4(%r8)
    movl %ecx, 8(%r8)
    movl %edx, 12(%r8)

1:  popq %rbx
    ret

/* MSR access */
.global rdmsr
rdmsr:
    movq %rdi, %rcx
    rdmsr
    shlq $32, %rdx
    orq %rdx, %rax
    ret

.global wrmsr
wrmsr:
    movq %rdi, %rcx
    movq %rsi, %rax
    movq %rsi, %rdx
    shrq $32, %rdx
    wrmsr
    ret

/* Timestamp counter */
.global rdtsc
rdtsc:
    rdtsc
    shlq $32, %rdx
    orq %rdx, %rax
    ret

.global _cycles
_cycles:
    rdtsc
    movl %eax, 0(%rdi)
    movl %edx, 4(%rdi)
    ret

/* GDT/IDT */
.global lgdt
lgdt:
    lgdt (%rdi)
    ret

.global lidt
lidt:
    lidt (%rdi)
    ret

.global sidt
sidt:
    sidt (%rdi)
    ret

.global ltr
ltr:
    ltr %di
    ret

/* Memory barriers */
.global mb386
mb386:
    xorl %eax, %eax
    cpuid
    ret

.global mb586
mb586:
    xorl %eax, %eax
    cpuid
    ret

.global mfence
mfence:
    mfence
    ret

.global sfence
sfence:
    sfence
    ret

.global lfence
lfence:
    lfence
    ret

/* Pause (for spinlocks) */
.global pause
pause:
    pause
    ret

/* Idle - halt until interrupt */
.global idle
idle:
    sti
    hlt
    ret

/* Interrupt control - TODO: need to understand 9front's interrupt model */
.global splhi
splhi:
    pushfq
    pop %rax
    cli
    ret

.global spllo
spllo:
_spllo:
    pushfq
    popq %rax
   testq $0x200, %rax  /* Test Interrupt Flag */
    jnz _alreadylo      /* If already enabled, skip */

    /* About to enable interrupts */
    sti

    /* Add a nop to ensure we can handle any immediate interrupt */
    nop
    nop

_alreadylo:
    ret

.global splx
splx:
    test $0x200, %rdi  /* IF flag */
    jz 1f
    sti
    ret
1:  cli
    ret

.global islo
islo:
    pushfq
    pop %rax
    and $0x200, %rax  /* Check IF flag */
    shr $9, %rax      /* Move to bit 0 */
    ret

/* Coherence (for MP synchronization) */
.global coherence_impl
coherence_impl:
    mfence
    ret

.global setlabel
setlabel:
    movq %rsp, 0(%rdi)         /* Save SP at offset 0 */
    movq (%rsp), %rax          /* Get return address from stack */
    movq %rax, 8(%rdi)         /* Save PC at offset 8 */
    movq %rbp, 16(%rdi)        /* Save RBP at offset 16 - CRITICAL for local variable access */
    movq %rbx, 24(%rdi)        /* Save RBX at offset 24 */
    movq %r12, 32(%rdi)        /* Save R12 at offset 32 */
    movq %r13, 40(%rdi)        /* Save R13 at offset 40 */
    movq %r14, 48(%rdi)        /* Save R14 at offset 48 */
    movq %r15, 56(%rdi)        /* Save R15 at offset 56 */
    xorl %eax, %eax            /* Return 0 */
    ret

.global _peekinst
_peekinst:
    xorl %eax, %eax
    ret

.global getdr6
getdr6:
    movq %dr6, %rax
    ret

.global putdr6
putdr6:
    movq %rdi, %dr6
    ret

.global tas
tas:
    movl $1, %eax
    xchgl %eax, (%rdi)
    ret

/* FPU instructions */
.global _fxsave
_fxsave:
    fxsave (%rdi)
    ret

.global _fxrstor
_fxrstor:
    fxrstor (%rdi)
    ret

.global _xsave
_xsave:
    movl %esi, %eax
    movl %edx, %edx
    xsave (%rdi)
    ret

.global _xrstor
_xrstor:
    movl %esi, %eax
    movl %edx, %edx
    xrstor (%rdi)
    ret

.global _xsaveopt
_xsaveopt:
    movl %esi, %eax
    movl %edx, %edx
    xsaveopt (%rdi)
    ret

.global _stts
_stts:
    movq %cr0, %rax
    orq $8, %rax
    movq %rax, %cr0
    ret

.global _clts
_clts:
    clts
    ret

.global _fninit
_fninit:
    fninit
    ret

.global _fwait
_fwait:
    fwait
    ret

.global _fnclex
_fnclex:
    fnclex
    ret

.global _fldcw
_fldcw:
    fldcw (%rdi)
    ret

.global _ldmxcsr
_ldmxcsr:
    ldmxcsr (%rdi)
    ret

.global putxcr0
putxcr0:
    xor %ecx, %ecx          /* XCR number = 0 */
    mov %rdi, %rdx          /* Copy 64-bit argument to RDX */
    shr $32, %rdx           /* Shift right 32, EDX now has high 32 bits */
    mov %edi, %eax          /* Copy low 32 bits to EAX */
    xsetbv                  /* Execute XSETBV with EDX:EAX value, ECX=0 */
    ret

.global _rdmsrinst
_rdmsrinst:
    movl %edi, %ecx
    rdmsr
    shlq $32, %rdx
    orq %rdx, %rax
    ret

.global _wrmsrinst
_wrmsrinst:
    movl %edi, %ecx
    movq %rsi, %rax
    movq %rsi, %rdx
    shrq $32, %rdx
    wrmsr
    ret

.global invlpg
invlpg:
    invlpg (%rdi)
    ret

.global gotolabel
gotolabel:
    movq 0(%rdi), %rsp         /* Restore SP from offset 0 */
    movq 8(%rdi), %rax         /* Load PC from offset 8 */
    movq 16(%rdi), %rbp        /* Restore RBP from offset 16 - CRITICAL for local variables */
    movq 24(%rdi), %rbx        /* Restore RBX from offset 24 */
    movq 32(%rdi), %r12        /* Restore R12 from offset 32 */
    movq 40(%rdi), %r13        /* Restore R13 from offset 40 */
    movq 48(%rdi), %r14        /* Restore R14 from offset 48 */
    movq 56(%rdi), %r15        /* Restore R15 from offset 56 */
    movq %rax, (%rsp)          /* Place PC on stack as return address */
    movl $1, %eax              /* Return 1 */
    ret

/* -----------------------------------------------------------------------
 * User/kernel transition helpers borrowed from 9front
 * --------------------------------------------------------------------- */
.global touser
touser:
    /* NOTE: Do NOT swapgs here! GS must remain pointing to Mach structure
     * so that interrupts and syscalls from userspace can access it.
     * The swapgs in syscallentry/trap handlers will handle the swap.
     * swapgs */

    /* Debug: print the user stack pointer we're about to use */
    pushq %rax
    pushq %rsi
    pushq %rdi                  /* save user SP argument */
    movq %rdi, %rsi             /* arg2 = user sp value */
    leaq touser_sp_msg(%rip), %rdi  /* arg1 = format string */
    xorq %rax, %rax             /* no vector args */
    call print
    popq %rdi                   /* restore user SP */
    popq %rsi
    popq %rax

    	/* Save user SP in a register before setting up IRET frame */
    	movq %rdi, %r12             /* r12 = user SP */
    
    	/* Build IRETQ frame on current kernel stack:
    	 * We need (from top to bottom): SS, RSP, RFLAGS, CS, RIP */
    	pushq $UD64SEL              /* SS - user data segment 64-bit (0x2B) */
    	pushq %r12                  /* RSP - user stack pointer */
    	pushq $0x002                /* RFLAGS - interrupts disabled */
    	pushq $UESEL                /* CS - user code segment */
    	pushq $UTZERO               /* RIP - entry point in initcode (no offset) */
    /* Debug: print the IRET frame */
    pushq %rax
    pushq %rdi
    pushq %rsi
    leaq touser_iret_msg(%rip), %rdi
    xorq %rax, %rax
    call print
    popq %rsi
    popq %rdi
    popq %rax

    /* Zero kernel pointers before entering userspace */
    xor %r15d, %r15d            /* RMACH = nil */
    xor %r14d, %r14d            /* RUSER = nil */
    xor %r12d, %r12d            /* clear our temp register */

    iretq

.section .rodata
touser_sp_msg:
    .string "touser: jumping to user mode with SP=%#llux\n"
touser_iret_msg:
    .string "touser: about to IRETQ to userspace\n"
spllo_before_sti:
    .string "spllo: before sti\n"
spllo_after_sti:
    .string "spllo: after sti\n"
_debug_vec_msg:
    .string "_intrcommon: Vector = %#llx\n"
_debug_rip_msg:
    .string "_intrcommon: RIP = %#llx\n"
_debug_cs_msg:
    .string "_intrcommon: CS = %#x\n"
_debug_rflags_msg:
    .string "_intrcommon: RFLAGS = %#llx\n"
_debug_syscallentry_msg:
    .string "syscallentry: up=%#p ureg=%#p rsp=%#p\n"

.global syscallentry
syscallentry:
    swapgs
    movq %gs:0, %rax            /* m */
    movq 16(%rax), %rbx         /* up */

    /* DEBUG: Check if up is valid before using it */
    testq %rbx, %rbx
    jz halt_up_null             /* if NULL, halt with code 0xDEAD0001 */
    testq %rbx, %rbx            /* test sign bit */
    jns halt_up_not_kernel      /* if bit 63 not set, halt with code 0xDEAD0002 */

    movq %rsp, %r13             /* save user stack */
    movq %rbx, %rsp             /* switch to kernel stack */

    /* DEBUG: Verify RSP actually switched to kernel address */
    testq %rsp, %rsp            /* test sign bit */
    jns halt_rsp_not_kernel     /* if bit 63 not set, halt with code 0xDEAD0003 */

    pushq $UDSEL                /* ss - user data segment */
    pushq %r13                  /* user sp */
    pushq %r11                  /* rflags */
    pushq $UESEL                /* cs */
    pushq %rcx                  /* user pc */
    pushq $0                    /* error code */
    pushq $64                   /* trap number (VectorSYSCALL) */

    /* Allocate 136 bytes: 128 for Ureg fields (ax-gs) + 8 for alignment padding
     * Hardware frame (type-ss) already pushed: 56 bytes
     * Total stack: 56 (7 pushes) + 136 = 192 bytes
     * 192 % 16 = 0 (correct alignment before call) */
    subq $136, %rsp             /* space for Ureg registers + alignment padding */

    /* Save all general purpose registers for potential noteret path
     * Note: ureg starts at rsp+8, so all offsets are +8 from struct offsets */
    movq %rax, 8(%rsp)          /* ureg->ax (offset 0 in struct) */
    movq %rbx, 16(%rsp)         /* ureg->bx (offset 8) */
    movq %rcx, 24(%rsp)         /* ureg->cx (offset 16, user pc saved by syscall) */
    movq %rdx, 32(%rsp)         /* ureg->dx (offset 24) */
    movq %rsi, 40(%rsp)         /* ureg->si (offset 32) */
    movq %rdi, 48(%rsp)         /* ureg->di (offset 40) */
    movq %rbp, 56(%rsp)         /* ureg->bp (offset 48, syscall number) */
    movq %r8,  64(%rsp)         /* ureg->r8 (offset 56) */
    movq %r9,  72(%rsp)         /* ureg->r9 (offset 64) */
    movq %r10, 80(%rsp)         /* ureg->r10 (offset 72) */
    movq %r11, 88(%rsp)         /* ureg->r11 (offset 80, rflags saved by syscall) */
    movq %r12, 96(%rsp)         /* ureg->r12 (offset 88) */
    movq %r13, 104(%rsp)        /* ureg->r13 (offset 96) */
    movq %r14, 112(%rsp)        /* save previous RUSER (ureg->r14, offset 104) */
    movq %r15, 120(%rsp)        /* save previous RMACH (ureg->r15, offset 112) */

    /* Save segment registers (ureg->ds, es, fs, gs at offsets 120-127) */
    movw %ds, 128(%rsp)         /* ureg->ds (offset 120) */
    movw %es, 130(%rsp)         /* ureg->es (offset 122) */
    movw %fs, 132(%rsp)         /* ureg->fs (offset 124) */
    movw %gs, 134(%rsp)         /* ureg->gs (offset 126) */

    movq %rax, %r15             /* update RMACH */
    movq 16(%rax), %r14         /* update RUSER = up */

    leaq 8(%rsp), %rdi          /* &ureg */
    call syscall

    /* Return path: check stack slot ((void**)&ureg)[-1] at (%rsp)
     * If noteret is set, jump directly to noteret.
     * Otherwise jump to syscallret (both use IRETQ). */
    movq (%rsp), %rax           /* load stack slot */
    testq %rax, %rax            /* check if noteret address is set */
    jnz noteret                 /* if set, use IRETQ path */
    jmp syscallret              /* normal return via IRETQ */

halt_up_null:
    movq $0xDEAD0001, %rax
    cli
    hlt
    jmp halt_up_null

halt_up_not_kernel:
    movq $0xDEAD0002, %rax
    cli
    hlt
    jmp halt_up_not_kernel

halt_rsp_not_kernel:
    movq $0xDEAD0003, %rax
    cli
    hlt
    jmp halt_rsp_not_kernel

halt_trap_m_null:
    movq $0xDEAD1001, %rax
    cli
    hlt
    jmp halt_trap_m_null

halt_trap_up_null:
    movq $0xDEAD1002, %rax
    cli
    hlt
    jmp halt_trap_up_null

halt_trap_up_not_kernel:
    movq $0xDEAD1003, %rax
    cli
    hlt
    jmp halt_trap_up_not_kernel

    jmp syscallret

/* NOTE: We use IRETQ for all returns instead of the faster SYSRET instruction.
 * This is because our GDT layout (inherited from 9front) is incompatible with SYSRET.
 *
 * SYSRET requires: CS = STAR[63:48] + 16, SS = STAR[63:48] + 8
 * Our GDT has: UESEL (CS64) = 0x2B, UD64SEL (SS64) = 0x33
 * To get CS=0x2B: STAR[63:48] must = 0x1B
 * To get SS=0x33: STAR[63:48] must = 0x2B
 * These are conflicting! The CS64 and SS64 segments are not at the required +16/+8 offsets.
 *
 * Reorganizing the GDT would be complex and risky. IRETQ works correctly and the
 * performance difference is negligible for our use case.
 */
.global forkret
.global syscallret
syscallret:
forkret:
    /* Syscall-specific return path (different stack layout than trap path) */
    popq %rax                   /* discard saved pointer */

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbp
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13

    /* After popping 14 quads, RSP points to r14. CS is at r14+48 */
    cmpq $KESEL, 48(%rsp)
    je _syscall_iretnested

    swapgs
    movq 8(%rsp), %r15
    movq 0(%rsp), %r14

_syscall_iretnested:
	/* Restore preserved registers */
	movq 0(%rsp), %r14
	movq 8(%rsp), %r15

	/* Restore data segments to User Selector (saved on stack) */
	/* Necessary because SYSRET doesn't restore them, and we need DPL 3 */
	movw 16(%rsp), %ds
	movw 18(%rsp), %es

	swapgs

	/* Prepare for SYSRETQ:
	 * RCX = RIP (User PC) - at offset 40
	 * R11 = RFLAGS - at offset 56
	 * RSP = User SP - at offset 64
	 */
	movq 40(%rsp), %rcx
	movq 56(%rsp), %r11
	movq 64(%rsp), %rsp
	sysretq

	/* Debug: print iretq frame values - DISABLED */
	/* 
	pushq %rdi
	pushq %rsi
	pushq %rax

	/* Frame is now at rsp+24: pc, cs, flags, sp, ss */
	/* movq 24(%rsp), %rsi         /* pc */
	/* movq 32(%rsp), %rdx         /* cs */
	/* movq 40(%rsp), %rcx         /* flags */
	/* movq 48(%rsp), %r8          /* sp */
	/* movq 56(%rsp), %r9          /* ss */
	/* leaq _iretq_debug_msg(%rip), %rdi
	/* xorq %rax, %rax
	/* call iprint

	/* popq %rax
	/* popq %rsi
	/* popq %rdi
	*/

	iretq
.section .rodata
_iretq_debug_msg:
    .string "IRETQ: pc=%#p cs=%#llx flags=%#llx sp=%#p ss=%#llx\n"

.text

.global _strayintr
_strayintr:
    pushq %rax
    movq 8(%rsp), %rax
    jmp _intrcommon

.global _strayintrx
_strayintrx:
    xchgq %rax, (%rsp)     /* Swap RAX with error code, like 9front */
_intrcommon:
    movzbq (%rax), %rax
    xchgq %rax, (%rsp)
    subq $24, %rsp

    /* Check CS like 9front does */
    cmpw $KESEL, 48(%rsp)
    je 1f

    /* Coming from user mode - need to set up kernel context */
    movq %r14, 0(%rsp)
    movq %r15, 8(%rsp)
    swapgs
    /* WORKAROUND: KVM doesn't virtualize GS base - use global m */
    movq m(%rip), %r15      /* Load m into r15 from global variable */

    /* DEBUG: Check m is valid - COMMENTED OUT TO SEE PANIC
    testq %r15, %r15
    jz halt_trap_m_null
    */

    movq 16(%r15), %r14     /* Load up into r14 */

    /* DEBUG: Check up is valid - COMMENTED OUT TO TEST
    testq %r14, %r14
    jz halt_trap_up_null
    testq %r14, %r14
    jns halt_trap_up_not_kernel
    */

    /* DEBUG: If we get here, m was valid. Store r14 value at a known location */
    movq %r14, intr_frame_r14(%rip)

    jmp 2f

1:
    /* Kernel-mode trap - GS already points at Mach */
    /* WORKAROUND: KVM doesn't properly virtualize GS base MSR, so use global m */
    /* TODO: Check if we can use GS on bare metal for better performance */
    movq m(%rip), %r15      /* Use global m pointer instead of %gs:0 */
    movq 16(%r15), %r14

2:

_intrnested:
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
    pushq %r9
    pushq %r8
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
	pushq %rbx
	pushq %rax

	/* Relocate Ureg to kernel stack if hardware left us on user stack */
	movq %rsp, %rax             /* current Ureg pointer */
	leaq intr_frame_before(%rip), %rdx
	movq %rax, (%rdx)
	xorq %rdx, %rdx
	movq %rdx, intr_frame_relocated(%rip)
	movq %r14, intr_frame_r14(%rip)
	testq %rax, %rax
	js 1f
	testq %r14, %r14
	je 1f
	/* dst = up - (UREG_SIZE + 16) */
	movq %r14, %rdx
	subq $(UREG_SIZE+16), %rdx
	movq %rdx, %rdi
	movq %rax, %rsi
	movq %rdx, %rsp
	movq $(UREG_SIZE/8), %rcx
	cld
	rep movsq
	movq %rsp, %rax
	movq $1, intr_frame_relocated(%rip)
1:
	leaq intr_frame_addr(%rip), %rdx
	movq %rax, (%rdx)
	movq 0xA0(%rax), %rdx        /* flags */
	movq %rdx, intr_frame_saved_flags(%rip)
	movq 0xA8(%rax), %rdx        /* saved SP */
	movq %rdx, intr_frame_saved_sp(%rip)
	movq 0xB0(%rax), %rdx        /* saved SS */
	movq %rdx, intr_frame_saved_ss(%rip)
	movq 0x98(%rax), %rdx        /* saved CS */
	movq %rdx, intr_frame_saved_cs(%rip)

	movq %rax, %rdi
	pushq %rax                  /* preserve stack slot for notifier */
	call trap
    jmp _intrestore

.global noteret
noteret:
_intrestore:
    popq %rax                   /* discard saved pointer */

	    popq %rax
	    popq %rbx
	    popq %rcx
	    popq %rdx
	    popq %rsi
	    popq %rdi
	    popq %rbp
	    popq %r8
	    popq %r9
	    popq %r10
	    popq %r11
	    popq %r12
	    popq %r13

	    /* After register pops, RSP points at 24-byte locals allocation */
	    /* Stack layout: [24 locals][vector][error][RIP][CS][RFLAGS][RSP?][SS?] */
	    /* CS is at offset 48 (24+8+8+8) */
	    cmpq $KESEL, 48(%rsp)
	    je _iretnested

	    /* User mode interrupt: restore R14/R15 and swap GS back */
	    swapgs
	    movq 8(%rsp), %r15        /* r15 was stored at offset 8 */
	    movq 0(%rsp), %r14        /* r14 was stored at offset 0 */

_iretnested:
	    /* Remove: 24-byte locals + vector + error = 40 bytes */
	    /* Leave hardware frame: RIP, CS, RFLAGS, [RSP, SS] */
	    /* IRETQ automatically handles whether RSP/SS are present based on privilege change */
	    addq $40, %rsp
	    iretq

.global putdr
putdr:
    /* putdr(int reg, uintptr val) */
    cmpq $0, %rdi
    je putdr0
    cmpq $1, %rdi
    je putdr1
    cmpq $2, %rdi
    je putdr2
    cmpq $3, %rdi
    je putdr3
    ret
putdr0: movq %rsi, %dr0; ret
putdr1: movq %rsi, %dr1; ret
putdr2: movq %rsi, %dr2; ret
putdr3: movq %rsi, %dr3; ret

.global putdr7
putdr7:
    movq %rdi, %dr7
    ret

.global putdr01236
putdr01236:
    movq 0(%rdi), %rax
    movq %rax, %dr0
    movq 8(%rdi), %rax
    movq %rax, %dr1
    movq 16(%rdi), %rax
    movq %rax, %dr2
    movq 24(%rdi), %rax
    movq %rax, %dr3
    movq 48(%rdi), %rax
    movq %rax, %dr6
    ret

.global cmpswap486
cmpswap486:
    /* Compare and swap: bool cmpswap486(long *addr, long old, long new) */
    /* rdi = addr, rsi = old, rdx = new */
    movl %esi, %eax        /* Load old value into eax */
    lock cmpxchgl %edx, (%rdi)  /* Atomic compare-and-swap */
    sete %al               /* Set AL to 1 if equal (swap succeeded) */
    movzbl %al, %eax       /* Zero-extend result to full register */
    ret

.global vectortable
vectortable:
    call _strayintr
    .byte 0x00        /* divide error */
    call _strayintr
    .byte 0x01        /* debug exception */
    call _strayintr
    .byte 0x02        /* NMI interrupt */
    call _strayintr
    .byte 0x03        /* breakpoint */
    call _strayintr
    .byte 0x04        /* overflow */
    call _strayintr
    .byte 0x05        /* bound */
    call _strayintr
    .byte 0x06        /* invalid opcode */
    call _strayintr
    .byte 0x07        /* no coprocessor available */
    call _strayintrx
    .byte 0x08        /* double fault */
    call _strayintr
    .byte 0x09        /* coprocessor segment overflow */
    call _strayintrx
    .byte 0x0a        /* invalid TSS */
    call _strayintrx
    .byte 0x0b        /* segment not available */
    call _strayintrx
    .byte 0x0c        /* stack exception */
    call _strayintrx
    .byte 0x0d        /* general protection error */
    call _strayintrx
    .byte 0x0e        /* page fault */
    call _strayintrx
    .byte 0x0f
    call _strayintr
    .byte 0x10        /* coprocessor error */
    call _strayintrx
    .byte 0x11        /* alignment check */
    call _strayintr
    .byte 0x12        /* machine check */
    call _strayintr
    .byte 0x13        /* simd error */
    call _strayintr
    .byte 0x14
    call _strayintr
    .byte 0x15
    call _strayintr
    .byte 0x16
    call _strayintr
    .byte 0x17
    call _strayintr
    .byte 0x18
    call _strayintr
    .byte 0x19
    call _strayintr
    .byte 0x1a
    call _strayintr
    .byte 0x1b
    call _strayintr
    .byte 0x1c
    call _strayintr
    .byte 0x1d
    call _strayintr
    .byte 0x1e
    call _strayintr
    .byte 0x1f
    call _strayintr
    .byte 0x20        /* VectorLAPIC */
    call _strayintr
    .byte 0x21
    call _strayintr
    .byte 0x22
    call _strayintr
    .byte 0x23
    call _strayintr
    .byte 0x24
    call _strayintr
    .byte 0x25
    call _strayintr
    .byte 0x26
    call _strayintr
    .byte 0x27
    call _strayintr
    .byte 0x28
    call _strayintr
    .byte 0x29
    call _strayintr
    .byte 0x2a
    call _strayintr
    .byte 0x2b
    call _strayintr
    .byte 0x2c
    call _strayintr
    .byte 0x2d
    call _strayintr
    .byte 0x2e
    call _strayintr
    .byte 0x2f
    call _strayintr
    .byte 0x30
    call _strayintr
    .byte 0x31
    call _strayintr
    .byte 0x32
    call _strayintr
    .byte 0x33
    call _strayintr
    .byte 0x34
    call _strayintr
    .byte 0x35
    call _strayintr
    .byte 0x36
    call _strayintr
    .byte 0x37
    call _strayintr
    .byte 0x38
    call _strayintr
    .byte 0x39
    call _strayintr
    .byte 0x3a
    call _strayintr
    .byte 0x3b
    call _strayintr
    .byte 0x3c
    call _strayintr
    .byte 0x3d
    call _strayintr
    .byte 0x3e
    call _strayintr
    .byte 0x3f
    call _strayintr
    .byte 0x40        /* was VectorSYSCALL */
    call _strayintr
    .byte 0x41
    call _strayintr
    .byte 0x42
    call _strayintr
    .byte 0x43
    call _strayintr
    .byte 0x44
    call _strayintr
    .byte 0x45
    call _strayintr
    .byte 0x46
    call _strayintr
    .byte 0x47
    call _strayintr
    .byte 0x48
    call _strayintr
    .byte 0x49
    call _strayintr
    .byte 0x4a
    call _strayintr
    .byte 0x4b
    call _strayintr
    .byte 0x4c
    call _strayintr
    .byte 0x4d
    call _strayintr
    .byte 0x4e
    call _strayintr
    .byte 0x4f
    call _strayintr
    .byte 0x50
    call _strayintr
    .byte 0x51
    call _strayintr
    .byte 0x52
    call _strayintr
    .byte 0x53
    call _strayintr
    .byte 0x54
    call _strayintr
    .byte 0x55
    call _strayintr
    .byte 0x56
    call _strayintr
    .byte 0x57
    call _strayintr
    .byte 0x58
    call _strayintr
    .byte 0x59
    call _strayintr
    .byte 0x5a
    call _strayintr
    .byte 0x5b
    call _strayintr
    .byte 0x5c
    call _strayintr
    .byte 0x5d
    call _strayintr
    .byte 0x5e
    call _strayintr
    .byte 0x5f
    call _strayintr
    .byte 0x60
    call _strayintr
    .byte 0x61
    call _strayintr
    .byte 0x62
    call _strayintr
    .byte 0x63
    call _strayintr
    .byte 0x64
    call _strayintr
    .byte 0x65
    call _strayintr
    .byte 0x66
    call _strayintr
    .byte 0x67
    call _strayintr
    .byte 0x68
    call _strayintr
    .byte 0x69
    call _strayintr
    .byte 0x6a
    call _strayintr
    .byte 0x6b
    call _strayintr
    .byte 0x6c
    call _strayintr
    .byte 0x6d
    call _strayintr
    .byte 0x6e
    call _strayintr
    .byte 0x6f
    call _strayintr
    .byte 0x70
    call _strayintr
    .byte 0x71
    call _strayintr
    .byte 0x72
    call _strayintr
    .byte 0x73
    call _strayintr
    .byte 0x74
    call _strayintr
    .byte 0x75
    call _strayintr
    .byte 0x76
    call _strayintr
    .byte 0x77
    call _strayintr
    .byte 0x78
    call _strayintr
    .byte 0x79
    call _strayintr
    .byte 0x7a
    call _strayintr
    .byte 0x7b
    call _strayintr
    .byte 0x7c
    call _strayintr
    .byte 0x7d
    call _strayintr
    .byte 0x7e
    call _strayintr
    .byte 0x7f
    call _strayintr
    .byte 0x80        /* Vector[A]PIC */
    call _strayintr
    .byte 0x81
    call _strayintr
    .byte 0x82
    call _strayintr
    .byte 0x83
    call _strayintr
    .byte 0x84
    call _strayintr
    .byte 0x85
    call _strayintr
    .byte 0x86
    call _strayintr
    .byte 0x87
    call _strayintr
    .byte 0x88
    call _strayintr
    .byte 0x89
    call _strayintr
    .byte 0x8a
    call _strayintr
    .byte 0x8b
    call _strayintr
    .byte 0x8c
    call _strayintr
    .byte 0x8d
    call _strayintr
    .byte 0x8e
    call _strayintr
    .byte 0x8f
    call _strayintr
    .byte 0x90
    call _strayintr
    .byte 0x91
    call _strayintr
    .byte 0x92
    call _strayintr
    .byte 0x93
    call _strayintr
    .byte 0x94
    call _strayintr
    .byte 0x95
    call _strayintr
    .byte 0x96
    call _strayintr
    .byte 0x97
    call _strayintr
    .byte 0x98
    call _strayintr
    .byte 0x99
    call _strayintr
    .byte 0x9a
    call _strayintr
    .byte 0x9b
    call _strayintr
    .byte 0x9c
    call _strayintr
    .byte 0x9d
    call _strayintr
    .byte 0x9e
    call _strayintr
    .byte 0x9f
    call _strayintr
    .byte 0xa0
    call _strayintr
    .byte 0xa1
    call _strayintr
    .byte 0xa2
    call _strayintr
    .byte 0xa3
    call _strayintr
    .byte 0xa4
    call _strayintr
    .byte 0xa5
    call _strayintr
    .byte 0xa6
    call _strayintr
    .byte 0xa7
    call _strayintr
    .byte 0xa8
    call _strayintr
    .byte 0xa9
    call _strayintr
    .byte 0xaa
    call _strayintr
    .byte 0xab
    call _strayintr
    .byte 0xac
    call _strayintr
    .byte 0xad
    call _strayintr
    .byte 0xae
    call _strayintr
    .byte 0xaf
    call _strayintr
    .byte 0xb0
    call _strayintr
    .byte 0xb1
    call _strayintr
    .byte 0xb2
    call _strayintr
    .byte 0xb3
    call _strayintr
    .byte 0xb4
    call _strayintr
    .byte 0xb5
    call _strayintr
    .byte 0xb6
    call _strayintr
    .byte 0xb7
    call _strayintr
    .byte 0xb8
    call _strayintr
    .byte 0xb9
    call _strayintr
    .byte 0xba
    call _strayintr
    .byte 0xbb
    call _strayintr
    .byte 0xbc
    call _strayintr
    .byte 0xbd
    call _strayintr
    .byte 0xbe
    call _strayintr
    .byte 0xbf
    call _strayintr
    .byte 0xc0
    call _strayintr
    .byte 0xc1
    call _strayintr
    .byte 0xc2
    call _strayintr
    .byte 0xc3
    call _strayintr
    .byte 0xc4
    call _strayintr
    .byte 0xc5
    call _strayintr
    .byte 0xc6
    call _strayintr
    .byte 0xc7
    call _strayintr
    .byte 0xc8
    call _strayintr
    .byte 0xc9
    call _strayintr
    .byte 0xca
    call _strayintr
    .byte 0xcb
    call _strayintr
    .byte 0xcc
    call _strayintr
    .byte 0xcd
    call _strayintr
    .byte 0xce
    call _strayintr
    .byte 0xcf
    call _strayintr
    .byte 0xd0
    call _strayintr
    .byte 0xd1
    call _strayintr
    .byte 0xd2
    call _strayintr
    .byte 0xd3
    call _strayintr
    .byte 0xd4
    call _strayintr
    .byte 0xd5
    call _strayintr
    .byte 0xd6
    call _strayintr
    .byte 0xd7
    call _strayintr
    .byte 0xd8
    call _strayintr
    .byte 0xd9
    call _strayintr
    .byte 0xda
    call _strayintr
    .byte 0xdb
    call _strayintr
    .byte 0xdc
    call _strayintr
    .byte 0xdd
    call _strayintr
    .byte 0xde
    call _strayintr
    .byte 0xdf
    call _strayintr
    .byte 0xe0
    call _strayintr
    .byte 0xe1
    call _strayintr
    .byte 0xe2
    call _strayintr
    .byte 0xe3
    call _strayintr
    .byte 0xe4
    call _strayintr
    .byte 0xe5
    call _strayintr
    .byte 0xe6
    call _strayintr
    .byte 0xe7
    call _strayintr
    .byte 0xe8
    call _strayintr
    .byte 0xe9
    call _strayintr
    .byte 0xea
    call _strayintr
    .byte 0xeb
    call _strayintr
    .byte 0xec
    call _strayintr
    .byte 0xed
    call _strayintr
    .byte 0xee
    call _strayintr
    .byte 0xef
    call _strayintr
    .byte 0xf0
    call _strayintr
    .byte 0xf1
    call _strayintr
    .byte 0xf2
    call _strayintr
    .byte 0xf3
    call _strayintr
    .byte 0xf4
    call _strayintr
    .byte 0xf5
    call _strayintr
    .byte 0xf6
    call _strayintr
    .byte 0xf7
    call _strayintr
    .byte 0xf8
    call _strayintr
    .byte 0xf9
    call _strayintr
    .byte 0xfa
    call _strayintr
    .byte 0xfb
    call _strayintr
    .byte 0xfc
    call _strayintr
    .byte 0xfd
    call _strayintr
    .byte 0xfe
    call _strayintr
    .byte 0xff

.global halt
halt:
    cli
    cmpl $0, nrdy(%rip)
    jne 1f
    sti
    hlt
    ret
1:
    sti
    ret

.global mwait
mwait:
    movq %rdi, %rax
    movl (%rax), %ecx
    testl %ecx, %ecx
    jne 2f
    xor %ecx, %ecx
    xor %edx, %edx
    .byte 0x0f,0x01,0xc8
    movl (%rax), %ecx
    testl %ecx, %ecx
    jne 2f
    xor %eax, %eax
    xor %edx, %edx
    .byte 0x0f,0x01,0xc9
2:
    ret

.global rdrandbuf
rdrandbuf:
    movq %rdi, %r8          /* buffer pointer */
    movq %rsi, %rcx         /* byte count */
    movq %rcx, %rdx
    shrq $2, %rcx
    jz 4f
3:
    rdrand %eax
    jnc 3b
    movl %eax, (%r8)
    addq $4, %r8
    loop 3b
4:
    movq %rdx, %rcx
    andq $3, %rcx
    jz 6f
5:
    rdrand %eax
    jnc 5b
    movb %al, (%r8)
    incq %r8
    loop 5b
6:
    ret

/* VMX instructions */
.global vmxon
.global vmxoff
.global vmclear
.global vmptrld
.global vmlaunch
.global vmread
.global vmwrite
.global invept
.global invvpid

/* VMXON - Enable VMX operation */
vmxon:
    movq %rdi, %rax          /* PA of VMXON region */
    .byte 0xf3, 0x0f, 0xc7, 0x30  /* VMXON (%rax) */
    setc %al                 /* Set AL to 1 if carry flag set (error) */
    movzx %al, %rax          /* Zero extend AL to RAX */
    ret

/* VMXOFF - Leave VMX operation */
vmxoff:
    .byte 0x0f, 0x01, 0xc4   /* VMXOFF */
    xorq %rax, %rax          /* Return 0 for success */
    ret

/* VMCLEAR - Clear VMCS */
vmclear:
    movq %rdi, %rax          /* PA of VMCS region */
    .byte 0x66, 0x0f, 0xc7, 0x30  /* VMCLEAR (%rax) */
    setc %al                 /* Set AL to 1 if carry flag set (error) */
    movzx %al, %rax          /* Zero extend AL to RAX */
    ret

/* VMPTRLD - Load VMCS pointer */
vmptrld:
    movq %rdi, %rax          /* PA of VMCS region */
    .byte 0x0f, 0xc7, 0x30   /* VMPTRLD (%rax) */
    setc %al                 /* Set AL to 1 if carry flag set (error) */
    movzx %al, %rax          /* Zero extend AL to RAX */
    ret

/* VMLAUNCH - Launch virtual machine */
vmlaunch:
    pushq %rbp
    movq %rsp, %rbp
    /* Save Ureg structure from first argument */
    movq %rdi, %r8
    /* Launch type in second argument */
    movl %esi, %edx
    
    /* Check launch type */
    testl %edx, %edx
    jz 1f
    
    /* VMRESUME */
    movq UREG_RAX(%r8), %rax
    movq UREG_RBX(%r8), %rbx
    movq UREG_RCX(%r8), %rcx
    movq UREG_RDX(%r8), %rdx
    movq UREG_RSI(%r8), %rsi
    movq UREG_RDI(%r8), %rdi
    movq UREG_RBP(%r8), %rbp
    movq UREG_R8(%r8), %r8
    movq UREG_R9(%r8), %r9
    movq UREG_R10(%r8), %r10
    movq UREG_R11(%r8), %r11
    movq UREG_R12(%r8), %r12
    movq UREG_R13(%r8), %r13
    movq UREG_R14(%r8), %r14
    movq UREG_R15(%r8), %r15
    movq UREG_RSP(%r8), %rsp
    movq UREG_RIP(%r8), %rbp
    pushq %rbp               /* Push RIP */
    movq UREG_EFL(%r8), %rbp
    pushq %rbp               /* Push RFLAGS */
    .byte 0x0f, 0x01, 0xc3   /* VMRESUME */
    jmp 2f
    
1:  /* VMLAUNCH */
    movq UREG_RAX(%r8), %rax
    movq UREG_RBX(%r8), %rbx
    movq UREG_RCX(%r8), %rcx
    movq UREG_RDX(%r8), %rdx
    movq UREG_RSI(%r8), %rsi
    movq UREG_RDI(%r8), %rdi
    movq UREG_RBP(%r8), %rbp
    /* Note: r8 is used for Ureg pointer, so skip */
    movq UREG_R9(%r8), %r9
    movq UREG_R10(%r8), %r10
    movq UREG_R11(%r8), %r11
    movq UREG_R12(%r8), %r12
    movq UREG_R13(%r8), %r13
    movq UREG_R14(%r8), %r14
    movq UREG_R15(%r8), %r15
    movq UREG_RSP(%r8), %rsp
    movq UREG_RIP(%r8), %rbp
    pushq %rbp               /* Push RIP */
    movq UREG_EFL(%r8), %rbp
    pushq %rbp               /* Push RFLAGS */
    .byte 0x0f, 0x01, 0xc2   /* VMLAUNCH */
    
2:  /* Error return */
    popq %rbp
    popq %rbp
    movq %rbp, %rsp
    popq %rbp
    movq $1, %rax           /* Return error */
    ret

/* VMREAD - Read VMCS field */
vmread:
    movl %edi, %eax          /* VMCS field encoding */
    movq %rsi, %rdx          /* Destination pointer */
    .byte 0x0f, 0x78, 0xc0   /* VMREAD EAX, RAX */
    setc %cl                 /* Set CL to 1 if carry flag set (error) */
    movq %rax, (%rdx)        /* Store result */
    movzx %cl, %rax          /* Zero extend CL to RAX */
    ret

/* VMWRITE - Write VMCS field */
vmwrite:
    movl %edi, %eax          /* VMCS field encoding */
    movq %rsi, %rdx          /* Value to write */
    .byte 0x0f, 0x79, 0xc2   /* VMWRITE RDX, EAX */
    setc %al                 /* Set AL to 1 if carry flag set (error) */
    movzx %al, %rax          /* Zero extend AL to RAX */
    ret

/* INVEPT - Invalidate EP TBL */
invept:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl %edi, %eax          /* INVEPT type */
    movq %rsi, 8(%rsp)       /* GPA */
    movq %rdx, (%rsp)        /* Reserved */
    .byte 0x66, 0x0f, 0x38, 0x80, 0x0c, 0x24  /* INVEPT %eax, (%rsp) */
    setc %al
    movzx %al, %rax
    leave
    ret

/* INVVPID - Invalidate VP ID */
invvpid:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl %edi, %eax          /* INVVPID type */
    movq %rsi, 8(%rsp)       /* VPID */
    movq %rdx, (%rsp)        /* Linear address */
    .byte 0x66, 0x0f, 0x38, 0x81, 0x0c, 0x24  /* INVVPID %eax, (%rsp) */
    setc %al
    movzx %al, %rax
    leave
    ret
