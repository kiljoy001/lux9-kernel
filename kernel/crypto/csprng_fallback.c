/*
 * ChaCha20-based CSPRNG Fallback
 *
 * Software cryptographically secure PRNG for systems without hardware RNG.
 * Uses randomized multi-source entropy collection with minimum 3 sources.
 *
 * SECURITY WARNING: This is a FALLBACK for development/testing only.
 * Production systems MUST use hardware RNG (TPM or RDRAND).
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* Monocypher uses these type names (match monocypher.c) */
typedef u8int  u8;
typedef u32int u32;
typedef u64int u64;

/* Monocypher ChaCha20 functions */
extern u64 crypto_chacha20_djb(u8 *cipher_text, const u8 *plain_text,
                                u64 text_size, const u8 key[32],
                                const u8 nonce[8], u64 ctr);
extern void crypto_blake2b(u8 *hash, u64 hash_size, const u8 *message, u64 message_size);

/* Helper to load little-endian u64 */
static u64
load64_le(const u8 s[8])
{
	return ((u64)s[0] <<  0) |
	       ((u64)s[1] <<  8) |
	       ((u64)s[2] << 16) |
	       ((u64)s[3] << 24) |
	       ((u64)s[4] << 32) |
	       ((u64)s[5] << 40) |
	       ((u64)s[6] << 48) |
	       ((u64)s[7] << 56);
}

/* CSPRNG state */
static struct {
	u8 key[32];
	u8 nonce[8];
	u64 counter;
	int initialized;
	u64 reseed_count;
} csprng_state;

/* Entropy source enumeration */
enum EntropySource {
	ENTROPY_TSC = 0,          /* CPU timestamp counter */
	ENTROPY_STACK_PTR,        /* Stack pointer (ASLR) */
	ENTROPY_HEAP_PTR,         /* Heap pointer (ASLR) */
	ENTROPY_FUNC_PTR,         /* Function pointer (ASLR) */
	ENTROPY_BOOT_TIME,        /* Boot timestamp */
	ENTROPY_MACHNO,           /* CPU number */
	ENTROPY_PID,              /* Current process ID */
	ENTROPY_MEM_LAYOUT,       /* Memory map values */
	ENTROPY_IOPORT,           /* I/O port noise (if available) */
	ENTROPY_INTERRUPT_COUNT,  /* Interrupt counters */
	ENTROPY_SOURCE_COUNT      /* Total number of sources */
};

/* Collect entropy from a specific source */
static u64
collect_entropy_source(enum EntropySource source)
{
	u64 entropy = 0;
	extern Mach *m;
	extern Proc *up;

	switch(source) {
	case ENTROPY_TSC:
		/* Timestamp counter - timing jitter */
		entropy = rdtsc();
		break;

	case ENTROPY_STACK_PTR:
		/* Stack pointer address (ASLR) */
		entropy = (u64)(uintptr)&entropy;
		break;

	case ENTROPY_HEAP_PTR:
		/* Use global data pointer as heap proxy */
		entropy = (u64)(uintptr)&csprng_state;
		break;

	case ENTROPY_FUNC_PTR:
		/* Function pointer (code ASLR) */
		entropy = (u64)(uintptr)collect_entropy_source;
		break;

	case ENTROPY_BOOT_TIME:
		/* Boot timestamp */
		entropy = fastticks(nil);
		break;

	case ENTROPY_MACHNO:
		/* CPU number mixed with m pointer */
		entropy = (u64)m->machno ^ (u64)(uintptr)m;
		break;

	case ENTROPY_PID:
		/* Process ID (if available) */
		if(up != nil)
			entropy = up->pid ^ (u64)(uintptr)up;
		else
			entropy = rdtsc(); /* Fallback to TSC */
		break;

	case ENTROPY_MEM_LAYOUT:
		/* Memory configuration entropy */
		extern Conf conf;
		entropy = conf.npage ^ conf.nproc ^ (u64)(uintptr)&conf;
		break;

	case ENTROPY_IOPORT:
		/* CMOS clock (I/O port 0x70/0x71) for additional jitter */
		/* Only safe on x86, adds low-quality timing noise */
		outb(0x70, 0x00); /* Select CMOS register 0 (seconds) */
		entropy = inb(0x71);
		entropy ^= rdtsc(); /* Mix with TSC for timing jitter */
		break;

	case ENTROPY_INTERRUPT_COUNT:
		/* Interrupt statistics */
		if(m != nil)
			entropy = rdtsc() ^ (u64)m->machno;
		else
			entropy = rdtsc();
		break;

	default:
		entropy = rdtsc(); /* Fallback */
		break;
	}

	return entropy;
}

/*
 * Randomized multi-source entropy collection
 *
 * Strategy:
 * 1. Use TSC low bits to select source order (meta-randomness)
 * 2. Collect from minimum 3 sources
 * 3. Mix sources with different TSC samples (timing jitter)
 * 4. Hash with BLAKE2b for entropy extraction
 */
