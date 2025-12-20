(** Blind Ledger - Phase 1: Cryptographic Primitives (Symbolic/Dolev-Yao)
    Replaces axioms with inductive types to verify logic without assuming concrete SHA256 properties.
    This guarantees "Uniqueness by Construction". *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* BASIC TYPES *)
(* ========================================================================= *)

Definition PAddr := Z.
Definition Len := Z.
Definition Proc := Z.
Definition Permissions := Z.

(** 
 * Secret Key 
 * Modeled as a simple wrapper around an integer (ID)
 *)
Inductive Secret := mkSecret (id : Z).

(* ========================================================================= *)
(* SYMBOLIC HASH TYPES (Inductive) *)
(* ========================================================================= *)

(**
 * Instead of (Bytes -> Bytes), we define hashes as constructors holding their pre-image.
 * This makes them injective by definition of Inductive types in Coq.
 *)

(** LeafHash = Hash(PA, Len) *)
Inductive LeafHash := 
  | Hash_Leaf (pa : PAddr) (len : Len).

(** ProcessHash = HMAC(Secret, LeafHash, Owner) *)
Inductive ProcessHash :=
  | Hash_Process (s : Secret) (lh : LeafHash) (o : Proc).

(** CapHash = Hash(ProcessHash, LeafHash) *)
Inductive CapHash :=
  | Hash_Cap (ph : ProcessHash) (lh : LeafHash).

(**
 * UserCapability structure
 *)
Record UserCapability := mkUserCapability {
  cap_hash : CapHash;
  cap_size : Z;
  cap_type : Z;
  cap_perms : Z;
}.

(* ========================================================================= *)
(* COMPUTATION FUNCTIONS (Constructors) *)
(* ========================================================================= *)

(** 
 * These functions now just wrap the data in the constructor.
 * This corresponds to the Dolev-Yao model where the attacker/verifier sees the "term".
 *)

Definition compute_leaf_hash (pa : PAddr) (len : Len) : LeafHash :=
  Hash_Leaf pa len.

Definition compute_process_hash (s : Secret) (lh : LeafHash) (o : Proc) : ProcessHash :=
  Hash_Process s lh o.

Definition compute_cap_hash (ph : ProcessHash) (lh : LeafHash) : CapHash :=
  Hash_Cap ph lh.

Definition mint_capability_hash (pa : PAddr) (len : Len) (s : Secret) (o : Proc) : CapHash :=
  let lh := compute_leaf_hash pa len in
  let ph := compute_process_hash s lh o in
  compute_cap_hash ph lh.

(* ========================================================================= *)
(* PROOFS (No Axioms!) *)
(* ========================================================================= *)

(**
 * Theorem: Usefulness of Inductive Types
 * Coq automatically generates injection principles for inductive types.
 * We just need to apply them.
 *)

Theorem cap_hash_uniqueness : forall pa1 len1 s1 o1 pa2 len2 s2 o2,
  mint_capability_hash pa1 len1 s1 o1 = mint_capability_hash pa2 len2 s2 o2 ->
  pa1 = pa2 /\ len1 = len2 /\ s1 = s2 /\ o1 = o2.
Proof.
  intros.
  unfold mint_capability_hash, compute_cap_hash, compute_process_hash, compute_leaf_hash in H.

  (* Exhaustively invert all our hash equalities *)
  repeat match goal with
  | [ H : Hash_Cap _ _ = Hash_Cap _ _ |- _ ] => inversion H; subst; clear H
  | [ H : Hash_Process _ _ _ = Hash_Process _ _ _ |- _ ] => inversion H; subst; clear H
  | [ H : Hash_Leaf _ _ = Hash_Leaf _ _ |- _ ] => inversion H; subst; clear H
  end.
  
  repeat split; reflexivity.
Qed.

(**
 * Helper for decidability of equality (needed for Maps)
 *)
Definition Secret_eq_dec : forall x y : Secret, {x = y} + {x <> y}.
Proof. decide equality; apply Z.eq_dec. Defined.

Definition LeafHash_eq_dec : forall x y : LeafHash, {x = y} + {x <> y}.
Proof. decide equality; apply Z.eq_dec. Defined.

Definition ProcessHash_eq_dec : forall x y : ProcessHash, {x = y} + {x <> y}.
Proof. 
  decide equality. 
  apply Z.eq_dec.
  apply LeafHash_eq_dec.
  apply Secret_eq_dec.
Defined.

Definition CapHash_eq_dec : forall x y : CapHash, {x = y} + {x <> y}.
Proof.
  decide equality.
  apply LeafHash_eq_dec.
  apply ProcessHash_eq_dec.
Defined.
