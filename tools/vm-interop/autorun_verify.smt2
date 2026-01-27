
; Autorun verification properties in SMT2 format
(set-logic QF_LIA)
(set-info :source |autorun_verified.rc formal verification|)

; State variables
(declare-fun lastline () Int)
(declare-fun totallines () Int)
(declare-fun newlastline () Int)
(declare-fun executed () Bool)

; Initial state
(assert (>= lastline 0))
(assert (>= totallines 0))

; Invariant: lastline never decreases
(assert (=> (and (>= lastline 0) (>= totallines lastline))
            (>= newlastline lastline)))

; Invariant: lastline <= totallines
(assert (<= lastline totallines))

; Property: No command loss
(assert (=> (> totallines lastline)
            (and executed (= newlastline totallines))))

; Property: Processing progress
(assert (=> (> totallines lastline)
            (> newlastline lastline)))

; Check satisfiability
(check-sat)
(get-model)
