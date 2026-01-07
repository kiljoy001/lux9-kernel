;; Minimal WASM test with _start for exec() testing
(module
  ;; Import lux9.print for output
  (import "lux9" "print" (func $print (param i32 i32)))
  
  ;; Memory for string data
  (memory 1)
  
  ;; Export memory
  (export "memory" (memory 0))
  
  ;; String: "Hello from WASM exec!\n"
  (data (i32.const 0) "Hello from WASM exec!\n")
  
  ;; _start function - called by kernel
  (func $_start
    ;; Print message
    i32.const 0     ;; ptr to string
    i32.const 22    ;; length of string
    call $print
  )
  
  ;; Export _start
  (export "_start" (func $_start))
)
