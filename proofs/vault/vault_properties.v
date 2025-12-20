Require Import Coq.Strings.String.
Require Import Coq.Bool.Bool.
Require Import vault_model.

(* ================================================================= *)
(* Security Properties *)
(* ================================================================= *)

(* 
  Theorem: Integrity Flow
  Initialize -> Write(d) -> Lock -> Unlock(pw) -> Read = d
*)
Theorem integrity_flow : forall v pw salt nonce d v_init v_written v_enc v_dec,
  vault_init v pw salt nonce = Some v_init ->
  vault_write v_init d = Some v_written ->
  vault_lock v_written = Some v_enc ->
  vault_unlock v_enc pw = Some v_dec ->
  v_dec.(v_data) = d.
Proof.
  intros v pw salt nonce d v_init v_written v_enc v_dec.
  intros Hinit Hwrite Hlock Hunlock.
  
  (* Unfold definitions to step through state changes *)
  unfold vault_init in Hinit.
  destruct v.(v_initialized); [discriminate | injection Hinit as Hinit_eq; subst].
  
  unfold vault_write in Hwrite.
  simpl in Hwrite.
  injection Hwrite as Hwrite_eq; subst.
  
  unfold vault_lock in Hlock.
  simpl in Hlock.
  injection Hlock as Hlock_eq; subst.
  
  unfold vault_unlock in Hunlock.
  simpl in Hunlock.
  (* The derived key matches because we used the same pw and salt *)
  destruct (strings_eq (Argon2id pw salt) (Argon2id pw salt)) eqn:Heq_key.
  - (* True case: proceed *)
    injection Hunlock as Hunlock_eq; subst.
    simpl.
    
    (* Apply the decryption inverse axiom *)
    rewrite encrypt_decrypt_inverse.
    reflexivity.
  - (* False case: contradiction *)
    assert (H_refl: Argon2id pw salt = Argon2id pw salt) by reflexivity.
    apply strings_eq_correct in H_refl.
    rewrite H_refl in Heq_key.
    discriminate.
Qed.

(*
  Theorem: Authentication Failure
  If we try to unlock with a wrong password, it fails (returns None).
  Assumption: Wrong password derives a different key.
*)
Theorem authentication_failure : forall v pw correct_pw,
  v.(v_locked) = true ->
  v.(v_initialized) = true ->
  v.(v_key) = Argon2id correct_pw v.(v_salt) ->
  Argon2id pw v.(v_salt) <> Argon2id correct_pw v.(v_salt) ->
  vault_unlock v pw = None.
Proof.
  intros v pw correct_pw Hlocked Hinit Hkey Hdiff.
  unfold vault_unlock.
  rewrite Hlocked.
  rewrite Hinit.
  simpl.
  
  (* Substitute the stored key *)
  rewrite Hkey.
  
  (* Now we check strings_eq *)
  destruct (strings_eq (Argon2id pw v.(v_salt)) (Argon2id correct_pw v.(v_salt))) eqn:Heq.
  - (* Case where they are equal *)
    apply strings_eq_correct in Heq.
    contradiction. (* We assumed they are different *)
  - (* Case where they are not equal, which is what we want *)
    reflexivity.
Qed.

(*
  Theorem: Confidentiality (Data in locked state is encrypted)
*)
Theorem confidentiality : forall v v_enc,
  vault_lock v = Some v_enc ->
  v_enc.(v_data) = XChaCha20_Encrypt v.(v_data) v.(v_key) v.(v_nonce).
Proof.
  intros v v_enc Hlock.
  unfold vault_lock in Hlock.
  destruct (negb v.(v_locked) && v.(v_initialized)); [| discriminate].
  injection Hlock as Hlock_eq; subst.
  simpl.
  reflexivity.
Qed.

(*
  Theorem: Wipe Safety
*)
Theorem wipe_safety : forall v,
  let v_wiped := vault_wipe v in
  v_wiped.(v_data) = WipedData /\
  v_wiped.(v_key) = WipedData.
Proof.
  intros v v_wiped.
  unfold vault_wipe.
  simpl.
  split.
  - rewrite secure_wipe_result. reflexivity.
  - rewrite secure_wipe_result. reflexivity.
  (* v_salt is also wiped *)
Qed.
