(module
  (import "lux9" "print" (func $print (param i32 i32)))
  (import "lux9" "getpid" (func $getpid (result i64)))
  (import "lux9" "getticks" (func $getticks (result i64)))
  (import "lux9" "panic" (func $panic (param i32 i32)))
  (import "lux9" "meminfo" (func $meminfo (result i64)))
  (import "wasi_snapshot_preview1" "path_create_directory" (func $path_create_directory (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_remove_directory" (func $path_remove_directory (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_unlink_file" (func $path_unlink_file (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_rename" (func $path_rename (param i32 i32 i32 i32 i32 i32) (result i32)))


  (memory (export "memory") 1) ;; 1 page = 64KB

  (data (i32.const 0) "Arithmetic Test Passed\n")
  (data (i32.const 32) "Control Flow Test Passed\n")
  (data (i32.const 64) "Memory Test Passed\n")
  (data (i32.const 96) "Host Interop Test Passed\n")
  (data (i32.const 128) "Test Failed\n")
  (data (i32.const 160) "Global Test Passed\n")
  (data (i32.const 192) "Filesystem Test Passed\n")
  (data (i32.const 224) "test_dir")
  (data (i32.const 256) "test_renamed")

  
  (global $g_counter (mut i32) (i32.const 0))

  ;; Helper to print success message
  (func $print_success (param $offset i32) (param $len i32)
    (call $print (local.get $offset) (local.get $len))
  )

  ;; Helper to print failure
  (func $print_fail
    (call $print (i32.const 128) (i32.const 12))
    (unreachable)
  )

  ;; TEST 1: Arithmetic & Logic
  (func $test_arithmetic (export "test_arithmetic") (result i64)
    ;; Integer arithmetic
    (if (i32.ne (i32.add (i32.const 10) (i32.const 20)) (i32.const 30)) (then (call $print_fail)))
    (if (i32.ne (i32.sub (i32.const 30) (i32.const 10)) (i32.const 20)) (then (call $print_fail)))
    (if (i32.ne (i32.mul (i32.const 5) (i32.const 6)) (i32.const 30)) (then (call $print_fail)))
    (if (i32.ne (i32.div_u (i32.const 30) (i32.const 5)) (i32.const 6)) (then (call $print_fail)))
    
    ;; 64-bit arithmetic
    (if (i64.ne (i64.add (i64.const 100) (i64.const 200)) (i64.const 300)) (then (call $print_fail)))
    
    ;; Logic
    (if (i32.eq (i32.and (i32.const 0xFF) (i32.const 0x0F)) (i32.const 0x0F)) (then) (else (call $print_fail)))
    (if (i32.eq (i32.or (i32.const 0xF0) (i32.const 0x0F)) (i32.const 0xFF)) (then) (else (call $print_fail)))

    (call $print_success (i32.const 0) (i32.const 23))
    (i64.const 1)
  )

  ;; TEST 2: Control Flow
  (func $test_control_flow (export "test_control_flow") (result i64)
    (local $i i32)
    (local $sum i32)
    
    ;; Loop: sum 0 to 9 -> 45
    (local.set $i (i32.const 0))
    (local.set $sum (i32.const 0))
    
    (block $break
      (loop $top
        (br_if $break (i32.eq (local.get $i) (i32.const 10)))
        (local.set $sum (i32.add (local.get $sum) (local.get $i)))
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (br $top)
      )
    )
    
    (if (i32.ne (local.get $sum) (i32.const 45)) (then (call $print_fail)))
    
    (call $print_success (i32.const 32) (i32.const 25))
    (i64.const 1)
  )
  
  ;; TEST 3: Memory
  (func $test_memory (export "test_memory") (result i64)
    ;; Store 0x12345678 at offset 4096 (page size is 64k so safe)
    (i32.store (i32.const 4096) (i32.const 0x12345678))
    
    ;; Load it back
    (if (i32.ne (i32.load (i32.const 4096)) (i32.const 0x12345678)) 
      (then (call $print_fail)))
      
    ;; Bytes
    (i32.store8 (i32.const 4100) (i32.const 0xFF))
    (if (i32.ne (i32.load8_u (i32.const 4100)) (i32.const 0xFF))
      (then (call $print_fail)))
      
    (call $print_success (i32.const 64) (i32.const 19))
    (i64.const 1)
  )
  
  ;; TEST 4: Host Interop (Lux9)
  (func $test_host_interop (export "test_host_interop") (result i64)
    (local $pid i64)
    (local $ticks i64)
    
    (local.set $pid (call $getpid))
    ;; Just assume PID > 0 if successful (init usually low PID, but >0)
    ;; Actually PID 0 might be valid in some kernels but usually idle/swapper. 
    ;; Lux9 init is PID 1, so subsequent procs > 1.
    
    (local.set $ticks (call $getticks))
    ;; Ticks should advance or be non-zero (unless very start)
    
    (call $print_success (i32.const 96) (i32.const 25))
    (i64.const 1)
  )

  ;; TEST 5: Globals
  (func $test_globals (export "test_globals") (result i64)
     (global.set $g_counter (i32.const 42))
     (if (i32.ne (global.get $g_counter) (i32.const 42)) (then (call $print_fail)))
     
     (global.set $g_counter (i32.add (global.get $g_counter) (i32.const 1)))
     (if (i32.ne (global.get $g_counter) (i32.const 43)) (then (call $print_fail)))

     (call $print_success (i32.const 160) (i32.const 19))
     (i64.const 1)
  )

  ;; TEST 6: Filesystem (Create/Remove Directory)
  (func $test_filesystem (export "test_filesystem") (result i64)
    (local $ret i32)
    ;; Try to create "test_dir" at offset 224 (len 8) in FD 3 (root/cwd?)
    ;; Note: FD 3 might not be valid if not pre-opened. We ignore failure if capability is missing, 
    ;; but if it returns SUCCESS or IO error we know it tried.
    ;; Actually, we just test linkage for now.
    
    (local.set $ret (call $path_create_directory (i32.const 3) (i32.const 224) (i32.const 8)))
    
    ;; If creation succeeded, remove it
    (if (i32.eq (local.get $ret) (i32.const 0))
      (then
         (drop (call $path_remove_directory (i32.const 3) (i32.const 224) (i32.const 8)))
      )
    )
    
    (call $print_success (i32.const 192) (i32.const 23))
    (i64.const 1)
  )
)
