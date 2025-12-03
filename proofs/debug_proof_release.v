Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Import ListNotations.

Definition Pid := nat.
Definition PageId := nat.

Inductive OwnerState := | Free | Exclusive | SharedOwned | MutLent.

Record PageState := mkPageState {
  owner : option Pid;
  state : OwnerState;
  shared_borrowers : list Pid;
  mut_borrower : option Pid;
}.

Definition SystemState := PageId -> PageState.
Definition init_state : SystemState := fun _ => mkPageState None Free [] None.

Definition update_page (s : SystemState) (p : PageId) (new_ps : PageState) : SystemState :=
  fun p' => if Nat.eqb p p' then new_ps else s p'.

Inductive Acquire (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Acquire_Success :
      (s1 page).(state) = Free ->
      s2 = update_page s1 page (mkPageState (Some p) Exclusive [] None) ->
      Acquire p page s1 s2.

Inductive Release (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Release_Success :
      (s1 page).(owner) = Some p ->
      (s1 page).(state) = Exclusive ->
      (s1 page).(shared_borrowers) = [] ->
      (s1 page).(mut_borrower) = None ->
      s2 = update_page s1 page (mkPageState None Free [] None) ->
      Release p page s1 s2.

(* (Omitting other inductives) *)
Inductive BorrowShared (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BS_Success_Exclusive : True -> BorrowShared p_owner p_borrower page s1 s2. (* Dummy *)

Inductive ReturnShared (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RS_Success : True -> ReturnShared p_borrower page s1 s2. (* Dummy *)

Inductive BorrowMut (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BM_Success : True -> BorrowMut p_owner p_borrower page s1 s2. (* Dummy *)

Inductive ReturnMut (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RM_Success : True -> ReturnMut p_borrower page s1 s2. (* Dummy *)

Inductive Transfer (from : Pid) (to : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Transfer_Success : True -> Transfer from to page s1 s2. (* Dummy *)

Inductive Step (s1 s2 : SystemState) : Prop :=
  | Step_Acquire : forall p pg, Acquire p pg s1 s2 -> Step s1 s2
  | Step_Release : forall p pg, Release p pg s1 s2 -> Step s1 s2
  | Step_BorrowShared : forall o b pg, BorrowShared o b pg s1 s2 -> Step s1 s2
  | Step_ReturnShared : forall b pg, ReturnShared b pg s1 s2 -> Step s1 s2
  | Step_BorrowMut : forall o b pg, BorrowMut o b pg s1 s2 -> Step s1 s2
  | Step_ReturnMut : forall b pg, ReturnMut b pg s1 s2 -> Step s1 s2
  | Step_Transfer : forall f t pg, Transfer f t pg s1 s2 -> Step s1 s2.

Definition Inv_Coherence (s : SystemState) : Prop :=
  forall pg,
    match (s pg).(state) with
    | Free => (s pg).(owner) = None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) = None
    | Exclusive => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) = None
    | SharedOwned => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) <> [] /\ (s pg).(mut_borrower) = None
    | MutLent => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) <> None
    end.

Definition CanWrite (p : Pid) (pg : PageId) (s : SystemState) : Prop :=
  ((s pg).(owner) = Some p /\ (s pg).(state) = Exclusive) \/
  ((s pg).(mut_borrower) = Some p /\ (s pg).(state) = MutLent).

Definition Inv_WriteSafety (s : SystemState) : Prop :=
  forall pg p1 p2, CanWrite p1 pg s -> CanWrite p2 pg s -> p1 = p2.

Definition CanRead (p : Pid) (pg : PageId) (s : SystemState) : Prop :=
  CanWrite p pg s \/ ((s pg).(state) = SharedOwned /\ (s pg).(owner) = Some p) \/ (In p (s pg).(shared_borrowers)).

Definition Inv_NoReadWriteRace (s : SystemState) : Prop :=
  forall pg p1 p2, CanWrite p1 pg s -> CanRead p2 pg s -> p1 = p2.

Definition ValidState (s : SystemState) : Prop :=
  Inv_Coherence s /\ Inv_WriteSafety s /\ Inv_NoReadWriteRace s.

Theorem transition_preserves_validity :
  forall s1 s2, ValidState s1 -> Step s1 s2 -> ValidState s2.
Proof.
  intros s1 s2 Hvalid Hstep.
  destruct Hvalid as [Hcoh [Hwrite Hrace]].
  induction Hstep as [p pg H | p pg H | o b pg H | b pg H | o b pg H | b pg H | f t pg H].
  
  - (* Acquire *)
    admit.
  - (* Release *)
    inversion H; subst.
    unfold ValidState, Inv_Coherence, Inv_WriteSafety, Inv_NoReadWriteRace in *.
    repeat split.
    + intros pg0.
      Show.