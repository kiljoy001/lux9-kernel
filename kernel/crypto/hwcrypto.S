/*
 * Hardware-Accelerated Cryptography
 *
 * Assembly routines for SHA extensions and AES-NI
 */

.text

/*
 * SHA256 using Intel SHA extensions
 *
 * void sha256_transform_hw(uint32_t state[8], const uint8_t block[64], uint32_t nblocks);
 *
 * Uses SHA256RNDS2, SHA256MSG1, SHA256MSG2 instructions
 * Based on Intel's SHA extensions programming guide
 */
.globl sha256_transform_hw
.type sha256_transform_hw, @function
sha256_transform_hw:
	/* Save registers */
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	/* Arguments:
	 * rdi = state pointer (uint32_t[8])
	 * rsi = data pointer (const uint8_t*)
	 * rdx = number of blocks
	 */
	movq	%rdx, %r12		/* Save block count */

	/* Load initial state into XMM registers
	 * SHA extensions use ABEF and CDGH order */
	movdqu	0(%rdi), %xmm0		/* ABCD */
	movdqu	16(%rdi), %xmm1		/* EFGH */

	/* Shuffle to ABEF and CDGH */
	pshufd	$0xB1, %xmm0, %xmm0	/* CDAB */
	pshufd	$0x1B, %xmm1, %xmm1	/* HGFE */
	movdqa	%xmm0, %xmm7		/* CDAB */
	palignr	$8, %xmm1, %xmm0	/* ABEF */
	pblendw	$0xF0, %xmm7, %xmm1	/* CDGH */

