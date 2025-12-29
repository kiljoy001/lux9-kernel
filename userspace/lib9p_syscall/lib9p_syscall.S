/* lib9p_syscall.S - Pure 9P userspace syscall wrappers
 *
 * This library replaces traditional syscalls with 9P message passing.
 * Each function constructs a Tsys* message in the exchange page,
 * rings the doorbell, and returns the result from the reply buffer.
 *
 * Exchange Page Layout:
 *   0x000 - 0xEFF: Request buffer
 *   0xF00 - 0xFFF: Control block
 *     0xF00: doorbell
 *     0xF04: status
 *     0xF08: req_head
 *     0xF0C: req_tail
 *     0xF10: rep_head
 *     0xF14: rep_size
 */

.section .text

/* Constants */
.equ EXCHANGE_PAGE_ADDR, 0x7FFFFEEFF000
.equ P9_REQUEST_OFFSET, 0x000
.equ P9_REPLY_OFFSET, 0x800
.equ P9_CONTROL_OFFSET, 0xF00

/* Tsys* message types (must match fcall.h) */
.equ Tsysopen,   130
.equ Rsysopen,   131
.equ Tsyscreate, 132
.equ Rsyscreate, 133
.equ Tsysclose,  134
.equ Rsysclose,  135
.equ Tsysread,   136
.equ Rsysread,   137
.equ Tsyswrite,  138
.equ Rsyswrite,  139
.equ Tsysexit,   146
.equ Rsysexit,   147

/* Control block offsets */
.equ CTL_DOORBELL,  0
.equ CTL_STATUS,    4
.equ CTL_REQ_HEAD,  8
.equ CTL_REQ_TAIL, 12
.equ CTL_REP_HEAD, 16
.equ CTL_REP_SIZE, 20

/* Status values */
.equ P9_STATUS_IDLE,     0
.equ P9_STATUS_PENDING,  1
.equ P9_STATUS_COMPLETE, 2
.equ P9_STATUS_ERROR,    3

/*
 * Internal helper: ring_doorbell
 * Issues syscall to trap into kernel - the syscall IS the doorbell.
 * Kernel reads message directly from exchange page.
 * Input: %rdi = exchange page base
 * Destroys: %rax, %rcx
 */
ring_doorbell:
    /* syscall is the doorbell - kernel checks exchange page on trap */
    syscall
    
    /* Wait for status != PENDING (kernel sets this) */
.Lwait_loop:
    movl P9_CONTROL_OFFSET + CTL_STATUS(%rdi), %eax
    cmpl $P9_STATUS_PENDING, %eax
    je .Lwait_loop
    
    ret

/*
 * p9_open - Open a file via 9P
 * int p9_open(const char *path, int mode)
 * 
 * Message format:
 *   size[4] type[1] tag[2] fid[4] name[s] mode[1]
 */
.global p9_open
p9_open:
    push %rbx
    push %r12
    push %r13
    
    mov %rdi, %r12          /* path */
    mov %esi, %r13d         /* mode */
    
    /* Calculate string length */
    mov %rdi, %rdi
    xor %rcx, %rcx
.Lstrlen_loop:
    cmpb $0, (%rdi, %rcx)
    je .Lstrlen_done
    inc %rcx
    jmp .Lstrlen_loop
.Lstrlen_done:
    mov %rcx, %rbx          /* strlen in %rbx */
    
    /* Get exchange page address */
    movabs $EXCHANGE_PAGE_ADDR, %rdi
    
    /* Build Tsysopen message:
     * size[4] = 4+1+2+4+2+strlen+1 = 14+strlen
     * type[1] = 130
     * tag[2] = 0
     * fid[4] = 0 (kernel assigns)
     * name[2+strlen] = path string with length prefix
     * mode[1] = mode
     */
    lea 14(%rbx), %eax
    movl %eax, 0(%rdi)           /* size */
    movb $Tsysopen, 4(%rdi)      /* type */
    movw $0, 5(%rdi)             /* tag */
    movl $0, 7(%rdi)             /* fid */
    movw %bx, 11(%rdi)           /* name length */
    
    /* Copy path string */
    lea 13(%rdi), %rcx           /* dest */
    mov %r12, %rsi               /* src */
    mov %rbx, %rdx               /* len */
.Lcopy_loop:
    test %rdx, %rdx
    jz .Lcopy_done
    movb (%rsi), %al
    movb %al, (%rcx)
    inc %rsi
    inc %rcx
    dec %rdx
    jmp .Lcopy_loop
.Lcopy_done:
    
    /* mode byte after string */
    lea 13(%rdi, %rbx), %rcx
    movb %r13b, (%rcx)
    
    /* Update control block */
    lea 14(%rbx), %eax
    movl $0, P9_CONTROL_OFFSET + CTL_REQ_HEAD(%rdi)
    movl %eax, P9_CONTROL_OFFSET + CTL_REQ_TAIL(%rdi)
    
    /* Ring doorbell and wait */
    call ring_doorbell
    
    /* Read result from reply: fd is in reply[7..10] (as fid) */
    movl P9_REPLY_OFFSET + 7(%rdi), %eax
    
    pop %r13
    pop %r12
    pop %rbx
    ret

/*
 * p9_close - Close a file via 9P
 * int p9_close(int fd)
 */
