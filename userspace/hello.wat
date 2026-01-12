(module
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (memory 1)
  (export "memory" (memory 0))
  (data (i32.const 0) "Hello from WASM Integration Test!\n")
  (func $start (export "_start")
    (local $iovs i32)
    (local $written i32)
    
    ;; Setup iovec at offset 100
    ;; iov.buf = 0 (offset of string)
    (i32.store (i32.const 100) (i32.const 0))
    ;; iov.buf_len = 34
    (i32.store (i32.const 104) (i32.const 34))
    
    ;; Call fd_write(1, 100, 1, 108)
    (call $fd_write
      (i32.const 1)   ;; fd=1 (stdout)
      (i32.const 100) ;; iovs ptr
      (i32.const 1)   ;; iovs len
      (i32.const 108) ;; nwritten ptr
    )
    drop
    
    (call $proc_exit (i32.const 0))
  )
)
