/*
 * test_il_opcodes.c - Comprehensive IL Opcode Test Suite
 *
 * Tests ALL supported QBE IL opcodes by:
 * 1. Constructing QBE IL text for each opcode
 * 2. Compiling to x86-64 via qbe_compile_page
 * 3. Executing generated code
 * 4. Verifying result
 *
 * Run early in boot to shake out IL->ASM bugs.
 */

/* Use standard kernel includes via -I flags */
typedef unsigned long uintptr;
typedef unsigned long usize;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
#define nil ((void *)0)

/* Kernel functions we need */
extern int print(char *fmt, ...);
extern void *xallocz(ulong size, int zero);
extern void xfree(void *p);
extern char *strncpy(char *dst, char *src, long n);

/* Physical address from virtual - use identity for now */
#define PADDR(va) ((uintptr)(va))

/* Forward declarations */
extern int qbe_compile_page(uintptr qbe_page, uintptr asm_page, char *errorbuf,
                            usize errorbuf_size);

/* Test result tracking */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_PAGE_SIZE 4096

/* Helper to print test result */
static void test_result(char *name, int passed) {
  tests_run++;
  if (passed) {
    tests_passed++;
    print("IL_TEST: %-20s PASS\n", name);
  } else {
    tests_failed++;
    print("IL_TEST: %-20s FAIL\n", name);
  }
}

/*
 * Compile QBE IL and execute, returning the result.
 * QBE IL should define a function that returns an int.
 */
static long compile_and_run(char *qbe_il, char *test_name) {
  void *qbe_page, *asm_page;
  char errbuf[256];
  long (*fn)(void);
  long result;
  int ret;

  qbe_page = xallocz(TEST_PAGE_SIZE, 1);
  asm_page = xallocz(TEST_PAGE_SIZE, 1);

  if (qbe_page == nil || asm_page == nil) {
    print("IL_TEST: %s - allocation failed\n", test_name);
    if (qbe_page)
      xfree(qbe_page);
    if (asm_page)
      xfree(asm_page);
    return -999999;
  }

  /* Copy QBE IL to page */
  strncpy(qbe_page, qbe_il, TEST_PAGE_SIZE - 1);

  /* Compile */
  ret = qbe_compile_page(PADDR(qbe_page), PADDR(asm_page), errbuf,
                         sizeof(errbuf));
  if (ret < 0) {
    print("IL_TEST: %s - compile failed: %s\n", test_name, errbuf);
    xfree(qbe_page);
    xfree(asm_page);
    return -999998;
  }

  /* Execute */
  fn = (long (*)(void))asm_page;
  result = fn();

  xfree(qbe_page);
  xfree(asm_page);
  return result;
  xfree(asm_page);
  return result;
}

static long compile_and_run_args(char *qbe_il, char *test_name, long arg0) {
  void *qbe_page, *asm_page;
  char errbuf[256];
  long (*fn)(long, long); // Support 2 args
  long result;
  int ret;

  qbe_page = xallocz(TEST_PAGE_SIZE, 1);
  asm_page = xallocz(TEST_PAGE_SIZE, 1);

  if (qbe_page == nil || asm_page == nil) {
    print("IL_TEST: %s - allocation failed\n", test_name);
    if (qbe_page)
      xfree(qbe_page);
    if (asm_page)
      xfree(asm_page);
    return -999999;
  }

  strncpy(qbe_page, qbe_il, TEST_PAGE_SIZE - 1);

  ret = qbe_compile_page(PADDR(qbe_page), PADDR(asm_page), errbuf,
                         sizeof(errbuf));
  if (ret < 0) {
    print("IL_TEST: %s - compile failed: %s\n", test_name, errbuf);
    xfree(qbe_page);
    xfree(asm_page);
    return -999998;
  }

  fn = (long (*)(long, long))asm_page;
  result = fn(arg0, 0);

  xfree(qbe_page);
  xfree(asm_page);
  return result;
}

/*============================================================================
 * ARITHMETIC TESTS
 *============================================================================*/

