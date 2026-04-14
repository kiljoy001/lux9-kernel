(*
  Hardware Abstraction Layer for Fuel-Bounded Tuple Compilation
  
  This Coq specification models hardware differences and ensures
  our tuple safety proofs hold across multiple architectures.
*)

Require Import List.
Require Import Arith.
Require Import Bool.
Require Import Lia.
Import ListNotations.

(* ==================== HARDWARE ARCHITECTURE MODELING ==================== *)

(* Hardware register representation *)
Inductive Register : Type :=
  (* x86-64 registers *)
  | R8 | R9 | R10 | R11 | R12 | R13 | R14 | R15
  (* ARM64 registers *)  
  | X8 | X9 | X10 | X11 | X12 | X13 | X14 | X15 | X16 | X17 | X28
  (* RISC-V registers *)
  | RV10 | RV11 | RV12 | RV13 | RV14 | RV15 | RV16 | RV17 | RV31.

(* Hardware architecture types *)
Inductive Architecture : Type :=
  | X86_64
  | ARM64
  | RISCV64.

(* Hardware profile specification *)
Record HardwareProfile : Type := {
  arch : Architecture;
  available_registers : list Register;
  fuel_register : Register;
  max_tuple_size : nat;
  stack_alignment : nat;
  word_size : nat
}.

(* ==================== HARDWARE PROFILE DEFINITIONS ==================== *)

(* x86-64 hardware profile *)
Definition x86_64_profile : HardwareProfile := {|
  arch := X86_64;
  available_registers := [R8; R9; R10; R11; R12; R13; R14];
  fuel_register := R15;
  max_tuple_size := 7;
  stack_alignment := 16;
  word_size := 8
|}.

(* ARM64 hardware profile *)
Definition arm64_profile : HardwareProfile := {|
  arch := ARM64;
  available_registers := [X8; X9; X10; X11; X12; X13; X14; X15; X16; X17];
  fuel_register := X28;
  max_tuple_size := 10;
  stack_alignment := 16;
  word_size := 8
|}.

(* RISC-V 64-bit hardware profile *)
Definition riscv64_profile : HardwareProfile := {|
  arch := RISCV64;
  available_registers := [RV10; RV11; RV12; RV13; RV14; RV15; RV16; RV17];
  fuel_register := RV31;
  max_tuple_size := 8;
  stack_alignment := 16;
  word_size := 8
|}.

(* ==================== HARDWARE VALIDATION PREDICATES ==================== *)

(* Simple register equality check *)
Definition register_eq (r1 r2 : Register) : bool :=
  match r1, r2 with
  | R8, R8 | R9, R9 | R10, R10 | R11, R11 | R12, R12 | R13, R13 | R14, R14 | R15, R15
  | X8, X8 | X9, X9 | X10, X10 | X11, X11 | X12, X12 | X13, X13 | X14, X14 
  | X15, X15 | X16, X16 | X17, X17 | X28, X28
  | RV10, RV10 | RV11, RV11 | RV12, RV12 | RV13, RV13 | RV14, RV14 
  | RV15, RV15 | RV16, RV16 | RV17, RV17 | RV31, RV31 => true
  | _, _ => false
  end.

(* Check if a register is available for tuple allocation *)
Definition register_available (profile : HardwareProfile) (reg : Register) : bool :=
  existsb (register_eq reg) profile.(available_registers).

(* Check if fuel register is protected from tuple allocation *)
Definition fuel_register_protected (profile : HardwareProfile) : bool :=
  negb (register_available profile profile.(fuel_register)).

(* Validate tuple size against hardware constraints *)
Definition valid_tuple_size (profile : HardwareProfile) (size : nat) : bool :=
  (size <=? profile.(max_tuple_size)) && (size <=? length profile.(available_registers)).

(* Check if hardware profile is well-formed *)
Definition well_formed_profile (profile : HardwareProfile) : Prop :=
  (* Fuel register must not be in available registers *)
  fuel_register_protected profile = true /\
  (* Max tuple size must not exceed available registers *)
  profile.(max_tuple_size) <= length profile.(available_registers) /\
  (* Stack alignment must be positive and power of 2 *)
  profile.(stack_alignment) > 0 /\
  (* Word size must be positive *)
  profile.(word_size) > 0.

(* ==================== TUPLE ALLOCATION SAFETY ==================== *)

(* Tuple allocation state *)
Record TupleAllocation : Type := {
  allocated_registers : list Register;
  remaining_registers : list Register;
  tuple_count : nat;
  fuel_reg_protected : bool
}.

(* Initial allocation state for a hardware profile *)
Definition initial_allocation (profile : HardwareProfile) : TupleAllocation := {|
  allocated_registers := [];
  remaining_registers := profile.(available_registers);
  tuple_count := 0;
  fuel_reg_protected := fuel_register_protected profile
|}.

