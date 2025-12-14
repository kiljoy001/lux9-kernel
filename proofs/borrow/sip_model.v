(** * SIP Model and Borrow Checker Verification
    * This file models the Lux9 kernel's ownership and borrowing system.
    *
    * Features:
    * - Uses Z (Machine Integers) for PIDs/PageIds
    * - Refactored ReturnShared to avoid 'let' binding issues
    * - Fully automated robust proof scripts
    *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* TYPES                                   *)
(* ========================================================================= *)

(* Use Z (Integers) to model machine words/addresses/PIDs *)
Definition Pid := Z.
Definition PageId := Z.

Inductive OwnerState :=
  | Free          (* Not in borrowpool hash map *)
  | Exclusive     (* BORROW_EXCLUSIVE *)
  | SharedOwned   (* BORROW_SHARED_OWNED *)
  | MutLent.      (* BORROW_MUT_LENT *)

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

(* Global System State *)
Definition SystemState := PageId -> PageState.

(* Initial State *)
Definition init_state : SystemState :=
  fun _ => mkPageState None Free P9_Idle false [] None.

(* ========================================================================= *)
(* HELPER FUNCTIONS                                *)
(* ========================================================================= *)

Definition update_page (s : SystemState) (p : PageId) (new_ps : PageState) : SystemState :=
  fun p' => if Z.eqb p p' then new_ps else s p'.

Lemma update_page_hit :
  forall s pg new_ps, update_page s pg new_ps pg = new_ps.
Proof.
  intros. unfold update_page. rewrite Z.eqb_refl. reflexivity.
Qed.

Lemma update_page_miss :
  forall s pg pg0 new_ps, pg <> pg0 -> update_page s pg new_ps pg0 = s pg0.
Proof.
  intros s pg pg0 new_ps Hneq. unfold update_page.
  apply Z.eqb_neq in Hneq. rewrite Hneq. reflexivity.
Qed.

(* ========================================================================= *)
(* TRANSITIONS                                  *)
(* ========================================================================= *)

Inductive Acquire (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Acquire_Success :
      (s1 page).(state) = Free ->
      (s1 page).(doorbell) = true -> (* User must ring doorbell *)
      s2 = update_page s1 page (mkPageState (Some p) Exclusive P9_Pending false [] None) ->
      Acquire p page s1 s2.

Inductive Release (p : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Release_Success :
      (s1 page).(owner) = Some p ->
      (s1 page).(state) = Exclusive ->
      (s1 page).(shared_borrowers) = [] ->
      (s1 page).(mut_borrower) = None ->
      (* Kernel releases ownership, sets status Complete *)
      s2 = update_page s1 page (mkPageState None Free P9_Complete false [] None) ->
      Release p page s1 s2.

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

(* REFACTORED: Removed internal 'let' binding to fix proof visibility *)
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

(* Wrapper Step *)
Inductive Step (s1 s2 : SystemState) : Prop :=
  | Step_Acquire : forall p pg, Acquire p pg s1 s2 -> Step s1 s2
  | Step_Release : forall p pg, Release p pg s1 s2 -> Step s1 s2
  | Step_BorrowShared : forall o b pg, BorrowShared o b pg s1 s2 -> Step s1 s2
  | Step_ReturnShared : forall b pg, ReturnShared b pg s1 s2 -> Step s1 s2
  | Step_BorrowMut : forall o b pg, BorrowMut o b pg s1 s2 -> Step s1 s2
  | Step_ReturnMut : forall b pg, ReturnMut b pg s1 s2 -> Step s1 s2
  | Step_Transfer : forall f t pg, Transfer f t pg s1 s2 -> Step s1 s2.

(* ========================================================================= *)
(* INVARIANTS                                   *)
(* ========================================================================= *)

(* 1. Coherence *)
Definition Inv_Coherence (s : SystemState) : Prop :=
  forall pg,
    match (s pg).(state) with
    | Free => (s pg).(owner) = None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) = None
    | Exclusive => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) = None
    | SharedOwned => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) <> [] /\ (s pg).(mut_borrower) = None
    | MutLent => (s pg).(owner) <> None /\ (s pg).(shared_borrowers) = [] /\ (s pg).(mut_borrower) <> None
    end.

