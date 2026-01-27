Require Import Bool Arith List Lia.

(* Coq proof: Correct VMX cleanup protocol ensures device availability *)

(* VMX file descriptor state *)
Inductive fd_state : Type :=
  | FD_CLOSED : fd_state
  | FD_OPEN   : fd_state.

(* VMX instance state *)  
Inductive vmx_state : Type :=
  | VMX_INIT    : vmx_state
  | VMX_READY   : vmx_state  
  | VMX_RUNNING : vmx_state
  | VMX_DEAD    : vmx_state
  | VMX_ENDING  : vmx_state.

(* VMX system configuration *)
Record vmx_system := {
  max_instances : nat;
  active_count  : nat; 
  available_memory : nat;
  ctlfd_state  : fd_state;
  regsfd_state : fd_state;
  mapfd_state  : fd_state;
  waitfd_state : fd_state
}.

(* Helper predicates *)
Definition all_fds_closed (sys : vmx_system) : Prop :=
  sys.(ctlfd_state) = FD_CLOSED /\
  sys.(regsfd_state) = FD_CLOSED /\
  sys.(mapfd_state) = FD_CLOSED /\
  sys.(waitfd_state) = FD_CLOSED.

Definition has_free_slot (sys : vmx_system) : Prop :=
  sys.(active_count) < sys.(max_instances).

Definition has_memory (sys : vmx_system) : Prop :=
  sys.(available_memory) > 0.

(* VMX allocation precondition *)
Definition can_allocate (sys : vmx_system) : Prop :=
  has_free_slot sys /\ has_memory sys /\ all_fds_closed sys.

(* VMX cleanup operation *)  
Definition cleanup_vmx (sys : vmx_system) : vmx_system :=
  {| max_instances := sys.(max_instances);
     active_count := sys.(active_count) - 1;
     available_memory := sys.(available_memory) + 1;
     ctlfd_state := FD_CLOSED;
     regsfd_state := FD_CLOSED; 
     mapfd_state := FD_CLOSED;
     waitfd_state := FD_CLOSED |}.

(* VMX allocation operation *)
Definition allocate_vmx (sys : vmx_system) : vmx_system :=
  if (sys.(active_count) <? sys.(max_instances)) && 
     (0 <? sys.(available_memory)) &&
     (match (sys.(ctlfd_state), sys.(regsfd_state), sys.(mapfd_state), sys.(waitfd_state)) with
      | (FD_CLOSED, FD_CLOSED, FD_CLOSED, FD_CLOSED) => true
      | _ => false
      end)
  then
    {| max_instances := sys.(max_instances);
       active_count := sys.(active_count) + 1;
       available_memory := sys.(available_memory) - 1;
       ctlfd_state := FD_OPEN;
       regsfd_state := FD_OPEN;
       mapfd_state := FD_OPEN;
       waitfd_state := FD_OPEN |}
  else sys.

(* Key theorem: proper cleanup enables subsequent allocation *)
Theorem cleanup_enables_allocation :
  forall sys : vmx_system,
    sys.(active_count) > 0 ->
    sys.(active_count) <= sys.(max_instances) ->
    sys.(available_memory) >= 0 ->
    let cleaned_sys := cleanup_vmx sys in
    has_free_slot cleaned_sys /\ 
    has_memory cleaned_sys /\ 
    all_fds_closed cleaned_sys.
Proof.
  intros sys H_active H_max H_mem.
  unfold cleanup_vmx, has_free_slot, has_memory, all_fds_closed.
  simpl.
  split.
  - (* has_free_slot *)
    lia.
  - split.
    + (* has_memory *)
      lia.  
    + (* all_fds_closed *)
      auto.
Qed.

(* Corollary: cleanup guarantees successful subsequent allocation *)
Theorem cleanup_guarantees_allocation :
  forall sys : vmx_system,
    sys.(active_count) > 0 ->
    sys.(active_count) < sys.(max_instances) ->  
    sys.(available_memory) >= 0 ->
    let cleaned_sys := cleanup_vmx sys in
    can_allocate cleaned_sys.
Proof.
  intros sys H_active H_less_max H_mem.
  unfold can_allocate.
  apply cleanup_enables_allocation; lia.
Qed.

(* Critical invariant: allocation after proper cleanup never fails *)
Theorem allocation_after_cleanup_succeeds :
  forall sys : vmx_system,
    sys.(active_count) > 0 ->
    sys.(active_count) < sys.(max_instances) ->
    sys.(available_memory) >= 0 ->
    let cleaned_sys := cleanup_vmx sys in
    let new_sys := allocate_vmx cleaned_sys in
    new_sys.(active_count) = cleaned_sys.(active_count) + 1.
Proof.
  intros sys H_active H_less_max H_mem.
  unfold cleanup_vmx, allocate_vmx.
  simpl.
  (* The allocation succeeds because cleanup satisfies all preconditions *)
  destruct (Nat.ltb_spec (sys.(active_count) - 1) sys.(max_instances));
  destruct (Nat.ltb_spec 0 (sys.(available_memory) + 1)); 
  simpl; try lia.
Qed.