static void test_add(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 10\n"
             "    %b =w copy 3\n"
             "    %r =w add %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "add");
  test_result("add", r == 13);
}

static void test_sub(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 10\n"
             "    %b =w copy 3\n"
             "    %r =w sub %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "sub");
  test_result("sub", r == 7);
}

static void test_mul(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 7\n"
             "    %b =w copy 6\n"
             "    %r =w mul %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "mul");
  test_result("mul", r == 42);
}

static void test_div(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 20\n"
             "    %b =w copy 4\n"
             "    %r =w div %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "div");
  test_result("div", r == 5);
}

static void test_udiv(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 21\n"
             "    %b =w copy 5\n"
             "    %r =w udiv %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "udiv");
  test_result("udiv", r == 4);
}

static void test_rem(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 17\n"
             "    %b =w copy 5\n"
             "    %r =w rem %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "rem");
  test_result("rem", r == 2);
}

static void test_urem(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 17\n"
             "    %b =w copy 5\n"
             "    %r =w urem %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "urem");
  test_result("urem", r == 2);
}

static void test_neg(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 42\n"
             "    %r =w neg %a\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "neg");
  test_result("neg", r == -42);
}

/*============================================================================
 * BITWISE TESTS
 *============================================================================*/

static void test_and(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 0xFF\n"
             "    %b =w copy 0x0F\n"
             "    %r =w and %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "and");
  test_result("and", r == 0x0F);
}

static void test_or(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 0xF0\n"
             "    %b =w copy 0x0F\n"
             "    %r =w or %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "or");
  test_result("or", r == 0xFF);
}

static void test_xor(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 0xFF\n"
             "    %b =w copy 0x0F\n"
             "    %r =w xor %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "xor");
  test_result("xor", r == 0xF0);
}

static void test_shl(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 1\n"
             "    %b =w copy 4\n"
             "    %r =w shl %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "shl");
  test_result("shl", r == 16);
}

static void test_shr(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 64\n"
             "    %b =w copy 3\n"
             "    %r =w shr %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "shr");
  test_result("shr", r == 8);
}

static void test_sar(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 64\n"
             "    %b =w copy 2\n"
             "    %r =w sar %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "sar");
  test_result("sar", r == 16);
}

/*============================================================================
 * COMPARISON TESTS (Word variants)
 *============================================================================*/

static void test_ceqw(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 5\n"
             "    %b =w copy 5\n"
             "    %r =w ceqw %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "ceqw");
  test_result("ceqw_true", r == 1);
}

static void test_cnew(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 5\n"
             "    %b =w copy 3\n"
             "    %r =w cnew %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "cnew");
  test_result("cnew_true", r == 1);
}

static void test_cslew(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 5\n"
             "    %b =w copy 1\n"
             "    %r =w cslew %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "cslew_false");
  test_result("cslew_false", r == 0);

  char *il2 = "export function w $test() {\n"
              "@start\n"
              "    %a =w copy 1\n"
              "    %b =w copy 5\n"
              "    %r =w cslew %a, %b\n"
              "    ret %r\n"
              "}\n";
  r = compile_and_run(il2, "cslew_true");
  test_result("cslew_true", r == 1);
}

static void test_csltw(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 3\n"
             "    %b =w copy 5\n"
             "    %r =w csltw %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "csltw");
  test_result("csltw", r == 1);
}

static void test_csgtw(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 5\n"
             "    %b =w copy 3\n"
             "    %r =w csgtw %a, %b\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "csgtw");
  test_result("csgtw", r == 1);
}

/*============================================================================
 * MEMORY TESTS
 *============================================================================*/

static void test_copy(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %a =w copy 99\n"
             "    ret %a\n"
             "}\n";
  long r = compile_and_run(il, "copy");
  test_result("copy", r == 99);
}

static void test_storel_loadl(void) {
  char *il = "export function l $test() {\n"
             "@start\n"
             "    %loc0 =l alloc8 8\n"
             "    %val =l copy 12345\n"
             "    storel %val, %loc0\n"
             "    %r =l loadl %loc0\n"
             "    ret %r\n"
             "}\n";
  long r = compile_and_run(il, "storel_loadl");
  test_result("storel_loadl", r == 12345);
}

