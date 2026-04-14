(module
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (memory 1)
  (export "memory" (memory 0))
  (data (i32.const 1024) "Hello from Lux9 WASI Shim!\n")
  
  (func $main (export "_start")
    
    ;; Setup iovec at offset 0
    ;; iovec.buf = 1024 (string offset)
    (i32.store (i32.const 0) (i32.const 1024))
    ;; iovec.len = 27 (string length)
    (i32.store (i32.const 4) (i32.const 27))
    
    ;; call fd_write(1, 0, 1, 64)
    ;; fd=1 (stdout)
    ;; iovs=0
    ;; iovs_len=1
    ;; nwritten_ptr=64
    (call $fd_write
      (i32.const 1)
      (i32.const 0)
      (i32.const 1)
      (i32.const 64)
    )
    drop
    
    ;; call proc_exit(0)
    (call $proc_exit (i32.const 0))
  )
)