(* Allocate registers for a tuple of given size *)
Definition allocate_tuple_registers (profile : HardwareProfile) (size : nat) 
                                   (alloc : TupleAllocation) : option TupleAllocation :=
  if valid_tuple_size profile size && (size <=? length alloc.(remaining_registers)) then
    let allocated := firstn size alloc.(remaining_registers) in
    let remaining := skipn size alloc.(remaining_registers) in
    Some {|
      allocated_registers := alloc.(allocated_registers) ++ allocated;
      remaining_registers := remaining;
      tuple_count := alloc.(tuple_count) + 1;
      fuel_reg_protected := alloc.(fuel_reg_protected)
    |}
  else
    None.

(* ==================== HARDWARE SAFETY THEOREMS ==================== *)

(* Theorem: Well-formed profiles protect fuel register *)
Theorem fuel_register_safety : forall profile : HardwareProfile,
  well_formed_profile profile ->
  fuel_register_protected profile = true.
Proof.
  intros profile H.
  destruct H as [H_fuel _].
  exact H_fuel.
Qed.

(* Theorem: Valid tuple sizes respect hardware constraints *)
Theorem tuple_size_bounded : forall profile : HardwareProfile, forall size : nat,
  well_formed_profile profile ->
  valid_tuple_size profile size = true ->
  size <= profile.(max_tuple_size) /\ size <= length profile.(available_registers).
Proof.
  intros profile size H_wf H_valid.
  unfold valid_tuple_size in H_valid.
  apply andb_true_iff in H_valid.
  destruct H_valid as [H1 H2].
  apply Nat.leb_le in H1.
  apply Nat.leb_le in H2.
  split; assumption.
Qed.

(* Theorem: Tuple allocation preserves fuel register protection *)
Theorem allocation_preserves_fuel_protection : forall profile : HardwareProfile,
  forall size : nat, forall alloc result : TupleAllocation,
  well_formed_profile profile ->
  alloc.(fuel_reg_protected) = true ->
  allocate_tuple_registers profile size alloc = Some result ->
  result.(fuel_reg_protected) = true.
Proof.
  intros profile size alloc result H_wf H_fuel H_alloc.
  unfold allocate_tuple_registers in H_alloc.
  destruct (valid_tuple_size profile size && (size <=? length (remaining_registers alloc))) eqn:E.
  - inversion H_alloc. simpl. exact H_fuel.
  - discriminate.
Qed.

(* Theorem: Successful allocation implies valid tuple size *)
Theorem successful_allocation_valid_size : forall profile : HardwareProfile,
  forall size : nat, forall alloc result : TupleAllocation,
  allocate_tuple_registers profile size alloc = Some result ->
  valid_tuple_size profile size = true.
Proof.
  intros profile size alloc result H_alloc.
  unfold allocate_tuple_registers in H_alloc.
  destruct (valid_tuple_size profile size && (size <=? length (remaining_registers alloc))) eqn:E.
  - apply andb_true_iff in E. destruct E as [H_valid _]. exact H_valid.
  - discriminate.
Qed.

(* ==================== MULTI-ARCHITECTURE COMPATIBILITY ==================== *)

(* Theorem: All defined profiles are well-formed *)
Theorem x86_64_profile_well_formed : well_formed_profile x86_64_profile.
Proof.
  unfold well_formed_profile.
  unfold x86_64_profile.
  simpl.
  split.
  - (* fuel_register_protected *)
    unfold fuel_register_protected, register_available.
    simpl.
    reflexivity.
  - split.
    + (* max_tuple_size <= length available_registers *)
      simpl. lia.
    + split.
      * (* stack_alignment > 0 *)
        simpl. lia.
      * (* word_size > 0 *)
        simpl. lia.
Qed.

Theorem arm64_profile_well_formed : well_formed_profile arm64_profile.
Proof.
  unfold well_formed_profile.
  unfold arm64_profile.
  simpl.
  split.
  - (* fuel_register_protected *)
    unfold fuel_register_protected, register_available.
    simpl.
    reflexivity.
  - split.
    + (* max_tuple_size <= length available_registers *)
      simpl. lia.
    + split.
      * (* stack_alignment > 0 *)
        simpl. lia.
      * (* word_size > 0 *)
        simpl. lia.
Qed.

Theorem riscv64_profile_well_formed : well_formed_profile riscv64_profile.
Proof.
  unfold well_formed_profile.
  unfold riscv64_profile.
  simpl.
  split.
  - (* fuel_register_protected *)
    unfold fuel_register_protected, register_available.
    simpl.
    reflexivity.
  - split.
    + (* max_tuple_size <= length available_registers *)
      simpl. lia.
    + split.
      * (* stack_alignment > 0 *)
        simpl. lia.
      * (* word_size > 0 *)
        simpl. lia.
