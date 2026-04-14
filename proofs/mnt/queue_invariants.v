(** * Queue Invariants - Safety properties for devmnt.c RPC queues
    *
    * Imports: types.v, tag.v, rpc.v, message_routing.v, channel_model.v
    * Proves: Queue integrity, ordering, and consistency properties
    *
    * Used by: Future devmnt_verified.v integration proof
    *
    * IMPLEMENTATION: kernel/9front-port/devmnt.c:1124-1158 (mountmux)
    *                 kernel/9front-port/devmnt.c:977-1027 (mountio)
    *                 kernel/include/portdat.h:300-309 (Mnt.queue)
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

Require Import mnt.types.
Require Import mnt.tag.
Require Import mnt.rpc.
Require Import mnt.message_routing.
Require Import mnt.channel_model.

Open Scope Z_scope.

(* ========================================================================= *)
(* CONSTANTS                                                                 *)
(* ========================================================================= *)

(** Maximum message size (from devmnt.c MAXRPC) *)
Definition MAXMSG : Z := 32768 + 24.  (* IOHDRSZ + data *)

(** Maximum concurrent RPCs (from devmnt.c) *)
Definition MAXRPC_COUNT : Z := 100.

(* ========================================================================= *)
(* QUEUE STATE MODEL                                                         *)
(* ========================================================================= *)

(** Mount connection state (Mnt structure) *)
Record MntState := mkMntState {
  mnt_queue : MsgQueue;        (* Pending RPCs *)
  mnt_tagmask : TagSet;        (* Allocated tags *)
  mnt_version : Z;             (* 9P protocol version *)
  mnt_msize : Z;               (* Maximum message size *)
  mnt_chan : Chan              (* Associated channel *)
}.

(** Well-formed mount state *)
Definition MntWellFormed (m : MntState) : Prop :=
  queue_tags_unique (mnt_queue m) /\
  NoDup (mnt_queue m) /\
  (forall msg, In msg (mnt_queue m) -> tag_allocated (mnt_tagmask m) (qmsg_tag msg)) /\
  mnt_msize m > 0 /\
  mnt_msize m <= MAXMSG /\
  ChanWellFormed (mnt_chan m).

(* ========================================================================= *)
(* QUEUE OPERATIONS                                                          *)
(* ========================================================================= *)

(** Enqueue RPC (mountio adds to tail) *)
Definition enqueue (r : QueuedMsg) (q : MsgQueue) : MsgQueue :=
  q ++ [r].

(** Queue is full (MAXRPC_COUNT limit) *)
Definition queue_full (q : MsgQueue) : Prop :=
  Z.of_nat (length q) >= MAXRPC_COUNT.

(** RPC is in-flight (in queue) *)
Definition rpc_inflight (tag : Z) (m : MntState) : Prop :=
  find_rpc_by_tag tag (mnt_queue m) <> None.

(* ========================================================================= *)
(* QUEUE INTEGRITY INVARIANTS                                                *)
(* ========================================================================= *)

(** Theorem: Enqueue preserves tag uniqueness *)
Theorem enqueue_preserves_uniqueness : forall r q,
  queue_tags_unique q ->
  NoDup q ->
  ~ In r q ->
  (forall m, In m q -> qmsg_tag m <> qmsg_tag r) ->
  queue_tags_unique (enqueue r q).
Proof.
  intros r q Huniq Hnodup Hnotin Hdiff.
  unfold enqueue, queue_tags_unique in *.
  intros m1 m2 H1 H2 Htag.
  apply in_app_or in H1. apply in_app_or in H2.
  destruct H1 as [H1 | H1], H2 as [H2 | H2].
  - (* Both from original queue *)
    apply Huniq; assumption.
  - (* m1 from q, m2 from [r] *)
    simpl in H2. destruct H2 as [H2 | H2].
    + subst. exfalso. apply (Hdiff m1 H1). exact Htag.
    + contradiction.
  - (* m1 from [r], m2 from q *)
    simpl in H1. destruct H1 as [H1 | H1].
    + subst. exfalso. apply (Hdiff m2 H2). symmetry. exact Htag.
    + contradiction.
  - (* Both from [r] *)
    simpl in H1, H2.
    destruct H1 as [H1 | H1], H2 as [H2 | H2];
      try contradiction; subst; reflexivity.
Qed.

(** Theorem: Enqueue preserves NoDup *)
Theorem enqueue_preserves_nodup : forall r q,
  NoDup q ->
  ~ In r q ->
  NoDup (enqueue r q).
Proof.
  intros r q Hnodup Hnotin.
  unfold enqueue.
  apply NoDup_app.
  - exact Hnodup.
  - constructor.
    + intro. contradiction.
    + constructor.
  - intros x Hx1 Hx2.
    simpl in Hx2. destruct Hx2 as [Hx2 | Hx2].
    + subst. contradiction.
    + contradiction.
Qed.

(** Theorem: Removing preserves well-formedness *)
Theorem remove_preserves_wellformed : forall tag m,
  MntWellFormed m ->
  MntWellFormed (mkMntState
    (remove_rpc_by_tag tag (mnt_queue m))
    (mnt_tagmask m)
    (mnt_version m)
    (mnt_msize m)
    (mnt_chan m)).
Proof.
  intros tag m H.
  destruct H as [Huniq [Hnodup [Htags [Hmsize_pos [Hmsize_max Hchan]]]]].
  unfold MntWellFormed. simpl.
  split; [| split; [| split; [| split; [| split]]]].
  - (* queue_tags_unique preserved *)
    unfold queue_tags_unique in *.
    intros m1 m2 H1 H2 Htag.
    (* Elements in removed queue were in original queue *)
    assert (Hin1: In m1 (mnt_queue m)).
    { apply (remove_subset tag). exact H1. }
    assert (Hin2: In m2 (mnt_queue m)).
    { apply (remove_subset tag). exact H2. }
    apply Huniq; assumption.
  - (* NoDup preserved *)
    induction (mnt_queue m) as [| x rest IH].
    + simpl. constructor.
    + simpl. destruct (Z.eqb (qmsg_tag x) tag) eqn:E.
      * (* x removed, result is rest *)
        inversion Hnodup as [| ? ? Hnotin Hrest]; subst.
        exact Hrest.
      * (* x kept, rest filtered *)
        simpl. inversion Hnodup as [| ? ? Hnotin Hrest]; subst.
        constructor.
        -- intro Hcontra.
           (* x not in remove_rpc_by_tag tag rest means x.tag <> tag and x not in rest *)
           assert (Hin: In x rest).
           { apply (remove_subset tag). exact Hcontra. }
           contradiction.
        -- apply IH.
           ++ unfold queue_tags_unique in *. intros.
              apply Huniq; try (right; assumption); assumption.
           ++ exact Hrest.
           ++ intros. apply Htags. right. assumption.
  - (* Tag allocation preserved *)
    intros msg Hin.
    apply Htags.
    induction (mnt_queue m) as [| x rest IH].
    + simpl in Hin. contradiction.
    + simpl in Hin. destruct (Z.eqb (qmsg_tag x) tag) eqn:E.
      * right. exact Hin.  (* x removed, msg is from rest *)
      * simpl in Hin. destruct Hin as [Hin | Hin].
        -- left. exact Hin.
        -- right.
           (* Use remove_subset to show msg was in rest *)
           apply (remove_subset tag). exact Hin.
  - (* msize > 0 preserved *)
    exact Hmsize_pos.
  - (* msize <= MAXMSG preserved *)
    exact Hmsize_max.
  - (* Chan well-formed preserved *)
    exact Hchan.
Qed.

(* ========================================================================= *)
(* QUEUE ORDERING PROPERTIES                                                 *)
(* ========================================================================= *)

(** Queue position (0-indexed from head) *)
Fixpoint queue_position (tag : Z) (q : MsgQueue) : option nat :=
  match q with
  | [] => None
  | m :: rest =>
      if Z.eqb (qmsg_tag m) tag
      then Some O
      else match queue_position tag rest with
           | None => None
           | Some n => Some (S n)
           end
  end.

Lemma queue_position_lt_length : forall tag q n,
  queue_position tag q = Some n ->
  (n < length q)%nat.
Proof.
  induction q as [| m rest IH]; intros n Hpos.
  - simpl in Hpos. discriminate.
  - simpl in Hpos.
    destruct (Z.eqb (qmsg_tag m) tag) eqn:E.
    + inversion Hpos; subst. simpl. lia.
    + remember (queue_position tag rest) as qp eqn:Erest.
      destruct qp as [n'|].
      * inversion Hpos; subst.
        assert (Hlen : (n' < length rest)%nat).
        { apply IH. rewrite Erest. reflexivity. }
        simpl. lia.
      * discriminate.
Qed.

Lemma queue_position_append_single : forall tag q r,
  queue_position tag (q ++ [r]) =
    match queue_position tag q with
    | Some n => Some n
    | None =>
        if Z.eqb (qmsg_tag r) tag
        then Some (length q)
        else None
    end.
Proof.
  induction q as [| m rest IH]; intros r.
  - simpl. destruct (Z.eqb (qmsg_tag r) tag); reflexivity.
  - simpl.
    destruct (Z.eqb (qmsg_tag m) tag) eqn:E; [reflexivity|].
    rewrite IH.
    destruct (queue_position tag rest) as [n'|] eqn:Erest.
    + reflexivity.
    + destruct (Z.eqb (qmsg_tag r) tag); reflexivity.
Qed.

Lemma queue_position_exists : forall tag q,
  In tag (map qmsg_tag q) ->
  exists n, queue_position tag q = Some n.
Proof.
  induction q as [| m rest IH]; intros Hin.
  - simpl in Hin. contradiction.
  - simpl in Hin. destruct Hin as [Hin | Hin].
    + subst. exists 0%nat. simpl. rewrite Z.eqb_refl. reflexivity.
    + destruct (Z.eqb (qmsg_tag m) tag) eqn:E.
      * exists 0%nat. simpl. rewrite E. reflexivity.
      * destruct (IH Hin) as [n Hn].
        exists (S n). simpl. rewrite E. rewrite Hn. reflexivity.
Qed.

Lemma queue_position_none_if_tag_not_in : forall tag q,
  ~ In tag (map qmsg_tag q) ->
  queue_position tag q = None.
Proof.
  induction q as [| m rest IH]; intros Hnotin.
  - reflexivity.
  - apply not_in_cons in Hnotin.
    destruct Hnotin as [Hneq Hnotin'].
    simpl.
    destruct (Z.eqb (qmsg_tag m) tag) eqn:E.
    + apply Z.eqb_eq in E. exfalso. apply Hneq. symmetry. exact E.
    + rewrite (IH Hnotin'). reflexivity.
Qed.

(** Enqueued-before relation *)
Definition enqueued_before (tag1 tag2 : Z) (q : MsgQueue) : Prop :=
  exists n1 n2,
    queue_position tag1 q = Some n1 /\
    queue_position tag2 q = Some n2 /\
    (n1 < n2)%nat.

(** Enqueue places RPC at tail when tag is fresh *)
Lemma enqueue_at_tail : forall r q tag,
  ~ In (qmsg_tag r) (map qmsg_tag q) ->
  In tag (map qmsg_tag q) ->
  enqueued_before tag (qmsg_tag r) (enqueue r q).
Proof.
  intros r q tag Hfresh Hin.
  unfold enqueued_before, enqueue.
  destruct (queue_position_exists tag q Hin) as [n Hn].
  exists n, (length q).
  split; [| split].
  - rewrite queue_position_append_single. rewrite Hn. reflexivity.
  - assert (Hnotin : queue_position (qmsg_tag r) q = None).
    { apply queue_position_none_if_tag_not_in. exact Hfresh. }
    rewrite queue_position_append_single.
    rewrite Hnotin.
    rewrite Z.eqb_refl.
    reflexivity.
  - apply queue_position_lt_length in Hn. exact Hn.
Qed.

(* ========================================================================= *)
(* QUEUE CONSISTENCY PROPERTIES                                              *)
(* ========================================================================= *)

(** Theorem: RPC in queue implies tag allocated *)
Theorem inflight_implies_allocated : forall m tag,
  MntWellFormed m ->
  rpc_inflight tag m ->
  tag_allocated (mnt_tagmask m) tag.
Proof.
  intros m tag Hwf Hinflight.
  destruct Hwf as [Huniq [Hnodup [Htags [Hmsize_pos [Hmsize_max Hchan]]]]].
  unfold rpc_inflight in Hinflight.
  destruct (find_rpc_by_tag tag (mnt_queue m)) as [msg|] eqn:Efind.
  - (* Found msg with tag *)
    assert (Hin: In msg (mnt_queue m)).
    { apply (find_In tag). exact Efind. }
    assert (Htag_eq: qmsg_tag msg = tag).
    { apply find_has_tag with (q := mnt_queue m). exact Efind. }
    rewrite <- Htag_eq.
    apply Htags. exact Hin.
  - exfalso. apply Hinflight. reflexivity.
Qed.

(** Tag in queue implies RPC inflight *)
Lemma allocated_implies_inflight : forall m tag,
  MntWellFormed m ->
  In tag (map qmsg_tag (mnt_queue m)) ->
  rpc_inflight tag m.
Proof.
  intros m tag Hwf Hin.
  destruct Hwf as [Huniq _].
  unfold rpc_inflight.
  apply in_map_iff in Hin.
  destruct Hin as [msg [Htag Hin]].
  assert (Hfind : find_rpc_by_tag tag (mnt_queue m) = Some msg).
  { apply mountmux_finds_tag; try exact Huniq; try exact Hin.
    exact Htag. }
  rewrite Hfind. discriminate.
Qed.

(** No tag reuse while RPC inflight *)
Theorem no_tag_reuse_while_inflight : forall m tag,
  MntWellFormed m ->
  rpc_inflight tag m ->
  tag_allocated (mnt_tagmask m) tag.
Proof.
  intros m tag Hwf Hinflight.
  apply (inflight_implies_allocated m tag); assumption.
Qed.

(* ========================================================================= *)
(* INTEGRATION WITH MESSAGE ROUTING                                          *)
(* ========================================================================= *)

(** Mux operation maintains well-formedness *)
Theorem mountmux_preserves_wellformed : forall tag m,
  MntWellFormed m ->
  rpc_inflight tag m ->
  MntWellFormed (mkMntState
    (remove_rpc_by_tag tag (mnt_queue m))
    (free_tag (mnt_tagmask m) tag)
    (mnt_version m)
    (mnt_msize m)
    (mnt_chan m)).
Proof.
  intros tag m Hwf Hinflight.
  destruct Hwf as [Huniq_orig [Hnodup_orig [Htags_orig [Hmsize_pos [Hmsize_max Hchan]]]]].
  (* Use remove_preserves_wellformed to get queue properties *)
  assert (Hwf_removed := remove_preserves_wellformed tag m).
  assert (Hwf_q: MntWellFormed (mkMntState
    (remove_rpc_by_tag tag (mnt_queue m))
    (mnt_tagmask m)
    (mnt_version m)
    (mnt_msize m)
    (mnt_chan m))).
  { apply Hwf_removed. unfold MntWellFormed.
    split; [| split; [| split; [| split; [| split]]]]; assumption. }
  clear Hwf_removed.
  destruct Hwf_q as [Huniq [Hnodup [Htags_removed [_ [_ _]]]]].
  (* Now prove wellformedness of state with freed tag *)
  unfold MntWellFormed. simpl.
  split; [| split; [| split; [| split; [| split]]]].
  - exact Huniq.
  - exact Hnodup.
  - (* Tags still allocated after freeing one *)
    intros msg Hin.
    unfold tag_allocated, free_tag. simpl.
    destruct (Z.eq_dec (qmsg_tag msg) tag) as [Heq | Hneq].
    + (* msg has freed tag - contradiction *)
      exfalso.
      assert (Hremoved: ~ In msg (remove_rpc_by_tag tag (mnt_queue m))).
      { apply (remove_not_in tag msg (mnt_queue m)); assumption. }
      contradiction.
    + (* Different tag, allocation preserved *)
      apply Z.eqb_neq in Hneq. rewrite Hneq.
      apply Htags_removed. exact Hin.
  - exact Hmsize_pos.
  - exact Hmsize_max.
  - exact Hchan.
Qed.

Print Assumptions enqueue_preserves_uniqueness.
Print Assumptions enqueue_preserves_nodup.
Print Assumptions remove_preserves_wellformed.
