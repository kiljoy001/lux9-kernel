(** * Pipe Safety - Memory and IPC safety properties
    * 
    * Imports: pipe/types, pipe/conservation
    * Proves: Queue duality, EOF propagation, double-free prevention
    *)

Require Import pipe.types.
Require Import pipe.conservation.


(* ========================================================================= *)
(* QUEUE DUALITY - Write to q[id], Read from q[1-id]                         *)
(* ========================================================================= *)

(** Write operation: adds data to queue id *)
Inductive PipeWrite (id : Z) (len : Z) (p1 p2 : PipeState) : Prop :=
  | PW_Q0 :
      id = 0 ->
      len > 0 ->
      p1.(q0_status) = QOpen ->
      p2 = mkPipe p1.(ref) p1.(qref0) p1.(qref1)
                  p1.(q0_status) p1.(q1_status)
                  (p1.(q0_len) + len) p1.(q1_len) ->
      PipeWrite id len p1 p2
  | PW_Q1 :
      id = 1 ->
      len > 0 ->
      p1.(q1_status) = QOpen ->
      p2 = mkPipe p1.(ref) p1.(qref0) p1.(qref1)
                  p1.(q0_status) p1.(q1_status)
                  p1.(q0_len) (p1.(q1_len) + len) ->
      PipeWrite id len p1 p2.

(** Read operation: removes data from queue (1-id) - the OTHER queue *)
Inductive PipeRead (id : Z) (len : Z) (p1 p2 : PipeState) : Prop :=
  | PR_Q0 :
      id = 0 ->  (* Reading from data (id=0) reads q[1] *)
      len > 0 ->
      p1.(q1_len) >= len ->
      p2 = mkPipe p1.(ref) p1.(qref0) p1.(qref1)
                  p1.(q0_status) p1.(q1_status)
                  p1.(q0_len) (p1.(q1_len) - len) ->
      PipeRead id len p1 p2
  | PR_Q1 :
      id = 1 ->  (* Reading from data1 (id=1) reads q[0] *)
      len > 0 ->
      p1.(q0_len) >= len ->
      p2 = mkPipe p1.(ref) p1.(qref0) p1.(qref1)
                  p1.(q0_status) p1.(q1_status)
                  (p1.(q0_len) - len) p1.(q1_len) ->
      PipeRead id len p1 p2.

(* ========================================================================= *)
(* DUALITY PROOFS                                                            *)
(* ========================================================================= *)

(** Write to id=0 increases q0_len *)
Theorem write_q0_increases_q0_len :
  forall len p1 p2,
  PipeWrite 0 len p1 p2 -> p2.(q0_len) = p1.(q0_len) + len.
Proof.
  intros len p1 p2 H. inversion H; subst.
  - simpl. reflexivity.
  - discriminate.
Qed.

(** Write to id=1 increases q1_len *)
Theorem write_q1_increases_q1_len :
  forall len p1 p2,
  PipeWrite 1 len p1 p2 -> p2.(q1_len) = p1.(q1_len) + len.
Proof.
  intros len p1 p2 H. inversion H; subst.
  - discriminate.
  - simpl. reflexivity.
Qed.

(** Read from id=0 decreases q1_len (duality!) *)
Theorem read_q0_decreases_q1_len :
  forall len p1 p2,
  PipeRead 0 len p1 p2 -> p2.(q1_len) = p1.(q1_len) - len.
Proof.
  intros len p1 p2 H. inversion H; subst.
  - simpl. reflexivity.
  - discriminate.
Qed.

(** Read from id=1 decreases q0_len (duality!) *)
Theorem read_q1_decreases_q0_len :
  forall len p1 p2,
  PipeRead 1 len p1 p2 -> p2.(q0_len) = p1.(q0_len) - len.
Proof.
  intros len p1 p2 H. inversion H; subst.
  - discriminate.  
  - simpl. reflexivity.
Qed.

