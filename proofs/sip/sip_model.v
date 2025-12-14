(** SIP-specific portion of the split proof. Borrow logic lives in
    proofs/borrow/borrow_core.v. *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Import ListNotations.

Require Import proofs.borrow.borrow_core.

Open Scope Z_scope.

(* SIP-aware acquire/release transitions that consult the doorbell flag. *)
Inductive Acquire (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Acquire_Success :
      (s1 page).(state) = Free ->
      (s1 page).(doorbell) = true ->
      s2 = update_page s1 page (mkPageState (Some p) Exclusive P9_Pending false [] None) ->
      Acquire p page s1 s2.

Inductive Release (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Release_Success :
      (s1 page).(owner) = Some p ->
      (s1 page).(state) = Exclusive ->
      (s1 page).(shared_borrowers) = [] ->
      (s1 page).(mut_borrower) = None ->
      s2 = update_page s1 page (mkPageState None Free P9_Complete false [] None) ->
      Release p page s1 s2.

(* Composite step: either SIP acquire/release or a borrow-level step. *)
Inductive StepSip (s1 s2 : SystemState) : Prop :=
  | Step_Acquire : forall p pg, Acquire p pg s1 s2 -> StepSip s1 s2
  | Step_Release : forall p pg, Release p pg s1 s2 -> StepSip s1 s2
  | Step_BorrowLift : forall s1' s2', StepBorrow s1' s2' -> s1 = s1' -> s2 = s2' -> StepSip s1 s2.

(* Invariants can reuse the borrow_core definitions. *)
Definition SipValidState := ValidState.