/*============================================================================
 * CONTROL FLOW TESTS
 *============================================================================*/

static void test_ret_imm(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    ret 42\n"
             "}\n";
  long r = compile_and_run(il, "ret_imm");
  test_result("ret_imm", r == 42);
}

static void test_jmp(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    jmp @end\n"
             "@fail\n"
             "    ret 0\n"
             "@end\n"
             "    ret 1\n"
             "}\n";
  long r = compile_and_run(il, "jmp");
  test_result("jmp", r == 1);
}

static void test_jnz_true(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %cond =w copy 1\n"
             "    jnz %cond, @yes, @no\n"
             "@yes\n"
             "    ret 1\n"
             "@no\n"
             "    ret 0\n"
             "}\n";
  long r = compile_and_run(il, "jnz_true");
  test_result("jnz_true", r == 1);
}

static void test_jnz_false(void) {
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %cond =w copy 0\n"
             "    jnz %cond, @yes, @no\n"
             "@yes\n"
             "    ret 1\n"
             "@no\n"
             "    ret 0\n"
             "}\n";
  long r = compile_and_run(il, "jnz_false");
  test_result("jnz_false", r == 0);
}

static void test_fact_qbe(void) {
  /* Exact QBE from TestFact failure, using %arg0 */
  char *il = "export function w $test() {\n"
             "@start\n"
             "    %t1 =l copy %arg0\n" /* USE %arg0 */
             "    storel %t1, %loc0\n"
             "    %t2 =w copy 1\n"
             "    storel %t2, %loc1\n"
             "@loop\n"
             "    %t1 =l loadl %loc0\n"
             "    %t2 =w copy 1\n"
             "    %t1 =w cslew %t1, %t2\n"
             "    jnz %t1, @end, @body\n"
             "@body\n"
             "    %t1 =l loadl %loc1\n"
             "    %t2 =l loadl %loc0\n"
             "    %t1 =w mul %t1, %t2\n"
             "    storel %t1, %loc1\n"
             "    %t1 =l loadl %loc0\n"
             "    %t2 =w copy 1\n"
             "    %t1 =w sub %t1, %t2\n"
             "    storel %t1, %loc0\n"
             "    jmp @loop\n"
             "@end\n"
             "    %t1 =l loadl %loc1\n"
             "    ret %t1\n"
             "}\n";
  long r = compile_and_run_args(il, "fact_qbe_arg", 5);
  test_result("fact_qbe_arg(120)", r == 120);
}

/*============================================================================
 * MAIN TEST RUNNER
 *============================================================================*/

void run_il_opcode_tests(void) {
  print("\n========================================\n");
  print("IL OPCODE TEST SUITE\n");
  print("========================================\n\n");

  /* Arithmetic */
  print("--- Arithmetic ---\n");
  test_add();
  test_sub();
  test_mul();
  test_div();
  test_udiv();
  test_rem();
  test_urem();
  test_neg();

  /* Bitwise */
  print("\n--- Bitwise ---\n");
  test_and();
  test_or();
  test_xor();
  test_shl();
  test_shr();
  test_sar();

  /* Comparison */
  print("\n--- Comparison ---\n");
  test_ceqw();
  test_cnew();
  test_csltw();
  test_cnew();
  test_csltw();
  test_cslew();
  test_csgtw();

  /* Memory */
  print("\n--- Memory ---\n");
  test_copy();
  test_storel_loadl();

  /* Control Flow */
  print("\n--- Control Flow ---\n");
  test_ret_imm();
  test_jmp();
  test_jnz_true();
  test_jnz_true();
  test_jnz_false();
  test_fact_qbe();

  /* Summary */
  print("\n========================================\n");
  print("RESULTS: %d/%d passed, %d failed\n", tests_passed, tests_run,
        tests_failed);
  print("========================================\n\n");

  if (tests_failed > 0) {
    print("WARNING: Some IL opcode tests failed!\n");
  } else {
    print("All IL opcode tests passed!\n");
  }
}
