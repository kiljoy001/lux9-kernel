(** Process State DAG Verification
    Formal model of kernel/proc_state_dag.c state machine.
    Proves: valid transitions, precondition safety, and reachability. *)

Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

(* ========================================================================= *)
(* PROCESS STATE DEFINITIONS *)
(* ========================================================================= *)

(** Process states from kernel/include/proc_state_dag.h *)
Inductive ProcState :=
  | ProcDead
  | ProcNew
  | ProcReady
  | ProcScheding
  | ProcRunning
  | ProcQueueing
  | ProcQueueingR
  | ProcQueueingW
  | ProcWakeme
  | ProcBroken
  | ProcStopped
  | ProcRendezvous
  | ProcWaitrelease
  | ProcMoribund.

(** State equality is decidable *)
Definition state_eq_dec : forall s1 s2 : ProcState, {s1 = s2} + {s1 <> s2}.
Proof. decide equality. Defined.

(* ========================================================================= *)
(* TRANSITION RELATION (from proc_state_dag.c:procstate_dag_init) *)
(* ========================================================================= *)

(** The valid edges in the process state DAG *)
Inductive valid_transition : ProcState -> ProcState -> Prop :=
  (* Dead → New *)
  | trans_dead_new       : valid_transition ProcDead ProcNew
  (* New → Ready *)
  | trans_new_ready      : valid_transition ProcNew ProcReady
  (* Ready → Running *)
  | trans_ready_running  : valid_transition ProcReady ProcRunning
  (* Running → various states *)
  | trans_running_wakeme     : valid_transition ProcRunning ProcWakeme
  | trans_running_scheding   : valid_transition ProcRunning ProcScheding
  | trans_running_moribund   : valid_transition ProcRunning ProcMoribund
  | trans_running_queueing   : valid_transition ProcRunning ProcQueueing
  | trans_running_rendezvous : valid_transition ProcRunning ProcRendezvous
  | trans_running_waitrelease: valid_transition ProcRunning ProcWaitrelease
  | trans_running_broken     : valid_transition ProcRunning ProcBroken
  | trans_running_stopped    : valid_transition ProcRunning ProcStopped
  (* Back to Ready *)
  | trans_wakeme_ready       : valid_transition ProcWakeme ProcReady
  | trans_scheding_ready     : valid_transition ProcScheding ProcReady
  | trans_queueing_ready     : valid_transition ProcQueueing ProcReady
  | trans_rendezvous_ready   : valid_transition ProcRendezvous ProcReady
  | trans_waitrelease_ready  : valid_transition ProcWaitrelease ProcReady
  | trans_stopped_ready      : valid_transition ProcStopped ProcReady
  (* Terminal: Moribund → Dead *)
  | trans_moribund_dead  : valid_transition ProcMoribund ProcDead.

(* ========================================================================= *)
(* PRECONDITIONS (from proc_state_dag.c) *)
(* ========================================================================= *)

(** Abstract process structure - models relevant fields from Proc *)
Record ProcAbstract := mkProc {
  proc_r    : option unit;    (* p->r: rendezvous pointer *)
  proc_mach : option unit;    (* p->mach: machine pointer *)
}.

(** Precondition functions from C code *)
Definition check_rendezvous_cleared (p : ProcAbstract) : bool :=
  match proc_r p with None => true | Some _ => false end.

Definition check_mach_cleared (p : ProcAbstract) : bool :=
  match proc_mach p with None => true | Some _ => false end.

Definition check_mach_set (p : ProcAbstract) : bool :=
  match proc_mach p with Some _ => true | None => false end.

Definition check_rendezvous_set (p : ProcAbstract) : bool :=
  match proc_r p with Some _ => true | None => false end.

(** Precondition requirements for specific transitions *)
Definition transition_precondition (from to : ProcState) (p : ProcAbstract) : bool :=
  match from, to with
  | ProcNew, ProcReady => check_mach_cleared p
  | ProcReady, ProcRunning => check_mach_set p
  | ProcRunning, ProcWakeme => check_rendezvous_set p
  | ProcWakeme, ProcReady => check_rendezvous_cleared p
  | ProcMoribund, ProcDead => check_mach_cleared p
  | _, _ => true  (* No precondition *)
  end.

(* ========================================================================= *)
(* SAFETY THEOREMS *)
(* ========================================================================= *)

(** Theorem 1: Dead is only reachable from Moribund *)
Theorem dead_only_from_moribund : forall from,
  valid_transition from ProcDead -> from = ProcMoribund.
Proof.
  intros from H.
  inversion H.
  reflexivity.
Qed.

(** Theorem 2: New is only reachable from Dead *)
Theorem new_only_from_dead : forall from,
  valid_transition from ProcNew -> from = ProcDead.
Proof.
  intros from H.
  inversion H.
  reflexivity.
Qed.

(** Theorem 3: Running can only be reached from Ready *)
Theorem running_only_from_ready : forall from,
  valid_transition from ProcRunning -> from = ProcReady.
Proof.
  intros from H.
  inversion H.
  reflexivity.
Qed.

(** Theorem 4: All non-terminal states can reach Ready (via some path) *)
Inductive can_reach : ProcState -> ProcState -> Prop :=
  | reach_refl : forall s, can_reach s s
  | reach_step : forall s1 s2 s3, 
      valid_transition s1 s2 -> can_reach s2 s3 -> can_reach s1 s3.

