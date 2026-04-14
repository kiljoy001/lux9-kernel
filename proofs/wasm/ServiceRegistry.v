Require Import Coq.Lists.List.
Require Import Coq.Strings.String.
Require Import Coq.ZArith.ZArith.

(* Abstract Identifier types *)
Definition SessionID := Z.
Definition SessionName := string.

(* 
 * Abstract Registry Model
 * A List of (ID, Name) pairs, enforced to be unique.
 *)
Inductive Registry :=
  | Empty
  | Entry (id : SessionID) (name : SessionName) (next : Registry).

(* Helper: Lookup by Name *)
Fixpoint lookup_by_name (r : Registry) (n : SessionName) : option SessionID :=
  match r with
  | Empty => None
  | Entry id name next => 
      if string_dec name n then Some id else lookup_by_name next n
  end.

(* Helper: Lookup by ID *)
Fixpoint lookup_by_id (r : Registry) (i : SessionID) : option SessionName :=
  match r with
  | Empty => None
  | Entry id name next =>
      if Z.eq_dec id i then Some name else lookup_by_id next i
  end.

(* Invariant: No Duplicate IDs *)
Fixpoint unique_ids (r : Registry) : Prop :=
  match r with
  | Empty => True
  | Entry id _ next => lookup_by_id next id = None /\ unique_ids next
  end.

(* Invariant: No Duplicate Names *)
Fixpoint unique_names (r : Registry) : Prop :=
  match r with
  | Empty => True
  | Entry _ name next => lookup_by_name next name = None /\ unique_names next
  end.

(* The Valid Registry Predicate *)
Definition valid_registry (r : Registry) : Prop :=
  unique_ids r /\ unique_names r.

(* Operation: Insert *)
Definition insert (r : Registry) (id : SessionID) (name : SessionName) : Registry :=
  Entry id name r.

(* Theorem: Insertion preserves validity if ID/Name are fresh *)
Theorem insert_preserves_validity : forall r id name,
  valid_registry r ->
  lookup_by_id r id = None ->
  lookup_by_name r name = None ->
  valid_registry (insert r id name).
Proof.
  intros r id name [Huid Huname] Hid Hname.
  split.
  - simpl. split; assumption.
  - simpl. split; assumption.
Qed.
