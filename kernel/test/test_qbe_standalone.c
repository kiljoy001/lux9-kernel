/*
 * test_qbe_standalone.c - Standalone QBE IL Test Suite
 *
 * Runs on host Linux for fast IL→ASM debugging without kernel boot.
 * Compiles QBE IL snippets and executes them to verify correctness.
 *
 * Build: gcc -o test_qbe test_qbe_standalone.c qbe_compile_standalone.c -O0 -g
 * Run:   ./test_qbe
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef uint64_t uintptr;
typedef size_t usize;
typedef unsigned long ulong;
typedef unsigned char u8int;
typedef int32_t s32int;
typedef uint32_t u32int;

#define TEST_PAGE_SIZE 4096
#define nil NULL

/* From qbe_compile.c - we'll include a stripped version */
extern int qbe_compile_page_standalone(char *qbe_il, uint8_t *code_out,
                                       size_t code_size, char *errbuf,
                                       size_t errsize);

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static int verbose = 1;

/* Helper to print test result */
static void test_result_v(const char *name, long actual, long expected) {
  tests_run++;
  int passed = (actual == expected);
  if (passed) {
    tests_passed++;
    printf("IL_TEST: %-20s \033[32mPASS\033[0m\n", name);
  } else {
    tests_failed++;
    printf("IL_TEST: %-20s \033[31mFAIL\033[0m", name);
    if (verbose) {
      printf("  (got %ld, expected %ld)", actual, expected);
    }
    printf("\n");
  }
}

#define test_result(name, actual, expected)                                    \
  test_result_v(name, actual, expected)

/*
 * Compile QBE IL and execute, returning the result.
 */
static long compile_and_run(const char *qbe_il, const char *test_name) {
  /* Allocate executable memory */
  void *code_page =
      mmap(NULL, TEST_PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code_page == MAP_FAILED) {
    printf("IL_TEST: %s - mmap failed\n", test_name);
    return -999999;
  }

  char errbuf[256] = {0};
  int ret = qbe_compile_page_standalone((char *)qbe_il, code_page,
                                        TEST_PAGE_SIZE, errbuf, sizeof(errbuf));
  if (ret < 0) {
    printf("IL_TEST: %s - compile failed: %s\n", test_name, errbuf);
    munmap(code_page, TEST_PAGE_SIZE);
    return -999998;
  }

  /* Execute */
  long (*fn)(void) = (long (*)(void))code_page;
  long result = fn();

  munmap(code_page, TEST_PAGE_SIZE);
  return result;
}

/*============================================================================
 * ARITHMETIC TESTS
 *============================================================================*/

static void test_add(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 10\n"
                   "    %b =w copy 3\n"
                   "    %r =w add %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "add");
  test_result("add", r, 13);
}

static void test_sub(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 10\n"
                   "    %b =w copy 3\n"
                   "    %r =w sub %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "sub");
  test_result("sub", r, 7);
}

static void test_mul(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 7\n"
                   "    %b =w copy 6\n"
                   "    %r =w mul %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "mul");
  test_result("mul", r, 42);
}

static void test_div(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 20\n"
                   "    %b =w copy 4\n"
                   "    %r =w div %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "div");
  test_result("div", r, 5);
}

static void test_neg(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 42\n"
                   "    %r =w neg %a\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "neg");
  test_result("neg", r, -42);
}

/*============================================================================
 * BITWISE TESTS
 *============================================================================*/

static void test_and(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 255\n"
                   "    %b =w copy 15\n"
                   "    %r =w and %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "and");
  test_result("and", r, 15);
}

static void test_or(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 240\n"
                   "    %b =w copy 15\n"
                   "    %r =w or %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "or");
  test_result("or", r, 255);
}

static void test_xor(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 255\n"
                   "    %b =w copy 15\n"
                   "    %r =w xor %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "xor");
  test_result("xor", r, 240);
}

static void test_shl(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 1\n"
                   "    %b =w copy 4\n"
                   "    %r =w shl %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "shl");
  test_result("shl", r, 16);
}

static void test_shr(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 64\n"
                   "    %b =w copy 3\n"
                   "    %r =w shr %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "shr");
  test_result("shr", r, 8);
}

/*============================================================================
 * COMPARISON TESTS
 *============================================================================*/

static void test_ceqw(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 5\n"
                   "    %b =w copy 5\n"
                   "    %r =w ceqw %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "ceqw");
  test_result("ceqw_true", r, 1);
}

static void test_cnew(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 5\n"
                   "    %b =w copy 3\n"
                   "    %r =w cnew %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "cnew");
  test_result("cnew_true", r, 1);
}

static void test_csltw(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 3\n"
                   "    %b =w copy 5\n"
                   "    %r =w csltw %a, %b\n"
                   "    ret %r\n"
                   "}\n";
  long r = compile_and_run(il, "csltw");
  test_result("csltw", r, 1);
}

/*============================================================================
 * MEMORY TESTS
 *============================================================================*/

static void test_copy(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %a =w copy 99\n"
                   "    ret %a\n"
                   "}\n";
  long r = compile_and_run(il, "copy");
  test_result("copy", r, 99);
}

/*============================================================================
 * CONTROL FLOW TESTS
 *============================================================================*/

static void test_ret_imm(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    ret 42\n"
                   "}\n";
  long r = compile_and_run(il, "ret_imm");
  test_result("ret_imm", r, 42);
}

static void test_jmp(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    jmp @end\n"
                   "@fail\n"
                   "    ret 0\n"
                   "@end\n"
                   "    ret 1\n"
                   "}\n";
  long r = compile_and_run(il, "jmp");
  test_result("jmp", r, 1);
}

static void test_jnz_true(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %cond =w copy 1\n"
                   "    jnz %cond, @yes, @no\n"
                   "@yes\n"
                   "    ret 1\n"
                   "@no\n"
                   "    ret 0\n"
                   "}\n";
  long r = compile_and_run(il, "jnz_true");
  test_result("jnz_true", r, 1);
}

static void test_jnz_false(void) {
  const char *il = "export function w $test() {\n"
                   "@start\n"
                   "    %cond =w copy 0\n"
                   "    jnz %cond, @yes, @no\n"
                   "@yes\n"
                   "    ret 1\n"
                   "@no\n"
                   "    ret 0\n"
                   "}\n";
  long r = compile_and_run(il, "jnz_false");
  test_result("jnz_false", r, 0);
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(int argc, char *argv[]) {
  printf("\n========================================\n");
  printf("STANDALONE QBE IL TEST SUITE\n");
  printf("========================================\n\n");

  /* Arithmetic */
  printf("--- Arithmetic ---\n");
  test_add();
  test_sub();
  test_mul();
  test_div();
  test_neg();

  /* Bitwise */
  printf("\n--- Bitwise ---\n");
  test_and();
  test_or();
  test_xor();
  test_shl();
  test_shr();

  /* Comparison */
  printf("\n--- Comparison ---\n");
  test_ceqw();
  test_cnew();
  test_csltw();

  /* Memory */
  printf("\n--- Memory ---\n");
  test_copy();

  /* Control Flow */
  printf("\n--- Control Flow ---\n");
  test_ret_imm();
  test_jmp();
  test_jnz_true();
  test_jnz_false();

  /* Summary */
  printf("\n========================================\n");
  printf("RESULTS: %d/%d passed, %d failed\n", tests_passed, tests_run,
         tests_failed);
  printf("========================================\n\n");

  return tests_failed > 0 ? 1 : 0;
}
