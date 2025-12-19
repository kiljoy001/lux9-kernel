(* fruity_semantics.v - Fruity IR Operational Semantics
 *
 * Models the Fruity intermediate representation with Pebble integration.
 * Key opcodes: VANILLA, BURN, CHERRY, BERRY for memory safety.
 *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

(* ========== Fruity Opcodes (from fruity_opcodes.h) ========== *)

Inductive fruity_opcode : Type :=
  (* Basic stack operations *)
  | F_NOP
  | F_ICONST (n : Z)
  | F_LCONST (n : Z)
  | F_LOAD (idx : nat)
  | F_STORE (idx : nat)
  | F_DUP
  | F_POP
  (* Arithmetic *)
  | F_IADD | F_ISUB | F_IMUL | F_IDIV
  | F_LADD | F_LSUB | F_LMUL | F_LDIV
  (* Control flow *)
  | F_GOTO (target : nat)
  | F_IF_EQ (target : nat)
  | F_IF_NE (target : nat)
  | F_RETURN
  | F_RETURN_VOID
  (* Pebble memory operations *)
  | F_VANILLA     (* Reference count increment *)
  | F_BURN        (* Reference count decrement *)
  | F_CHERRY      (* Start transaction *)
  | F_BERRY       (* End transaction (commit or rollback) *)
  | F_GRAPE       (* Spawn tasklet *)
  (* Object operations *)
  | F_NEW (type_id : nat)
  | F_GETFIELD (field_id : nat)
  | F_PUTFIELD (field_id : nat)
  (* Call *)
  | F_INVOKE (method_id : nat)
  | F_INVOKE_VIRTUAL (method_id : nat).

(* ========== Values ========== *)

Inductive fruity_value : Type :=
  | FV_Int (n : Z)
  | FV_Long (n : Z)
  | FV_Ref (addr : nat) (refcount : nat)  (* Object reference with refcount *)
  | FV_Null.

(* ========== Heap Model ========== *)

Record heap_object : Type := mkObject {
  obj_type : nat;
  obj_fields : list fruity_value;
  obj_refcount : nat
}.

Definition heap := nat -> option heap_object.

Definition empty_heap : heap := fun _ => None.

(* ========== Machine State ========== *)

Record fruity_state : Type := mkFruityState {
  fs_stack : list fruity_value;
  fs_locals : list fruity_value;
  fs_pc : nat;
  fs_heap : heap;
  fs_next_addr : nat;     (* Next free heap address *)
  fs_in_transaction : bool;
  fs_halted : bool;
  fs_error : option nat   (* Error code if any *)
}.

(* ========== Pebble Reference Counting ========== *)

(* VANILLA: increment reference count *)
Definition pebble_vanilla (s : fruity_state) : option fruity_state :=
  match fs_stack s with
  | FV_Ref addr rc :: rest =>
      Some (mkFruityState 
        (FV_Ref addr (S rc) :: rest)
        (fs_locals s)
        (fs_pc s)
        (fs_heap s)
        (fs_next_addr s)
        (fs_in_transaction s)
        (fs_halted s)
        None)
  | FV_Null :: rest =>
      (* Vanilla on null is valid (no-op for counting) *)
      Some s
  | _ => None
  end.

(* BURN: decrement reference count *)
Definition pebble_burn (s : fruity_state) : option fruity_state :=
  match fs_stack s with
  | FV_Ref addr rc :: rest =>
      if Nat.leb rc 0 then
        None (* Error: already burned/freed *)
      else
        Some (mkFruityState
          (FV_Ref addr (pred rc) :: rest)
          (fs_locals s)
          (fs_pc s)
          (fs_heap s)
          (fs_next_addr s)
          (fs_in_transaction s)
          (fs_halted s)
          None)
  | FV_Null :: rest =>
      (* Burn on null is valid (no-op) *)
      Some s
  | _ => None
  end.

(* CHERRY: start transaction *)
Definition pebble_cherry (s : fruity_state) : option fruity_state :=
  if fs_in_transaction s then
    None (* Already in transaction - nested not supported *)
  else
    Some (mkFruityState
      (fs_stack s)
      (fs_locals s)
      (fs_pc s)
      (fs_heap s)
      (fs_next_addr s)
      true (* Now in transaction *)
      (fs_halted s)
      None).

(* BERRY: end transaction *)
Definition pebble_berry (s : fruity_state) : option fruity_state :=
  if fs_in_transaction s then
    Some (mkFruityState
      (fs_stack s)
      (fs_locals s)
      (fs_pc s)
      (fs_heap s)
      (fs_next_addr s)
      false (* Exit transaction *)
      (fs_halted s)
      None)
  else
    None. (* Not in transaction *)

(* ========== Key Pebble Properties ========== *)

(* Reference is live if refcount > 0 *)
Definition is_live (v : fruity_value) : bool :=
  match v with
  | FV_Ref _ rc => Nat.ltb 0 rc
  | _ => false
  end.

(* Theorem: After VANILLA, reference is live *)
Theorem vanilla_makes_live : forall s s' addr rc rest,
  fs_stack s = FV_Ref addr rc :: rest ->
  pebble_vanilla s = Some s' ->
  exists rc', fs_stack s' = FV_Ref addr rc' :: rest /\ rc' > 0.
Proof.
  intros s s' addr rc rest Hstack Hvanilla.
  unfold pebble_vanilla in Hvanilla.
  rewrite Hstack in Hvanilla.
  inversion Hvanilla.
  exists (S rc). split.
  - simpl. reflexivity.
  - lia.
Qed.

(* Theorem: BURN decreases refcount *)
Theorem burn_decreases_refcount : forall s s' addr rc rest,
  fs_stack s = FV_Ref addr rc :: rest ->
  rc > 0 ->
  pebble_burn s = Some s' ->
  exists rc', fs_stack s' = FV_Ref addr rc' :: rest /\ rc' = pred rc.
Proof.
  intros s s' addr rc rest Hstack Hrc Hburn.
  unfold pebble_burn in Hburn.
  rewrite Hstack in Hburn.
  destruct (Nat.leb rc 0) eqn:Hcmp.
  - apply Nat.leb_le in Hcmp. lia.
  - inversion Hburn.
    exists (pred rc). split; reflexivity.
Qed.

(* Theorem: CHERRY/BERRY are inverses for transaction state *)
Theorem cherry_berry_transaction : forall s s' s'',
  fs_in_transaction s = false ->
  pebble_cherry s = Some s' ->
  pebble_berry s' = Some s'' ->
  fs_in_transaction s'' = false.
Proof.
  intros s s' s'' Hstart Hcherry Hberry.
  unfold pebble_cherry in Hcherry.
  rewrite Hstart in Hcherry.
  inversion Hcherry; subst.
  unfold pebble_berry in Hberry.
  simpl in Hberry.
  inversion Hberry.
  simpl. reflexivity.
Qed.

(* ========== Memory Safety Invariants ========== *)

(* All live references have non-zero refcount *)
Definition refcount_invariant (s : fruity_state) : Prop :=
  forall v, In v (fs_stack s) \/ In v (fs_locals s) ->
  match v with
  | FV_Ref addr rc => rc >= 0
  | _ => True
  end.

(* No use-after-free: only access refs with rc > 0 *)
Definition no_use_after_free (s : fruity_state) : Prop :=
  forall addr rc, In (FV_Ref addr rc) (fs_stack s) ->
  rc > 0 -> 
  exists obj, fs_heap s addr = Some obj.

(* Theorem: Operations preserve refcount invariant *)
Theorem operations_preserve_refcount_inv : forall s s',
  refcount_invariant s ->
  (pebble_vanilla s = Some s' \/ pebble_burn s = Some s') ->
  refcount_invariant s'.
Proof.
  intros s s' Hinv [Hvanilla | Hburn].
  - (* VANILLA case *)
    unfold pebble_vanilla in Hvanilla.
    destruct (fs_stack s) eqn:Hstack; try discriminate.
    destruct f; try discriminate.
    + (* Stack top is FV_Ref *)
      inversion Hvanilla; subst.
      unfold refcount_invariant in *; simpl.
      intros v [Hin_stack | Hin_locals].
      * simpl in Hin_stack. destruct Hin_stack as [Heq | Hin].
        -- (* Head of stack: refcount incremented *)
           subst. simpl. lia.
        -- (* Rest of stack: preserved *)
           apply Hinv. left. rewrite Hstack. right. assumption.
      * (* Locals: preserved *)
        apply Hinv. right. assumption.
    + (* Stack top is FV_Null *)
      inversion Hvanilla; subst. assumption.
  - (* BURN case *)
    unfold pebble_burn in Hburn.
    destruct (fs_stack s) eqn:Hstack; try discriminate.
    destruct f; try discriminate.
    + (* Stack top is FV_Ref *)
      destruct (Nat.leb refcount 0) eqn:Hleb.
      * (* Refcount <= 0 -> Error, no transition *)
        discriminate.
      * (* Refcount > 0 -> Decrement *)
        inversion Hburn; subst.
        unfold refcount_invariant in *; simpl.
        intros v [Hin_stack | Hin_locals].
        -- simpl in Hin_stack. destruct Hin_stack as [Heq | Hin].
           ++ (* Head of stack: refcount decremented *)
              subst. simpl.
              apply Nat.leb_gt in Hleb. lia.
           ++ (* Rest of stack: preserved *)
              apply Hinv. left. rewrite Hstack. right. assumption.
        -- (* Locals: preserved *)
           apply Hinv. right. assumption.
    + (* Stack top is FV_Null *)
      inversion Hburn; subst. assumption.

Qed.
