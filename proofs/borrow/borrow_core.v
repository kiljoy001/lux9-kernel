(** Core borrow checker model extracted from the unified SIP proof.
    This file keeps the ownership/borrow logic; SIP/doorbell concerns live in a
    separate module. *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* TYPES *)
(* ========================================================================= *)

Definition Pid := Z.
Definition PageId := Z.

Inductive OwnerState :=
  | Free
  | Exclusive
  | SharedOwned
  | MutLent.

Inductive SipStatus :=
  | P9_Idle
  | P9_Pending
  | P9_Complete
  | P9_Error.

Record PageState := mkPageState {
  owner : option Pid;
  state : OwnerState;
  sip_status : SipStatus;
  doorbell : bool;
  shared_borrowers : list Pid;
  mut_borrower : option Pid;
}.

Definition SystemState := PageId -> PageState.

Definition init_state : SystemState :=
  fun _ => mkPageState None Free P9_Idle false [] None.

(* ========================================================================= *)
(* HELPERS *)
(* ========================================================================= *)

Definition update_page (s : SystemState) (p : PageId) (new_ps : PageState) : SystemState :=
  fun p' => if Z.eqb p p' then new_ps else s p'.

Lemma update_page_hit :
  forall s pg new_ps, update_page s pg new_ps pg = new_ps.
Proof. intros; unfold update_page; rewrite Z.eqb_refl; reflexivity. Qed.

Lemma update_page_miss :
  forall s pg pg0 new_ps, pg <> pg0 -> update_page s pg new_ps pg0 = s pg0.
Proof.
  intros; unfold update_page; apply Z.eqb_neq in H; rewrite H; reflexivity.
Qed.

(* ========================================================================= *)
(* BORROW-ONLY TRANSITIONS *)
(* ========================================================================= *)

Inductive BorrowShared (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BS_Success_Exclusive :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = Exclusive ->
      ~ In p_borrower (s1 page).(shared_borrowers) ->
      s2 = update_page s1 page (mkPageState (Some p_owner) SharedOwned (s1 page).(sip_status) (s1 page).(doorbell) [p_borrower] None) ->
      BorrowShared p_owner p_borrower page s1 s2
  | BS_Success_Shared :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = SharedOwned ->
      ~ In p_borrower (s1 page).(shared_borrowers) ->
      s2 = update_page s1 page (mkPageState (Some p_owner) SharedOwned (s1 page).(sip_status) (s1 page).(doorbell) (p_borrower :: (s1 page).(shared_borrowers)) None) ->
      BorrowShared p_owner p_borrower page s1 s2.

Inductive ReturnShared (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RS_Success_StillShared :
      forall new_list,
      (s1 page).(state) = SharedOwned ->
      In p_borrower (s1 page).(shared_borrowers) ->
      new_list = remove Z.eq_dec p_borrower (s1 page).(shared_borrowers) ->
      new_list <> [] ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) SharedOwned (s1 page).(sip_status) (s1 page).(doorbell) new_list None) ->
      ReturnShared p_borrower page s1 s2
  | RS_Success_BackToExclusive :
      (s1 page).(state) = SharedOwned ->
      In p_borrower (s1 page).(shared_borrowers) ->
      remove Z.eq_dec p_borrower (s1 page).(shared_borrowers) = [] ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) Exclusive (s1 page).(sip_status) (s1 page).(doorbell) [] None) ->
      ReturnShared p_borrower page s1 s2.

Inductive BorrowMut (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BM_Success :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some p_owner) MutLent (s1 page).(sip_status) (s1 page).(doorbell) [] (Some p_borrower)) ->
      BorrowMut p_owner p_borrower page s1 s2.

Inductive ReturnMut (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RM_Success :
      (s1 page).(state) = MutLent ->
      (s1 page).(mut_borrower) = Some p_borrower ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) Exclusive (s1 page).(sip_status) (s1 page).(doorbell) [] None) ->
      ReturnMut p_borrower page s1 s2.