(** Write-Read duality: Write to q[0], Read from q[1-0]=q[1] is symmetric *)
Theorem write_read_duality :
  forall len p1 p2 p3,
  PipeWrite 1 len p1 p2 ->  (* Write to data1 (q[1]) *)
  PipeRead 0 len p2 p3 ->   (* Read from data (q[0]) reads q[1] *)
  p3.(q1_len) = p1.(q1_len). (* Net effect: q1 length unchanged *)
Proof.
  intros len p1 p2 p3 Hw Hr.
  inversion Hw; subst; try discriminate.
  inversion Hr; subst; try discriminate.
  simpl in *. lia.
Qed.

(* ========================================================================= *)
(* EOF PROPAGATION                                                           *)
(* ========================================================================= *)

(** Close writer: closes the queue, signaling EOF to readers *)
Theorem close_writer_signals_eof :
  forall id p1 p2,
  PipeCloseWriter id p1 p2 ->
  (id = 0 -> p2.(q0_status) = QClosed) /\
  (id = 1 -> p2.(q1_status) = QClosed).
Proof.
  intros id p1 p2 H. inversion H; subst.
  - split; intro Heq.
    + unfold decr_ref, close_q0. simpl. reflexivity.
    + discriminate.
  - split; intro Heq.
    + discriminate.
    + unfold decr_ref, close_q1. simpl. reflexivity.
Qed.

(** Close reader: closes the OTHER queue (broken pipe signal) *)
Theorem close_reader_signals_broken_pipe :
  forall id p1 p2,
  PipeCloseReader id p1 p2 ->
  (id = 0 -> p2.(q1_status) = QClosed) /\
  (id = 1 -> p2.(q0_status) = QClosed).
Proof.
  intros id p1 p2 H. inversion H; subst.
  - split; intro Heq.
    + unfold decr_ref, close_q1. simpl. reflexivity.
    + discriminate.
  - split; intro Heq.
    + discriminate.
    + unfold decr_ref, close_q0. simpl. reflexivity.
Qed.

(* ========================================================================= *)
(* DOUBLE-FREE PREVENTION                                                    *)
(* ========================================================================= *)

(** A closed queue cannot be written to *)
Theorem closed_queue_no_write :
  forall id len p1 p2,
  (id = 0 /\ p1.(q0_status) = QClosed) \/
  (id = 1 /\ p1.(q1_status) = QClosed) ->
  ~ PipeWrite id len p1 p2.
Proof.
  intros id len p1 p2 Hclosed Hwrite.
  destruct Hclosed as [[Hid Hq0] | [Hid Hq1]].
  - subst. inversion Hwrite; subst.
    + rewrite Hq0 in H1. discriminate.
    + discriminate.
  - subst. inversion Hwrite; subst.
    + discriminate.
    + rewrite Hq1 in H1. discriminate.
Qed.

(** ref=0 implies pipe is freed - no further operations possible *)
Theorem ref_zero_prevents_ops :
  forall p1 p2,
  p1.(ref) = 0 -> ~ PipeWalkClone p1 p2.
Proof.
  intros p1 p2 Href Hclone.
  inversion Hclone. lia.
Qed.

(** Freeing already freed pipe is impossible (ref would be negative) *)
Theorem double_free_prevented :
  forall p1 p2 p3,
  Inv_WellFormed p1 ->
  p1.(ref) = 1 ->
  PipeClose p1 p2 ->
  p2.(ref) = 0 ->
  ~ PipeClose p2 p3.
Proof.
  intros p1 p2 p3 Hwf Href1 Hclose1 Href2 Hclose2.
  inversion Hclose2. lia.
Qed.

(* ========================================================================= *)
(* INVERSE THEOREMS                                                          *)
(* ========================================================================= *)

(** Inverse: Read is the inverse of write (for queue length) *)
Theorem read_inverse_of_write_q0 :
  forall len p1 p2 p3,
  len > 0 ->
  p1.(q0_status) = QOpen ->
  PipeWrite 0 len p1 p2 ->
  p1.(q1_len) >= len ->
  PipeRead 1 len p2 p3 ->
  p3.(q0_len) = p1.(q0_len).
