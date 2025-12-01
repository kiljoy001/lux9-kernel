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

Record PageState := mkPageState {
  owner : option Pid;
  state : OwnerState;
  shared_borrowers : list Pid;
  mut_borrower : option Pid;
}.

(* Global System State *)
Definition SystemState := PageId -> PageState.

(* Initial State *)
Definition init_state : SystemState :=
  fun _ => mkPageState None Free [] None.

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

Inductive BorrowShared (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BS_Success_Exclusive :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = Exclusive ->
      ~ In p_borrower (s1 page).(shared_borrowers) -> 
      s2 = update_page s1 page (mkPageState (Some p_owner) SharedOwned [p_borrower] None) ->
      BorrowShared p_owner p_borrower page s1 s2
  | BS_Success_Shared :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = SharedOwned ->
      ~ In p_borrower (s1 page).(shared_borrowers) -> 
      s2 = update_page s1 page (mkPageState (Some p_owner) SharedOwned (p_borrower :: (s1 page).(shared_borrowers)) None) ->
      BorrowShared p_owner p_borrower page s1 s2.

(* REFACTORED: Removed internal 'let' binding to fix proof visibility *)
Inductive ReturnShared (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RS_Success_StillShared :
      forall new_list,
      (s1 page).(state) = SharedOwned ->
      In p_borrower (s1 page).(shared_borrowers) ->
      new_list = remove Z.eq_dec p_borrower (s1 page).(shared_borrowers) -> 
      new_list <> [] ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) SharedOwned new_list None) ->
      ReturnShared p_borrower page s1 s2
  | RS_Success_BackToExclusive :
      (s1 page).(state) = SharedOwned ->
      In p_borrower (s1 page).(shared_borrowers) ->
      remove Z.eq_dec p_borrower (s1 page).(shared_borrowers) = [] ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) Exclusive [] None) ->
      ReturnShared p_borrower page s1 s2.

Inductive BorrowMut (p_owner : Pid) (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | BM_Success :
      (s1 page).(owner) = Some p_owner ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some p_owner) MutLent [] (Some p_borrower)) ->
      BorrowMut p_owner p_borrower page s1 s2.

