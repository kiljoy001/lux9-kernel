(** Blind Ledger - Phase 5: Implementation Refinement (Symbolic)
    Refines the Abstract State (Map) to a Concrete Representation (modeled as FMap/RBTree)
    and proves correctness of the implementation logic. *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Coq.FSets.FMapList.
Require Import Coq.FSets.FMapFacts.
Require Import Coq.Structures.Orders.
Require Import BlindLedger.ledger_crypto.
Require Import BlindLedger.ledger_state.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* CONCRETE STATE MODEL *)
(* ========================================================================= *)

(**
 * To use FMap with our Inductive CapHash types, we must define an Ordering.
 * We can map the inductive structure to something ordered (like Z or list Z) 
 * or define a recursive comparison.
 * 
 * For simplicity, let's assume we can serialize hashes to Z for comparison.
 *)

Fixpoint secret_to_Z (s : Secret) : Z :=
  match s with mkSecret z => z end.

Fixpoint leaf_to_Z (l : LeafHash) : Z :=
  match l with Hash_Leaf pa len => pa + len (* Injective-enough for ordering? No. Just a proxy *) end.

(** 
 * Better approach: Use the Decidable Equality we already proved!
 * Coq's FMapList only strictly needs 'OrderedType'. 
 * We will assume an arbitrary total order exists (it does for countable types).
 *)

Module CapHashAsOrderedType.
  Definition t := CapHash.
  
  Definition eq (x y : t) := x = y.
  Definition eq_refl := @eq_refl t.
  Definition eq_sym := @eq_sym t.
  Definition eq_trans := @eq_trans t.
 
  (* Axiomatize total order for map implementation *)
  Axiom compare : t -> t -> comparison.
  
  Definition lt (x y : t) := compare x y = Lt.
  
  Axiom lt_not_eq : forall x y, lt x y -> ~ eq x y.
  Axiom lt_trans : forall x y z, lt x y -> lt y z -> lt x z.
  Axiom lt_strorder : StrictOrder lt.
  Axiom lt_compat : Proper (eq ==> eq ==> iff) lt.
  Axiom compare_spec : forall x y, CompareSpec (eq x y) (lt x y) (lt y x) (compare x y).
  
 Definition eq_equiv : Equivalence eq.
  Proof.
    split; unfold eq.
    - intro; reflexivity.
    - intros x y H; symmetry; assumption.
    - intros x y z H1 H2; transitivity y; assumption.
  Qed.
  
  Definition eq_dec := CapHash_eq_dec.
  
End CapHashAsOrderedType.

Module CMap := FMapList.Make(CapHashAsOrderedType).

Definition ConcreteState := CMap.t BlindLedgerEntry.

(* ========================================================================= *)
(* REFINEMENT RELATION *)
(* ========================================================================= *)

Definition Refinement (c : ConcreteState) (a : LedgerState) : Prop :=
  forall k, CMap.find k c = a k.

(* ========================================================================= *)
(* CONCRETE OPERATIONS *)
(* ========================================================================= *)

Definition concrete_mint (c : ConcreteState) (pa : PAddr) (len : Len) (o : Proc) 
                         (p : Permissions) (s : Secret) 
                         : (ConcreteState * UserCapability * LedgerError) :=
  
  (* Dummy hash for error flow *)
  let dummy_hash := Hash_Cap (Hash_Process (mkSecret 0) (Hash_Leaf 0 0) 0) (Hash_Leaf 0 0) in
                       
  if (Z.leb len 0) then (c, mkUserCapability dummy_hash 0 0 0, LE_Invalid) else
  
  let cap_hash := mint_capability_hash pa len s o in
  let lh := compute_leaf_hash pa len in
  let ph := compute_process_hash s lh o in
  
  match CMap.find cap_hash c with
  | Some _ => (c, mkUserCapability dummy_hash 0 0 0, LE_Invalid)
  | None =>
      let cap := mkUserCapability cap_hash len 1 p in
      let entry := mkEntry cap pa len o p s State_Active ph lh in
      (CMap.add cap_hash entry c, cap, LE_Ok)
  end.

(* ========================================================================= *)
(* REFINEMENT PROOFS *)
(* ========================================================================= *)

Theorem mint_refinement : forall (c : ConcreteState) (a : LedgerState) 
                                 (pa : PAddr) (len : Len) (o : Proc) 
                                 (p : Permissions) (s : Secret),
  Refinement c a ->
  let (c', cap_c, err_c) := concrete_mint c pa len o p s in
  let (a', cap_a, err_a) := op_mint a pa len o p s in
  Refinement c' a' /\ cap_c = cap_a /\ err_c = err_a.
Proof.
  intros c a pa len o p s Href.
  unfold concrete_mint, op_mint.
  
  (* Case 1: Invalid Length *)
  destruct (Z.leb len 0) eqn:Hlen.
  { split; [exact Href|split; reflexivity]. }
  
  (* Case 2: Valid Length *)
  remember (mint_capability_hash pa len s o) as k.
  
  (* Check if key exists using Refinement *)
  specialize (Href k).
  rewrite Href.
  
  destruct (a k) as [entry|].
  
  (* Subcase 2a: Key Exists (Duplicate) *)
  { split; [exact Href|split; reflexivity]. }
  
  (* Subcase 2b: Key Missing (Success) *)
  {
    split.
    - (* Prove Refinement for new state *)
      intro k'.
      unfold update_state.
      destruct (CapHash_eq_dec k' k).
      + subst. rewrite CMap.add_eq_o; [reflexivity|reflexivity].
      + rewrite CMap.add_neq_o; [|intro Hneq; apply n; unfold CapHashAsOrderedType.eq in Hneq; exact Hneq].
        rewrite Href.
        reflexivity.
    - split; reflexivity.
  }
Qed.