Lemma ready_reachable_from_running : can_reach ProcRunning ProcReady.
Proof.
  apply reach_step with ProcScheding.
  - apply trans_running_scheding.
  - apply reach_step with ProcReady.
    + apply trans_scheding_ready.
    + apply reach_refl.
Qed.

Lemma running_reachable_from_ready : can_reach ProcReady ProcRunning.
Proof.
  apply reach_step with ProcRunning.
  - apply trans_ready_running.
  - apply reach_refl.
Qed.

(** Theorem 5: Dead is a terminal state (no outgoing transitions except to New) *)
Theorem dead_outgoing : forall to,
  valid_transition ProcDead to -> to = ProcNew.
Proof.
  intros to H.
  inversion H.
  reflexivity.
Qed.

(** Theorem 6: Broken is a terminal state (no outgoing transitions) *)
Theorem broken_terminal : forall to,
  valid_transition ProcBroken to -> False.
Proof.
  intros to H.
  inversion H.
Qed.

(** Theorem 7: QueueingR and QueueingW have no transitions defined *)
Theorem queueingR_unused : forall from to,
  valid_transition from to -> from <> ProcQueueingR /\ to <> ProcQueueingR.
Proof.
  intros from to H.
  inversion H; split; discriminate.
Qed.

Theorem queueingW_unused : forall from to,
  valid_transition from to -> from <> ProcQueueingW /\ to <> ProcQueueingW.
Proof.
  intros from to H.
  inversion H; split; discriminate.
Qed.

(* ========================================================================= *)
(* PRECONDITION SAFETY *)
(* ========================================================================= *)

(** Safe transition: valid edge AND precondition satisfied *)
Definition safe_transition (from to : ProcState) (p : ProcAbstract) : Prop :=
  valid_transition from to /\ transition_precondition from to p = true.

(** Theorem 8: Ready→Running requires mach to be set *)
Theorem ready_running_requires_mach : forall p,
  safe_transition ProcReady ProcRunning p -> check_mach_set p = true.
Proof.
  intros p [Hvalid Hpre].
  simpl in Hpre.
  exact Hpre.
Qed.

(** Theorem 9: Running→Wakeme requires rendezvous to be set *)
Theorem running_wakeme_requires_rendezvous : forall p,
  safe_transition ProcRunning ProcWakeme p -> check_rendezvous_set p = true.
Proof.
  intros p [Hvalid Hpre].
  simpl in Hpre.
  exact Hpre.
Qed.

(** Theorem 10: Wakeme→Ready requires rendezvous to be cleared *)
Theorem wakeme_ready_requires_clear : forall p,
  safe_transition ProcWakeme ProcReady p -> check_rendezvous_cleared p = true.
Proof.
  intros p [Hvalid Hpre].
  simpl in Hpre.
  exact Hpre.
Qed.

(* ========================================================================= *)
(* LIVENESS: All waiting states can eventually return to Ready *)
(* ========================================================================= *)

(** All "sleeping" states have a path to Ready *)
Theorem wakeme_can_reach_ready : can_reach ProcWakeme ProcReady.
Proof.
  apply reach_step with ProcReady.
  - apply trans_wakeme_ready.
  - apply reach_refl.
Qed.

Theorem queueing_can_reach_ready : can_reach ProcQueueing ProcReady.
Proof.
  apply reach_step with ProcReady.
  - apply trans_queueing_ready.
  - apply reach_refl.
Qed.

Theorem rendezvous_can_reach_ready : can_reach ProcRendezvous ProcReady.
Proof.
  apply reach_step with ProcReady.
  - apply trans_rendezvous_ready.
  - apply reach_refl.
Qed.

Theorem stopped_can_reach_ready : can_reach ProcStopped ProcReady.
Proof.
  apply reach_step with ProcReady.
  - apply trans_stopped_ready.
  - apply reach_refl.
Qed.

(* ========================================================================= *)
(* DECIDABILITY *)
(* ========================================================================= *)

(** Decidable transition validity for runtime checks *)
Definition valid_transition_dec (from to : ProcState) : bool :=
  match from, to with
  | ProcDead, ProcNew => true
  | ProcNew, ProcReady => true
  | ProcReady, ProcRunning => true
  | ProcRunning, ProcWakeme => true
  | ProcRunning, ProcScheding => true
  | ProcRunning, ProcMoribund => true
  | ProcRunning, ProcQueueing => true
  | ProcRunning, ProcRendezvous => true
  | ProcRunning, ProcWaitrelease => true
  | ProcRunning, ProcBroken => true
  | ProcRunning, ProcStopped => true
  | ProcWakeme, ProcReady => true
  | ProcScheding, ProcReady => true
  | ProcQueueing, ProcReady => true
  | ProcRendezvous, ProcReady => true
  | ProcWaitrelease, ProcReady => true
  | ProcStopped, ProcReady => true
  | ProcMoribund, ProcDead => true
  | _, _ => false
  end.

(** Correctness of decision procedure *)
Theorem valid_transition_dec_correct : forall from to,
  valid_transition_dec from to = true <-> valid_transition from to.
Proof.
  intros from to.
  split.
  - intros H.
    destruct from, to; try discriminate H;
    constructor.
  - intros H.
    inversion H; reflexivity.
Qed.
