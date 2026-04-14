(module
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit"
    (func $proc_exit (param i32)))

  (memory 1)
  (export "memory" (memory 0))

  (data (i32.const 0) "WASM smoke test OK\n")

  (func (export "_start")
    (i32.store (i32.const 64) (i32.const 0))
    (i32.store (i32.const 68) (i32.const 19))
    (drop
      (call $fd_write
        (i32.const 1)
        (i32.const 64)
        (i32.const 1)
        (i32.const 72)))
    (call $proc_exit (i32.const 0)))
)
