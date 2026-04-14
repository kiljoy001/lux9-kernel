(** Blind Ledger - Phase 4: Temporal Logic & Epochs
    Defines epoch-based validity and expiration logic. *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Require Import BlindLedger.ledger_crypto.
Require Import BlindLedger.ledger_state. (* For EntryState/BlindLedgerEntry *)
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* TIME MODEL *)
(* ========================================================================= *)

Definition Epoch := Z.

Record TimeState := mkTimeState {
  current_epoch : Epoch;
}.

Definition init_time : TimeState := mkTimeState 1.

(* ========================================================================= *)
(* VALIDITY RULES *)
(* ========================================================================= *)

Definition is_valid_entry (entry : BlindLedgerEntry) (ts : TimeState) : bool :=
  match entry.(ent_state) with
  | State_Active => true 
  | _ => false
  end.

Definition ExpirationPeriod := 100.

(**
 * Since I cannot easily modify ledger_state.v without recompiling everything,
 * I will define a "RichEntry" here that wraps BlindLedgerEntry with an epoch.
 *)
Record RichEntry := mkRichEntry {
  base_entry : BlindLedgerEntry;
  creation_epoch : Epoch;
}.

Definition entry_is_fresh (re : RichEntry) (ts : TimeState) : bool :=
  Z.leb (ts.(current_epoch) - re.(creation_epoch)) ExpirationPeriod.

(* ========================================================================= *)
(* TEMPORAL THEOREMS *)
(* ========================================================================= *)

Theorem expiration_monotonic : forall re t1 t2,
  t1.(current_epoch) <= t2.(current_epoch) ->
  entry_is_fresh re t2 = true ->
  entry_is_fresh re t1 = true.
Proof.
  intros re t1 t2 Htime Hfresh2.
  unfold entry_is_fresh in *.
  apply Z.leb_le in Hfresh2.
  apply Z.leb_le.
  lia.
Qed.
