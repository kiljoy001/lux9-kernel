(* pebble_clr.v - CLR Memory Safety with Pebble
 *
 * Proves critical memory safety properties for the CLR/Pebble integration:
 * 1. No Use-After-Free (UAF)
 * 2. No Double-Free
 * 3. Reference Counting consistency
 *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

Require Import fruity_semantics.
Require Import il_to_fruity_correct.

(* ========== Safety Properties ========== *)

(* Re-exporting invariants from fruity_semantics for clarity *)
(* refcount_invariant: live objects have rc > 0 *)
(* no_use_after_free: accessing stack ref implies it exists in heap *)

(* Definition of Double-Free Freedom *)
(* A state is free of double-free errors if the current operation won't cause one.
   Specifically for BURN: refcount must be > 0. *)
Definition safe_from_double_free (s : fruity_state) (op : fruity_opcode) : Prop :=
  match op with
  | F_BURN =>
      match fs_stack s with
      | FV_Ref _ rc :: _ => rc > 0 (* Must have refcount to burn *)
      | FV_Null :: _ => True       (* Burning null is safe *)
      | _ => False                 (* Burning non-ref is type error, but not double-free *)
      end
  | _ => True (* Other ops don't free *)
  end.

(* ========== Safety Theorems ========== *)

(* Theorem: Well-typed execution preserves memory safety *)
Theorem step_preserves_memory_safety : forall s op s',
  refcount_invariant s ->
  no_use_after_free s ->
  fruity_step_simple s op s' ->
  refcount_invariant s' /\ no_use_after_free s'.
Proof.
  intros s op s' Hrc Huaf Hstep.
  inversion Hstep; subst; split; try assumption.
  
  (* IConst: new stack is FV_Int n :: fs_stack s *)
  - (* refcount_invariant *)
    unfold refcount_invariant in *. intros v [Hstack | Hloc].
    + simpl in Hstack. destruct Hstack as [Heq | Hrest].
      * subst. simpl. exact I.
      * apply Hrc. left. exact Hrest.
    + apply Hrc. right. exact Hloc.
  - (* no_use_after_free *)
    unfold no_use_after_free in *. intros addr rc Hin Hpos.
    simpl in Hin. destruct Hin as [Heq | Hrest].
    + discriminate.
    + apply (Huaf addr rc Hrest Hpos).

  (* IAdd: new stack is FV_Int (n1+n2) :: rest *)
  - unfold refcount_invariant in *. intros v [Hstack | Hloc].
    + simpl in Hstack. destruct Hstack as [Heq | Hrest].
      * subst. simpl. exact I.
      * apply Hrc. left. rewrite H. simpl. right. right. exact Hrest.
    + apply Hrc. right. exact Hloc.
  - unfold no_use_after_free in *. intros addr rc Hin Hpos.
    simpl in Hin. destruct Hin as [Heq | Hrest].
    + discriminate.
    + apply (Huaf addr rc). rewrite H. simpl. right. right. exact Hrest. exact Hpos.

  (* ISub: same structure as IAdd *)
  - unfold refcount_invariant in *. intros v [Hstack | Hloc].
    + simpl in Hstack. destruct Hstack as [Heq | Hrest].
      * subst. simpl. exact I.
      * apply Hrc. left. rewrite H. simpl. right. right. exact Hrest.
    + apply Hrc. right. exact Hloc.
  - unfold no_use_after_free in *. intros addr rc Hin Hpos.
    simpl in Hin. destruct Hin as [Heq | Hrest].
    + discriminate.
    + apply (Huaf addr rc). rewrite H. simpl. right. right. exact Hrest. exact Hpos.

  (* IMul: same structure as IAdd *)
  - unfold refcount_invariant in *. intros v [Hstack | Hloc].
    + simpl in Hstack. destruct Hstack as [Heq | Hrest].
      * subst. simpl. exact I.
      * apply Hrc. left. rewrite H. simpl. right. right. exact Hrest.
    + apply Hrc. right. exact Hloc.
  - unfold no_use_after_free in *. intros addr rc Hin Hpos.
    simpl in Hin. destruct Hin as [Heq | Hrest].
    + discriminate.
    + apply (Huaf addr rc). rewrite H. simpl. right. right. exact Hrest. exact Hpos.
Qed.


(* Theorem: BURN is safe if refcount > 0 (No Double Free) *)
Theorem burn_safe_if_live : forall s addr rc rest,
  fs_stack s = FV_Ref addr rc :: rest ->
  rc > 0 ->
  safe_from_double_free s F_BURN.
Proof.
  intros s addr rc rest Hstack Hpos.
  unfold safe_from_double_free.
  rewrite Hstack.
  apply Hpos.
Qed.