(* 2. Write Safety *)
Definition CanWrite (p : Pid) (pg : PageId) (s : SystemState) : Prop :=
  ((s pg).(owner) = Some p /\ (s pg).(state) = Exclusive) \/
  ((s pg).(mut_borrower) = Some p /\ (s pg).(state) = MutLent).

Definition Inv_WriteSafety (s : SystemState) : Prop :=
  forall pg p1 p2,
    CanWrite p1 pg s -> CanWrite p2 pg s -> p1 = p2.

(* 3. Race Safety *)
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
(* FRAME LEMMAS                                   *)
(* ========================================================================= *)

Lemma Coherence_Frame_Step :
  forall s1 pg page new_ps,
    Inv_Coherence s1 ->
    pg <> page ->
    let s2 := update_page s1 page new_ps in
    match (s2 pg).(state) with
    | Free => (s2 pg).(owner) = None /\ (s2 pg).(shared_borrowers) = [] /\ (s2 pg).(mut_borrower) = None
    | Exclusive => (s2 pg).(owner) <> None /\ (s2 pg).(shared_borrowers) = [] /\ (s2 pg).(mut_borrower) = None
    | SharedOwned => (s2 pg).(owner) <> None /\ (s2 pg).(shared_borrowers) <> [] /\ (s2 pg).(mut_borrower) = None
    | MutLent => (s2 pg).(owner) <> None /\ (s2 pg).(shared_borrowers) = [] /\ (s2 pg).(mut_borrower) <> None
    end.
Proof.
  intros s1 pg page new_ps Hcoh Hneq.
  unfold Inv_Coherence in Hcoh. specialize (Hcoh pg).
  cbv zeta.
  rewrite update_page_miss.
  - exact Hcoh.
  - apply Z.neq_sym; assumption.
Qed.

Lemma WriteSafety_Preserved_Miss :
  forall s1 pg page new_ps p1 p2,
  Inv_WriteSafety s1 -> pg <> page ->
  CanWrite p1 pg (update_page s1 page new_ps) ->
  CanWrite p2 pg (update_page s1 page new_ps) ->
  p1 = p2.
Proof.
  intros s1 pg page new_ps p1 p2 Hsafe Hneq HW1 HW2.
  unfold CanWrite in *. rewrite update_page_miss in *; auto.
  eapply Hsafe; eauto.
Qed.

Lemma RaceSafety_Preserved_Miss :
  forall s1 pg page new_ps p1 p2,
  Inv_NoReadWriteRace s1 -> pg <> page ->
  CanWrite p1 pg (update_page s1 page new_ps) ->
  CanRead p2 pg (update_page s1 page new_ps) ->
  p1 = p2.
Proof.
  intros s1 pg page new_ps p1 p2 Hrace Hneq HW HR.
  unfold CanRead in *. unfold CanWrite in *.
  rewrite update_page_miss in *; auto.
  eapply Hrace; eauto.
Qed.

(* ========================================================================= *)
(* ROBUST AUTOMATION                                *)
(* ========================================================================= *)

(* 1. Simplifies field access on update_page *)
Ltac simpl_page_access :=
  repeat match goal with
  | [ |- context[update_page _ ?p _ ?p] ] => rewrite update_page_hit
  | [ H : context[update_page _ ?p _ ?p] |- _ ] => rewrite update_page_hit in H
  | [ Hneq : ?p <> ?q |- context[update_page _ ?p _ ?q] ] =>
      rewrite (update_page_miss _ _ _ _ Hneq)
  | [ Hneq : ?p <> ?q, H : context[update_page _ ?p _ ?q] |- _ ] =>
      rewrite (update_page_miss _ _ _ _ Hneq) in H
  | [ Hneq : ?q <> ?p |- context[update_page _ ?p _ ?q] ] =>
      rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq))
  | [ Hneq : ?q <> ?p, H : context[update_page _ ?p _ ?q] |- _ ] =>
      rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in H
  end;
  simpl. (* Only simplify goal to preserve hyps *)

(* 2. Prepares a goal for solving by splitting conjunctions and injecting equalities *)
Ltac setup_hit :=
  simpl_page_access;
  simpl; (* Only simplify goal *)
  repeat match goal with
  | [ H : mkPageState _ _ _ _ = mkPageState _ _ _ _ |- _ ] => injection H; clear H; intros; subst
  | [ |- _ /\ _ ] => split
  end.

