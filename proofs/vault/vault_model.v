Require Import Coq.Lists.List.
Require Import Coq.Strings.String.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Import ListNotations.
Local Open Scope string_scope.

(* ================================================================= *)
(* Abstract Crypto Primitives Model *)
(* ================================================================= *)

(* We treat data, keys, salts, and nonces as strings for simplicity in this abstract model *)
Definition Data := string.
Definition Key := string.
Definition Salt := string.
Definition Nonce := string.
Definition Password := string.

(* 
   We assume ideal cryptographic functions.
   In a real implementation, these map to Monocypher primitives.
*)

Parameter Argon2id : Password -> Salt -> Key.
Parameter XChaCha20_Encrypt : Data -> Key -> Nonce -> Data.
Parameter XChaCha20_Decrypt : Data -> Key -> Nonce -> Data.
Parameter SecureWipe : Data -> Data.

(* Axioms defining the properties of these primitives *)

(* 1. Decryption mirrors Encryption *)
Axiom encrypt_decrypt_inverse : forall (d : Data) (k : Key) (n : Nonce),
  XChaCha20_Decrypt (XChaCha20_Encrypt d k n) k n = d.

(* 2. Wiping produces "zeroed" data (represented as empty string for abstract model logic, 
      or a specific "wiped" constant) *)
Definition WipedData : Data := "".
Axiom secure_wipe_result : forall (d : Data),
  SecureWipe d = WipedData.

(* ================================================================= *)
(* Vault State Machine *)
(* ================================================================= *)

Record VaultState : Type := mkVault {
  v_data : Data;
  v_key : Key;
  v_salt : Salt;
  v_nonce : Nonce;
  v_locked : bool;
  v_initialized : bool;
}.

(* Initial state of the vault *)
Definition init_vault_state : VaultState := {|
  v_data := WipedData;
  v_key := "";
  v_salt := "";
  v_nonce := "";
  v_locked := true;
  v_initialized := false
|}.

(* Helper for equality checking *)
Parameter strings_eq : string -> string -> bool.
Axiom strings_eq_correct : forall s1 s2, strings_eq s1 s2 = true <-> s1 = s2.

(* Operations *)

(* Initialize Vault *)
Definition vault_init (v : VaultState) (pw : Password) (salt : Salt) (nonce : Nonce) : option VaultState :=
  if v.(v_initialized) then None
  else Some {| 
    v_data := WipedData; (* Starts empty *)
    v_key := Argon2id pw salt;
    v_salt := salt;
    v_nonce := nonce;
    v_locked := false; (* Unlocked on init *)
    v_initialized := true
  |}.

(* Write Data *)
Definition vault_write (v : VaultState) (d : Data) : option VaultState :=
  if negb v.(v_locked) && v.(v_initialized) then
    Some {|
      v_data := d;
      v_key := v.(v_key);
      v_salt := v.(v_salt);
      v_nonce := v.(v_nonce);
      v_locked := v.(v_locked);
      v_initialized := v.(v_initialized)
    |}
  else None.

(* Read Data *)
Definition vault_read (v : VaultState) : option Data :=
  if negb v.(v_locked) && v.(v_initialized) then Some v.(v_data)
  else if v.(v_locked) && v.(v_initialized) then 
      (* If locked, the data in memory is encrypted *)
      Some v.(v_data) 
  else None.

(* Lock Vault *)
Definition vault_lock (v : VaultState) : option VaultState :=
  if negb v.(v_locked) && v.(v_initialized) then
    Some {|
      v_data := XChaCha20_Encrypt v.(v_data) v.(v_key) v.(v_nonce);
      v_key := v.(v_key); (* In C code, key remains in memory until wipe *)
      v_salt := v.(v_salt);
      v_nonce := v.(v_nonce);
      v_locked := true;
      v_initialized := v.(v_initialized)
    |}
  else None.

(* Unlock Vault *)
Definition vault_unlock (v : VaultState) (pw : Password) : option VaultState :=
  if v.(v_locked) && v.(v_initialized) then
    let derived_key := Argon2id pw v.(v_salt) in
    if strings_eq derived_key v.(v_key) then
      Some {|
        v_data := XChaCha20_Decrypt v.(v_data) derived_key v.(v_nonce);
        v_key := v.(v_key);
        v_salt := v.(v_salt);
        v_nonce := v.(v_nonce);
        v_locked := false;
        v_initialized := v.(v_initialized)
      |}
    else None (* Wrong password *)
  else None.

(* Wipe Vault *)
Definition vault_wipe (v : VaultState) : VaultState := {|
  v_data := SecureWipe v.(v_data);
  v_key := SecureWipe v.(v_key);
  v_salt := SecureWipe v.(v_salt); (* Salt usually not secret, but wipe everything *)
  v_nonce := "";
  v_locked := true;
  v_initialized := false
|}.
