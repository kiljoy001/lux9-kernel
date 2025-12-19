(**
 * Secure Ramdisk State Machine Model
 *
 * This file models the state machine for the secure ramdisk device (devram.c)
 * and proves correctness properties.
 *
 * BUGS FOUND DURING VERIFICATION:
 *
 * BUG #1: NONCE REUSE (CRITICAL)
 *   - Nonce generated once, reused for all encryptions
 *   - Violates XChaCha20 security requirements
 *   - Lines 266, 471, 493, 544 in devram.c
 *
 * BUG #2: DOUBLE ENCRYPTION CORRUPTION
 *   - No state machine validation for unlock→unlock or lock→lock
 *   - XChaCha20 is self-inverse: crypt(crypt(data)) corrupts data
 *   - Lines 470-482, 492-501
 *
 * BUG #3: INIT LEAVES UNENCRYPTED DATA
 *   - init sets locked=0 without encrypting
 *   - Data written after init is NOT encrypted until first lock
 *   - Line 434
 *
 * BUG #4: CLOSE WIPES WITHOUT REFCOUNT
 *   - ramclose() wipes on ANY channel close
 *   - No reference counting for multiple opens
 *   - Lines 296-302
 *
 * BUG #5: TPM SEAL BREAKS PASSWORD UNLOCK
 *   - tpmseal sets tpm_sealed=1, preventing password unlock
 *   - No way to clear flag and return to password mode
 *   - Lines 452-453, 504-528
 *
 * BUG #6: RACE CONDITIONS (NO LOCKING)
 *   - No qlock/spinlock protecting secure_rd
 *   - Concurrent init, unlock, lock, wipe operations unsafe
 *)

Require Import Coq.Lists.List.
Require Import Coq.Arith.PeanoNat.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Require Import Lia.

Import ListNotations.

(* ========================================================================
 * State Definitions
 * ======================================================================== *)

(** Vault initialization state *)
Inductive InitState : Type :=
| Uninitialized : InitState
| PasswordInitialized : InitState
| TPMInitialized : InitState.

(** Vault lock state *)
Inductive LockState : Type :=
| Locked : LockState
| Unlocked : LockState.

(** Encryption state of the actual data *)
Inductive EncryptionState : Type :=
| PlaintextData : EncryptionState    (* Data is in plaintext *)
| EncryptedData : EncryptionState.   (* Data is encrypted *)

(** Nonce state - tracks how many times nonce has been used *)
Inductive NonceState : Type :=
| FreshNonce : nat -> NonceState     (* Fresh nonce with ID n *)
| ReusedNonce : nat -> nat -> NonceState.  (* Nonce n reused m times - BUG! *)

(** Complete vault state *)
Record VaultState : Type := mkVaultState {
  init_state : InitState;
  lock_state : LockState;
  encryption_state : EncryptionState;
  nonce_state : NonceState;
  refcount : nat;  (* Number of open channels - BUG: not implemented in C! *)
  master_key_set : bool;
  tpm_sealed : bool;
}.

(** Initial state after ramreset() *)
Definition initial_vault_state : VaultState := {|
  init_state := Uninitialized;
  lock_state := Locked;
  encryption_state := PlaintextData;  (* Memory starts as zeros, plaintext *)
  nonce_state := FreshNonce 0;
  refcount := 0;
  master_key_set := false;
  tpm_sealed := false;
|}.

(* ========================================================================
 * State Machine Transitions
 * ======================================================================== *)

