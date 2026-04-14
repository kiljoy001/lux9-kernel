(*
 * MSGORD 2024 Model with Resource Key Conflicts
 * This model implements the 2024 concurrency semantics where conflicts
 * are based on explicit resource-key matching, not just parent-chain analysis.
 *
 * Based on kernel/include/msgord.h and kernel/msgord.c
 *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Lia.
Import ListNotations.

(* Message IDs and parameters *)
Definition MsgId := Z.

Definition K_PARAM : Z := 3.

(* Message color (BLUE = safe, RED = conflicting) *)
Inductive Color :=
  | Blue
  | Red.

(* Message states *)
Inductive MsgState :=
  | Pending
  | Ordered
  | Delivered
  | Complete.

(* Message with resource keys for 2024 model *)
Record GhostMsg := mkMsg {
  gm_id : MsgId;
  gm_timestamp : Z;
  gm_parents : list MsgId;
  gm_resource_count : Z;
  gm_resource_keys : list MsgId;
  gm_color : Color;
  gm_state : MsgState;
}.

(* DAG is a list of messages *)
Definition MsgOrd := list GhostMsg.

(* ======================================================================= *)
(* 2024-SPECIFIC DEFINITIONS                                             *)
(* ======================================================================= *)

(* Check if msg1 has gm_id as a parent *)
Definition is_parent (msg1 msg2 : GhostMsg) : bool :=
  existsb (Z.eqb msg2.(gm_id)) msg1.(gm_parents).

(* Determine if two messages conflict: share a key OR one has zero resources *)
(* This is the CORE 2024 model change *)
Definition msgord_messages_conflict (a b : GhostMsg) : Prop :=
  (a.(gm_resource_count) = 0) ∨ (b.(gm_resource_count) = 0) ∨
  (exists i j, List.nth a.(gm_resource_keys) i = Some (List.nth b.(gm_resource_keys) j)) ∨
  (List.exists (fun k => existsb (Z.eqb k) a.(gm_parents)) b.(gm_resource_keys)) ∨
  (List.exists (fun k => existsb (Z.eqb k) b.(gm_parents)) a.(gm_resource_keys)).

(* Simplified anticone: count conflicting pending non-parents *)
Fixpoint anticone (dag : MsgOrd) (msg : GhostMsg) : Z := 
  match dag with
  | [] => 0
  | gm :: rest =>
      let count_rest := anticone rest msg in
      if Z.eqb gm.(gm_id) msg.(gm_id) || gm.(gm_state) <> Pending then count_rest
      else if ~is_parent msg gm && msgord_messages_conflict msg gm then 1 + count_rest
      else count_rest
  end.

(* ======================================================================= *)
(* THEOREMS (placeholder with admit)                                      *)
(* These proofs need to be completed to verify the 2024 model          *)
(* ======================================================================= *)

(* 1. Anticone is bounded in the 2024 model *)
Theorem anticone_bounded_2024 :
  forall dag msg, anticone dag msg ≤ K_PARAM + 10.
Proof. admit.

(* 2. Frontier parent selection maintains DAG acyclicity *)
Theorem frontier_parents_acyclic_2024 :
  forall dag msg parent_id,
    List.Mem parent_id msg.(gm_parents) → 
      exists p, p ∈ dag ∧ p.(gm_id) = parent_id ∧ p.(gm_timestamp) < msg.(gm_timestamp).
Proof. admit.

(* 3. Conflict detection is sound and complete *) 
Theorem conflict_detection_2024 :
  forall a b, msgord_messages_conflict a b ↔
    a = b ∨ a.(gm_resource_count) = 0 ∨ b.(gm_resource_count) = 0 ∨
    (∃ i j, List.nth a.(gm_resource_keys) i = Some (List.nth b.(gm_resource_keys) j)).
Proof. admit.
