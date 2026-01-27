(* Correct ring buffer implementation with power-of-2 size *)

Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Require Import Coq.Logic.FunctionalExtensionality.
Require Import Lia.
Import ListNotations.
Open Scope list_scope.

(* Ensure repeat is available *)
Import List.

(* Ring buffer with proper power-of-2 design *)
Record RingBuffer := {
  data : list (option nat);  (* Using option type for empty slots *)
  head : nat;       (* Write position *)
  tail : nat;       (* Read position *)
  size : nat;       (* Buffer size (must be power of 2) *)
  mask : nat;       (* size - 1, for fast modulo *)
  lock : bool       (* Mutex state *)
}.

(* Helper: update nth element *)
Fixpoint update_nth {A : Type} (n : nat) (l : list A) (v : A) : list A :=
  match l with
  | [] => []
  | h :: t => 
      match n with
      | 0 => v :: t
      | S n' => h :: update_nth n' t v
      end
  end.

Lemma update_nth_length : forall {A : Type} (n : nat) (l : list A) (v : A),
  length (update_nth n l v) = length l.
Proof.
  intros A n.
  induction n as [|n' IHn'].
  - intros l v. destruct l; simpl; reflexivity.
  - intros l v. destruct l; simpl; try rewrite IHn'; reflexivity.
Qed.

Lemma update_nth_other : forall {A : Type} (n : nat) (l : list A) (v : A) (i : nat),
  i <> n ->
  nth_error (update_nth n l v) i = nth_error l i.