Proof.
  intros len p1 p2 p3 Hlen Hopen Hw Hr_cond Hr.
  inversion Hw; subst; try discriminate.
  inversion Hr; subst; try discriminate.
  simpl. lia.
Qed.

(** Inverse: Write then read returns queue to original length *)
Theorem write_read_inverse_q1 :
  forall len p1 p2 p3,
  len > 0 ->
  p1.(q1_status) = QOpen ->
  PipeWrite 1 len p1 p2 ->
  PipeRead 0 len p2 p3 ->
  p3.(q1_len) = p1.(q1_len).
Proof.
  intros len p1 p2 p3 Hlen Hopen Hw Hr.
  inversion Hw; subst; try discriminate.
  inversion Hr; subst; try discriminate.
  simpl. lia.
Qed.

(** Inverse: Queue status is preserved across write *)
Theorem write_preserves_queue_status :
  forall id len p1 p2,
  PipeWrite id len p1 p2 ->
  p2.(q0_status) = p1.(q0_status) /\
  p2.(q1_status) = p1.(q1_status).
Proof.
  intros id len p1 p2 H.
  inversion H; subst; simpl; split; reflexivity.
Qed.

(** Inverse: Queue status is preserved across read *)
Theorem read_preserves_queue_status :
  forall id len p1 p2,
  PipeRead id len p1 p2 ->
  p2.(q0_status) = p1.(q0_status) /\
  p2.(q1_status) = p1.(q1_status).
Proof.
  intros id len p1 p2 H.
  inversion H; subst; simpl; split; reflexivity.
Qed.

(** Inverse: Reference count is preserved across write *)
Theorem write_preserves_ref :
  forall id len p1 p2,
  PipeWrite id len p1 p2 ->
  p2.(ref) = p1.(ref).
Proof.
  intros id len p1 p2 H.
  inversion H; subst; simpl; reflexivity.
Qed.

(** Inverse: Reference count is preserved across read *)
Theorem read_preserves_ref :
  forall id len p1 p2,
  PipeRead id len p1 p2 ->
  p2.(ref) = p1.(ref).
Proof.
  intros id len p1 p2 H.
  inversion H; subst; simpl; reflexivity.
Qed.

(** Inverse: Multiple writes accumulate, multiple reads drain *)
Theorem writes_accumulate :
  forall len1 len2 p1 p2 p3,
  PipeWrite 0 len1 p1 p2 ->
  PipeWrite 0 len2 p2 p3 ->
  p3.(q0_len) = p1.(q0_len) + len1 + len2.
Proof.
  intros len1 len2 p1 p2 p3 H1 H2.
  inversion H1; subst; try discriminate.
  inversion H2; subst; try discriminate.
  simpl. lia.
Qed.

(** Inverse: Closed queue remains closed (irreversible) *)
Theorem close_irreversible :
  forall id len p1 p2,
  (id = 0 /\ p1.(q0_status) = QClosed) ->
  ~ PipeWrite id len p1 p2.
Proof.
  intros id len p1 p2 [Hid Hclosed] Hwrite.
  subst. inversion Hwrite; subst.
  - rewrite Hclosed in H1. discriminate.
  - discriminate.
Qed.

(** Inverse: Duality symmetry - operations on id and 1-id are symmetric *)
Theorem duality_symmetry :
  forall len p1 p2 p3,
  PipeWrite 0 len p1 p2 ->
  PipeRead 1 len p2 p3 ->
  p3.(q0_len) = p1.(q0_len) /\
  p3.(q1_len) = p1.(q1_len).
Proof.
  intros len p1 p2 p3 Hw Hr.
  inversion Hw; subst; try discriminate.
  inversion Hr; subst; try discriminate.
  simpl. split; lia.
Qed.
