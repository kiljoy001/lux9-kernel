# KUnit-Style Tests

This directory contains a lightweight KUnit-style host test harness for Lux9.

## Run

From repo root:

```bash
make kunit
```

This compiles and runs host-side unit tests. It does not boot QEMU.

## Structure

- `kunit.h`: minimal KUnit-style types and `KUNIT_EXPECT_*` macros
- `kunit_main.c`: suite/case runner and summary output
- `libc9_*_test.c`: host-safe libc helper suites
- `blind_cap_test.c`: critical capability/crypto unit coverage
- `capability_stubs.c`: deterministic `nsec()` and `randombytes()` for tests

## Adding Tests

1. Add a new `*_test.c` file in this directory.
2. Define test functions with signature:

```c
static void my_test(struct kunit *test)
```

3. Add them to a `struct kunit_case[]` and expose a `const struct kunit_suite`.
4. Register the suite in `kunit_main.c`.
5. Add any required source files to `KUNIT_HOST_SRCS` in `GNUmakefile`.

## System Servers With KUnit

KUnit here is host-side unit testing, not full server runtime testing.

Use KUnit for:
- parser/validator logic inside servers
- capability checks and policy decisions
- deterministic state-machine transitions
- helper functions with no hard dependency on scheduler/proc/chan globals

Do not use KUnit alone for:
- `/dev` and `/srv` mount/attach/create/read/write end-to-end behavior
- process parenting, wait/restart supervision, and crash loops
- lock/scheduler timing interactions

For system servers, test in two layers:
1. KUnit: isolated logic with deterministic stubs/mocks.
2. QEMU integration: boot image, start server, exercise real device/filesystem
   paths, and validate runtime behavior.
