; Memory corruption analysis for VMX Alpine boot
; The crash shows: pool panic with fault read addr=0x0

(set-logic QF_BV)
(set-option :produce-models true)

; Memory regions and pointers
(declare-const bootimg_ptr (_ BitVec 64))
(declare-const bootsize (_ BitVec 64))
(declare-const kernel32_ptr (_ BitVec 64))
(declare-const kernel32_size (_ BitVec 64))
(declare-const setup_size (_ BitVec 64))
(declare-const kernelfile_ptr (_ BitVec 64))
(declare-const tmpfile_fd (_ BitVec 32))

; Known values from the crash
(assert (= bootsize #x009dc7c0))  ; 10337280 bytes
(assert (= setup_size #x00003c00)) ; 15360 bytes
(assert (= kernel32_size #x009d8bc0)) ; 10321920 bytes

; Memory layout constraints
; kernel32 = bootimg + setup_size
(assert (= kernel32_ptr (bvadd bootimg_ptr setup_size)))

; The sizes should add up correctly
(assert (= bootsize (bvadd setup_size kernel32_size)))

; Pool header shows corruption pattern: 0a110c09 009dc7c0 0021786d
; This suggests memory was freed while still in use

; Check for double-free scenarios
(declare-const bootimg_freed (_ BitVec 1))
(declare-const kernelfile_written (_ BitVec 1))
(declare-const argv_modified (_ BitVec 1))

; The bug scenario: bootimg is freed before kernel data is written
(assert (= bootimg_freed #b1))
(assert (= kernelfile_written #b1))

; The modified VMX code path:
; 1. Extract Alpine kernel (allocates bootimg)
; 2. Check if Linux kernel
; 3. Write complete kernel to temp file
; 4. FREE bootimg <-- This happens
; 5. Set argv[0] = kernelfile
; 6. Call loadkernel(argv[0])

; The problem: if we write kernel32_ptr data AFTER freeing bootimg
(declare-const write_after_free (_ BitVec 1))
(assert (= write_after_free
  (ite (and (= bootimg_freed #b1)
            (bvuge kernel32_ptr bootimg_ptr)
            (bvult kernel32_ptr (bvadd bootimg_ptr bootsize)))
       #b1 #b0)))

; Check if null pointer access could occur
(declare-const null_access (_ BitVec 1))
(assert (= null_access
  (ite (or (= bootimg_ptr #x0000000000000000)
           (= kernel32_ptr #x0000000000000000)
           (= kernelfile_ptr #x0000000000000000))
       #b1 #b0)))

; Find the bug condition
(assert (or (= write_after_free #b1) (= null_access #b1)))

(check-sat)
(get-model)