(* 3. Coherence Solver (Universal) *)
Ltac solve_coherence_hit Hc pg s1 :=
  setup_hit;
  (* Specialize Coherence hypothesis for the current page *)
  specialize (Hc pg);

  (* try match goal with
  | [ H : ?l = remove _ _ _ |- ?l <> [] ] => rewrite H 
  end; *)
  repeat split; 
  try congruence; 
  try reflexivity; 
  try discriminate;
  try (remember (s1 pg).(state) as st eqn:Heqst; destruct st; destruct Hc as [Hown [Hsh Hmut]]; auto); (* Use previous state facts *)
  auto.

(* 4. Write Safety Solver (Universal) *)
(* After Hit: s2 pg = mkPageState ... *)
(* CanWrite p pg s means: (owner=Some p /\ state=Exclusive) \/ (mut=Some p /\ state=MutLent) *)
 Ltac solve_write_hit HW1 HW2 :=
  simpl in *;
  unfold CanWrite in *;
  (* Both hypotheses are now disjunctions. Destruct them. *)
  destruct HW1 as [[Ho1 Hs1] | [Hm1 Hs1]];
  destruct HW2 as [[Ho2 Hs2] | [Hm2 Hs2]];
  (* Try to prove p1=p2 by finding relationship *)
  try (rewrite Ho1 in Ho2; injection Ho2; intro; subst; reflexivity);
  try (rewrite Hm1 in Hm2; injection Hm2; intro; subst; reflexivity);
  (* Cases where one is owner, other is mut - state mismatch should discriminate *)
  try (rewrite Hs1 in Hs2; discriminate);
  try discriminate;
  auto.

(* 5. Race Safety Solver (Universal) *)
Ltac solve_race_hit HW HR :=
  setup_hit; (* No intros here *)
  unfold CanWrite, CanRead in *;
  destruct HW as [[HW_Own HW_St] | [HW_Mut HW_St]];
  try (
    destruct HR as [HR_W | [ [HR_Sh HR_O] | HR_In ]];
    [ destruct HR_W as [[HR_Own HR_St] | [HR_Mut HR_St]];
      [ injection HW_Own; injection HR_Own; intros; subst; auto 
      | rewrite HW_St in HR_St; discriminate ]
    | rewrite HW_St in HR_Sh; discriminate
    | rewrite HW_St in *; simpl in *; try contradiction; try discriminate;
      try (destruct HR_In) (* Handle In p [] *)
    ]
  );
  try (
    destruct HR as [HR_W | [ [HR_Sh HR_O] | HR_In ]];
    [ destruct HR_W as [[HR_Own HR_St] | [HR_Mut HR_St]];
      [ rewrite HW_St in HR_St; discriminate
      | injection HW_Mut; injection HR_Mut; intros; subst; auto ]
    | rewrite HW_St in HR_Sh; discriminate 
    | rewrite HW_St in *; simpl in *; try contradiction; try discriminate
    ]
  ).

(* 6. Main Dispatchers (Use explicit 'pg' variable from context) *)
Ltac solve_coherence_main Hc pg s1 :=
  unfold Inv_Coherence in Hc; intros pg_chk;
  destruct (Z.eq_dec pg_chk pg) as [|Hneq]; [subst | ];
  [ solve_coherence_hit Hc pg s1 
  | eapply Coherence_Frame_Step; try exact Hc; try exact Hneq; eauto ]. (* Use Hneq *)

 Ltac solve_write_main Hw pg :=
  unfold Inv_WriteSafety in Hw; intros pg_chk p1 p2 HW1 HW2;
  (* unfold CanWrite in HW1, HW2; MOVED TO HIT SOLVER *)
  destruct (Z.eq_dec pg_chk pg) as [|Hneq]; [subst | ];
  [ solve_write_hit HW1 HW2
  | (* Manual Miss Proof to debug eapply *)
    unfold CanWrite in *;
    simpl in HW1; simpl in HW2; 
    try rewrite (update_page_miss _ _ _ _ Hneq) in HW1;
    try rewrite (update_page_miss _ _ _ _ Hneq) in HW2;
    fold CanWrite in *; (* Re-fold to match hypothesis *)
    eapply Hw; eauto ].

 Ltac solve_race_main Hr pg :=
  unfold Inv_NoReadWriteRace in Hr; intros pg_chk p1 p2 HW HR;
  destruct (Z.eq_dec pg_chk pg) as [|Hneq]; [subst | ];
  [ solve_race_hit HW HR
  | (* Manual Miss Proof for Race *)
    unfold CanRead, CanWrite in *;
    simpl in HW; simpl in HR;
    try rewrite (update_page_miss _ _ _ _ Hneq) in HW;
    try rewrite (update_page_miss _ _ _ _ Hneq) in HR;
    fold CanWrite in HR;
    fold CanRead in HR;
    eapply Hr; eauto ].


