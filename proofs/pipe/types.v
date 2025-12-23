(** * Pipe Types - Base definitions for Pipe device verification
    * 
    * This module defines the core types used across all Pipe proofs.
    * Import this into conservation.v, safety.v, etc.
    *)

Require Export Coq.ZArith.ZArith.
Require Export Coq.Bool.Bool.
Require Export Coq.Lists.List.
Require Export Lia.
Export ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* QUEUE STATE MODEL                                                         *)
(* ========================================================================= *)

(* Queue can be open or closed *)
Inductive QueueStatus : Type :=
  | QOpen : QueueStatus
  | QClosed : QueueStatus.

Definition queue_is_open (q : QueueStatus) : bool :=
  match q with
  | QOpen => true
  | QClosed => false
  end.

(* ========================================================================= *)
(* PIPE STATE MODEL (matches struct Pipe in devpipe.c)                       *)
(* ========================================================================= *)

Record PipeState := mkPipe {
  (* Reference count - total references to this pipe *)
  ref : Z;
  
  (* Per-queue reference counts *)
  qref0 : Z;    (* references to q[0] / data *)
  qref1 : Z;    (* references to q[1] / data1 *)
  
  (* Queue status *)
  q0_status : QueueStatus;
  q1_status : QueueStatus;
  
  (* Queue contents length (for properties) *)
  q0_len : Z;
  q1_len : Z;
}.

(* Initial state after pipeattach *)
Definition initial_pipe : PipeState :=
  mkPipe 1 0 0 QOpen QOpen 0 0.

(* ========================================================================= *)
(* CHANNEL STATE                                                             *)
(* ========================================================================= *)

Inductive ChanType : Type :=
  | ChanDir : ChanType      (* Directory channel *)
  | ChanData0 : ChanType    (* data file, id=0 *)
  | ChanData1 : ChanType.   (* data1 file, id=1 *)

Inductive OpenMode : Type :=
  | ModeRead : OpenMode
  | ModeWrite : OpenMode
  | ModeRdWr : OpenMode.

Record ChanState := mkChan {
  chan_type : ChanType;
  chan_open : bool;
  chan_mode : OpenMode;
}.

(* ========================================================================= *)
(* HELPER FUNCTIONS                                                          *)
(* ========================================================================= *)

Definition incr_ref (p : PipeState) : PipeState :=
  mkPipe (p.(ref) + 1) p.(qref0) p.(qref1)
         p.(q0_status) p.(q1_status) p.(q0_len) p.(q1_len).

Definition decr_ref (p : PipeState) : PipeState :=
  mkPipe (p.(ref) - 1) p.(qref0) p.(qref1)
         p.(q0_status) p.(q1_status) p.(q0_len) p.(q1_len).

Definition incr_qref0 (p : PipeState) : PipeState :=
  mkPipe p.(ref) (p.(qref0) + 1) p.(qref1)
         p.(q0_status) p.(q1_status) p.(q0_len) p.(q1_len).

Definition incr_qref1 (p : PipeState) : PipeState :=
  mkPipe p.(ref) p.(qref0) (p.(qref1) + 1)
         p.(q0_status) p.(q1_status) p.(q0_len) p.(q1_len).

Definition close_q0 (p : PipeState) : PipeState :=
  mkPipe p.(ref) p.(qref0) p.(qref1)
         QClosed p.(q1_status) p.(q0_len) p.(q1_len).

Definition close_q1 (p : PipeState) : PipeState :=
  mkPipe p.(ref) p.(qref0) p.(qref1)
         p.(q0_status) QClosed p.(q0_len) p.(q1_len).

(* ========================================================================= *)
(* WELL-FORMEDNESS INVARIANTS                                                *)
(* ========================================================================= *)

Definition Inv_RefPositive (p : PipeState) : Prop :=
  p.(ref) >= 0 /\ p.(qref0) >= 0 /\ p.(qref1) >= 0.

Definition Inv_QueueLenNonNeg (p : PipeState) : Prop :=
  p.(q0_len) >= 0 /\ p.(q1_len) >= 0.

Definition Inv_WellFormed (p : PipeState) : Prop :=
  Inv_RefPositive p /\ Inv_QueueLenNonNeg p.

(* Key lemma: initial_pipe is well-formed *)
Lemma initial_pipe_wellformed : Inv_WellFormed initial_pipe.
Proof.
  unfold Inv_WellFormed, Inv_RefPositive, Inv_QueueLenNonNeg, initial_pipe.
  simpl. lia.
Qed.

(* Incrementing ref preserves well-formedness *)
Lemma incr_ref_preserves_wf : forall p,
  Inv_WellFormed p -> Inv_WellFormed (incr_ref p).
Proof.
  intros p [Hpos Hlen].
  unfold Inv_WellFormed, Inv_RefPositive, Inv_QueueLenNonNeg, incr_ref in *.
  simpl in *. lia.
Qed.

(* Decrementing ref preserves non-negativity if ref > 0 *)
Lemma decr_ref_preserves_wf : forall p,
  Inv_WellFormed p -> p.(ref) > 0 -> Inv_WellFormed (decr_ref p).
Proof.
  intros p [Hpos Hlen] Href.
  unfold Inv_WellFormed, Inv_RefPositive, Inv_QueueLenNonNeg, decr_ref in *.
  simpl in *. lia.
Qed.
