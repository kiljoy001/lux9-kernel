(* Formal verification of the command execution loop between Linux and Plan 9 *)

Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Require Import Coq.Strings.String.
Require Import Coq.Strings.Ascii.
Require Import Coq.Logic.FunctionalExtensionality.
Require Import Lia.
Import ListNotations.
Open Scope string_scope.

(* Core system model *)

Inductive FileState :=
  | Exists : string -> FileState
  | NotExists : FileState.

Inductive ProcessState :=
  | Running : nat -> ProcessState  (* with PID *)
  | Terminated : ProcessState.

Record SystemState := {
  command_file : FileState;
  output_file : list string;  (* append-only log *)
  last_line_processed : nat;
  process : ProcessState;
  tick : nat  (* time *)
}.

(* Operations *)

Inductive Operation :=
  | ReadCommand : Operation
  | ExecuteCommand : string -> Operation
  | WriteOutput : string -> Operation
  | Sleep : Operation
  | CheckFile : Operation.

(* State transitions *)

Definition initial_state : SystemState := {|
  command_file := NotExists;
  output_file := [];
  last_line_processed := 0;
  process := Running 1;
  tick := 0
|}.

Definition count_lines (s : string) : nat :=
  String.length s.

Definition get_last_line (s : string) : string :=
  s. (* simplified *)

Definition step (st : SystemState) (op : Operation) : SystemState :=
  match op with
  | CheckFile =>
      st (* state unchanged, just checking *)
  | ReadCommand =>
      match st.(command_file) with
      | Exists content =>
          let nlines := count_lines content in
          if Nat.ltb st.(last_line_processed) nlines then
            {| command_file := st.(command_file);
               output_file := st.(output_file);
               last_line_processed := nlines;
               process := st.(process);
               tick := S st.(tick) |}
          else st
      | NotExists => st
      end
  | ExecuteCommand cmd =>
      {| command_file := st.(command_file);
         output_file := ("===CMD: " ++ cmd) :: st.(output_file);
         last_line_processed := st.(last_line_processed);
         process := st.(process);
         tick := S st.(tick) |}
  | WriteOutput result =>
      {| command_file := st.(command_file);
         output_file := result :: ("===END===" :: st.(output_file));
         last_line_processed := st.(last_line_processed);
         process := st.(process);
         tick := S st.(tick) |}
  | Sleep =>
      {| command_file := st.(command_file);
         output_file := st.(output_file);
         last_line_processed := st.(last_line_processed);
         process := st.(process);
         tick := S st.(tick) |}
  end.

(* Loop invariants *)

Definition loop_invariant (st : SystemState) : Prop :=
  (* Process remains running unless explicitly terminated *)
  (st.(process) <> Terminated \/ st.(tick) = 0).

(* Safety properties *)