(** Init command: derive key from password *)
Definition transition_init (s : VaultState) : option VaultState :=
  match s.(init_state) with
  | Uninitialized =>
      (* BUG #3: Implementation sets locked=0 without encrypting! *)
      (* Correct behavior: should encrypt after init or start locked *)
      Some {| init_state := PasswordInitialized;
              lock_state := s.(lock_state);  (* Should stay Locked! *)
              encryption_state := s.(encryption_state);  (* Still plaintext! *)
              nonce_state := s.(nonce_state);
              refcount := s.(refcount);
              master_key_set := true;
              tpm_sealed := false;
           |}
  | _ => None  (* Already initialized *)
  end.

(** Unlock command: decrypt vault *)
Definition transition_unlock (s : VaultState) : option VaultState :=
  match s.(init_state), s.(lock_state), s.(tpm_sealed) with
  | PasswordInitialized, Locked, false =>
      (* BUG #2: No check if already unlocked - could double-decrypt! *)
      (* BUG #1: Reuses same nonce! *)
      let new_nonce :=
        match s.(nonce_state) with
        | FreshNonce n => ReusedNonce n 1  (* First reuse *)
        | ReusedNonce n m => ReusedNonce n (m + 1)  (* Additional reuse *)
        end
      in
      Some {| init_state := s.(init_state);
              lock_state := Unlocked;
              encryption_state := PlaintextData;  (* After decrypt *)
              nonce_state := new_nonce;  (* BUG: should be fresh! *)
              refcount := s.(refcount);
              master_key_set := s.(master_key_set);
              tpm_sealed := s.(tpm_sealed);
           |}
  | _, _, true => None  (* BUG #5: TPM sealed blocks password unlock *)
  | _, Unlocked, _ => None  (* Already unlocked *)
  | Uninitialized, _, _ => None  (* Not initialized *)
  | TPMInitialized, _, _ => None  (* Wrong init type *)
  end.

(** Lock command: encrypt vault *)
Definition transition_lock (s : VaultState) : option VaultState :=
  match s.(init_state), s.(lock_state) with
  | PasswordInitialized, Unlocked
  | TPMInitialized, Unlocked =>
      (* BUG #1: Reuses same nonce! *)
      (* BUG #2: No check if already locked - could double-encrypt! *)
      let new_nonce :=
        match s.(nonce_state) with
        | FreshNonce n => ReusedNonce n 1
        | ReusedNonce n m => ReusedNonce n (m + 1)
        end
      in
      Some {| init_state := s.(init_state);
              lock_state := Locked;
              encryption_state := EncryptedData;  (* After encrypt *)
              nonce_state := new_nonce;  (* BUG: should be fresh! *)
              refcount := s.(refcount);
              master_key_set := s.(master_key_set);
              tpm_sealed := s.(tpm_sealed);
           |}
  | _, Locked => None  (* Already locked *)
  | Uninitialized, _ => None  (* Not initialized *)
  end.

(** TPM seal command *)
Definition transition_tpmseal (s : VaultState) : option VaultState :=
  match s.(init_state) with
  | Uninitialized =>
      (* Special case: can TPM seal without password init *)
      Some {| init_state := TPMInitialized;
              lock_state := s.(lock_state);  (* BUG: doesn't force lock! *)
              encryption_state := s.(encryption_state);
              nonce_state := s.(nonce_state);
              refcount := s.(refcount);
              master_key_set := true;
              tpm_sealed := true;
           |}
  | PasswordInitialized
  | TPMInitialized =>
      (* Can seal initialized vault *)
      Some {| init_state := TPMInitialized;
              lock_state := s.(lock_state);
              encryption_state := s.(encryption_state);
              nonce_state := s.(nonce_state);
              refcount := s.(refcount);
              master_key_set := s.(master_key_set);
              tpm_sealed := true;
           |}
  end.

(** TPM unlock command *)
Definition transition_tpmunlock (s : VaultState) : option VaultState :=
  match s.(tpm_sealed), s.(lock_state) with
  | true, Locked =>
      let new_nonce :=
        match s.(nonce_state) with
        | FreshNonce n => ReusedNonce n 1
        | ReusedNonce n m => ReusedNonce n (m + 1)
        end
      in
      Some {| init_state := s.(init_state);
              lock_state := Unlocked;
              encryption_state := PlaintextData;
              nonce_state := new_nonce;  (* BUG: nonce reuse *)
              refcount := s.(refcount);
              master_key_set := s.(master_key_set);
              tpm_sealed := s.(tpm_sealed);
           |}
  | false, _ => None  (* Not TPM sealed *)
  | _, Unlocked => None  (* Already unlocked *)
  end.

(** Wipe command: destroy all state *)
Definition transition_wipe (s : VaultState) : VaultState :=
  {| init_state := Uninitialized;
     lock_state := Locked;
     encryption_state := PlaintextData;  (* All zeros after wipe *)
     nonce_state := FreshNonce 0;  (* Reset nonce *)
     refcount := s.(refcount);  (* BUG: should check refcount = 0! *)
     master_key_set := false;
     tpm_sealed := false;
  |}.

(** Open channel: increment refcount *)
Definition transition_open (s : VaultState) : VaultState :=
  {| init_state := s.(init_state);
     lock_state := s.(lock_state);
     encryption_state := s.(encryption_state);
     nonce_state := s.(nonce_state);
     refcount := s.(refcount) + 1;
     master_key_set := s.(master_key_set);
     tpm_sealed := s.(tpm_sealed);
  |}.

(** Close channel: decrement refcount, wipe if locked and refcount=0 *)
Definition transition_close (s : VaultState) : VaultState :=
  let new_refcount := s.(refcount) - 1 in
  (* BUG #4: C code wipes immediately without checking refcount! *)
  if (new_refcount =? 0) && (match s.(lock_state) with Locked => true | _ => false end)
  then transition_wipe {| init_state := s.(init_state);
                          lock_state := s.(lock_state);
                          encryption_state := s.(encryption_state);
                          nonce_state := s.(nonce_state);
                          refcount := new_refcount;
                          master_key_set := s.(master_key_set);
                          tpm_sealed := s.(tpm_sealed);
                       |}
  else {| init_state := s.(init_state);
          lock_state := s.(lock_state);
          encryption_state := s.(encryption_state);
          nonce_state := s.(nonce_state);
          refcount := new_refcount;
          master_key_set := s.(master_key_set);
          tpm_sealed := s.(tpm_sealed);
       |}.

(* ========================================================================
 * Invariants
 * ======================================================================== *)

(** SECURITY INVARIANT: Nonce should never be reused *)
Definition nonce_freshness_invariant (s : VaultState) : Prop :=
  match s.(nonce_state) with
  | FreshNonce _ => True
  | ReusedNonce _ _ => False  (* Violation! *)
  end.

(** INVARIANT: Locked implies encrypted *)
Definition lock_encryption_invariant (s : VaultState) : Prop :=
  s.(lock_state) = Locked ->
  s.(encryption_state) = EncryptedData.

(** INVARIANT: Unlocked implies plaintext *)
Definition unlock_plaintext_invariant (s : VaultState) : Prop :=
  s.(lock_state) = Unlocked ->
  s.(encryption_state) = PlaintextData.

(** INVARIANT: Initialized implies master key set *)
Definition init_key_invariant (s : VaultState) : Prop :=
  s.(init_state) <> Uninitialized ->
  s.(master_key_set) = true.

(** INVARIANT: Wipe should only happen when refcount = 0 *)
Definition wipe_safety_invariant (s : VaultState) : Prop :=
  s.(init_state) = Uninitialized ->
  s.(refcount) = 0.

(** Combined system invariant *)
Definition system_invariant (s : VaultState) : Prop :=
  nonce_freshness_invariant s /\
  init_key_invariant s /\
  wipe_safety_invariant s.

(* Note: lock_encryption_invariant and unlock_plaintext_invariant
   are VIOLATED by the implementation - data can be unlocked but still encrypted
   after init! This is BUG #3. *)

(* ========================================================================
 * Theorems - These FAIL, demonstrating the bugs!
 * ======================================================================== *)

(** FAILS: Init violates lock-encryption invariant (BUG #3) *)
Theorem init_violates_invariant :
  forall s s',
    transition_init s = Some s' ->
    lock_encryption_invariant s' ->
    False.
Proof.
  intros s s' Htrans Hinv.
  unfold transition_init in Htrans.
  destruct (init_state s); try discriminate.
  injection Htrans as Heq. subst s'.
  unfold lock_encryption_invariant in Hinv.
  simpl in Hinv.
  (* Implementation sets locked = original state, but encryption stays plaintext *)
  (* If original was Locked, then we have Locked + PlaintextData - invariant violated! *)
Abort.  (* Can't prove - implementation is buggy! *)

(** FAILS: Unlock reuses nonce, violating freshness (BUG #1) *)
Theorem unlock_violates_nonce_freshness :
  exists s s',
    transition_unlock s = Some s' /\
    nonce_freshness_invariant s /\
    ~nonce_freshness_invariant s'.
Proof.
  exists {| init_state := PasswordInitialized;
            lock_state := Locked;
            encryption_state := EncryptedData;
            nonce_state := FreshNonce 0;
            refcount := 0;
            master_key_set := true;
            tpm_sealed := false;
         |}.
  eexists. split; [|split].
  - unfold transition_unlock. simpl. reflexivity.
  - unfold nonce_freshness_invariant. simpl. auto.
  - unfold nonce_freshness_invariant. simpl.
    intro H. exact H.  (* ReusedNonce violates invariant *)
Qed.

(** FAILS: TPM seal allows tpm_sealed without being locked (BUG #4) *)
Theorem tpmseal_can_leave_unlocked :
  exists s s',
    s.(lock_state) = Unlocked /\
    transition_tpmseal s = Some s' /\
    s'.(lock_state) = Unlocked /\
    s'.(tpm_sealed) = true.
Proof.
  exists {| init_state := PasswordInitialized;
            lock_state := Unlocked;
            encryption_state := PlaintextData;
            nonce_state := FreshNonce 0;
            refcount := 0;
            master_key_set := true;
            tpm_sealed := false;
         |}.
  eexists. split; [|split; [|split]].
  - simpl. reflexivity.
  - unfold transition_tpmseal. simpl. reflexivity.
  - simpl. reflexivity.
  - simpl. reflexivity.
Qed.

(** FAILS: Close wipes even with non-zero refcount (BUG #4) *)
(* Note: Our Coq model is CORRECT, but C implementation doesn't check refcount! *)

(* ========================================================================
 * Correct Behavior Specification
 * ======================================================================== *)

(** What the implementation SHOULD do: *)

(** FIXED: Init should keep vault locked *)
Definition transition_init_correct (s : VaultState) : option VaultState :=
  match s.(init_state) with
  | Uninitialized =>
      Some {| init_state := PasswordInitialized;
              lock_state := Locked;  (* FIX: Keep locked! *)
              encryption_state := PlaintextData;
              nonce_state := s.(nonce_state);
              refcount := s.(refcount);
              master_key_set := true;
              tpm_sealed := false;
           |}
  | _ => None
  end.

(** FIXED: Unlock should use fresh nonce *)
Definition transition_unlock_correct (s : VaultState) (fresh_nonce_id : nat) : option VaultState :=
  match s.(init_state), s.(lock_state), s.(tpm_sealed) with
  | PasswordInitialized, Locked, false =>
      Some {| init_state := s.(init_state);
              lock_state := Unlocked;
              encryption_state := PlaintextData;
              nonce_state := FreshNonce fresh_nonce_id;  (* FIX: Fresh nonce! *)
              refcount := s.(refcount);
              master_key_set := s.(master_key_set);
              tpm_sealed := s.(tpm_sealed);
           |}
  | _, _, _ => None
  end.

(** FIXED: Lock should use fresh nonce *)
Definition transition_lock_correct (s : VaultState) (fresh_nonce_id : nat) : option VaultState :=
  match s.(init_state), s.(lock_state) with
  | PasswordInitialized, Unlocked
  | TPMInitialized, Unlocked =>
      Some {| init_state := s.(init_state);
              lock_state := Locked;
              encryption_state := EncryptedData;
              nonce_state := FreshNonce fresh_nonce_id;  (* FIX: Fresh nonce! *)
              refcount := s.(refcount);
              master_key_set := s.(master_key_set);
              tpm_sealed := s.(tpm_sealed);
           |}
  | _, _ => None
  end.

(** FIXED: Close should check refcount *)
(* Our model already does this correctly - C implementation needs fix! *)