Inductive Transfer (from : Pid) (to : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Transfer_Success :
      (s1 page).(owner) = Some from ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some to) Exclusive (s1 page).(sip_status) (s1 page).(doorbell) [] None) ->
      Transfer from to page s1 s2.

Inductive StepBorrow (s1 s2 : SystemState) : Prop :=
  | Step_BorrowShared : forall o b pg, BorrowShared o b pg s1 s2 -> StepBorrow s1 s2
  | Step_ReturnShared : forall b pg, ReturnShared b pg s1 s2 -> StepBorrow s1 s2
  | Step_BorrowMut : forall o b pg, BorrowMut o b pg s1 s2 -> StepBorrow s1 s2
  | Step_ReturnMut : forall b pg, ReturnMut b pg s1 s2 -> StepBorrow s1 s2
  | Step_Transfer : forall f t pg, Transfer f t pg s1 s2 -> StepBorrow s1 s2.

(* ========================================================================= *)
(* INVARIANTS *)
(* ========================================================================= *)

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
  forall pg p1 p2,
    CanWrite p1 pg s -> CanWrite p2 pg s -> p1 = p2.

Definition CanRead (p : Pid) (pg : PageId) (s : SystemState) : Prop :=
  CanWrite p pg s \/
  ((s pg).(state) = SharedOwned /\ (s pg).(owner) = Some p) \/
  (In p (s pg).(shared_borrowers)).

Definition Inv_NoReadWriteRace (s : SystemState) : Prop :=
  forall pg p1 p2,
    CanWrite p1 pg s -> CanRead p2 pg s -> p1 = p2.

Definition ValidState (s : SystemState) : Prop :=
  Inv_Coherence s /\ Inv_WriteSafety s /\ Inv_NoReadWriteRace s.

(* ========================================================================= *)
(* LOCAL COHERENCE WITNESS *)
(* ========================================================================= *)

Definition coherent_page (ps : PageState) : Prop :=
  match ps.(state) with
  | Free => ps.(owner) = None /\ ps.(shared_borrowers) = [] /\ ps.(mut_borrower) = None
  | Exclusive => exists p, ps.(owner) = Some p /\ ps.(shared_borrowers) = [] /\ ps.(mut_borrower) = None
  | SharedOwned => exists p l, ps.(owner) = Some p /\ ps.(shared_borrowers) = l /\ l <> [] /\ ps.(mut_borrower) = None
  | MutLent => exists p m, ps.(owner) = Some p /\ ps.(shared_borrowers) = [] /\ ps.(mut_borrower) = Some m
  end.

Lemma coherent_page_implies_write_unique :
  forall ps,
    coherent_page ps ->
    forall p1 p2,
      (ps.(owner) = Some p1 /\ ps.(state) = Exclusive \/
       ps.(mut_borrower) = Some p1 /\ ps.(state) = MutLent) ->
      (ps.(owner) = Some p2 /\ ps.(state) = Exclusive \/
       ps.(mut_borrower) = Some p2 /\ ps.(state) = MutLent) ->
      p1 = p2.
Proof.
  intros ps _ p1 p2 H1 H2.
  destruct (state ps); simpl in *;
    try (destruct H1 as [[? ?]|[? ?]]; discriminate);
    try (destruct H1 as [[? ?]|[Hm1 _]]; try discriminate;
         destruct H2 as [[? ?]|[Hm2 _]]; try discriminate; congruence);
    try (destruct H1 as [[Ho1 _]|[Hm1 Hs1]]; try discriminate;
         destruct H2 as [[Ho2 _]|[Hm2 Hs2]]; try discriminate; congruence).
Qed.

Lemma coherent_page_implies_no_rwr :
  forall ps,
    coherent_page ps ->
    forall p1 p2,
      (ps.(owner) = Some p1 /\ ps.(state) = Exclusive \/
       ps.(mut_borrower) = Some p1 /\ ps.(state) = MutLent) ->
      ((ps.(owner) = Some p2 /\ ps.(state) = Exclusive \/
        ps.(mut_borrower) = Some p2 /\ ps.(state) = MutLent) \/
       (ps.(state) = SharedOwned /\ ps.(owner) = Some p2) \/
       In p2 ps.(shared_borrowers)) ->
      p1 = p2.