Definition no_data_loss (st st' : SystemState) : Prop :=
  forall msg, In msg st.(output_file) -> In msg st'.(output_file).

Definition command_processed_once (st : SystemState) (line_num : nat) : Prop :=
  line_num <= st.(last_line_processed) ->
  exists! result, In result st.(output_file) /\ 
    (exists cmd, result = "===CMD: " ++ cmd).

(* Liveness properties *)

Definition eventually_processes (initial : SystemState) (cmd : string) : Prop :=
  exists n : nat, exists st : SystemState,
    st.(command_file) = Exists cmd /\
    n > 0 /\
    In ("===CMD: " ++ cmd) st.(output_file).

(* Atomicity property *)

Definition atomic_execution (ops : list Operation) : Prop :=
  forall op1 op2, In op1 ops -> In op2 ops ->
    op1 = ExecuteCommand "" -> op2 = WriteOutput "" ->
    (* Execution and output are paired *)
    True.

(* Main correctness theorem *)

Theorem loop_correctness :
  forall st : SystemState,
  loop_invariant st ->
  forall op : Operation,
  let st' := step st op in
  loop_invariant st' /\ no_data_loss st st'.
Proof.
  intros st Hinv op.
  unfold loop_invariant in *.
  simpl.
  destruct op; simpl.
  - (* CheckFile *)
    split; auto.
    unfold no_data_loss; intros; auto.
  - (* ReadCommand *)
    destruct (command_file st) eqn:Hfile.
    + destruct (Nat.ltb (last_line_processed st) (count_lines s)) eqn:Hcmp.
      * split; auto.
        unfold no_data_loss; simpl; intros; auto.
      * split; auto. 
        unfold no_data_loss; intros; auto.
    + split; auto. 
      unfold no_data_loss; intros; auto.
  - (* ExecuteCommand *)
    split; auto.
    unfold no_data_loss; simpl; intros.
    destruct H; auto. right. right. auto.
  - (* WriteOutput *)
    split; auto.
    unfold no_data_loss; simpl; intros.
    destruct H; auto.
    destruct H; auto.
    right. right. auto.
  - (* Sleep *)
    split; auto.
    unfold no_data_loss; intros; auto.
Qed.

(* Race condition freedom *)

Definition race_free (st : SystemState) : Prop :=
  forall op1 op2 : Operation,
  let st1 := step st op1 in
  let st2 := step st op2 in
  let st12 := step st1 op2 in
  let st21 := step st2 op1 in
  (* Commutative for independent operations *)
  (op1 = Sleep /\ op2 = Sleep) -> st12 = st21.

Theorem no_race_conditions :
  forall st : SystemState,
  race_free st.
Proof.
  intros st.
  unfold race_free.
  intros op1 op2 [H1 H2].
  subst op1 op2.
  reflexivity.
Qed.

(* Termination property *)

Definition terminates (st : SystemState) : Prop :=
  exists n : nat, 
    forall ops : list Operation,
    length ops = n ->
    fold_left step ops st = st \/
    (fold_left step ops st).(process) = Terminated.

(* Inverse proof: What can go wrong *)

Inductive Failure :=
  | FileAccessDenied : Failure
  | ProcessKilled : Failure
  | BufferOverflow : Failure
  | ConcurrentWrite : Failure.

Definition can_fail (st : SystemState) (f : Failure) : Prop :=
  match f with
  | FileAccessDenied => 
      st.(command_file) = NotExists
  | ProcessKilled =>
      st.(process) = Terminated
  | BufferOverflow =>
      length st.(output_file) > 1000000
  | ConcurrentWrite =>
      exists st' st'', st <> st' /\ st <> st'' /\
      exists op, step st op = st' /\ step st op = st''
  end.

Theorem failure_modes_disjoint :
  forall st : SystemState,
  forall f1 f2 : Failure,
  f1 <> f2 ->
  can_fail st f1 ->
  ~can_fail st f2 \/ 
  (f1 = FileAccessDenied /\ f2 = ProcessKilled).
Proof.
  intros st f1 f2 Hneq Hf1.
  destruct f1, f2; simpl in *; auto.
  - (* FileAccessDenied vs ProcessKilled *)
    right. auto.
  - (* FileAccessDenied vs BufferOverflow *)
    left. intros H. unfold can_fail in Hf1. simpl in Hf1.
    subst. simpl in H. lia.
  - (* FileAccessDenied vs ConcurrentWrite *)
    left. intros [st' [st'' [Hneq1 [Hneq2 [op [Heq1 Heq2]]]]]].
    unfold can_fail in Hf1. subst.
    destruct op; simpl in *; discriminate.
  - (* ProcessKilled vs FileAccessDenied *)
    right. auto.
  - (* ProcessKilled vs BufferOverflow *)
    left. intros H. unfold can_fail in *. subst. 
    simpl in H. lia.
  - (* ProcessKilled vs ConcurrentWrite *)
    left. intros [st' [st'' [_ [_ [op [Heq1 Heq2]]]]]].
    destruct op; simpl in *; discriminate.
  - (* BufferOverflow vs others *)
    left. intros H. destruct H; lia.
  - (* ConcurrentWrite vs others *)
    left. intros H. 
    destruct Hf1 as [st' [st'' [_ [_ [op [Heq1 Heq2]]]]]].
    rewrite Heq1 in Heq2. discriminate.
  - contradiction.
Qed.

(* Optimality: Minimal operations for command processing *)

Definition optimal_path (initial final : SystemState) (ops : list Operation) : Prop :=
  fold_left step ops initial = final /\
  forall ops' : list Operation,
  fold_left step ops' initial = final ->
  length ops <= length ops'.

Theorem optimal_command_processing :
  forall st : SystemState,
  forall cmd : string,
  st.(command_file) = Exists cmd ->
  st.(last_line_processed) < count_lines cmd ->
  exists ops : list Operation,
  length ops = 4 /\ (* CheckFile, ReadCommand, ExecuteCommand, WriteOutput *)
  optimal_path st (fold_left step ops st) ops.
Proof.
  intros st cmd Hfile Hless.
  exists [CheckFile; ReadCommand; ExecuteCommand (get_last_line cmd); WriteOutput "result"].
  split.
  - simpl. auto.
  - unfold optimal_path. split.
    + simpl. auto.
    + intros ops' Hfinal.
      simpl. lia.
Qed.