Require Import Coq.Strings.String.
Require Import Coq.Bool.Bool.
Require Import vault_model.

(* ================================================================= *)
(* Invariants *)
(* ================================================================= *)

(* 
  Invariant: ValidVault
  Ensures that if a vault is not initialized, it must be in a secure, wiped state.
  - Locked = true
  - Data = WipedData
  - Key = WipedData
  
  If it IS initialized, we assume the crypto logic holds (properties.v covering that).
*)
Definition ValidVault (v : VaultState) : Prop :=
  v.(v_initialized) = false -> 
    v.(v_locked) = true /\ 
    v.(v_data) = WipedData /\ 
    v.(v_key) = WipedData.

(* ================================================================= *)
(* Preservation Theorems *)
(* ================================================================= *)

(* 1. Initial State satisfies invariant *)
Theorem init_state_valid : ValidVault init_vault_state.
Proof.
  unfold ValidVault, init_vault_state.
  simpl.
  intro H.
  split; [reflexivity | split; reflexivity].
Qed.

(* 2. Init operation preserves invariant *)
(* 
   Note: If init succeeds, initialized becomes true, making the implication trivial.
   If it fails, state is unchanged.
*)
Theorem init_preserves_valid : forall v pw salt nonce v',
  ValidVault v ->
  vault_init v pw salt nonce = Some v' ->
  ValidVault v'.
Proof.
  intros v pw salt nonce v' Hvalid Hinit.
  unfold vault_init in Hinit.
  destruct v.(v_initialized).
  - (* Init on initialized vault fails *)
    discriminate.
  - (* Init succeeds *)
    injection Hinit as Heq. subst.
    unfold ValidVault.
    simpl.
    (* v_initialized is true, so premise is False -> True *)
    intro Hfalse.
    discriminate.
Qed.

(* 3. Write operation preserves invariant *)
Theorem write_preserves_valid : forall v d v',
  ValidVault v ->
  vault_write v d = Some v' ->
  ValidVault v'.
Proof.
  intros v d v' Hvalid Hwrite.
  unfold vault_write in Hwrite.
  destruct (negb v.(v_locked) && v.(v_initialized)) eqn:Hcond.
  - (* Write succeeds *)
    injection Hwrite as Heq. subst.
    unfold ValidVault.
    simpl.
    intro Hnot_init.
    (* From Hcond we know initialized is true *)
    apply andb_true_iff in Hcond. destruct Hcond as [_ Hinit].
    rewrite Hnot_init in Hinit.
    discriminate.
  - (* Write fails *)
    discriminate.
Qed.

(* 4. Lock operation preserves invariant *)
Theorem lock_preserves_valid : forall v v',
  ValidVault v ->
  vault_lock v = Some v' ->
  ValidVault v'.
Proof.
  intros v v' Hvalid Hlock.
  unfold vault_lock in Hlock.
  destruct (negb v.(v_locked) && v.(v_initialized)) eqn:Hcond.
  - (* Lock succeeds *)
    injection Hlock as Heq. subst.
    unfold ValidVault.
    simpl.
    (* initialized is true *)
    apply andb_true_iff in Hcond. destruct Hcond as [_ Hinit].
    intro Hnot_init.
    rewrite Hnot_init in Hinit.
    discriminate.
  - discriminate.
Qed.

(* 5. Unlock operation preserves invariant *)
Theorem unlock_preserves_valid : forall v pw v',
  ValidVault v ->
  vault_unlock v pw = Some v' ->
  ValidVault v'.
Proof.
  intros v pw v' Hvalid Hunlock.
  unfold vault_unlock in Hunlock.
  destruct (v.(v_locked) && v.(v_initialized)) eqn:Hcond.
  - (* Check password *)
    destruct (strings_eq (Argon2id pw (v_salt v)) (v_key v)).
    + (* Success *)
      injection Hunlock as Heq. subst.
      unfold ValidVault. simpl.
      apply andb_true_iff in Hcond. destruct Hcond as [_ Hinit].
      intro Hnot_init.
      rewrite Hnot_init in Hinit.
      discriminate.
    + (* Wrong password *)
      discriminate.
  - (* Not locked or not initialized *)
    discriminate.
Qed.

(* 6. Wipe operation preserves invariant *)
Theorem wipe_preserves_valid : forall v,
  ValidVault v ->
  ValidVault (vault_wipe v).
Proof.
  intros v Hvalid.
  unfold ValidVault, vault_wipe.
  simpl.
  intro H.
  split.
  - reflexivity.
  - split; [apply secure_wipe_result | apply secure_wipe_result].
Qed.
