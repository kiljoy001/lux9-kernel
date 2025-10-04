/* 9front l.s converted to GAS syntax for x86_64 */
.code64
.section .text

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

/* CPUID */
.global cpuid
cpuid:
    pushq %rbx
    pushq %rcx
    pushq %rdx
    
    movl %edi, %eax      /* function in rdi */
    movl %esi, %ecx      /* subfunction in rsi */
    
    cpuid
    
    /* Store results: rax in *rdx (3rd param), rest in struct */
    movq 24(%rsp), %r8   /* get output pointer from stack */
    test %r8, %r8
    jz 1f
    movl %eax, 0(%r8)
    movl %ebx, 4(%r8)
    movl %ecx, 8(%r8)
    movl %edx, 12(%r8)
    
1:  popq %rdx
    popq %rcx
    popq %rbx
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

/* GDT/IDT */
.global lgdt
lgdt:
    lgdt (%rdi)
    ret

.global lidt
lidt:
    lidt (%rdi)
    ret

.global ltr
ltr:
    ltr %di
    ret

/* Memory barriers */
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
    pushfq
    pop %rax
    sti
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


/* Exception return stubs */
.global noteret
noteret:
    iretq

.global forkret
forkret:
    ret

.global setlabel
setlabel:
    movq %rsp, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq (%rsp), %rax
    movq %rax, 16(%rdi)
    xorl %eax, %eax
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
    movl %esi, %eax
    movl %edx, %edx
    xor %ecx, %ecx
    xsetbv
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
    push %rdi
    mov $'G', %al
    mov $0x3F8, %dx
    outb %al, %dx
    pop %rdi

    movq 0(%rdi), %rsp

    push %rax
    mov $'H', %al
    mov $0x3F8, %dx
    outb %al, %dx
    pop %rax

    movq 8(%rdi), %rbp

    push %rax
    mov $'I', %al
    mov $0x3F8, %dx
    outb %al, %dx
    pop %rax

    movq 16(%rdi), %rax
    movq %rax, (%rsp)

    push %rax
    mov $'J', %al
    mov $0x3F8, %dx
    outb %al, %dx
    pop %rax

    movl $1, %eax
    ret

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

.global syscallentry
syscallentry:
    /* Syscall entry point stub */
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
    /* Interrupt vector table - array of function pointers */
    .space 256*8
