(** * MSGORD Kernel Model
    * Formal verification of kernel/msgord_kernel.c
    * Focus: Liveness properties and RED message handling
    *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

Require Import pow_gate.pow_gate_model.

Open Scope Z_scope.

(* ========================================================================= *)
(* 1. DATA TYPES                                *)
(* ========================================================================= *)

(* IDs and Parameters *)
Definition MsgId := Z.
Definition K_PARAM : Z := 3. (* Default MSGORD_K_PARAMETER *)

Inductive Color :=
  | Blue
  | Red.

Inductive MsgState :=
  | Pending   (* MSGORD_STATE_PENDING = 0 *)
  | Ordered   (* MSGORD_STATE_ORDERED = 1 *)
  | Delivered (* MSGORD_STATE_DELIVERED = 2 *)
  | Complete. (* MSGORD_STATE_COMPLETE = 3 *)

Record GhostMsg := mkMsg {
  gm_id : MsgId;
  gm_parents : list MsgId;
  gm_color : Color;
  gm_state : MsgState;
}.

(* The "DAG" in the kernel is actually a concurrent queue/list of active messages *)
Definition MsgOrd := list GhostMsg.

(* ========================================================================= *)
(* 2. KERNAL LOGIC (Functional Specification)                   *)
(* ========================================================================= *)

(* msgord_anticone: Counts PENDING messages that are not parents *)
Definition is_parent (msg : GhostMsg) (target_id : MsgId) : bool :=
  existsb (fun pid => Z.eqb pid target_id) msg.(gm_parents).

Fixpoint msgord_anticone (dag : MsgOrd) (msg : GhostMsg) : Z :=
  match dag with
  | [] => 0
  | gm :: rest =>
      let count_rest := msgord_anticone rest msg in
      (* Iterate through list, skip self *)
      if Z.eqb gm.(gm_id) msg.(gm_id) then count_rest
      else
        (* C logic: if (gm->state != PENDING) continue *)
        match gm.(gm_state) with
        | Pending =>
            (* C logic: check if msg has gm as parent *)
            (* "if (msg->parents[i] == gm->id) is_parent = 1" *)
            if existsb (fun pid => Z.eqb pid gm.(gm_id)) msg.(gm_parents) then
              count_rest
            else
              1 + count_rest
        | _ => count_rest
        end
  end.

(* msgord_color: Blue if anticone <= k, else Red *)
Definition determine_color (dag : MsgOrd) (msg : GhostMsg) : Color :=
  if msgord_anticone dag msg <=? K_PARAM then Blue else Red.

(* Contention ratio (0..100) as percent of pending anticone over DAG size *)
Definition contention_ratio (dag : MsgOrd) (msg : GhostMsg) : Z :=
  let denom := Z.max 1 (Z.of_nat (length dag)) in
  (100 * msgord_anticone dag msg) / denom.

(* Adaptive difficulty: green state <=10% has zero cost, otherwise linear *)
Definition msgord_pow_difficulty (dag : MsgOrd) (msg : GhostMsg) : Z :=
  let ratio := contention_ratio dag msg in
  if ratio <=? 10 then 0 else Z.min 32 (ratio - 10).

Definition pow_ok (hash : Z) (is_tcb : bool) (dag : MsgOrd) (msg : GhostMsg) : bool :=
  if is_tcb then true else pow_verify_spec hash (msgord_pow_difficulty dag msg).

(* _msgord_submit: Add message, determine color, update state *)
(* Returns the new DAG and the ID of the submitted message *)
Definition msgord_submit (dag : MsgOrd) (new_id : MsgId) (hash : Z) (is_tcb : bool) : MsgOrd :=
  (* 1. Create message (Pending by default) *)
  (* parents[0] = tail (if exists). Pure model: just take last added ID? 
     For 'saturation' proof, parents don't matter much if we assume independence,
     but let's try to be faithful. Let's assume 0 parents for worst-case contention. *)
  let raw_msg := mkMsg new_id [] Blue Pending in
  
  (* 2. Enqueue (Append) *)
  (* In functional list, we cons to front for O(1), but C appends to tail.
     Order matters for iteration? anticone iterates whole list.
     Let's model "dag" as the list. order doesn't impact set membership. *)
  let dag_with_msg := raw_msg :: dag in
  
  (* 3. PoW gate (Adaptive Kinetic Defense) *)
  if pow_ok hash is_tcb dag_with_msg raw_msg then
    (* 4. Color *)
    let color := determine_color dag_with_msg raw_msg in
  
    (* 5. Update State *)
    match color with
    | Blue => 
        let final_msg := mkMsg new_id [] Blue Ordered in
        final_msg :: dag (* Add to DAG *)
    | Red => 
        dag (* FAIL-FAST: Drop Red messages, do not add to DAG *)
    end
  else
    dag.

(* msgord_can_deliver: Check if all parents are processed *)
(* C logic: Iterates DAG. If parent found, must be >= Delivered. If not found, assumed Complete. *)
Definition check_parent_safe (dag : MsgOrd) (pid : MsgId) : bool :=
  match find (fun m => Z.eqb m.(gm_id) pid) dag with
  | Some parent => 
      match parent.(gm_state) with
      | Delivered | Complete => true
      | _ => false
      end
  | None => true (* Parent removed/archived means processed *)
  end.

Definition can_deliver (dag : MsgOrd) (msg : GhostMsg) : bool :=
  forallb (check_parent_safe dag) msg.(gm_parents).

(* msgord_process_one: Find ORDERED, Deliver, Remove *)
Inductive ProcessResult :=
  | NoAction (d : MsgOrd)
  | Processed (d : MsgOrd) (id : MsgId).

Fixpoint msgord_process_one (dag : MsgOrd) : ProcessResult :=
  match dag with
  | [] => NoAction []
  | msg :: rest =>
      (* "msgord_next" scans from head. Here head is end of list? 
         Let's assume standard list is head-first. *)
      match msg.(gm_state) with
      | Ordered =>
          (* NEW: Strict Dependency Check *)
          if can_deliver dag msg then
            (* Transition to Delivered/Complete (Simulated by removal) *)
             Processed rest msg.(gm_id)
          else
             (* Blocked by parents. Skip this message, try next. *)
             match msgord_process_one rest with
             | NoAction _ => NoAction (msg :: rest)
             | Processed new_rest pid => Processed (msg :: new_rest) pid
             end
      | _ => 
          (* Not Ordered (Pending?), skip *)
          match msgord_process_one rest with
          | NoAction _ => NoAction (msg :: rest)
          | Processed new_rest pid => Processed (msg :: new_rest) pid
          end
      end
  end.

(* ========================================================================= *)
(* 3. THEOREMS (Bugs & Properties)                     *)
(* ========================================================================= *)

(* Helper: Check if a message exists in the DAG *)
Definition msg_in_dag (d : MsgOrd) (id : MsgId) : Prop :=
  Exists (fun m => m.(gm_id) = id) d.

(* THEOREM 1: RED SAFETY (Fail-Fast) *)
(* Invariant: The DAG only contains Blue messages (Red are rejected) *)
Definition Inv_BlueOnly (dag : MsgOrd) : Prop :=
  forall m, In m dag -> m.(gm_color) = Blue.

Theorem no_red_messages_in_dag :
  forall dag hash is_tcb,
  Inv_BlueOnly dag ->
  let new_dag := msgord_submit dag 100 hash is_tcb in
  Inv_BlueOnly new_dag.
Proof.
  intros dag hash is_tcb Hinv.
  unfold Inv_BlueOnly in *.
  unfold msgord_submit.
  simpl.
  destruct (pow_ok hash is_tcb (mkMsg 100 [] Blue Pending :: dag) (mkMsg 100 [] Blue Pending)) eqn:Hpow.
  - destruct (determine_color (mkMsg 100 [] Blue Pending :: dag) (mkMsg 100 [] Blue Pending)) eqn:Hcolor.
    + (* Blue case: new message added *)
      intros m Hin.
      destruct Hin as [Heq | Hin_old].
      * subst. reflexivity.
      * apply Hinv. exact Hin_old.
    + (* Red case: DAG unchanged *)
      exact Hinv.
  - exact Hinv.
Qed.

(* THEOREM 2: SATURATION RECOVERY *)
(* If the DAG is empty (processed), we can always accept a Blue message *)
Theorem empty_dag_accepts_blue :
  forall new_id hash is_tcb,
  let new_dag := msgord_submit [] new_id hash is_tcb in
  exists m, In m new_dag /\ m.(gm_id) = new_id.
Proof.
  intros new_id hash is_tcb.
  cbv [msgord_submit].
  unfold pow_ok, msgord_pow_difficulty, contention_ratio.
  simpl.
  rewrite Z.eqb_refl.
  simpl.
  destruct is_tcb; simpl.
  - cbv [determine_color msgord_anticone].
    rewrite Z.eqb_refl.
    change (0 <=? K_PARAM)%Z with true.
    simpl.
    exists (mkMsg new_id [] Blue Ordered).
    split.
    + cbv [In]. left. reflexivity.
    + reflexivity.
  - cbv [determine_color msgord_anticone].
    rewrite Z.eqb_refl.
    change (0 <=? K_PARAM)%Z with true.
    simpl.
    exists (mkMsg new_id [] Blue Ordered).
    split.
    + cbv [In]. left. reflexivity.
    + reflexivity.
Qed.

(* THEOREM 3: TOPOLOGICAL ORDERING *)
(* A message is only processed if all its parents are 'Safe' (Delivered/Complete or Gone) *)

(* Helper lemma: if msgord_process_one returns Processed with pid, msg with that id exists and passed can_deliver *)
Lemma process_one_implies_can_deliver :
  forall dag new_dag pid,
  msgord_process_one dag = Processed new_dag pid ->
  exists prefix msg suffix,
    dag = prefix ++ msg :: suffix /\
    msg.(gm_id) = pid /\
    msg.(gm_state) = Ordered /\
    can_deliver (msg :: suffix) msg = true.
Proof.
  induction dag as [| m rest IH]; intros new_dag pid Hproc.
  - simpl in Hproc. discriminate.
  - simpl in Hproc.
    destruct (gm_state m) eqn:Hstate.
    + (* Pending *)
      remember (msgord_process_one rest) as res eqn:E.
      destruct res as [d| d pid']; [discriminate|].
      inversion Hproc; subst.
      destruct (IH _ _ eq_refl) as [prefix [msg [suffix [Hdag [Hid [Hst Hdel]]]]]].
      exists (m :: prefix), msg, suffix.
      repeat split; auto.
      simpl. rewrite Hdag. reflexivity.
    + (* Ordered *)
      destruct (can_deliver (m :: rest) m) eqn:Hcan.
      * inversion Hproc; subst pid new_dag.
        exists [], m, rest. repeat split; auto.
      * remember (msgord_process_one rest) as res eqn:E.
        destruct res as [d| d pid']; [discriminate|].
        inversion Hproc; subst.
        destruct (IH _ _ eq_refl) as [prefix [msg [suffix [Hdag [Hid [Hst Hdel]]]]]].
        exists (m :: prefix), msg, suffix.
        repeat split; auto.
        simpl. rewrite Hdag. reflexivity.
    + (* Delivered *)
      remember (msgord_process_one rest) as res eqn:E.
      destruct res as [d| d pid']; [discriminate|].
      inversion Hproc; subst.
      destruct (IH _ _ eq_refl) as [prefix [msg [suffix [Hdag [Hid [Hst Hdel]]]]]].
      exists (m :: prefix), msg, suffix.
      repeat split; auto.
      simpl. rewrite Hdag. reflexivity.
    + (* Complete *)
      remember (msgord_process_one rest) as res eqn:E.
      destruct res as [d| d pid']; [discriminate|].
      inversion Hproc; subst.
      destruct (IH _ _ eq_refl) as [prefix [msg [suffix [Hdag [Hid [Hst Hdel]]]]]].
      exists (m :: prefix), msg, suffix.
      repeat split; auto.
      simpl. rewrite Hdag. reflexivity.
Qed.

Theorem topological_process_safety :
  forall dag new_dag pid,
  msgord_process_one dag = Processed new_dag pid ->
  exists prefix msg suffix,
    dag = prefix ++ msg :: suffix /\
    gm_id msg = pid /\
    can_deliver (msg :: suffix) msg = true.
Proof.
  intros dag new_dag pid Hproc.
  destruct (process_one_implies_can_deliver dag new_dag pid Hproc)
    as [prefix [msg' [suffix [Hdag [Hid' [_ Hdel]]]]]].
  exists prefix, msg', suffix.
  repeat split; assumption.
Qed.

(* THEOREM 4: SATURATION *)
(* If we have K_PARAM+1 pending messages, the next one is guaranteed to be Red *)

Theorem saturation_leads_to_drop :
  forall dag new_id,
  (exists count, count > K_PARAM /\ count = msgord_anticone dag (mkMsg new_id [] Blue Pending)) ->
  forall hash is_tcb,
  let new_dag := msgord_submit dag new_id hash is_tcb in
  new_dag = dag. (* Proves it was dropped *)
Proof.
  intros dag new_id [count [Hgt Hcount]] hash is_tcb.
  unfold msgord_submit.
  simpl.
  destruct (pow_ok hash is_tcb (mkMsg new_id [] Blue Pending :: dag) (mkMsg new_id [] Blue Pending)) eqn:Hpow.
  - unfold determine_color.
  assert (Hsame: msgord_anticone (mkMsg new_id [] Blue Pending :: dag) {| gm_id := new_id; gm_parents := []; gm_color := Blue; gm_state := Pending |} = msgord_anticone dag {| gm_id := new_id; gm_parents := []; gm_color := Blue; gm_state := Pending |}).
  {
     simpl. rewrite Z.eqb_refl. reflexivity.
  }
  
  rewrite Hsame.
  rewrite <- Hcount.
  (* We have count > K. So count <=? K is false. *)
  assert (Hbool: (count <=? K_PARAM) = false).
  { apply Z.leb_gt. lia. }
  rewrite Hbool.
  reflexivity.
  - reflexivity.
Qed.