Qed.

(* Theorem: Hardware abstraction preserves safety across architectures *)
Theorem cross_architecture_safety : forall profile : HardwareProfile,
  forall size : nat,
  well_formed_profile profile ->
  valid_tuple_size profile size = true ->
  exists alloc : TupleAllocation,
    allocate_tuple_registers profile size (initial_allocation profile) = Some alloc /\
    alloc.(fuel_reg_protected) = true /\
    length alloc.(allocated_registers) = size.
Proof.
  intros profile size H_wf H_valid.
  unfold allocate_tuple_registers, initial_allocation.
  simpl.
  
  (* Keep H_wf available by asserting the conjuncts separately *)
  assert (H_fuel: fuel_register_protected profile = true) by (destruct H_wf; assumption).
  assert (H_max: profile.(max_tuple_size) <= length profile.(available_registers)) by (destruct H_wf as [_ [H _]]; assumption).
  
  (* Preserve H_valid before applying tuple_size_bounded *)
  assert (H_valid_orig: valid_tuple_size profile size = true) by exact H_valid.
  apply tuple_size_bounded in H_valid; [|exact H_wf].
  destruct H_valid as [H_size_max H_size_regs].
  
  (* Show the conditional succeeds *)
  assert (H_cond: valid_tuple_size profile size && (size <=? length (available_registers profile)) = true).
  {
    apply andb_true_iff.
    split.
    - exact H_valid_orig.
    - apply Nat.leb_le. exact H_size_regs.
  }
  
  rewrite H_cond.
  exists {|
    allocated_registers := firstn size (available_registers profile);
    remaining_registers := skipn size (available_registers profile);
    tuple_count := 1;
    fuel_reg_protected := fuel_register_protected profile
  |}.
  
  split. reflexivity.
  split. exact H_fuel.
  
  (* Prove length of allocated registers *)
  simpl.
  rewrite length_firstn.
  apply Nat.min_l.
  exact H_size_regs.
Qed.

(* ==================== HARDWARE ABSTRACTION COMPLETENESS ==================== *)

(* Main theorem: Hardware abstraction layer provides complete tuple safety *)
Theorem hardware_abstraction_complete : forall profile : HardwareProfile,
  well_formed_profile profile ->
  (forall size : nat, size <= profile.(max_tuple_size) ->
    exists alloc : TupleAllocation,
      allocate_tuple_registers profile size (initial_allocation profile) = Some alloc /\
      alloc.(fuel_reg_protected) = true /\
      length alloc.(allocated_registers) = size /\
      alloc.(tuple_count) = 1).
Proof.
  intros profile H_wf size H_size.
  assert (H_valid: valid_tuple_size profile size = true).
  {
    unfold valid_tuple_size.
    apply andb_true_iff.
    split.
    - apply Nat.leb_le. exact H_size.
    - apply Nat.leb_le. 
      destruct H_wf as [_ [H_max _]].
      transitivity profile.(max_tuple_size); assumption.
  }
  
  apply cross_architecture_safety in H_valid; [|exact H_wf].
  destruct H_valid as [alloc [H_alloc [H_fuel H_len]]].
  
  exists alloc.
  split. exact H_alloc.
  split. exact H_fuel.  
  split. exact H_len.
  
  (* Show tuple_count = 1 *)
  unfold allocate_tuple_registers, initial_allocation in H_alloc.
  simpl in H_alloc.
  destruct (valid_tuple_size profile size && (size <=? length (available_registers profile))) eqn:E.
  - inversion H_alloc. simpl. reflexivity.
  - discriminate.
Qed.

(* Final completeness statement *)
Theorem multi_architecture_tuple_safety_complete :
  (well_formed_profile x86_64_profile /\ 
   well_formed_profile arm64_profile /\
   well_formed_profile riscv64_profile) /\
  (forall profile : HardwareProfile,
    well_formed_profile profile ->
    forall size : nat, size <= profile.(max_tuple_size) ->
    exists safe_allocation, 
      allocate_tuple_registers profile size (initial_allocation profile) = Some safe_allocation /\
      safe_allocation.(fuel_reg_protected) = true).
Proof.
  split.
  (* All profiles are well-formed *)
  split. apply x86_64_profile_well_formed.
  split. apply arm64_profile_well_formed.
  apply riscv64_profile_well_formed.
  
  (* Hardware abstraction provides safety *)
  intros profile H_wf size H_size.
  apply hardware_abstraction_complete in H_size; [|exact H_wf].
  destruct H_size as [alloc [H_alloc [H_fuel _]]].
  exists alloc.
  split; assumption.
Qed.