.global p9_close
p9_close:
    push %rbx
    mov %edi, %ebx              /* fd */
    
    movabs $EXCHANGE_PAGE_ADDR, %rdi
    
    /* Build Tsysclose message:
     * size[4] = 11
     * type[1] = 134
     * tag[2] = 0
     * fid[4] = fd
     */
    movl $11, 0(%rdi)            /* size */
    movb $Tsysclose, 4(%rdi)     /* type */
    movw $0, 5(%rdi)             /* tag */
    movl %ebx, 7(%rdi)           /* fid */
    
    /* Update control block */
    movl $0, P9_CONTROL_OFFSET + CTL_REQ_HEAD(%rdi)
    movl $11, P9_CONTROL_OFFSET + CTL_REQ_TAIL(%rdi)
    
    call ring_doorbell
    
    xor %eax, %eax              /* return 0 */
    pop %rbx
    ret

/*
 * p9_read - Read from a file via 9P
 * ssize_t p9_read(int fd, void *buf, size_t count)
 */
.global p9_read
p9_read:
    push %rbx
    push %r12
    push %r13
    
    mov %edi, %ebx              /* fd */
    mov %rsi, %r12              /* buf */
    mov %rdx, %r13              /* count */
    
    movabs $EXCHANGE_PAGE_ADDR, %rdi
    
    /* Build Tsysread message:
     * size[4] = 23
     * type[1] = 136
     * tag[2] = 0
     * fid[4] = fd
     * offset[8] = 0 (current pos)
     * count[4] = count
     */
    movl $23, 0(%rdi)            /* size */
    movb $Tsysread, 4(%rdi)      /* type */
    movw $0, 5(%rdi)             /* tag */
    movl %ebx, 7(%rdi)           /* fid */
    movq $0, 11(%rdi)            /* offset */
    movl %r13d, 19(%rdi)         /* count */
    
    /* Update control block */
    movl $0, P9_CONTROL_OFFSET + CTL_REQ_HEAD(%rdi)
    movl $23, P9_CONTROL_OFFSET + CTL_REQ_TAIL(%rdi)
    
    call ring_doorbell
    
    /* Get count from reply[11] and copy data from reply[15..] */
    movl P9_REPLY_OFFSET + 11(%rdi), %ebx  /* count returned */
    
    /* Copy data to user buffer */
    lea P9_REPLY_OFFSET + 15(%rdi), %rsi   /* src */
    mov %r12, %rcx                          /* dest */
    mov %rbx, %rdx                          /* len */
.Lread_copy:
    test %rdx, %rdx
    jz .Lread_done
    movb (%rsi), %al
    movb %al, (%rcx)
    inc %rsi
    inc %rcx
    dec %rdx
    jmp .Lread_copy
.Lread_done:
    
    mov %ebx, %eax              /* return count */
    
    pop %r13
    pop %r12
    pop %rbx
    ret

/*
 * p9_write - Write to a file via 9P
 * ssize_t p9_write(int fd, const void *buf, size_t count)
 */
.global p9_write
p9_write:
    push %rbx
    push %r12
    push %r13
    
    mov %edi, %ebx              /* fd */
    mov %rsi, %r12              /* buf */
    mov %rdx, %r13              /* count */
    
    movabs $EXCHANGE_PAGE_ADDR, %rdi
    
    /* Build Tsyswrite message:
     * size[4] = 23 + count
     * type[1] = 138
     * tag[2] = 0
     * fid[4] = fd
     * offset[8] = 0 (current pos)
     * count[4] = count
     * data[count]
     */
    lea 23(%r13), %eax
    movl %eax, 0(%rdi)           /* size */
    movb $Tsyswrite, 4(%rdi)     /* type */
    movw $0, 5(%rdi)             /* tag */
    movl %ebx, 7(%rdi)           /* fid */
    movq $0, 11(%rdi)            /* offset */
    movl %r13d, 19(%rdi)         /* count */
    
    /* Copy data from user buffer to request */
    mov %r12, %rsi               /* src */
    lea 23(%rdi), %rcx           /* dest */
    mov %r13, %rdx               /* len */
.Lwrite_copy:
    test %rdx, %rdx
    jz .Lwrite_done
    movb (%rsi), %al
    movb %al, (%rcx)
    inc %rsi
    inc %rcx
    dec %rdx
    jmp .Lwrite_copy
.Lwrite_done:
    
    /* Update control block */
    lea 23(%r13), %eax
    movl $0, P9_CONTROL_OFFSET + CTL_REQ_HEAD(%rdi)
    movl %eax, P9_CONTROL_OFFSET + CTL_REQ_TAIL(%rdi)
    
    call ring_doorbell
    
    /* Get count from reply[7] */
    movl P9_REPLY_OFFSET + 7(%rdi), %eax
    
    pop %r13
    pop %r12
    pop %rbx
    ret

/*
 * p9_exit - Exit process via 9P
 * void p9_exit(const char *status) __attribute__((noreturn))
 */
.global p9_exit
p9_exit:
    mov %rdi, %rsi              /* status string (or 0) */
    
    movabs $EXCHANGE_PAGE_ADDR, %rdi
    
    /* Build Tsysexit message:
     * size[4] = 7 (minimal, no status string)
     * type[1] = 146
     * tag[2] = 0
     */
    movl $7, 0(%rdi)             /* size */
    movb $Tsysexit, 4(%rdi)      /* type */
    movw $0, 5(%rdi)             /* tag */
    
    /* Update control block */
    movl $0, P9_CONTROL_OFFSET + CTL_REQ_HEAD(%rdi)
    movl $7, P9_CONTROL_OFFSET + CTL_REQ_TAIL(%rdi)
    
    /* Ring doorbell - should not return */
    movl $1, P9_CONTROL_OFFSET + CTL_DOORBELL(%rdi)
    syscall
    
.Lexit_loop:
    jmp .Lexit_loop