Proof.
  intros ps Hcoh p1 p2 HW HR.
  unfold coherent_page in Hcoh.
  destruct (state ps) eqn:Hst; simpl in *.
  all: try (destruct HW as [[? ?]|[? ?]]; discriminate).
  all: try (
    destruct Hcoh as [p0 [Ho0 [Hsh0 Hm0]]];
    destruct HW as [[Ho1 _]|[Hm1 Hs1]]; try discriminate;
    destruct HR as [HR1|[HR2|HR3]];
    [destruct HR1 as [[Ho2 _]|[Hm2 Hs2]]; try discriminate; congruence
    |destruct HR2 as [Hst' Hown]; discriminate
    |rewrite Hsh0 in HR3; contradiction]).
  all: try (
    destruct Hcoh as [p0 [m0 [Ho0 [Hsh0 Hm0]]]];
    destruct HW as [[Ho1 Hs1]|[Hm1 _]]; try discriminate;
    destruct HR as [HR1|[HR2|HR3]];
    [destruct HR1 as [[Ho2 Hs2]|[Hm2 Hs2]]; try discriminate; congruence
    |destruct HR2 as [Hst' Hown]; discriminate
    |rewrite Hsh0 in HR3; contradiction]).
Qed.

Lemma coherent_page_inv_coherence :
  forall ps,
    coherent_page ps ->
    match ps.(state) with
    | Free => ps.(owner) = None /\ ps.(shared_borrowers) = [] /\ ps.(mut_borrower) = None
    | Exclusive => ps.(owner) <> None /\ ps.(shared_borrowers) = [] /\ ps.(mut_borrower) = None
    | SharedOwned => ps.(owner) <> None /\ ps.(shared_borrowers) <> [] /\ ps.(mut_borrower) = None
    | MutLent => ps.(owner) <> None /\ ps.(shared_borrowers) = [] /\ ps.(mut_borrower) <> None
    end.
Proof.
  intros ps Hc.
  destruct ps as [o st ss db sh mut]; simpl in *.
  destruct st; simpl in *;
    [destruct Hc as [? [? ?]]; tauto
    |destruct Hc as [p [Ho [Hsh Hm]]]; cbn in *;
       split; [rewrite Ho; discriminate|]; split; [exact Hsh|exact Hm]
    |destruct Hc as [p [l [Ho [Hsh [Hn Hm]]]]]; cbn in *;
       split; [rewrite Ho; discriminate|]; split; [rewrite Hsh; exact Hn|exact Hm]
    |destruct Hc as [p [m [Ho [Hsh Hm]]]]; cbn in *;
       split; [rewrite Ho; discriminate|]; split; [exact Hsh|rewrite Hm; discriminate] ].
Qed.

(* ========================================================================= *)
(* PRESERVATION FOR BORROW-LEVEL STEPS *)
(* ========================================================================= *)

Lemma update_preserves_valid :
  forall s pg new_ps,
    coherent_page new_ps ->
    ValidState s ->
    ValidState (update_page s pg new_ps).
Proof.
  intros s pg new_ps Hcoh [Hcoher [Hws Hnrwr]].
  unfold ValidState; repeat split.
  
  (* Inv_Coherence *)
  - unfold Inv_Coherence. intro pg0.
    destruct (Z.eq_dec pg pg0) as [Heq|Hneq].
    + subst. rewrite update_page_hit.
      apply coherent_page_inv_coherence. exact Hcoh.
    + rewrite (update_page_miss s pg pg0 new_ps Hneq).
      apply Hcoher.
  
  (* Inv_WriteSafety *)
  - unfold Inv_WriteSafety. intros pg0 p1 p2 Hw1 Hw2.
    destruct (Z.eq_dec pg pg0) as [Heq|Hneq].
    + subst. unfold CanWrite in Hw1, Hw2.
      rewrite update_page_hit in Hw1, Hw2.
      apply (coherent_page_implies_write_unique new_ps Hcoh p1 p2 Hw1 Hw2).
    + assert (CanWrite p1 pg0 s) as Hw1'.
      { unfold CanWrite in *. rewrite (update_page_miss s pg pg0 new_ps Hneq) in Hw1. exact Hw1. }
      assert (CanWrite p2 pg0 s) as Hw2'.
      { unfold CanWrite in *. rewrite (update_page_miss s pg pg0 new_ps Hneq) in Hw2. exact Hw2. }
      apply (Hws pg0 p1 p2 Hw1' Hw2').
  
  (* Inv_NoReadWriteRace *)
  - unfold Inv_NoReadWriteRace. intros pg0 p1 p2 Hw Hr.
    destruct (Z.eq_dec pg pg0) as [Heq|Hneq].
    + subst. unfold CanWrite in Hw. unfold CanRead in Hr. unfold CanWrite in Hr.
      rewrite update_page_hit in Hw, Hr.
      apply (coherent_page_implies_no_rwr new_ps Hcoh p1 p2 Hw Hr).
    + assert (CanWrite p1 pg0 s) as Hw'.
      { unfold CanWrite in *. rewrite (update_page_miss s pg pg0 new_ps Hneq) in Hw. exact Hw. }
      assert (CanRead p2 pg0 s) as Hr'.
      { unfold CanRead, CanWrite in *. rewrite (update_page_miss s pg pg0 new_ps Hneq) in Hr. exact Hr. }
      apply (Hnrwr pg0 p1 p2 Hw' Hr').
Qed.

(* Helper: any transition's new PageState is coherent *)

Lemma borrow_shared_coherent :
  forall o b pg s1 s2,
    ValidState s1 ->
    BorrowShared o b pg s1 s2 ->
    coherent_page (s2 pg).
Proof.
  intros o b pg s1 s2 [Hcoh _] Hbs.
  inversion Hbs; subst.
  - (* BS_Success_Exclusive *)
    rewrite update_page_hit. simpl.
    exists o, [b]. repeat split; try reflexivity. discriminate.
  - (* BS_Success_Shared *)
    rewrite update_page_hit. simpl.
    exists o, (b :: shared_borrowers (s1 pg)).
    repeat split; try reflexivity. discriminate.
Qed.

Lemma return_shared_coherent :
  forall b pg s1 s2,
    ValidState s1 ->
    ReturnShared b pg s1 s2 ->
    coherent_page (s2 pg).
Proof.
  intros b pg s1 s2 [Hcoh _] Hrs.
  inversion Hrs.
  - (* RS_Success_StillShared *)
    subst. rewrite update_page_hit. simpl.
    specialize (Hcoh pg). rewrite H in Hcoh.
    simpl in Hcoh. destruct Hcoh as [Hown [Hsh Hm]].
    exists (match owner (s1 pg) with Some p => p | None => 0 end).
    exists (remove Z.eq_dec b (shared_borrowers (s1 pg))).
    repeat split; try reflexivity.
    + destruct (owner (s1 pg)) eqn:Ho; [reflexivity|contradiction].
    + exact H2.
  - (* RS_Success_BackToExclusive *)
    subst. rewrite update_page_hit. simpl.
    specialize (Hcoh pg). rewrite H in Hcoh.
    simpl in Hcoh. destruct Hcoh as [Hown _].
    exists (match owner (s1 pg) with Some p => p | None => 0 end).
    repeat split.
    destruct (owner (s1 pg)) eqn:Ho; [reflexivity|contradiction].
Qed.

Lemma borrow_mut_coherent :
  forall o b pg s1 s2,
    ValidState s1 ->
    BorrowMut o b pg s1 s2 ->
    coherent_page (s2 pg).
Proof.
  intros o b pg s1 s2 [Hcoh _] Hbm.
  inversion Hbm; subst.
  rewrite update_page_hit. simpl.
  exists o; exists b; repeat split; reflexivity.
Qed.

Lemma return_mut_coherent :
  forall b pg s1 s2,
    ValidState s1 ->
    ReturnMut b pg s1 s2 ->
    coherent_page (s2 pg).
Proof.
  intros b pg s1 s2 [Hcoh _] Hrm.
  inversion Hrm; subst.
  rewrite update_page_hit. simpl.
  specialize (Hcoh pg). destruct (state (s1 pg)) eqn:Hst; try discriminate.
  simpl in Hcoh. destruct Hcoh as [Hown _].
  exists (match owner (s1 pg) with Some p => p | None => 0 end).
  repeat split.
  destruct (owner (s1 pg)) eqn:Ho; [reflexivity|contradiction].
Qed.

Lemma transfer_coherent :
  forall f t pg s1 s2,
    ValidState s1 ->
    Transfer f t pg s1 s2 ->
    coherent_page (s2 pg).
Proof.
  intros f t pg s1 s2 [Hcoh _] Ht.
  inversion Ht; subst.
  rewrite update_page_hit. simpl.
  exists t. repeat split; reflexivity.
Qed.

(* Helper: extract the page and new state from each step *)
Lemma step_borrow_is_update :
  forall s1 s2,
    StepBorrow s1 s2 ->
    exists pg new_ps, s2 = update_page s1 pg new_ps.
Proof.
  intros s1 s2 Hstep.
  destruct Hstep.
  - inversion H; subst; exists pg; eexists; reflexivity.
  - inversion H; subst; exists pg; eexists; reflexivity.
  - inversion H; subst; exists pg; eexists; reflexivity.
  - inversion H; subst; exists pg; eexists; reflexivity.
  - inversion H; subst; exists pg; eexists; reflexivity.
Qed.

Lemma borrow_step_preserves_valid :
  forall s1 s2,
    ValidState s1 ->
    StepBorrow s1 s2 ->
    ValidState s2.
Proof.
  intros s1 s2 Hvalid Hstep.
  destruct Hstep.
  - (* Step_BorrowShared *)
    inversion H; subst.
    + apply update_preserves_valid; [|exact Hvalid].
      simpl. exists o, [b]. repeat split; try reflexivity. discriminate.
    + apply update_preserves_valid; [|exact Hvalid].
      simpl. exists o, (b :: shared_borrowers (s1 pg)).
      repeat split; try reflexivity. discriminate.
  - (* Step_ReturnShared *)
    inversion H; subst.
    + apply update_preserves_valid; [|exact Hvalid].
      simpl. destruct Hvalid as [Hcoh _].
      specialize (Hcoh pg). rewrite H0 in Hcoh. simpl in Hcoh.
      destruct Hcoh as [Hown [Hsh Hm]].
      exists (match owner (s1 pg) with Some p => p | None => 0 end).
      exists (remove Z.eq_dec b (shared_borrowers (s1 pg))).
      repeat split; try reflexivity.
      * destruct (owner (s1 pg)) eqn:Ho; [reflexivity|contradiction].
      * assumption.
    + apply update_preserves_valid; [|exact Hvalid].
      simpl. destruct Hvalid as [Hcoh _].
      specialize (Hcoh pg). rewrite H0 in Hcoh. simpl in Hcoh.
      destruct Hcoh as [Hown _].
      exists (match owner (s1 pg) with Some p => p | None => 0 end).
      repeat split.
      destruct (owner (s1 pg)) eqn:Ho; [reflexivity|contradiction].
  - (* Step_BorrowMut *)
    inversion H; subst.
    apply update_preserves_valid; [|exact Hvalid].
    simpl. exists o, b. repeat split; reflexivity.
  - (* Step_ReturnMut *)
    inversion H; subst.
    apply update_preserves_valid; [|exact Hvalid].
    simpl. destruct Hvalid as [Hcoh _].
    specialize (Hcoh pg). rewrite H0 in Hcoh. simpl in Hcoh.
    destruct Hcoh as [Hown _].
    exists (match owner (s1 pg) with Some p => p | None => 0 end).
    repeat split.
    destruct (owner (s1 pg)) eqn:Ho; [reflexivity|contradiction].
  - (* Step_Transfer *)
    inversion H; subst.
    apply update_preserves_valid; [|exact Hvalid].
    simpl. exists t. repeat split; reflexivity.
Qed.


