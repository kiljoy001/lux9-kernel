(** * SIP Model and Borrow Checker Verification
    * This file models the Lux9 kernel's ownership and borrowing system
    * to prove isolation and safety properties.
    *)

Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Import ListNotations.

(* --- Types --- *)

Definition Pid := nat.
Definition PageId := nat.

Inductive OwnerState :=
  | Free
  | Exclusive
  | SharedOwned
  | MutLent.

Record PageState := mkPageState {
  owner : option Pid;
  state : OwnerState;
  shared_borrowers : list Pid;
  mut_borrower : option Pid;
}.

(* Global System State *)
Definition SystemState := PageId -> PageState.

(* Initial State: All pages are Free, no owners, no borrowers *)
Definition init_state : SystemState :=
  fun _ => mkPageState None Free [] None.

(* --- Helper Functions --- *)

Definition update_page (s : SystemState) (p : PageId) (new_ps : PageState) : SystemState :=
  fun p' => if Nat.eqb p p' then new_ps else s p'.

Lemma update_page_hit :
  forall s pg new_ps, update_page s pg new_ps pg = new_ps.
Proof.
  intros. unfold update_page. rewrite Nat.eqb_refl. reflexivity.
Qed.

Lemma update_page_miss :
  forall s pg pg0 new_ps, pg <> pg0 -> update_page s pg new_ps pg0 = s pg0.
Proof.
  intros s pg pg0 new_ps Hneq. unfold update_page.
  apply Nat.eqb_neq in Hneq. rewrite Hneq. reflexivity.
Qed.

Lemma owner_update_hit :
  forall s pg new_ps, owner (update_page s pg new_ps pg) = owner new_ps.
Proof. intros; rewrite update_page_hit; reflexivity. Qed.

(* --- Transitions --- *)

(* Acquire: Process acquires a free page *)
Inductive Acquire (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Acquire_Success :
      (s1 page).(state) = Free ->
      s2 = update_page s1 page (mkPageState (Some p) Exclusive [] None) ->
      Acquire p page s1 s2.

(* Release: Owner releases an exclusive page *)
Inductive Release (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Release_Success :
      (s1 page).(owner) = Some p ->
      (s1 page).(state) = Exclusive ->
      (s1 page).(shared_borrowers) = [] ->
      (s1 page).(mut_borrower) = None ->
      s2 = update_page s1 page (mkPageState None Free [] None) ->
      Release p page s1 s2.

(* Borrow Shared: Owner lends page immutably *)
Inductive BorrowShared (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BS_Success_Exclusive :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some p_owner) SharedOwned [p_borrower] None) ->
      BorrowShared p_owner p_borrower page s1 s2
  | BS_Success_Shared :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = SharedOwned ->
      s2 = update_page s1 page (mkPageState (Some p_owner) SharedOwned (p_borrower :: (s1 page).(shared_borrowers)) None) ->
      BorrowShared p_owner p_borrower page s1 s2.

(* Return Shared: Borrower returns immutable page *)
Inductive ReturnShared (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RS_Success_StillShared :
      (s1 page).(state) = SharedOwned ->
      In p_borrower (s1 page).(shared_borrowers) ->
      let new_list := remove Nat.eq_dec p_borrower (s1 page).(shared_borrowers) in
      new_list <> [] ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) SharedOwned new_list None) ->
      ReturnShared p_borrower page s1 s2
  | RS_Success_BackToExclusive :
      (s1 page).(state) = SharedOwned ->
      In p_borrower (s1 page).(shared_borrowers) ->
      remove Nat.eq_dec p_borrower (s1 page).(shared_borrowers) = [] ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) Exclusive [] None) ->
      ReturnShared p_borrower page s1 s2.

(* Borrow Mutable: Owner lends page mutably *)
Inductive BorrowMut (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BM_Success :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some p_owner) MutLent [] (Some p_borrower)) ->
      BorrowMut p_owner p_borrower page s1 s2.

(* Return Mutable: Borrower returns mutable page *)
Inductive ReturnMut (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RM_Success :
      (s1 page).(state) = MutLent ->
      (s1 page).(mut_borrower) = Some p_borrower ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) Exclusive [] None) ->
      ReturnMut p_borrower page s1 s2.

(* Transfer: Transfer ownership (e.g. Exchange Prepare/Accept combined) *)
Inductive Transfer (from : Pid) (to : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Transfer_Success :
      (s1 page).(owner) = Some from ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some to) Exclusive [] None) ->
      Transfer from to page s1 s2.

