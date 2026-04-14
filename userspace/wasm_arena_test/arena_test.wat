(module
  ;; Memory is initially 1 page (64KB), can grow up to 20 pages (1.25MB)
  (memory (export "memory") 1 20)

  ;; Test 1: Progressive allocation (should succeed up to ~16 pages / 1MB)
  (func (export "test_progressive_alloc") (result i32)
    (local $pages_allocated i32)
    (local $i i32)
    (local $result i32)

    (local.set $pages_allocated (i32.const 0))
    (local.set $i (i32.const 0))

    (block $exit
      (loop $continue
        ;; Try to grow by 1 page
        (local.set $result (memory.grow (i32.const 1)))

        ;; If memory.grow returned -1, allocation failed
        (br_if $exit (i32.eq (local.get $result) (i32.const -1)))

        ;; Success, increment counter
        (local.set $pages_allocated (i32.add (local.get $pages_allocated) (i32.const 1)))

        ;; Increment loop counter and check if we've done 20 iterations
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (br_if $continue (i32.lt_u (local.get $i) (i32.const 20)))
      )
    )

    (local.get $pages_allocated)
  )

  ;; Test 2: Single large allocation (should fail if > 1MB)
  (func (export "test_large_alloc") (result i32)
    (local $result i32)

    ;; Try to allocate 20 pages (1.25MB) at once
    (local.set $result (memory.grow (i32.const 20)))

    ;; If failed (returned -1), return SUCCESS (1)
    (if (result i32) (i32.eq (local.get $result) (i32.const -1))
      (then (i32.const 1))  ;; SUCCESS: Limit enforced
      (else (i32.const 0))  ;; FAILURE: Allowed to exceed limit
    )
  )

  ;; Test 3: Exact budget allocation (should succeed)
  (func (export "test_exact_budget") (result i32)
    (local $result i32)

    ;; Allocate exactly 16 pages (1MB)
    (local.set $result (memory.grow (i32.const 16)))

    ;; If failed (returned -1), return FAILURE (0)
    (if (result i32) (i32.eq (local.get $result) (i32.const -1))
      (then (i32.const 0))  ;; FAILURE: Rejected exact budget
      (else (i32.const 1))  ;; SUCCESS: Accepted exact budget
    )
  )

  ;; Test 4: Refill trigger test
  (func (export "test_refill") (result i32)
    (local $result1 i32)
    (local $result2 i32)

    ;; First allocation: 13 pages (drops below low water mark)
    (local.set $result1 (memory.grow (i32.const 13)))
    (if (i32.eq (local.get $result1) (i32.const -1))
      (then (return (i32.const 10)))  ;; ERROR: First allocation failed
    )

    ;; Second allocation: 4 pages (should succeed after refill)
    (local.set $result2 (memory.grow (i32.const 4)))
    (if (i32.eq (local.get $result2) (i32.const -1))
      (then (return (i32.const 20)))  ;; ERROR: Refill didn't work
    )

    (i32.const 1)  ;; SUCCESS: Refill triggered and worked
  )

  ;; Test 5: Token conservation check
  (func (export "test_conservation") (result i32)
    (local $result i32)

    ;; Allocate 8 pages
    (local.set $result (memory.grow (i32.const 8)))
    (if (i32.eq (local.get $result) (i32.const -1))
      (then (return (i32.const 0)))  ;; FAILURE: Allocation failed
    )

    ;; TODO: Add syscall to deallocate WASM pages
    ;; For now, this tests allocation path only

    (i32.const 1)  ;; SUCCESS
  )
)
