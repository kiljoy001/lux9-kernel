/* Phase 4b: Performance Benchmarking Implementation */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "benchmark.h"

BenchState benchstate;

/* Architecture-specific TSC reading */
extern uvlong rdtsc(void);

static const char* boot_stage_names[] = {
	"START",
	"MMU_INIT",
	"IDT_INIT",
	"MEMORY_INIT",
	"DEVICE_INIT",
	"PROC_INIT",
	"SCHEDULER"
};

static const char* category_names[] = {
	"BOOT",
	"CONTEXT_SWITCH",
	"MEMORY_ALLOC",
	"SYSCALL",
	"IPC",
	"DEVICE_IO"
};

void
benchmark_init(void)
{
	memset(&benchstate, 0, sizeof(benchstate));
	benchstate.enabled = 1;
	benchstate.result_count = 0;
}

void
benchmark_enable(void)
{
	benchstate.enabled = 1;
}

void
benchmark_disable(void)
{
	benchstate.enabled = 0;
}

uvlong
benchmark_rdtsc(void)
{
	return rdtsc();
}

void
benchmark_boot_start(void)
{
	if(!benchstate.enabled)
		return;
	benchstate.boot_start_tsc = rdtsc();
	benchstate.boot_stages[BOOT_STAGE_START] = benchstate.boot_start_tsc;
}

void
benchmark_boot_stage(int stage)
{
	if(!benchstate.enabled || stage >= BOOT_STAGE_MAX)
		return;
	benchstate.boot_stages[stage] = rdtsc();
}

void
benchmark_boot_end(void)
{
	if(!benchstate.enabled)
		return;
	benchstate.boot_end_tsc = rdtsc();
}

void
benchmark_start(BenchResult *result, const char *name, BenchCategory cat)
{
	if(!benchstate.enabled || !result)
		return;

	memset(result, 0, sizeof(BenchResult));
	strncpy(result->name, (char*)name, sizeof(result->name)-1);
	result->name[sizeof(result->name)-1] = '\0';
	result->category = cat;
	result->start_tsc = rdtsc();
	result->min_ns = ~0ULL;
	result->max_ns = 0;
}

void
benchmark_end(BenchResult *result)
{
	uvlong elapsed_tsc;

	if(!benchstate.enabled || !result)
		return;

	result->end_tsc = rdtsc();
	elapsed_tsc = result->end_tsc - result->start_tsc;

	/* Convert TSC to nanoseconds (assume 3GHz for now) */
	result->duration_ns = (elapsed_tsc * 1000) / 3000;

	if(result->iterations > 0) {
		result->avg_ns = result->duration_ns / result->iterations;
	}
}

void
benchmark_record(BenchResult *result)
{
	if(!benchstate.enabled || !result)
		return;

	if(benchstate.result_count >= 256) {
		iprint("benchmark: result buffer full\n");
		return;
	}

	memmove(&benchstate.results[benchstate.result_count],
	        result, sizeof(BenchResult));
	benchstate.result_count++;
}

void
benchmark_print_boot_time(void)
{
	int i;
	uvlong total_tsc, stage_tsc, prev_tsc;

	if(!benchstate.enabled)
		return;

	total_tsc = benchstate.boot_end_tsc - benchstate.boot_start_tsc;

	iprint("\n=== BOOT TIME ANALYSIS ===\n");
	iprint("Total boot time: %llud TSC cycles (~%llud ms)\n",
	       total_tsc, (total_tsc * 1000) / 3000000);

	iprint("\nBoot stages:\n");
	prev_tsc = benchstate.boot_stages[BOOT_STAGE_START];

	for(i = 1; i < BOOT_STAGE_MAX; i++) {
		if(benchstate.boot_stages[i] == 0)
			continue;

		stage_tsc = benchstate.boot_stages[i] - prev_tsc;
		iprint("  %-15s: %8llud TSC (~%5llud ms)\n",
		       boot_stage_names[i],
		       stage_tsc,
		       (stage_tsc * 1000) / 3000000);
		prev_tsc = benchstate.boot_stages[i];
	}
	iprint("\n");
}

