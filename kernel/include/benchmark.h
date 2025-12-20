/* Phase 4b: Performance Benchmarking & Validation Framework */
#pragma once

#include "u.h"

/* Benchmark categories */
typedef enum {
	BENCH_BOOT = 0,
	BENCH_CONTEXT_SWITCH,
	BENCH_MEMORY_ALLOC,
	BENCH_SYSCALL,
	BENCH_IPC,
	BENCH_DEVICE_IO,
	BENCH_MAX
} BenchCategory;

/* Benchmark result structure */
typedef struct BenchResult {
	char name[64];
	BenchCategory category;
	uvlong start_tsc;      /* Start timestamp counter */
	uvlong end_tsc;        /* End timestamp counter */
	uvlong duration_ns;    /* Duration in nanoseconds */
	uvlong iterations;     /* Number of iterations */
	uvlong min_ns;         /* Minimum time */
	uvlong max_ns;         /* Maximum time */
	uvlong avg_ns;         /* Average time */
	int passed;            /* Test passed/failed */
} BenchResult;

/* Boot stage markers */
typedef enum {
	BOOT_STAGE_START = 0,
	BOOT_STAGE_MMU_INIT,
	BOOT_STAGE_IDT_INIT,
	BOOT_STAGE_MEMORY_INIT,
	BOOT_STAGE_DEVICE_INIT,
	BOOT_STAGE_PROC_INIT,
	BOOT_STAGE_SCHEDULER,
	BOOT_STAGE_MAX
} BootStage;

/* Global benchmark state */
typedef struct BenchState {
	int enabled;
	uvlong boot_stages[BOOT_STAGE_MAX];
	BenchResult results[256];
	int result_count;
	uvlong boot_start_tsc;
	uvlong boot_end_tsc;
} BenchState;

/* Benchmark API */
void benchmark_init(void);
void benchmark_enable(void);
void benchmark_disable(void);

/* Boot time tracking */
void benchmark_boot_start(void);
void benchmark_boot_stage(int stage);
void benchmark_boot_end(void);

/* Generic benchmark helpers */
uvlong benchmark_rdtsc(void);
void benchmark_start(BenchResult *result, const char *name, BenchCategory cat);
void benchmark_end(BenchResult *result);
void benchmark_record(BenchResult *result);

/* Specific benchmarks */
void benchmark_context_switch(void);
void benchmark_memory_alloc(void);
void benchmark_syscall(void);

/* Reporting */
void benchmark_print_results(void);
void benchmark_print_boot_time(void);
void benchmark_print_summary(void);

/* Validation checks */
int validate_boot_time(void);     /* Boot should be < 1 second */
int validate_context_switch(void); /* Context switch < 10us */
int validate_memory_alloc(void);   /* Allocation < 100ns */
int validate_all(void);

/* Global benchmark state */
extern BenchState benchstate;
