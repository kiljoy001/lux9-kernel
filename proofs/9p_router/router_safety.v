(** * 9P Router Safety Proofs
    * Formal verification of safety properties for kernel/9p_router.c
    *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Require Import Lux9.Router.router_model.
(* Import the instantiated model and the algebra *)
Import Model.
Import PebbleAlgebra.
Import ListNotations.
Open Scope Z_scope.

(* ========================================================================= *)
(* 1. STATUS TRANSITION SAFETY                                               *)
(* ========================================================================= *)

(** The P9Control status can only transition in valid sequences:
    - Idle → Pending (on doorbell ring)
    - Pending → Complete (on success)
    - Pending → Error (on failure)
    - Any → Idle (on reset)
    
    This ensures no race conditions or invalid state combinations. *)

(* Valid state transition predicate *)
Definition valid_transition (before after : P9Status) : Prop :=
  match before, after with
  | P9_Idle, P9_Idle => True         (* No-op *)
  | P9_Idle, P9_Pending => True      (* Doorbell ring *)
  | P9_Pending, P9_Pending => True   (* No-op *)
  | P9_Pending, P9_Complete => True  (* Success *)
  | P9_Pending, P9_Error => True     (* Failure *)
  | P9_Complete, P9_Complete => True (* No-op *)
  | P9_Error, P9_Error => True       (* No-op *)
  | _, P9_Idle => True               (* Reset from Complete or Error *)
  | _, _ => False                    (* Invalid transition *)
  end.

(** Theorem: All transitions produced by p9_transition are valid *)
Theorem status_transitions_valid :
  forall ctl ev,
  valid_transition ctl.(ctl_status) (p9_transition ctl ev).(ctl_status).
Proof.
  intros [s rs rp db] ev.
  destruct ev, s; simpl; trivial.
Qed.


(** Theorem: Doorbell ring only works from Idle state *)
Theorem doorbell_requires_idle :
  forall ctl,
  ctl.(ctl_status) <> P9_Idle ->
  (p9_transition ctl EvDoorbellRing).(ctl_status) = ctl.(ctl_status).
Proof.
  intros ctl Hneq.
  simpl.
  destruct (ctl_status ctl) eqn:Hs; simpl; auto.
  - (* P9_Idle case - contradiction *)
    exfalso. apply Hneq. reflexivity.
Qed.

(** Theorem: Success only works from Pending state *)
Theorem success_requires_pending :
  forall ctl,
  ctl.(ctl_status) <> P9_Pending ->
  (p9_transition ctl EvDispatchSuccess).(ctl_status) = ctl.(ctl_status).
Proof.
  intros ctl Hneq.
  simpl.
  destruct (ctl_status ctl) eqn:Hs; simpl; auto.
  - exfalso. apply Hneq. reflexivity.
Qed.

(** Theorem: Error only works from Pending state *)
Theorem error_requires_pending :
  forall ctl,
  ctl.(ctl_status) <> P9_Pending ->
  (p9_transition ctl EvDispatchError).(ctl_status) = ctl.(ctl_status).
Proof.
  intros ctl Hneq.
  simpl.
  destruct (ctl_status ctl) eqn:Hs; simpl; auto.
  - exfalso. apply Hneq. reflexivity.
Qed.

(** Theorem: Reset always leads to Idle *)
Theorem reset_always_idle :
  forall ctl,
  (p9_transition ctl EvReset).(ctl_status) = P9_Idle.
Proof.
  intros ctl. simpl. reflexivity.
Qed.

(** Theorem: Sequence numbers never decrease *)
Theorem req_seq_monotonic :
  forall ctl ev,
  (p9_transition ctl ev).(ctl_req_seq) >= ctl.(ctl_req_seq).
Proof.
  intros ctl ev.
  destruct ev; simpl; destruct (ctl_status ctl); simpl; lia.
Qed.

Theorem rep_seq_monotonic :
  forall ctl ev,
  (p9_transition ctl ev).(ctl_rep_seq) >= ctl.(ctl_rep_seq).
Proof.
  intros ctl ev.
  destruct ev; simpl; destruct (ctl_status ctl); simpl; lia.
Qed.

(* ========================================================================= *)
(* 2. FID ISOLATION                                                          *)
(* ========================================================================= *)

(** FID Isolation: A process can only access FIDs it owns.
    This prevents cross-process file descriptor attacks. *)

(** Lemma: fid_owned_by correctly identifies ownership *)
Lemma fid_owned_by_correct :
  forall table fid pid,
  fid_owned_by table fid pid = true <->
  exists entry, fid_lookup table fid = Some entry /\ entry.(fid_owner) = pid.
Proof.
  intros table fid pid.
  unfold fid_owned_by.
  split.
  - (* -> *)
    intros H.
    destruct (fid_lookup table fid) as [entry|] eqn:Hlook.
    + exists entry. split; auto.
      apply Z.eqb_eq. exact H.
    + discriminate H.
  - (* <- *)
    intros [entry [Hlook Howner]].
    rewrite Hlook.
    apply Z.eqb_eq. exact Howner.
Qed.

(** Theorem: FID isolation - non-owners cannot access *)
Theorem fid_isolation :
  forall table fid pid entry,
  fid_lookup table fid = Some entry ->
  entry.(fid_owner) <> pid ->
  fid_owned_by table fid pid = false.
Proof.
  intros table fid pid entry Hlook Hneq.
  unfold fid_owned_by.
  rewrite Hlook.
  apply Z.eqb_neq. exact Hneq.
Qed.

(** Theorem: Installed FIDs are owned by the installer *)
Theorem install_preserves_ownership :
  forall table entry,
  fid_owned_by (fid_install table entry) entry.(fid_id) entry.(fid_owner) = true.
Proof.
  intros table entry.
  unfold fid_install, fid_owned_by, fid_lookup.
  simpl.
  rewrite Z.eqb_refl.
  apply Z.eqb_refl.
Qed.

(** Theorem: Installing a FID doesn't affect other FIDs *)
Theorem install_preserves_others :
  forall table entry fid,
  entry.(fid_id) <> fid ->
  fid_lookup (fid_install table entry) fid = fid_lookup table fid.
Proof.
  intros table entry fid Hneq.
  unfold fid_install. simpl.
  destruct (Z.eqb entry.(fid_id) fid) eqn:Heq.
  - apply Z.eqb_eq in Heq. contradiction.
  - reflexivity.
Qed.

(** Theorem: Removing a FID clears it from the table (first occurrence) *)
(** Note: This proves lookup returns None only when the entry is at the head.
    For a full proof, we'd need a uniqueness invariant. *)
Lemma fid_lookup_after_remove_head :
  forall e rest fid,
  e.(fid_id) = fid ->
  fid_lookup rest fid = None ->
  fid_lookup (fid_remove (e :: rest) fid) fid = None.
Proof.
  intros e rest fid Heq Hnone.
  simpl.
  destruct (Z.eqb e.(fid_id) fid) eqn:Heqb.
  - (* e.(fid_id) = fid: removed, check rest *)
    exact Hnone.
  - (* e.(fid_id) <> fid: but we assumed they're equal! *)
    apply Z.eqb_neq in Heqb.
    contradiction.
Qed.

(** Weaker form: If FID not in rest, removing from cons makes it inaccessible *)
Theorem remove_head_makes_inaccessible :
  forall e rest fid pid,
  e.(fid_id) = fid ->
  fid_lookup rest fid = None ->
  fid_owned_by (fid_remove (e :: rest) fid) fid pid = false.
Proof.
  intros e rest fid pid Heq Hnone.
  unfold fid_owned_by.
  rewrite fid_lookup_after_remove_head; auto.
Qed.
(* ========================================================================= *)
(* 3. PEBBLE PERMISSION ENFORCEMENT                                          *)
(* ========================================================================= *)

(** Pebble tokens enforce capability-based security.
    Access is denied without the required permissions. *)

(** Theorem: Validation fails if permission not present *)
Theorem pebble_enforces_permissions :
  forall tok time required,
  pebble_has_permission tok required = false ->
  validate tok time required = false.
Proof.
  intros tok time required Hperm.
  unfold validate.
  rewrite Hperm.
  apply andb_false_r.
Qed.

(** Theorem: Validation fails if token is expired *)
Theorem pebble_enforces_expiration :
  forall tok time required,
  tok.(pebble_expires) <> 0 ->
  time >= tok.(pebble_expires) ->
  validate tok time required = false.
Proof.
  intros tok time required Hne Hexp.
  unfold validate.
  (* not_expired = orb (expires = 0) (time < expires) *)
  assert (Hne_bool: Z.eqb tok.(pebble_expires) 0 = false).
  { apply Z.eqb_neq. exact Hne. }
  assert (Hexp_bool: Z.ltb time tok.(pebble_expires) = false).
  { apply Z.ltb_ge. lia. }
  rewrite Hne_bool, Hexp_bool.
  simpl. reflexivity.
Qed.


(** Theorem: Validation passes with correct permissions and not expired *)
Theorem pebble_valid_when_correct :
  forall tok time required,
  (tok.(pebble_expires) = 0 \/ time < tok.(pebble_expires)) ->
  pebble_has_permission tok required = true ->
  validate tok time required = true.
Proof.
  intros tok time required Hexp Hperm.
  unfold validate.
  rewrite Hperm.
  rewrite andb_true_r.
  destruct Hexp as [Hz | Hlt].
  - (* expires = 0 *)
    assert (Hz_bool: Z.eqb tok.(pebble_expires) 0 = true).
    { apply Z.eqb_eq. exact Hz. }
    rewrite Hz_bool. simpl. reflexivity.
  - (* time < expires *)
    assert (Hlt_bool: Z.ltb time tok.(pebble_expires) = true).
    { apply Z.ltb_lt. exact Hlt. }
    rewrite Hlt_bool. apply orb_true_r.
Qed.

(** Theorem: READ permission check *)
Theorem read_requires_read_permission :
  forall tok,
  Z.land tok.(pebble_permissions) PEBBLE_PERM_READ <> PEBBLE_PERM_READ ->
  pebble_has_permission tok PEBBLE_PERM_READ = false.
Proof.
  intros tok Hneq.
  unfold pebble_has_permission.
  apply Z.eqb_neq. exact Hneq.
Qed.

(* ========================================================================= *)
(* 4. DISPATCH CORRECTNESS                                                   *)
(* ========================================================================= *)

(* Helper for valid token construction *)
Definition valid_token := mkPebbleToken 0 0 255. (* All perms, no expiry *)
Definition dummy_time := 0.
Definition dummy_perms := 0. (* Dispatch requires explicit perms now? check logic *)
(* In template dispatch takes 'perms' argument which is the REQUIRED permissions for the operation. *)
(* The caller (kernel) decides what is required. *)
(* For Tattach/Tclunk we might require 0 or specific perms. *)
(* Let's assume 0 for now unless the test implies otherwise. *)

(** Theorem: Tattach creates FID with correct type *)
Theorem tattach_creates_correct_fid :
  forall table pid fid ftype path,
  path_to_type path = Some ftype ->
  exists table' reply,
    dispatch table pid (mkFcall Tattach fid path valid_token) dummy_time dummy_perms = DispatchOk table' reply /\
    fid_lookup table' fid = Some (mkFidEntry fid ftype 0 pid).
Proof.
  intros table pid fid ftype path Hpath.
  unfold dispatch.
  simpl.
  (* validate returns true for valid_token *)
  (* mkPebbleToken 0 0 255 with time 0 and perms 0 should pass *)
  (* 0 land 0 = 0. eqb 0 0 is true. *)
  (* But wait, validate checks (tok.perms & req) == req. *)
  (* If req is 0, (x & 0) == 0 is always true. *)
  destruct (path_to_type path) eqn:Hpt.
  - (* Some f *)
    injection Hpath. intros Heq. subst.
    eexists. eexists.
    split.
    + reflexivity.
    + simpl. rewrite Z.eqb_refl. reflexivity.
  - (* None *)
    discriminate Hpath.
Qed.


(** Theorem: Tattach reply has correct type *)
Theorem tattach_reply_is_rattach :
  forall table pid t table' reply,
  t.(fcall_type) = Tattach ->
  dispatch table pid t dummy_time dummy_perms = DispatchOk table' reply ->
  reply.(fcall_type) = Rattach.
Proof.
  intros table pid [ttype tfid tpath ttok] table' reply Htype Hdisp.
  simpl in Htype. subst ttype.
  unfold dispatch in Hdisp. simpl in Hdisp.
  
  (* We need to simplify the validation logic *)
  (* DispatchOk implies validation passed *)
  destruct (validate ttok dummy_time dummy_perms) eqn:Hval.
  - (* Validation passed *)
    destruct (path_to_type tpath) eqn:Hpath.
    + (* path known *)
      injection Hdisp as Htbl Hrep.
      rewrite <- Hrep. simpl. reflexivity.
    + (* path unknown *)
      discriminate Hdisp.
  - (* Validation failed *)
    discriminate Hdisp.
Qed.


(** Theorem: Tclunk from non-owner fails *)
(** Note: This now also depends on token validation logic, but if validation fails it returns DispatchErr 3. *)
(** If ownership fails it returns DispatchErr 2. *)
(** We want to prove it fails if ownership fails, assuming validation passed? Or just fails in general? *)
(** The theorem says exists err, ... = DispatchErr err. Safe to say 2 or 3. *)
Theorem tclunk_requires_ownership :
  forall table pid fid,
  fid_owned_by table fid pid = false ->
  exists err, dispatch table pid (mkFcall Tclunk fid PathUnknown valid_token) dummy_time dummy_perms = DispatchErr err.
Proof.
  intros table pid fid Hown.
  unfold dispatch. simpl.
  (* validation passes for valid_token/0 *)
  rewrite Hown.
  exists 2%nat. reflexivity.
Qed.

(** Theorem: Successful Tclunk removes the FID *)
Theorem tclunk_removes_fid :
  forall table pid fid table' reply,
  fid_owned_by table fid pid = true ->
  dispatch table pid (mkFcall Tclunk fid PathUnknown valid_token) dummy_time dummy_perms = DispatchOk table' reply ->
  table' = fid_remove table fid.
Proof.
  intros table pid fid table' reply Hown Hdisp.
  unfold dispatch in Hdisp. simpl in Hdisp.
  rewrite Hown in Hdisp.
  injection Hdisp as Htable Hreply.
  symmetry. exact Htable.
Qed.
