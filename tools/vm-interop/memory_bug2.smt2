; Refined memory corruption analysis
; Looking at the actual VMX code flow

(set-logic QF_BV)
(set-option :produce-models true)

; The actual crash data from pool panic:
; block 449b60 hdr 0a110c09 009dc7c0 0021786d
; The 009dc7c0 = 10337280 bytes (bootsize)

; Memory pointers
(declare-const bootimg (_ BitVec 64))
(declare-const kernelfile_str (_ BitVec 64))
(declare-const argv_0 (_ BitVec 64))
(declare-const fd (_ BitVec 64))

; Sizes
(declare-const bootsize (_ BitVec 64))
(assert (= bootsize #x009dc7c0))

; States in the code flow
(declare-const state_extracted (_ BitVec 1))
(declare-const state_linux_detected (_ BitVec 1))
(declare-const state_file_created (_ BitVec 1))
(declare-const state_data_written (_ BitVec 1))
(declare-const state_fd_closed (_ BitVec 1))
(declare-const state_bootimg_freed (_ BitVec 1))
(declare-const state_argv_assigned (_ BitVec 1))

; The bug pattern from the code:
; Line 726: snprint(kernelfile, sizeof(kernelfile), "/tmp/vmx_kernel.%d", getpid());
; Line 727: int fd = create(kernelfile, OWRITE|ORCLOSE, 0600);
; Line 728-729: if(fd < 0 || write(fd, bootimg, bootsize) != bootsize)
; Line 731: close(fd);
; Line 733: free(bootimg);
; Line 736: argv[0] = kernelfile;  <-- kernelfile is a LOCAL VARIABLE!

; The problem: kernelfile is a stack-allocated array (line 726)
; When we assign argv[0] = kernelfile, we're pointing to stack memory
; That will be invalid when loadkernel() is called

(declare-const kernelfile_on_stack (_ BitVec 1))
(assert (= kernelfile_on_stack #b1))

; Bug condition: argv[0] points to stack memory that goes out of scope
(declare-const use_after_scope (_ BitVec 1))
(assert (= use_after_scope
  (ite (and (= kernelfile_on_stack #b1)
            (= state_argv_assigned #b1))
       #b1 #b0)))

; This causes loadkernel to read invalid memory
(assert (= use_after_scope #b1))

(check-sat)
(get-model)