(* A single small-step relation that bundles all the actions above. *)
Inductive Step (s1 s2 : SystemState) : Prop :=
  | Step_Acquire : forall p pg, Acquire p pg s1 s2 -> Step s1 s2
  | Step_Release : forall p pg, Release p pg s1 s2 -> Step s1 s2
  | Step_BorrowShared : forall o b pg, BorrowShared o b pg s1 s2 -> Step s1 s2
  | Step_ReturnShared : forall b pg, ReturnShared b pg s1 s2 -> Step s1 s2
  | Step_BorrowMut : forall o b pg, BorrowMut o b pg s1 s2 -> Step s1 s2
  | Step_ReturnMut : forall b pg, ReturnMut b pg s1 s2 -> Step s1 s2
  | Step_Transfer : forall f t pg, Transfer f t pg s1 s2 -> Step s1 s2.

(* --- Invariants --- *)

(* 1. Coherence: State matches fields *)
Definition Inv_Coherence (s : SystemState) : Prop :=
  forall pg,
    match (s pg).(state) with
    | Free => (s pg).(owner) = None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) = None
    | Exclusive => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) = None
    | SharedOwned => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) <> [] /\ (s pg).(mut_borrower) = None
    | MutLent => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) <> None
    end.

(* 2. Safety: No Aliasing of Mutable Access *)
(* A process has Write access if it is Owner (Exclusive) OR MutBorrower *)
Definition CanWrite (p : Pid) (pg : PageId) (s : SystemState) : Prop :=
  ((s pg).(owner) = Some p /\ (s pg).(state) = Exclusive) \/
  ((s pg).(mut_borrower) = Some p /\ (s pg).(state) = MutLent).

Definition Inv_WriteSafety (s : SystemState) : Prop :=
  forall pg p1 p2,
    CanWrite p1 pg s -> CanWrite p2 pg s -> p1 = p2.

(* 3. Safety: No Write/Read Aliasing *)
(* A process has Read access if CanWrite OR SharedBorrower OR (Owner and SharedOwned) *)
Definition CanRead (p : Pid) (pg : PageId) (s : SystemState) : Prop :=
  CanWrite p pg s \/
  ((s pg).(state) = SharedOwned /\ (s pg).(owner) = Some p) \/
  (In p (s pg).(shared_borrowers)).

Definition Inv_NoReadWriteRace (s : SystemState) : Prop :=
  forall pg p1 p2,
    CanWrite p1 pg s -> CanRead p2 pg s -> p1 = p2.

(* Combined Invariant *)
Definition ValidState (s : SystemState) : Prop :=
  Inv_Coherence s /\ Inv_WriteSafety s /\ Inv_NoReadWriteRace s.

(* --- Proofs --- *)

Lemma init_valid : ValidState init_state.
Proof.
  split; [|split].
  - unfold Inv_Coherence, init_state. intros. simpl. repeat split; auto.
  - unfold Inv_WriteSafety, init_state, CanWrite. intros. simpl in *.
    destruct H; destruct H; discriminate.
  - unfold Inv_NoReadWriteRace, init_state, CanWrite, CanRead. intros. simpl in *.
    destruct H; destruct H; discriminate.
Qed.

Theorem transition_preserves_validity :
  forall s1 s2, ValidState s1 -> Step s1 s2 -> ValidState s2.
Proof.
  Admitted.

(* --- Key Safety Properties --- *)

(* 1. Exclusive Ownership means no one else can write *)
Theorem Exclusive_Write_Safety :
  forall s pg p1 p2,
    ValidState s ->
    (s pg).(state) = Exclusive ->
    (s pg).(owner) = Some p1 ->
    CanWrite p2 pg s ->
    p1 = p2.
Proof.
  intros s pg p1 p2 Valid Excl Own HW.
  unfold ValidState, Inv_WriteSafety in Valid. destruct Valid as [_ [HSafe _]].
  unfold CanWrite in HW.
  assert (CanWrite p1 pg s).
  { unfold CanWrite. left. split; auto. }
  apply HSafe with (pg := pg); auto.
Qed.

(* 2. Shared Ownership means no one can write (immutability) *)
Theorem Shared_No_Write :
  forall s pg p,
    ValidState s ->
    (s pg).(state) = SharedOwned ->
    ~ CanWrite p pg s.
Proof.
  intros s pg p Valid Shared HW.
  unfold CanWrite in HW.
  destruct HW as [[_ Excl] | [_ Mut]].
  - rewrite Shared in Excl. discriminate.
  - rewrite Shared in Mut. discriminate.
Qed.

(* 3. Mutable Borrow means owner cannot read *)
Theorem MutBorrow_Owner_NoRead :
  forall s pg p_owner p_borrower,
    ValidState s ->
    (s pg).(owner) = Some p_owner ->
    (s pg).(mut_borrower) = Some p_borrower ->
    (s pg).(state) = MutLent ->
    ~ CanRead p_owner pg s.
Proof. Admitted.