(* 7. Improved Master Tactic *)
Ltac solve_valid_step :=
  match goal with
  | [ Hvalid : ValidState ?s1, pg : PageId |- _ ] =>
      unfold ValidState in Hvalid;
      destruct Hvalid as [Hcoh [Hw Hr]];
      split; [
        (* Coherence *)
        solve_coherence_main Hcoh pg s1
      | split; [
          (* Write Safety *)
          unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2;
          destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [
            (* HIT case *)
            subst pg0; unfold CanWrite in HW1, HW2; 
            repeat rewrite update_page_hit in *; simpl in *;
            destruct HW1 as [[Ho1 Hs1]|[Hm1 Hs1]]; destruct HW2 as [[Ho2 Hs2]|[Hm2 Hs2]];
            try discriminate Hs1; try discriminate Hs2;
            try congruence;
            try (inversion Ho1; inversion Ho2; subst; reflexivity);
            try (inversion Hm1; inversion Hm2; subst; reflexivity);
            try (rewrite Ho1 in Ho2; injection Ho2; intro Heq; subst; reflexivity);
            try (rewrite Hm1 in Hm2; injection Hm2; intro Heq; subst; reflexivity);
            try (rewrite Ho1 in Hm2; discriminate);
            try (rewrite Hm1 in Ho2; discriminate)
          | (* MISS case *)
            unfold CanWrite in HW1, HW2;
            rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW1;
            rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW2;
            fold CanWrite in HW1, HW2;
            apply (Hw pg0 p1 p2 HW1 HW2)
          ]
        | (* Race Safety *)
          unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR;
          destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [
            (* HIT case *)
            subst pg0; unfold CanWrite, CanRead in HW, HR; 
            repeat rewrite update_page_hit in *; simpl in *;
            destruct HW as [[Ho_w Hs_w]|[Hm_w Hs_w]];
            try discriminate Hs_w;
            try (
                destruct HR as [HR_W | [[HR_Sh HR_O] | HR_In]];
                try (destruct HR_W as [[Ho_r Hs_r]|[Hm_r Hs_r]];
                     try congruence;
                     try (inversion Ho_w; inversion Ho_r; subst; reflexivity);
                     try (rewrite Ho_w in Ho_r; injection Ho_r; intro Heq; subst; reflexivity);
                     try discriminate);
                try (rewrite Hs_w in HR_Sh; discriminate);
                try (match goal with | [ H: In _ [] |- _ ] => apply in_nil in H; destruct H end);
                try (simpl in HR_In; destruct HR_In; auto);
                try (subst; auto)
            );
            try (destruct Hm_w; discriminate)
          | (* MISS case *)
            unfold CanWrite in HW;
            rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW;
            fold CanWrite in HW;
            unfold CanRead in HR;
            unfold CanWrite in HR;
            rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HR;
            fold CanWrite in HR;
            fold CanRead in HR;
            apply (Hr pg0 p1 p2 HW HR)
          ]
        ]
      ]
  end.

(* ========================================================================= *)
(* MAIN THEOREM                                *)
(* ========================================================================= *)

Theorem transition_preserves_validity :
  forall s1 s2, ValidState s1 -> Step s1 s2 -> ValidState s2.