void
benchmark_print_results(void)
{
	int i;
	BenchResult *r;

	if(!benchstate.enabled)
		return;

	iprint("\n=== BENCHMARK RESULTS ===\n");
	iprint("%-30s %-15s %12s %12s %6s\n",
	       "Name", "Category", "Duration(ns)", "Avg(ns)", "Pass");
	iprint("--------------------------------------------------------------------------------\n");

	for(i = 0; i < benchstate.result_count; i++) {
		r = &benchstate.results[i];
		iprint("%-30s %-15s %12llud %12llud %6s\n",
		       r->name,
		       category_names[r->category],
		       r->duration_ns,
		       r->avg_ns,
		       r->passed ? "PASS" : "FAIL");
	}
	iprint("\n");
}

void
benchmark_print_summary(void)
{
	benchmark_print_boot_time();
	benchmark_print_results();
}

/* Context switch benchmark */
void
benchmark_context_switch(void)
{
	BenchResult result;
	int i, iterations = 1000;
	uvlong start, end, elapsed;

	if(!benchstate.enabled)
		return;

	benchmark_start(&result, "context_switch_overhead", BENCH_CONTEXT_SWITCH);

	start = rdtsc();
	for(i = 0; i < iterations; i++) {
		/* Measure scheduler overhead */
		sched();
	}
	end = rdtsc();

	elapsed = end - start;
	result.iterations = iterations;
	result.avg_ns = ((elapsed * 1000) / 3000) / iterations;
	result.passed = (result.avg_ns < 10000); /* < 10us */

	benchmark_end(&result);
	benchmark_record(&result);
}

/* Memory allocation benchmark */
void
benchmark_memory_alloc(void)
{
	BenchResult result;
	int i, iterations = 10000;
	uvlong start, end;
	void *p;

	if(!benchstate.enabled)
		return;

	benchmark_start(&result, "small_alloc_free", BENCH_MEMORY_ALLOC);

	start = rdtsc();
	for(i = 0; i < iterations; i++) {
		p = mallocz(64, 1);
		if(p)
			free(p);
	}
	end = rdtsc();

	result.end_tsc = end;
	result.iterations = iterations;
	result.duration_ns = ((end - start) * 1000) / 3000;
	result.avg_ns = result.duration_ns / iterations;
	result.passed = (result.avg_ns < 1000); /* < 1us */

	benchmark_record(&result);
}

/* Validation functions */
int
validate_boot_time(void)
{
	uvlong boot_time_ms;

	if(!benchstate.enabled)
		return 1;

	boot_time_ms = ((benchstate.boot_end_tsc - benchstate.boot_start_tsc) * 1000) / 3000000;

	/* Boot should complete in < 5 seconds */
	return boot_time_ms < 5000;
}

int
validate_context_switch(void)
{
	int i;
	BenchResult *r;

	for(i = 0; i < benchstate.result_count; i++) {
		r = &benchstate.results[i];
		if(r->category == BENCH_CONTEXT_SWITCH) {
			return r->avg_ns < 10000; /* < 10us */
		}
	}

	return 1; /* Not tested yet */
}

int
validate_memory_alloc(void)
{
	int i;
	BenchResult *r;

	for(i = 0; i < benchstate.result_count; i++) {
		r = &benchstate.results[i];
		if(r->category == BENCH_MEMORY_ALLOC) {
			return r->avg_ns < 1000; /* < 1us */
		}
	}

	return 1; /* Not tested yet */
}

int
validate_all(void)
{
	int all_passed = 1;

	iprint("\n=== VALIDATION SUMMARY ===\n");

	if(!validate_boot_time()) {
		iprint("FAIL: Boot time exceeds threshold\n");
		all_passed = 0;
	} else {
		iprint("PASS: Boot time within acceptable range\n");
	}

	if(!validate_context_switch()) {
		iprint("FAIL: Context switch too slow\n");
		all_passed = 0;
	} else {
		iprint("PASS: Context switch performance acceptable\n");
	}

	if(!validate_memory_alloc()) {
		iprint("FAIL: Memory allocation too slow\n");
		all_passed = 0;
	} else {
		iprint("PASS: Memory allocation performance acceptable\n");
	}

	iprint("\nOverall: %s\n\n", all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

	return all_passed;
}