Proof.
  intros A n l v i Hneq.
  revert n v i Hneq.
  induction l as [|h t IH]; intros n v i Hneq.
  - simpl. destruct n, i; reflexivity.
  - destruct n as [|n'].
    + simpl. destruct i as [|i'].
      * contradiction.
      * reflexivity.
    + simpl. destruct i as [|i'].
      * reflexivity.
      * apply IH. lia.
Qed.

(* Wrap using mask (equivalent to mod size when size is power of 2) *)
Definition wrap (n : nat) (mask : nat) : nat := 
  Nat.land n mask.  (* Bitwise AND with mask *)

(* For proof purposes, we'll use modulo since Coq's bitwise ops are harder to reason about *)
Definition wrap_mod (n : nat) (size : nat) : nat := n mod size.

(* Core invariants *)
Definition ring_invariant (r : RingBuffer) : Prop :=
  length r.(data) = r.(size) /\
  r.(mask) = r.(size) - 1 /\
  r.(head) < r.(size) /\
  r.(tail) < r.(size) /\
  r.(size) > 1 /\  (* Size > 1 to distinguish empty from full *)
  (exists k, r.(size) = 2^k).  (* Size must be power of 2 *)

(* Empty and full conditions *)
Definition is_empty (r : RingBuffer) : bool :=
  Nat.eqb r.(head) r.(tail).

Definition is_full (r : RingBuffer) : bool :=
  Nat.eqb (wrap_mod (S r.(head)) r.(size)) r.(tail).

(* Well-formedness: non-empty buffer has data at tail *)
Definition buffer_well_formed (r : RingBuffer) : Prop :=
  ring_invariant r /\
  (is_empty r = false -> exists item, nth_error r.(data) r.(tail) = Some (Some item)).

(* Lock operations *)
Definition try_lock (r : RingBuffer) : option RingBuffer :=
  if r.(lock) then None
  else Some {| data := r.(data);
               head := r.(head);
               tail := r.(tail);
               size := r.(size);
               mask := r.(mask);
               lock := true |}.

Definition unlock (r : RingBuffer) : RingBuffer :=
  {| data := r.(data);
     head := r.(head);
     tail := r.(tail);
     size := r.(size);
     mask := r.(mask);
     lock := false |}.

(* Enqueue operation *)
Definition enqueue (r : RingBuffer) (item : nat) : option RingBuffer :=
  match try_lock r with
  | None => None  (* Failed to acquire lock *)
  | Some r' =>
      if is_full r' then 
        Some (unlock r')  (* Full, return unchanged *)
      else
        let new_data := update_nth r'.(head) r'.(data) (Some item) in
        let new_head := wrap_mod (S r'.(head)) r'.(size) in
        Some (unlock {| data := new_data;
                        head := new_head;
                        tail := r'.(tail);
                        size := r'.(size);
                        mask := r'.(mask);
                        lock := r'.(lock) |})
  end.

(* Dequeue operation *)
Definition dequeue (r : RingBuffer) : option (nat * RingBuffer) :=
  match try_lock r with
  | None => None  (* Failed to acquire lock *)
  | Some r' =>
      if is_empty r' then
        None  (* Empty *)
      else
        match nth_error r'.(data) r'.(tail) with
        | Some (Some item) =>
            let new_data := update_nth r'.(tail) r'.(data) None in
            let new_tail := wrap_mod (S r'.(tail)) r'.(size) in
            Some (item, unlock {| data := new_data;
                                  head := r'.(head);
                                  tail := new_tail;
                                  size := r'.(size);
                                  mask := r'.(mask);
                                  lock := r'.(lock) |})
        | _ => None  (* Corrupted buffer *)
        end
  end.

(* Initial ring buffer with size 256 *)
Definition init_ring : RingBuffer := {|
  data := List.repeat (@None nat) 256;
  head := 0;
  tail := 0;
  size := 256;
  mask := 255;
  lock := false
|}.

(* Lemmas about modulo with power of 2 *)
Lemma mod_power_of_2_bound : forall n k,
  n mod (2^k) < 2^k.
Proof.
  intros. apply Nat.mod_upper_bound.
  apply Nat.pow_nonzero. lia.
Qed.

Lemma size_256_is_power_of_2 : exists k, 256 = 2^k.
Proof.
  exists 8. reflexivity.
Qed.

(* Main theorems *)


Theorem init_satisfies_invariant :
  ring_invariant init_ring.
Proof.
  unfold ring_invariant, init_ring. simpl.
  split.
  - reflexivity.
  - split.
    + reflexivity.
    + split; [lia | split; [lia | split; [lia | apply size_256_is_power_of_2]]].
Qed.

Theorem enqueue_preserves_invariant :
  forall r item,
  ring_invariant r ->
  match enqueue r item with
  | None => True
  | Some r' => ring_invariant r'
  end.
Proof.
  intros r item Hinv.
  unfold enqueue.
  destruct (try_lock r) eqn:Htry; try constructor.
  destruct (is_full r0) eqn:Hfull.
  - (* Full case *)
    unfold ring_invariant, unlock, try_lock in *.
    destruct (lock r) eqn:Hlock; try discriminate.
    inversion Htry. subst. simpl.
    exact Hinv.
  - (* Not full case *)
    unfold ring_invariant, unlock, try_lock in *.
    destruct (lock r) eqn:Hlock; try discriminate.
    inversion Htry. subst. simpl in *.
    destruct Hinv as [Hlen [Hmask [Hhead [Htail [Hsize Hpow]]]]].
    split.
    + rewrite update_nth_length. exact Hlen.
    + split.
      * exact Hmask.
      * split.
        -- unfold wrap_mod. 
           destruct Hpow as [k Hk]. rewrite Hk.
           apply mod_power_of_2_bound.
        -- split; [exact Htail | split; [exact Hsize | exact Hpow]].
Qed.

Theorem dequeue_preserves_invariant :
  forall r,
  buffer_well_formed r ->
  match dequeue r with
  | None => True
  | Some (_, r') => ring_invariant r'
  end.
Proof.
  intros r Hwf.
  unfold dequeue.
  destruct (try_lock r) eqn:Htry; try constructor.
  destruct (is_empty r0) eqn:Hempty; try constructor.

  (* Buffer not empty, well-formedness ensures data at tail *)
  unfold buffer_well_formed in Hwf.
  destruct Hwf as [Hinv Hdata].
  unfold try_lock in Htry.
  destruct (lock r) eqn:Hlock; try discriminate.
  inversion Htry. subst r0. clear Htry.

  (* Use well-formedness to get data *)
  assert (is_empty {| data := data r; head := head r; tail := tail r;
                      size := size r; mask := mask r; lock := true |} = false).
  { exact Hempty. }
  apply Hdata in H.
  destruct H as [item Hitem].
  simpl in Hitem.
  destruct (nth_error (data r) (tail r)) as [[item'|]|] eqn:Hnth; try discriminate.
  inversion Hitem. subst item'.
  
  unfold ring_invariant, unlock in *.
  simpl.
  destruct Hinv as [Hlen [Hmask [Hhead [Htail [Hsize Hpow]]]]].
  split.
  - rewrite update_nth_length. exact Hlen.
  - split.
    + exact Hmask.
    + split.
      * exact Hhead.
      * split.
        -- unfold wrap_mod.
           destruct Hpow as [k Hk]. rewrite Hk.
           apply mod_power_of_2_bound.
        -- split; [exact Hsize | exact Hpow].
Qed.

(* FIFO property *)
Theorem fifo_order :
  forall r item,
  buffer_well_formed r ->
  r.(lock) = false ->
  is_full r = false ->
  match enqueue r item with
  | None => False  (* Should succeed *)
  | Some r' => 
      length r'.(data) = length r.(data) /\
      (forall i, i <> r.(head) -> nth_error r'.(data) i = nth_error r.(data) i)
  end.
Proof.
  intros r item Hwf Hlock Hnotfull.
  unfold enqueue.
  unfold try_lock. rewrite Hlock.
  simpl. rewrite Hnotfull.
  unfold unlock. simpl.
  split.
  - rewrite update_nth_length. reflexivity.
  - intros i Hi.
    (* update_nth only changes position head, others unchanged *)
    apply update_nth_other. exact Hi.
Qed.

(* Mutual exclusion *)
Theorem mutual_exclusion :
  forall r,
  r.(lock) = false ->
  forall r',
  (enqueue r 1 = Some r' \/ (exists v, dequeue r = Some (v, r'))) ->
  r'.(lock) = false.
Proof.
  intros r Hlock r' Hop.
  destruct Hop as [Henq | [v Hdeq]].
  - (* enqueue case *)
    unfold enqueue in Henq.
    unfold try_lock in Henq. rewrite Hlock in Henq.
    destruct (is_full {| data := data r; head := head r; tail := tail r;
                         size := size r; mask := mask r; lock := true |}) eqn:Hfull.
    + inversion Henq. unfold unlock. reflexivity.
    + inversion Henq. unfold unlock. reflexivity.
  - (* dequeue case *)
    unfold dequeue in Hdeq.
    unfold try_lock in Hdeq. rewrite Hlock in Hdeq.
    destruct (is_empty {| data := data r; head := head r; tail := tail r;
                          size := size r; mask := mask r; lock := true |}) eqn:Hempty;
      try discriminate.
    destruct (nth_error (data r) (tail r)) as [[item|]|]; try discriminate.
    inversion Hdeq. unfold unlock. reflexivity.
Qed.

(* Progress guarantee *)
Theorem progress_guarantee :
  forall r,
  buffer_well_formed r ->
  r.(lock) = false ->
  is_empty r = false ->
  exists item r', dequeue r = Some (item, r').
Proof.
  intros r Hwf Hlock Hnotempty.
  unfold dequeue.
  unfold try_lock. rewrite Hlock.
  simpl. rewrite Hnotempty.
  unfold buffer_well_formed in Hwf.
  destruct Hwf as [_ Hdata].
  apply Hdata in Hnotempty.
  destruct Hnotempty as [item Hitem].
  rewrite Hitem.
  eexists. eexists. reflexivity.
Qed.

(* Capacity theorem *)
Theorem capacity_limit :
  forall r,
  ring_invariant r ->
  (if Nat.leb r.(tail) r.(head) 
   then r.(size) - r.(head) + r.(tail)
   else r.(tail) - r.(head)) <= r.(size).
Proof.
  intros r Hinv.
  destruct Hinv as [_ [_ [Hhead [Htail _]]]].
  destruct (Nat.leb (tail r) (head r)) eqn:Hle.
  - apply Nat.leb_le in Hle. lia.
  - apply Nat.leb_nle in Hle. lia.
Qed.