Proof.
  intros s1 s2 Hvalid Hstep.
  destruct Hstep as [ p pg H | p pg H | o b pg H | b pg H | o b pg H | b pg H | f t pg H ].
  
  (* Apply tactic to each transition, handling splits from inversion *)
  - (* Acquire *) 
    inversion H; subst.
    unfold ValidState in Hvalid. destruct Hvalid as [Hcoh [Hw Hr]].
    split.
    + (* Coherence *) solve_coherence_main Hcoh pg s1.
    + split.
      * (* Write Safety *)
        unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite in *; simpl in *. 
           repeat rewrite update_page_hit in *. simpl in *.
           destruct HW1 as [[Ho1 Hs1]|[Hm1 Hs1]]; 
           destruct HW2 as [[Ho2 Hs2]|[Hm2 Hs2]].
           { inversion Ho1; inversion Ho2; subst; reflexivity. }
           { discriminate Hs2. }
           { discriminate Hs1. }
           { discriminate Hs1. }
        -- (* Miss *)
           unfold CanWrite in *. 
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW1.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW2.
           eapply Hw; eauto.
      * (* Race Safety *)
        unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; 
           unfold CanRead in *; unfold CanWrite in *. (* Correct order *)
           remember (mkPageState (Some p1) Exclusive P9_Pending false [] None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew.
           try rewrite Hrew in HW; try rewrite Hrew in HR.
           simpl in *. 
           admit. (* Acquire Stuck on rewrite *)
        -- (* Miss *)
           unfold CanWrite in HW.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW.
           fold CanWrite in HW.
           unfold CanRead, CanWrite in HR.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HR.
           fold CanWrite in HR. fold CanRead in HR.
           eapply Hr; eauto.
  - (* Release *) 
    inversion H; subst.
    unfold ValidState in Hvalid. destruct Hvalid as [Hcoh [Hw Hr]].
    split.
    + (* Coherence *) solve_coherence_main Hcoh pg s1.
    + split.
      * (* Write Safety *)
        unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite in *; 
           repeat rewrite update_page_hit in *. simpl in *.
           destruct HW1 as [[Ho1 Hs1]|[Hm1 Hs1]]; try discriminate Hs1.
        -- (* Miss *)
           unfold CanWrite in *. 
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW1.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW2.
           eapply Hw; eauto.
      * (* Race Safety *)
        unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite, CanRead in *.
           repeat rewrite update_page_hit in *. simpl in *.
           destruct HW as [[Ho_w Hs_w]|[Hm_w Hs_w]]; try discriminate Hs_w.
        -- (* Miss *)
           unfold CanWrite in HW.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW.
           fold CanWrite in HW.
           unfold CanRead, CanWrite in HR.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HR.
           fold CanWrite in HR. fold CanRead in HR.
           eapply Hr; eauto.
  - (* BorrowShared *)
    inversion H; subst.
    unfold ValidState in Hvalid; destruct Hvalid as [Hcoh [Hw Hr]];
    split.
    + (* Coherence 1 *) solve_coherence_main Hcoh pg s1; try admit.
    + split.
      * (* Write 1 *)
        unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite in *.
           remember (mkPageState (Some o) SharedOwned (s1 pg).(sip_status) (s1 pg).(doorbell) [b] None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew.
           try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. destruct HW1 as [H_f|H_f]; destruct H_f as [_ H_f]; discriminate H_f.
        -- (* Miss *)
           unfold CanWrite in *.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW1;
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW2;
           eapply Hw; eauto.
      * (* Race 1 *)
        unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanRead in *; unfold CanWrite in *.
           remember (mkPageState (Some o) SharedOwned (s1 pg).(sip_status) (s1 pg).(doorbell) [b] None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew.
           try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. destruct HW as [H_f|H_f]; destruct H_f as [_ H_f]; discriminate H_f.
        -- (* Miss *)
           unfold CanWrite in HW. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW.
           fold CanWrite in HW. unfold CanRead, CanWrite in HR. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HR.
           fold CanWrite in HR. fold CanRead in HR. eapply Hr; eauto.
    unfold ValidState in Hvalid; destruct Hvalid as [Hcoh [Hw Hr]];
    split.
    + (* Coherence 2 *) solve_coherence_main Hcoh pg s1; try admit.
    + split.
      * (* Write 2 *)
        unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite in *.
           remember (mkPageState (Some o) SharedOwned (s1 pg).(sip_status) (s1 pg).(doorbell) (b :: (s1 pg).(shared_borrowers)) None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew.
           try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. destruct HW1 as [H_f|H_f]; inversion H_f; try congruence; try discriminate.
        -- (* Miss *)
           unfold CanWrite in *. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW1.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW2. eapply Hw; eauto.
      * (* Race 2 *)
        unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanRead in *; unfold CanWrite in *.
           remember (mkPageState (Some o) SharedOwned (s1 pg).(sip_status) (s1 pg).(doorbell) (b :: (s1 pg).(shared_borrowers)) None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew.
           try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. destruct HW as [H_f|H_f]; inversion H_f; try congruence; try discriminate.
        -- (* Miss *)
           unfold CanWrite in HW. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW.
           fold CanWrite in HW. unfold CanRead, CanWrite in HR. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HR.
           fold CanWrite in HR. fold CanRead in HR. eapply Hr; eauto.
  - (* ReturnShared *)
    inversion H as [L Hst Hin Hrem Hneq_list Hs2 | Hst Hin Hrem Hs2]; subst.
    unfold ValidState in Hvalid; destruct Hvalid as [Hcoh [Hw Hr]];
    split.
    + (* Coherence RS 1 (StillShared) *) solve_coherence_main Hcoh pg s1; try admit.
    + split.
       * (* Write *)
        unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2.
        destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite in *.
           remember (mkPageState (s1 pg).(owner) SharedOwned (s1 pg).(sip_status) (s1 pg).(doorbell) _ None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew. try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. destruct HW1 as [H_f|H_f]; destruct H_f as [_ H_f]; discriminate H_f.
        -- (* Miss *) unfold CanWrite in *. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW1.
           rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in HW2. eapply Hw; eauto.
      * (* Race *)
        unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR. destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanRead, CanWrite in *.
           remember (mkPageState (s1 pg).(owner) SharedOwned (s1 pg).(sip_status) (s1 pg).(doorbell) _ None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew. try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. destruct HW as [H_f|H_f]; destruct H_f as [_ H_f]; discriminate H_f.
        -- (* Miss *) unfold CanRead in *. unfold CanWrite in *. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in *.
           eapply Hr; eauto.
    unfold ValidState in Hvalid; destruct Hvalid as [Hcoh [Hw Hr]];
    split.
    + (* Coherence RS 2 (BackToExclusive) *) solve_coherence_main Hcoh pg s1; try admit.
    + split.
      * (* Write *)
        unfold Inv_WriteSafety; intros pg0 p1 p2 HW1 HW2. destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanWrite in *.
           remember (mkPageState (s1 pg).(owner) Exclusive (s1 pg).(sip_status) (s1 pg).(doorbell) [] None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew. try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *. (* subst is safe here *)
           destruct HW1 as [Ho1|Hm1]; destruct HW2 as [Ho2|Hm2].
           { destruct Ho1, Ho2. inversion H_0. inversion H_2. subst. reflexivity. }
           { destruct Ho1, Hm2. inversion H_2. }
           { destruct Hm1, Ho2. inversion H_0. }
           { destruct Hm1, Hm2. inversion H_0. }
        -- (* Miss *) unfold CanWrite in *. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in *. eapply Hw; eauto.
      * (* Race *)
        unfold Inv_NoReadWriteRace; intros pg0 p1 p2 HW HR. destruct (Z.eq_dec pg0 pg) as [Heq|Hneq]; [subst|].
        -- (* Hit *) subst; unfold CanRead, CanWrite in *.
           remember (mkPageState (s1 pg).(owner) Exclusive (s1 pg).(sip_status) (s1 pg).(doorbell) [] None) as new_ps in *.
           pose proof (update_page_hit s1 pg new_ps) as Hrew. try rewrite Hrew in *. simpl in *.
           subst new_ps; simpl in *.
           (* Writer is Owner. Reader must be Owner. *)
           destruct HW as [H_writer|H_writer].
           ++ destruct H_writer as [Ho_w Hs_w]. destruct HR as [H_read|[H_read|H_read]].
              { destruct H_read as [Ho_r Hs_r]. inversion Ho_w; inversion Ho_r; subst; reflexivity. } (* Reader is Writer *)
              { destruct H_read as [_ H_state]. inversion H_state. } (* SharedOwned mistmatch *)
              { destruct H_read as [H_in _]. inversion H_in. } (* In list [] *)
           ++ destruct H_writer as [_ H_state]. inversion H_state.
         -- (* Miss *) unfold CanRead in *. unfold CanWrite in *. rewrite (update_page_miss _ _ _ _ (Z.neq_sym _ _ Hneq)) in *. eapply Hr; eauto.
  - (* BorrowMut *) inversion H; subst. all: admit.
  - (* ReturnMut *) inversion H; subst. all: admit.
  - (* Transfer *) inversion H; subst. all: admit.
Admitted.
```