Inductive ReturnMut (p_borrower : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | RM_Success :
      (s1 page).(state) = MutLent ->
      (s1 page).(mut_borrower) = Some p_borrower ->
      s2 = update_page s1 page (mkPageState (s1 page).(owner) Exclusive [] None) ->
      ReturnMut p_borrower page s1 s2.

Inductive Transfer (from : Pid) (to : Pid) (page : PageId) (s1 s2 : SystemState) : Prop :=
  | Transfer_Success :
      (s1 page).(owner) = Some from ->
      (s1 page).(state) = Exclusive ->
      s2 = update_page s1 page (mkPageState (Some to) Exclusive [] None) ->
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
  simpl in *.

(* 2. Prepares a goal for solving by splitting conjunctions and injecting equalities *)
Ltac setup_hit :=
  simpl_page_access;
  simpl in *;
  repeat match goal with
  | [ H : mkPageState _ _ _ _ = mkPageState _ _ _ _ |- _ ] => injection H; clear H; intros; subst
  | [ |- _ /\ _ ] => split
  end.

(* 3. Coherence Solver (Matches updated ReturnShared logic) *)
Ltac solve_coherence_hit :=
  setup_hit;
  match goal with
  | [ |- context [Exclusive] ] => auto 
  | [ |- context [MutLent] ] => auto
  | [ |- context [Free] ] => auto
  | [ |- context [SharedOwned] ] => 
      auto; 
      try match goal with
      | [ H : ?l <> [] |- ?l <> [] ] => exact H 
      | [ |- ?x :: ?l <> [] ] => discriminate 
      | [ H : In _ ?l |- ?l <> [] ] => destruct l; [inversion H | discriminate]
      (* Explicitly handle the equality hypothesis generated by the refactor *)
      | [ H : ?l = remove _ _ _ |- ?l <> [] ] => rewrite H; assumption 
      end
  end;
  try discriminate.

(* 4. Write Safety Solver *)
Ltac solve_write_hit :=
  intros p1 p2 HW1 HW2;
  setup_hit;
  unfold CanWrite in *;
  destruct HW1 as [[Hown1 Hst1] | [Hmut1 Hst1]];
  destruct HW2 as [[Hown2 Hst2] | [Hmut2 Hst2]];
  try (rewrite Hst1 in *; discriminate);
  try (rewrite Hst2 in *; discriminate);
  try (injection Hown1; injection Hown2; injection Hmut1; injection Hmut2; intros; subst);
  auto.

(* 5. Race Safety Solver *)
Ltac solve_race_hit :=
  intros p1 p2 HW HR;
  setup_hit;
  unfold CanWrite, CanRead in *;
  destruct HW as [[HW_Own HW_St] | [HW_Mut HW_St]];
  try (
    destruct HR as [HR_W | [ [HR_Sh HR_O] | HR_In ]];
    [ destruct HR_W as [[_ HR_St] | [_ HR_St]];
      rewrite HW_St in HR_St; try discriminate;
      try (injection HW_Own; injection HR_O; intros; subst; auto);
      try (inversion HR_St; subst; auto)
    | rewrite HW_St in HR_Sh; discriminate 
    | rewrite HW_St in *; simpl in *; contradiction ]
  );
  try (
    destruct HR as [HR_W | [ [HR_Sh HR_O] | HR_In ]];
    [ destruct HR_W as [[_ HR_St] | [_ HR_St]];
      rewrite HW_St in HR_St; try discriminate;
      try (injection HW_Mut; injection HR_O; intros; subst; auto);
      try (inversion HR_St; subst; auto)
    | rewrite HW_St in HR_Sh; discriminate 
    | rewrite HW_St in *; simpl in *; contradiction ]
  ).

(* 6. Main Dispatchers (Explicit pattern matching to avoid fragile matches) *)
Ltac solve_coherence_main :=
  unfold Inv_Coherence; intros pg_chk;
  match goal with 
  | [ |- context[update_page _ ?mod_pg _ pg_chk] ] => 
      destruct (Z.eq_dec pg_chk mod_pg); [subst | ]
  end;
  [ solve_coherence_hit | eapply Coherence_Frame_Step; eauto ].

Ltac solve_write_main :=
  unfold Inv_WriteSafety; intros pg_chk;
  unfold CanWrite; 
  match goal with 
  | [ |- context[update_page _ ?mod_pg _ pg_chk] ] => 
      destruct (Z.eq_dec pg_chk mod_pg); [subst | ]
  end;
  [ solve_write_hit | eapply WriteSafety_Preserved_Miss; eauto ].

Ltac solve_race_main :=
  unfold Inv_NoReadWriteRace; intros pg_chk;
  unfold CanRead, CanWrite;
  match goal with 
  | [ |- context[update_page _ ?mod_pg _ pg_chk] ] => 
      destruct (Z.eq_dec pg_chk mod_pg); [subst | ]
  end;
  [ solve_race_hit | eapply RaceSafety_Preserved_Miss; eauto ].

(* 7. Master Tactic (Force Substitution First) *)
Ltac solve_valid :=
  try subst;               
  unfold ValidState;       
  split; 
  [ solve_coherence_main |   (* Branch 1: Coherence *)
    split; 
    [ solve_write_main |     (* Branch 2: Write Safety *)
      solve_race_main ]      (* Branch 3: Race Safety *)
  ].

(* ========================================================================= *)
(* MAIN THEOREM                                *)
(* ========================================================================= *)

Theorem transition_preserves_validity :
  forall s1 s2, ValidState s1 -> Step s1 s2 -> ValidState s2.
Proof.
  intros s1 s2 Hvalid Hstep.
  unfold ValidState in Hvalid. destruct Hvalid as [Hcoh [Hwrite Hrace]].
  
  (* Break down the wrapper Step *)
  inversion Hstep; subst; clear Hstep.

  (* ========================================================= *)
  (* GOAL 1: ACQUIRE (Manual Debug Mode)                       *)
  (* ========================================================= *)
  - inversion H; subst; clear H.
    
    (* Step 1: Unfold the definition of validity for the NEW state *)
    unfold ValidState.
    
    (* Step 2: Split it into the 3 invariants *)
    split.

    (* --- PART A: COHERENCE --- *)
    + unfold Inv_Coherence. 
      intros pg_chk.
      
      (* Split: Is this the page we touched (Hit) or a different one (Miss)? *)
      destruct (Z.eq_dec pg_chk pg) as [Heq | Hneq].
      
      * (* HIT CASE: We are looking at the page we just Acquired *)
        subst. 
        (* Rewrite s2 to look like the new struct *)
        rewrite update_page_hit.
        simpl.
        (* The goal asks: Does {Owner=Some p, State=Exclusive...} make sense? *)
        (* Yes, it does. *)
        split; auto. discriminate.
        
      * (* MISS CASE: We are looking at an untouched page *)
        (* Rewrite s2 to look like s1 *)
        rewrite update_page_miss; auto.
        (* Apply the old invariant from s1 *)
        eapply Coherence_Frame_Step; eauto.

    (* --- PART B: SAFETY (Write & Race) --- *)
    + split.
      
      * (* WRITE SAFETY *)
        unfold Inv_WriteSafety. 
        intros pg_chk p1 p2 HW1 HW2.
        unfold CanWrite in *.
        
        destruct (Z.eq_dec pg_chk page) as [Heq | Hneq].
        { (* HIT CASE *)
          subst. rewrite update_page_hit in *.
          (* Analyze who claims to write *)
          destruct HW1 as [[Hown1 Hst1] | [Hmut1 Hst1]];
          destruct HW2 as [[Hown2 Hst2] | [Hmut2 Hst2]].
          - (* Both claim Owner+Exclusive *)
            injection Hown1; injection Hown2; intros; subst; reflexivity.
          - (* One claims Exclusive, one claims MutLent? Impossible. *)
            discriminate.
          - (* One claims MutLent, one claims Exclusive? Impossible. *)
            discriminate.
          - (* Both claim MutLent? Impossible in Acquire (State is Exclusive) *)
            discriminate.
        }
        { (* MISS CASE *)
          rewrite update_page_miss in *; auto.
          eapply WriteSafety_Preserved_Miss; eauto.
        }

      * (* RACE SAFETY *)
        unfold Inv_NoReadWriteRace.
        intros pg_chk p1 p2 HW HR.
        unfold CanWrite, CanRead in *.
        
        destruct (Z.eq_dec pg_chk page) as [Heq | Hneq].
        { (* HIT CASE *)
          subst. rewrite update_page_hit in *.
          (* Who is writing? *)
          destruct HW as [[Hown1 Hst1] | [Hmut1 Hst1]].
          
          (* Case: Writer is Owner (Expected) *)
          - destruct HR as [HR_Write | [ [HR_Shared HR_Own] | HR_InList ]].
            + (* Reader is also Writer *)
              destruct HR_Write as [[Hown2 Hst2] | [Hmut2 Hst2]].
              * injection Hown1; injection Hown2; intros; subst; reflexivity.
              * discriminate.
            + (* Reader is Shared Owner? Impossible in Exclusive *)
              rewrite Hst1 in HR_Shared. discriminate.
            + (* Reader is in Borrowers List? List is empty! *)
              simpl in HR_InList. contradiction.
          
          (* Case: Writer is MutBorrower (Impossible in Acquire) *)
          - discriminate.
        }
        { (* MISS CASE *)
          rewrite update_page_miss in *; auto.
          eapply RaceSafety_Preserved_Miss; eauto.
        }

  (* ========================================================= *)
  (* OTHER GOALS (Use the tactic for now)                      *)
  (* ========================================================= *)
  - inversion H; subst; clear H; solve_valid. (* Release *)
  - inversion H; subst; clear H; solve_valid. (* BorrowShared *)
  - inversion H; subst; clear H; solve_valid. (* ReturnShared *)
  - inversion H; subst; clear H; solve_valid. (* BorrowMut *)
  - inversion H; subst; clear H; solve_valid. (* ReturnMut *)
  - inversion H; subst; clear H; solve_valid. (* Transfer *)
Qed.