static void
collect_randomized_entropy(u8 output[32])
{
	u8 entropy_buffer[256];
	u64 *buf64 = (u64*)entropy_buffer;
	int idx = 0;
	int sources_used = 0;
	u64 tsc_seed = rdtsc();

	/* Use TSC low bits to randomize source selection order */
	u64 source_order = tsc_seed;

	/* Minimum 3 sources, maximum all sources */
	int min_sources = 3;
	int max_sources = ENTROPY_SOURCE_COUNT;

	/* Collect from randomized sources */
	for(int attempt = 0; attempt < max_sources && sources_used < max_sources; attempt++) {
		/* Select source based on TSC-derived randomness */
		enum EntropySource src = (source_order + attempt * 7) % ENTROPY_SOURCE_COUNT;

		/* Add timing jitter between collections */
		u64 tsc_before = rdtsc();
		u64 entropy = collect_entropy_source(src);
		u64 tsc_after = rdtsc();
		u64 timing_jitter = tsc_after - tsc_before;

		/* Mix entropy with timing jitter */
		buf64[idx++] = entropy ^ timing_jitter ^ rdtsc();
		sources_used++;

		/* Stop if buffer nearly full */
		if(idx >= 30) /* Leave room for final values */
			break;
	}

	/* Enforce minimum 3 sources */
	while(sources_used < min_sources && idx < 31) {
		buf64[idx++] = rdtsc() ^ collect_entropy_source(sources_used % ENTROPY_SOURCE_COUNT);
		sources_used++;
	}

	/* Add final mixing values */
	buf64[idx++] = rdtsc();
	buf64[idx++] = (u64)(uintptr)&entropy_buffer;

	/* Hash all collected entropy with BLAKE2b */
	crypto_blake2b(output, 32, entropy_buffer, idx * sizeof(u64));
}

/*
 * Initialize CSPRNG with randomized multi-source entropy
 */
static void
csprng_init(void)
{
	u8 seed_material[64];

	print("WARNING: Initializing ChaCha20 CSPRNG with software entropy\n");
	print("WARNING: Production systems MUST have TPM or RDRAND!\n");

	/* Collect randomized entropy for key */
	collect_randomized_entropy(seed_material);

	/* Wait for timing jitter */
	for(int i = 0; i < 100; i++)
		rdtsc();

	/* Collect randomized entropy for nonce */
	collect_randomized_entropy(seed_material + 32);

	/* Initialize CSPRNG state */
	memmove(csprng_state.key, seed_material, 32);
	memmove(csprng_state.nonce, seed_material + 32, 8);
	csprng_state.counter = load64_le(seed_material + 40);
	csprng_state.initialized = 1;
	csprng_state.reseed_count = 0;

	print("CSPRNG: Initialized with randomized multi-source entropy\n");
	print("CSPRNG: Key derived from %d potential entropy sources\n", ENTROPY_SOURCE_COUNT);
}

/*
 * Reseed CSPRNG periodically for forward secrecy
 */
static void
csprng_reseed(void)
{
	u8 new_seed[32];

	collect_randomized_entropy(new_seed);

	/* Mix new entropy with existing key */
	for(int i = 0; i < 32; i++)
		csprng_state.key[i] ^= new_seed[i];

	/* Update nonce from new entropy */
	memmove(csprng_state.nonce, new_seed + 24, 8);

	csprng_state.reseed_count++;
}

/*
 * Generate 64-bit random value using ChaCha20 CSPRNG
 */
u64int
chacha20_csprng_u64(void)
{
	u8 output[8];
	u8 zeros[8] = {0};

	if(!csprng_state.initialized)
		csprng_init();

	/* Reseed every 1MB of output (128K calls) */
	if((csprng_state.counter & 0x1FFFF) == 0 && csprng_state.counter > 0)
		csprng_reseed();

	/* Generate random bytes using ChaCha20 */
	crypto_chacha20_djb(output, zeros, 8,
	                     csprng_state.key,
	                     csprng_state.nonce,
	                     csprng_state.counter++);

	return load64_le(output);
}

/*
 * Fill buffer with random bytes
 */
void
chacha20_csprng_fill(u8 *buf, ulong len)
{
	u8 zeros[256];

	if(!csprng_state.initialized)
		csprng_init();

	memset(zeros, 0, sizeof(zeros));

	while(len > 0) {
		ulong chunk = len > 256 ? 256 : len;

		crypto_chacha20_djb(buf, zeros, chunk,
		                     csprng_state.key,
		                     csprng_state.nonce,
		                     csprng_state.counter++);

		buf += chunk;
		len -= chunk;

		/* Reseed periodically */
		if((csprng_state.counter & 0x1FFFF) == 0)
			csprng_reseed();
	}
}

/*
 * Get CSPRNG statistics (for debugging)
 */
void
csprng_stats(void)
{
	if(!csprng_state.initialized) {
		print("CSPRNG: Not initialized\n");
		return;
	}

	print("CSPRNG: Initialized=%d Counter=%llud Reseeds=%llud\n",
	      csprng_state.initialized,
	      csprng_state.counter,
	      csprng_state.reseed_count);
}