.Lblock_loop:
	/* Save current hash */
	movdqa	%xmm0, %xmm4		/* Save ABEF */
	movdqa	%xmm1, %xmm5		/* Save CDGH */

	/* Load message schedule (W0-W3) */
	movdqu	0(%rsi), %xmm2		/* W0 W1 W2 W3 */
	pshufb	.Lshuf_mask(%rip), %xmm2

	movdqu	16(%rsi), %xmm3		/* W4 W5 W6 W7 */
	pshufb	.Lshuf_mask(%rip), %xmm3

	/* Process all 64 rounds in 16 groups of 4 */

	/* Rounds 0-3 */
	movdqa	%xmm2, %xmm6
	paddd	.LK256+0(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1

	/* Rounds 4-7 */
	movdqa	%xmm3, %xmm6
	paddd	.LK256+16(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm3, %xmm2

	/* Load W8-W11 */
	movdqu	32(%rsi), %xmm4
	pshufb	.Lshuf_mask(%rip), %xmm4

	/* Rounds 8-11 */
	movdqa	%xmm4, %xmm6
	paddd	.LK256+32(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm4, %xmm3

	/* Load W12-W15 */
	movdqu	48(%rsi), %xmm5
	pshufb	.Lshuf_mask(%rip), %xmm5

	/* Rounds 12-15 */
	movdqa	%xmm5, %xmm6
	paddd	.LK256+48(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm5, %xmm7
	palignr	$4, %xmm4, %xmm7
	paddd	%xmm7, %xmm2
	sha256msg2 %xmm5, %xmm2
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm5, %xmm4

	/* Rounds 16-19 */
	movdqa	%xmm2, %xmm6
	paddd	.LK256+64(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm2, %xmm7
	palignr	$4, %xmm5, %xmm7
	paddd	%xmm7, %xmm3
	sha256msg2 %xmm2, %xmm3
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm2, %xmm5

	/* Rounds 20-23 */
	movdqa	%xmm3, %xmm6
	paddd	.LK256+80(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm3, %xmm7
	palignr	$4, %xmm2, %xmm7
	paddd	%xmm7, %xmm4
	sha256msg2 %xmm3, %xmm4
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm3, %xmm2

	/* Rounds 24-27 */
	movdqa	%xmm4, %xmm6
	paddd	.LK256+96(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm4, %xmm7
	palignr	$4, %xmm3, %xmm7
	paddd	%xmm7, %xmm5
	sha256msg2 %xmm4, %xmm5
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm4, %xmm3

	/* Rounds 28-31 */
	movdqa	%xmm5, %xmm6
	paddd	.LK256+112(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm5, %xmm7
	palignr	$4, %xmm4, %xmm7
	paddd	%xmm7, %xmm2
	sha256msg2 %xmm5, %xmm2
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm5, %xmm4

	/* Rounds 32-35 */
	movdqa	%xmm2, %xmm6
	paddd	.LK256+128(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm2, %xmm7
	palignr	$4, %xmm5, %xmm7
	paddd	%xmm7, %xmm3
	sha256msg2 %xmm2, %xmm3
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm2, %xmm5

	/* Rounds 36-39 */
	movdqa	%xmm3, %xmm6
	paddd	.LK256+144(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm3, %xmm7
	palignr	$4, %xmm2, %xmm7
	paddd	%xmm7, %xmm4
	sha256msg2 %xmm3, %xmm4
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm3, %xmm2

	/* Rounds 40-43 */
	movdqa	%xmm4, %xmm6
	paddd	.LK256+160(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm4, %xmm7
	palignr	$4, %xmm3, %xmm7
	paddd	%xmm7, %xmm5
	sha256msg2 %xmm4, %xmm5
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm4, %xmm3

	/* Rounds 44-47 */
	movdqa	%xmm5, %xmm6
	paddd	.LK256+176(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm5, %xmm7
	palignr	$4, %xmm4, %xmm7
	paddd	%xmm7, %xmm2
	sha256msg2 %xmm5, %xmm2
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm5, %xmm4

	/* Rounds 48-51 */
	movdqa	%xmm2, %xmm6
	paddd	.LK256+192(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm2, %xmm7
	palignr	$4, %xmm5, %xmm7
	paddd	%xmm7, %xmm3
	sha256msg2 %xmm2, %xmm3
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1
	sha256msg1 %xmm2, %xmm5

	/* Rounds 52-55 */
	movdqa	%xmm3, %xmm6
	paddd	.LK256+208(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm3, %xmm7
	palignr	$4, %xmm2, %xmm7
	paddd	%xmm7, %xmm4
	sha256msg2 %xmm3, %xmm4
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1

	/* Rounds 56-59 */
	movdqa	%xmm4, %xmm6
	paddd	.LK256+224(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	movdqa	%xmm4, %xmm7
	palignr	$4, %xmm3, %xmm7
	paddd	%xmm7, %xmm5
	sha256msg2 %xmm4, %xmm5
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1

	/* Rounds 60-63 */
	movdqa	%xmm5, %xmm6
	paddd	.LK256+240(%rip), %xmm6
	sha256rnds2 %xmm1, %xmm0
	pshufd	$0x0E, %xmm6, %xmm6
	sha256rnds2 %xmm0, %xmm1

	/* Add saved hash to current hash */
	movdqu	0(%rdi), %xmm6
	movdqu	16(%rdi), %xmm7
	pshufd	$0xB1, %xmm6, %xmm6
	pshufd	$0x1B, %xmm7, %xmm7
	movdqa	%xmm6, %xmm2
	palignr	$8, %xmm7, %xmm6
	pblendw	$0xF0, %xmm2, %xmm7

	paddd	%xmm6, %xmm0		/* Add ABEF */
	paddd	%xmm7, %xmm1		/* Add CDGH */

	/* Next block */
	addq	$64, %rsi
	decq	%r12
	jnz	.Lblock_loop

	/* Shuffle back to ABCD and EFGH order */
	pshufd	$0x1B, %xmm0, %xmm0	/* FEBA */
	pshufd	$0xB1, %xmm1, %xmm1	/* GHCD */
	movdqa	%xmm0, %xmm7
	pblendw	$0xF0, %xmm1, %xmm0	/* ABCD */
	palignr	$8, %xmm7, %xmm1	/* EFGH */

	/* Store result */
	movdqu	%xmm0, 0(%rdi)
	movdqu	%xmm1, 16(%rdi)

	/* Restore registers */
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbp
	popq	%rbx
	ret

/* Byte shuffle mask for endianness conversion */
.align 16
.Lshuf_mask:
	.long	0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f

/* SHA256 K constants - all 64 round constants */
.align 64
.LK256:
	.long	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
	.long	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
	.long	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
	.long	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174
	.long	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc
	.long	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da
	.long	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7
	.long	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967
	.long	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13
	.long	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85
	.long	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3
	.long	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070
	.long	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5
	.long	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3
	.long	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208
	.long	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2


/*
 * Check if SHA extensions are available
 * Returns 1 if available, 0 otherwise
 */
.globl hwcrypto_have_sha
.type hwcrypto_have_sha, @function
hwcrypto_have_sha:
	/* This is set by cpuidentify(), just return m->havesha */
	/* For now, return 0 as placeholder - will be replaced with proper check */
	xorl	%eax, %eax
	ret

/*
 * Check if AES-NI is available
 * Returns 1 if available, 0 otherwise
 */
.globl hwcrypto_have_aes
.type hwcrypto_have_aes, @function
hwcrypto_have_aes:
	/* This is set by cpuidentify(), just return m->haveaes */
	/* For now, return 0 as placeholder - will be replaced with proper check */
	xorl	%eax, %eax
	ret
