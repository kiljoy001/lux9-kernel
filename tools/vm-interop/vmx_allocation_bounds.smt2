; SMT2 theorem: VMX device allocation bounds and cleanup protocol
; This proves the conditions under which VMX allocation succeeds/fails

(set-info :status unsat)
(set-logic QF_LIA)

; VMX system state
(declare-fun nvmxtab () Int)  ; Current number of allocated VMX tabs
(declare-fun MAX_VMX () Int)  ; Maximum VMX instances (system limit) 
(declare-fun memory_available () Int) ; Available 4KB-aligned memory pages
(declare-fun active_vmx_instances () Int) ; Currently active instances

; VMX instance lifecycle states  
(declare-fun vmx_init () Int)
(declare-fun vmx_ready () Int)  
(declare-fun vmx_running () Int)
(declare-fun vmx_dead () Int)
(declare-fun vmx_ending () Int)

; File descriptor management
(declare-fun ctlfd_open () Bool)
(declare-fun regsfd_open () Bool) 
(declare-fun mapfd_open () Bool)
(declare-fun waitfd_open () Bool)

; Define constants
(assert (= vmx_init 0))
(assert (= vmx_ready 1))
(assert (= vmx_running 2))
(assert (= vmx_dead 3))
(assert (= vmx_ending 4))

; System constraints
(assert (>= MAX_VMX 1))
(assert (>= nvmxtab 0))
(assert (<= nvmxtab MAX_VMX))
(assert (>= memory_available 0))
(assert (>= active_vmx_instances 0))
(assert (<= active_vmx_instances nvmxtab))

; VMX allocation preconditions (when allocation should succeed)
(declare-fun can_allocate_vmx () Bool)
(assert (= can_allocate_vmx 
  (and 
    (< active_vmx_instances MAX_VMX)  ; Have free slots
    (>= memory_available 1)           ; Have memory for 4KB alignment
    (not (and ctlfd_open regsfd_open mapfd_open waitfd_open)) ; Previous instance cleaned up
  )
))

; The "no free devices" error condition  
(declare-fun no_free_devices_error () Bool)
(assert (= no_free_devices_error
  (or
    (>= active_vmx_instances MAX_VMX)  ; All VMX slots occupied
    (< memory_available 1)             ; Insufficient memory
    (and ctlfd_open regsfd_open mapfd_open waitfd_open) ; FDs not cleaned up
  )
))

; Critical theorem: allocation and error conditions are mutually exclusive
(assert (and can_allocate_vmx no_free_devices_error))

; This should be UNSAT - proving allocation success and failure cannot occur simultaneously
(check-sat)