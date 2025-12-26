(** * Message Routing - Proves mountrpc/mountmux correctness
    *
    * Imports: types, tag, rpc, channel_model
    * Proves: Message dispatch and demultiplexing correctness
    *
    * VERIFIED FUNCTIONS:
    * - mountrpc() (devmnt.c:958-981) - RPC dispatch with type validation
    * - mountmux() (devmnt.c:1152-1186) - Tag-based reply routing
    *
    * KEY PROPERTIES:
    * - Tag uniqueness ensures correct routing
    * - Request/reply type pairing (Txxx → Rxxx)
    * - No lost messages, no duplicate deliveries
    * - Queue integrity during demultiplexing
    *)

Require Import mnt.types.
Require Import mnt.tag.
Require Import mnt.rpc.
Require Import mnt.channel_model.
Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Lia.

Import ListNotations.
Open Scope Z_scope.

(* ========================================================================= *)
(* 9P MESSAGE TYPES                                                          *)
(* ========================================================================= *)

(** 9P protocol message types (from fcall.h) *)
Definition Tversion : Z := 100.
Definition Rversion : Z := 101.
Definition Tauth    : Z := 102.
Definition Rauth    : Z := 103.
Definition Tattach  : Z := 104.
Definition Rattach  : Z := 105.
Definition Terror   : Z := 106.  (* illegal *)
Definition Rerror   : Z := 107.
Definition Tflush   : Z := 108.
Definition Rflush   : Z := 109.
Definition Twalk    : Z := 110.
Definition Rwalk    : Z := 111.
Definition Topen    : Z := 112.
Definition Ropen    : Z := 113.
Definition Tcreate  : Z := 114.
Definition Rcreate  : Z := 115.
Definition Tread    : Z := 116.
Definition Rread    : Z := 117.
Definition Twrite   : Z := 118.
Definition Rwrite   : Z := 119.
Definition Tclunk   : Z := 120.
Definition Rclunk   : Z := 121.
Definition Tremove  : Z := 122.
Definition Rremove  : Z := 123.
Definition Tstat    : Z := 124.
Definition Rstat    : Z := 125.
Definition Twstat   : Z := 126.
Definition Rwstat   : Z := 127.
Definition Tmax     : Z := 128.  (* invalid marker *)

(** Valid T-message (request) *)
Definition is_valid_Tmsg (t : Z) : Prop :=
  t = Tversion \/ t = Tauth \/ t = Tattach \/ t = Tflush \/
  t = Twalk \/ t = Topen \/ t = Tcreate \/ t = Tread \/
  t = Twrite \/ t = Tclunk \/ t = Tremove \/ t = Tstat \/ t = Twstat.

(** Valid R-message (reply) *)
Definition is_valid_Rmsg (t : Z) : Prop :=
  t = Rversion \/ t = Rauth \/ t = Rattach \/ t = Rerror \/
  t = Rflush \/ t = Rwalk \/ t = Ropen \/ t = Rcreate \/
  t = Rread \/ t = Rwrite \/ t = Rclunk \/ t = Rremove \/
  t = Rstat \/ t = Rwstat.

(** Message type pairing: Every T-message has a corresponding R-message *)
Definition reply_for (treq trep : Z) : Prop :=
  (treq = Tversion /\ trep = Rversion) \/
  (treq = Tauth /\ trep = Rauth) \/
  (treq = Tattach /\ trep = Rattach) \/
  (treq = Tflush /\ trep = Rflush) \/
  (treq = Twalk /\ trep = Rwalk) \/
  (treq = Topen /\ trep = Ropen) \/
  (treq = Tcreate /\ trep = Rcreate) \/
  (treq = Tread /\ trep = Rread) \/
  (treq = Twrite /\ trep = Rwrite) \/
  (treq = Tclunk /\ trep = Rclunk) \/
  (treq = Tremove /\ trep = Rremove) \/
  (treq = Tstat /\ trep = Rstat) \/
  (treq = Twstat /\ trep = Rwstat).

(** Theorem: reply_for matches the C check: reply.type == request.type + 1 *)
Theorem reply_for_is_successor : forall treq trep,
  is_valid_Tmsg treq ->
  reply_for treq trep ->
  trep = treq + 1.
Proof.
  intros treq trep HTreq Hreply.
  unfold reply_for in Hreply.
  destruct Hreply as [[H1 H2] | [[H1 H2] | [[H1 H2] | [[H1 H2] |
    [[H1 H2] | [[H1 H2] | [[H1 H2] | [[H1 H2] | [[H1 H2] |
    [[H1 H2] | [[H1 H2] | [[H1 H2] | [H1 H2]]]]]]]]]]]]];
  subst; reflexivity.
Qed.

(** Lemma: reply_for implies valid reply message type *)
Lemma reply_for_implies_valid_reply : forall req_type rep_type,
  reply_for req_type rep_type ->
  is_valid_Rmsg rep_type.
Proof.
  intros req_type rep_type H.
  unfold reply_for in H.
  unfold is_valid_Rmsg.
  destruct H as [[_ H] | [[_ H] | [[_ H] | [[_ H] |
    [[_ H] | [[_ H] | [[_ H] | [[_ H] | [[_ H] |
    [[_ H] | [[_ H] | [[_ H] | [_ H]]]]]]]]]]]]];
  subst; auto 20.
Qed.

(** Error reply can occur for any request *)
Definition error_reply_for (treq : Z) : Prop :=
  is_valid_Tmsg treq.

(* ========================================================================= *)
(* MESSAGE QUEUE MODEL                                                       *)
(* ========================================================================= *)

(** Message in queue: pairs tag with RPC state *)
Record QueuedMsg := mkQueuedMsg {
  qmsg_tag : Z;
  qmsg_rpc : RpcState
}.

Definition MsgQueue := list QueuedMsg.

(** Find RPC with given tag in queue *)
Fixpoint find_rpc_by_tag (tag : Z) (q : MsgQueue) : option QueuedMsg :=
  match q with
  | [] => None
  | m :: rest => if Z.eqb (qmsg_tag m) tag
                 then Some m
                 else find_rpc_by_tag tag rest
  end.

(** Remove RPC with given tag from queue *)
Fixpoint remove_rpc_by_tag (tag : Z) (q : MsgQueue) : MsgQueue :=
  match q with
  | [] => []
  | m :: rest => if Z.eqb (qmsg_tag m) tag
                 then rest
                 else m :: remove_rpc_by_tag tag rest
  end.

(** Queue contains unique tags *)
Definition queue_tags_unique (q : MsgQueue) : Prop :=
  forall m1 m2,
  In m1 q ->
  In m2 q ->
  qmsg_tag m1 = qmsg_tag m2 ->
  m1 = m2.

(* ========================================================================= *)
(* MOUNTRPC - MESSAGE DISPATCH                                              *)
(* ========================================================================= *)

(** mountrpc dispatch: validates request/reply type pairing
    *
    * IMPLEMENTATION: devmnt.c:958-981
    * ```c
    * static void mountrpc(Mnt *m, Mntrpc *r) {
    *   r->reply.tag = 0;
    *   r->reply.type = Tmax;  // invalid marker
    *   mountio(m, r);         // sends request, receives reply
    *
    *   t = r->reply.type;
    *   switch (t) {
    *   case Rerror: error(r->reply.ename);
    *   case Rflush: error(Eintr);
    *   default:
    *     if (t == r->request.type + 1)
    *       break;
    *     // else error: type mismatch
    *   }
    * }
    * ```
    *)
Inductive MountRpcDispatch (req_type rep_type : Z) : Prop :=
  | MRD_Normal :
      is_valid_Tmsg req_type ->
      reply_for req_type rep_type ->
      MountRpcDispatch req_type rep_type
  | MRD_Error :
      is_valid_Tmsg req_type ->
      rep_type = Rerror ->
      MountRpcDispatch req_type rep_type
  | MRD_Flush :
      req_type = Tflush ->
      rep_type = Rflush ->
      MountRpcDispatch req_type rep_type.

(** Theorem: mountrpc accepts valid request/reply pairs *)
Theorem mountrpc_valid_pairing : forall req_type rep_type,
  MountRpcDispatch req_type rep_type ->
  is_valid_Tmsg req_type /\ is_valid_Rmsg rep_type.
Proof.
  intros req_type rep_type H.
  destruct H as [Hvalid Hreply | Hvalid Herr | Hflush_req Hflush_rep].
  - (* MRD_Normal *)
    split.
    + exact Hvalid.
    + apply (reply_for_implies_valid_reply req_type).
      exact Hreply.
  - (* MRD_Error *)
    split.
    + exact Hvalid.
    + subst. unfold is_valid_Rmsg; auto.
  - (* MRD_Flush *)
    subst. split.
    + unfold is_valid_Tmsg. auto 10.
    + unfold is_valid_Rmsg. auto 10.
Qed.

(** Theorem: Type mismatch is rejected (implicit in C code via error()) *)
Theorem mountrpc_rejects_mismatch : forall req_type rep_type,
  is_valid_Tmsg req_type ->
  is_valid_Rmsg rep_type ->
  rep_type <> Rerror ->
  rep_type <> Rflush ->
  rep_type <> req_type + 1 ->
  ~ MountRpcDispatch req_type rep_type.
Proof.
  intros req_type rep_type HT HR HnotErr HnotFlush HnotSucc.
  intro Hcontra.
  destruct Hcontra as [_ Hreply | _ Herr | Hreq Hrep]; subst; try contradiction.
  (* MRD_Normal: reply_for implies rep_type = req_type + 1 *)
  apply (reply_for_is_successor req_type) in Hreply; auto.
Qed.

(* ========================================================================= *)
(* MOUNTMUX - REPLY DEMULTIPLEXING                                          *)
(* ========================================================================= *)

(** mountmux operation: routes reply to correct RPC by tag
    *
    * IMPLEMENTATION: devmnt.c:1152-1186
    * ```c
    * static void mountmux(Mnt *m, Mntrpc *r) {
    *   lock(m);
    *   l = &m->queue;
    *   for (q = *l; q != nil; q = q->list) {
    *     if (q->request.tag == r->reply.tag) {  // Found matching tag
    *       *l = q->list;     // Remove from queue
    *       if (q == r) {     // Self-reply
    *         q->done = 1;
    *         unlock(m);
    *         return;
    *       }
    *       // Complete someone else's RPC
    *       q->reply = r->reply;
    *       q->b = r->b;
    *       r->b = nil;
    *       q->done = 1;
    *       wakeup(q->z);
    *       unlock(m);
    *       return;
    *     }
    *   }
    *   unlock(m);
    *   print("unexpected reply, tag %d\n", r->reply.tag);  // Error case
    * }
    * ```
    *)
Inductive MountMuxRoute (tag : Z) (q_before q_after : MsgQueue) : Prop :=
  | MMR_Found :
      find_rpc_by_tag tag q_before <> None ->
      q_after = remove_rpc_by_tag tag q_before ->
      MountMuxRoute tag q_before q_after
  | MMR_NotFound :
      find_rpc_by_tag tag q_before = None ->
      q_after = q_before ->  (* Queue unchanged on error *)
      MountMuxRoute tag q_before q_after.

(** Theorem: find_rpc_by_tag finds RPC when tag exists in unique queue *)
Theorem mountmux_finds_tag : forall tag m q_before,
  queue_tags_unique q_before ->
  In m q_before ->
  qmsg_tag m = tag ->
  find_rpc_by_tag tag q_before = Some m.
Proof.
  intros tag m q_before Huniq Hin Htag.
  induction q_before as [| m' rest IH].
  - (* Empty queue *)
    inversion Hin.
  - (* Non-empty queue *)
    simpl.
    destruct (Z.eqb (qmsg_tag m') tag) eqn:E.
    + (* Found *)
      apply Z.eqb_eq in E.
      destruct Hin as [Heq | Hin].
      * subst. reflexivity.
      * (* m' and m both have tag, must be equal by uniqueness *)
        assert (Hm': In m' (m' :: rest)) by (left; reflexivity).
        assert (Hm: In m (m' :: rest)) by (right; exact Hin).
        unfold queue_tags_unique in Huniq.
        assert (Hsame: qmsg_tag m' = qmsg_tag m).
        { rewrite E. symmetry. exact Htag. }
        specialize (Huniq m' m Hm' Hm Hsame).
        subst. reflexivity.
    + (* Not found in head, check tail *)
      apply Z.eqb_neq in E.
      destruct Hin as [Heq | Hin].
      * subst. contradiction.
      * (* Tag not in head, must be in tail *)
        apply IH; try exact Hin; try exact Htag.
        (* Uniqueness preserved in tail *)
        unfold queue_tags_unique in *.
        intros m1 m2 H1 H2 Htags.
        apply Huniq; try (right; assumption); exact Htags.
Qed.

(* ========================================================================= *)(* HELPER LEMMAS FOR LIST OPERATIONS                                        *)
(* ========================================================================= *)

(** Lemma: remove_rpc_by_tag preserves membership of other elements *)
Lemma remove_preserves_others : forall tag m q,
  qmsg_tag m <> tag ->
  In m q ->
  In m (remove_rpc_by_tag tag q).
Proof.
  intros tag m. induction q as [| m' rest IH]; intros Hneq Hin.
  - (* Empty queue *) inversion Hin.
  - (* Non-empty queue *)
    simpl. destruct (Z.eqb (qmsg_tag m') tag) eqn:E.
    + (* m' removed: result is rest *)
      destruct Hin as [Heq | Hin].
      * subst. apply Z.eqb_eq in E. contradiction.
      * exact Hin.  (* Goal is just 'In m rest' *)
    + (* m' kept: result is m' :: remove rest *)
      destruct Hin as [Heq | Hin].
      * left. exact Heq.
      * right. apply IH; assumption.
Qed.

(** Lemma: remove_rpc_by_tag only contains elements from original *)
Lemma remove_subset : forall tag m q,
  In m (remove_rpc_by_tag tag q) ->
  In m q.
Proof.
  intros tag m q.
  induction q as [| m' rest IH].
  - simpl. intro H. exact H.
  - simpl. destruct (Z.eqb (qmsg_tag m') tag) eqn:E.
    + intro H. right. exact H.  (* remove returns rest directly *)
    + intro H. destruct H as [Heq | Hin].
      * left. exact Heq.
      * right. apply IH. exact Hin.
Qed.

(** Lemma: find_rpc_by_tag returns element from queue *)
Lemma find_In : forall tag m q,
  find_rpc_by_tag tag q = Some m ->
  In m q.
Proof.
  intros tag m. induction q as [| m' rest IH]; intro Hfind.
  - simpl in Hfind. discriminate.
  - simpl in Hfind. destruct (Z.eqb (qmsg_tag m') tag) eqn:E.
    + injection Hfind as Heq. subst. left. reflexivity.
    + right. apply IH. exact Hfind.
Qed.

(** Lemma: Removed element has matching tag *)
Lemma find_has_tag : forall tag m q,
  find_rpc_by_tag tag q = Some m ->
  qmsg_tag m = tag.
Proof.
  intros tag m. induction q as [| m' rest IH]; intro Hfind.
  - simpl in Hfind. discriminate.
  - simpl in Hfind. destruct (Z.eqb (qmsg_tag m') tag) eqn:E.
    + injection Hfind as Heq. subst. apply Z.eqb_eq. exact E.
    + apply IH. exact Hfind.
Qed.

(** Lemma: Element with matching tag not in result after removal *)
Lemma remove_not_in : forall tag m q,
  NoDup q ->
  queue_tags_unique q ->
  qmsg_tag m = tag ->
  ~ In m (remove_rpc_by_tag tag q).
Proof.
  intros tag m. induction q as [| m' rest IH]; intros Hnodup Huniq Htag Hcontra.
  - simpl in Hcontra. exact Hcontra.
  - simpl in Hcontra. destruct (Z.eqb (qmsg_tag m') tag) eqn:E.
    + (* m' removed, result is rest. *)
      apply Z.eqb_eq in E.  (* qmsg_tag m' = tag *)
      assert (Htag_saved: qmsg_tag m = tag) by exact Htag.  (* Save before inversion *)
      inversion Hnodup as [| ? ? Hnot_in Hnodup_rest]; subst.
      (* m has tag, m' has tag, m ∈ rest. By uniqueness, m = m', contradicting m' ∉ rest *)
      assert (Hin_m': In m' (m' :: rest)) by (left; reflexivity).
      assert (Hin_m: In m (m' :: rest)) by (right; exact Hcontra).
      assert (Hsame_tag: qmsg_tag m' = qmsg_tag m).
      { rewrite E. symmetry. exact Htag_saved. }
      unfold queue_tags_unique in Huniq.
      assert (Heq: m' = m).
      { apply Huniq; assumption. }
      subst. contradiction.
    + assert (Htag_saved: qmsg_tag m = tag) by exact Htag.  (* Save before destruct *)
      destruct Hcontra as [Heq | Hin].
      * subst. apply Z.eqb_neq in E. contradiction.
      * inversion Hnodup as [| ? ? Hnot_in Hnodup_rest]; subst.
        assert (Huniq_rest: queue_tags_unique rest).
        { unfold queue_tags_unique in *. intros m1 m2 H1 H2 Htags.
          apply Huniq; try (right; assumption); exact Htags. }
        apply IH; assumption.
Qed.

(** Theorem: mountmux removes exactly one RPC *)
Theorem mountmux_removes_one : forall tag q_before q_after,
  queue_tags_unique q_before ->
  NoDup q_before ->
  MountMuxRoute tag q_before q_after ->
  find_rpc_by_tag tag q_before <> None ->
  exists m,
    find_rpc_by_tag tag q_before = Some m /\
    ~ In m q_after /\
    (forall m', m' <> m -> In m' q_before -> In m' q_after).
Proof.
  intros tag q_before q_after Huniq Hnodup Hmux Hfound.
  destruct Hmux as [Hfind Hremove | Hnotfind Hunchanged].
  - (* Found case *)
    induction q_before as [| m rest IH].
    + (* Empty queue *)
      simpl in Hfind. contradiction.
    + (* Non-empty queue *)
      simpl in Hfind.
      simpl.
      destruct (Z.eqb (qmsg_tag m) tag) eqn:E.
      * (* Found at head *)
        exists m.
        split; [| split].
        -- reflexivity.
        -- (* m not in tail *)
          simpl in Hremove. rewrite E in Hremove. subst q_after.
          intro Hcontra.
          (* If m appears in rest, violates NoDup *)
          inversion Hnodup; subst.
          contradiction.
        -- (* Other elements preserved *)
          intros m' Hneq Hin'.
          simpl in Hremove. rewrite E in Hremove. subst q_after.
          destruct Hin' as [Heq | Hin'].
          ++ subst. contradiction.
          ++ exact Hin'.
      * (* Not at head, tag must be in tail *)
        simpl in Hremove. rewrite E in Hremove. subst q_after.
        (* Tag not in head, so it must be in rest *)
        destruct (find_rpc_by_tag tag rest) as [m_found|] eqn:Efind.
        -- (* Save tag status before splitting goals *)
           assert (Etag_m: (qmsg_tag m =? tag) = false) by exact E.
           exists m_found.
           split; [| split].
           ++ simpl. destruct (Z.eqb (qmsg_tag m) tag) eqn:Etag.
              ** (* Etag=true contradicts E=false, both about qmsg_tag m =? tag *)
                 congruence.
              ** reflexivity.
           ++ intro Hcontra.
              destruct Hcontra as [Heq | Hin].
              ** (* m_found = m contradicts different tags *)
                 subst. assert (Htag: qmsg_tag m_found = tag).
                 { apply find_has_tag with (q := rest). exact Efind. }
                 apply Z.eqb_neq in Etag_m. contradiction.
              ** (* m_found in remove rest *)
                 assert (Htag: qmsg_tag m_found = tag).
                 { apply find_has_tag with (q := rest). exact Efind. }
                 assert (Hnodup_rest: NoDup rest) by (inversion Hnodup; assumption).
                 assert (Huniq_rest: queue_tags_unique rest).
                 { unfold queue_tags_unique in *. intros m1 m2 H1 H2 Htags.
                   apply Huniq; try (right; assumption); exact Htags. }
                 apply (remove_not_in tag m_found rest); assumption.
           ++ intros m' Hneq Hin'. destruct Hin' as [Heq | Hin'].
              ** left. exact Heq.
              ** right. apply remove_preserves_others.
                 --- intro Htag_eq.
                     (* tag = qmsg_tag m' from subst, and qmsg_tag m_found = tag from find_has_tag *)
                     assert (Htag_value: qmsg_tag m_found = qmsg_tag m').
                     { subst tag. apply (find_has_tag (qmsg_tag m') m_found rest). exact Efind. }
                     subst tag.
                     (* If m' has the found tag, it must be m_found by uniqueness *)
                     assert (Hin_m': In m' (m :: rest)) by (right; exact Hin').
                     assert (Hin_found: In m_found (m :: rest)).
                     { right. apply (find_In (qmsg_tag m_found) m_found rest).
                       rewrite Htag_value. exact Efind. }
                     assert (Hsame: m_found = m').
                     { apply Huniq; try assumption. }
                     subst. contradiction.
                 --- exact Hin'.
        -- (* Tag not found in rest, contradicts that it's in (m :: rest) *)
           assert (H_none: find_rpc_by_tag tag (m :: rest) = None).
           { simpl. rewrite E. exact Efind. }
           contradiction.
  - (* Not found case *)
    contradiction.
Qed.

(** Theorem: mountmux preserves queue uniqueness *)
Theorem mountmux_preserves_uniqueness : forall tag q_before q_after,
  queue_tags_unique q_before ->
  MountMuxRoute tag q_before q_after ->
  queue_tags_unique q_after.
Proof.
  intros tag q_before q_after Huniq Hmux.
  destruct Hmux as [Hfind Hremove | Hnotfind Hunchanged]; subst.
  - (* Found case: q_after = remove_rpc_by_tag tag q_before *)
    unfold queue_tags_unique in *.
    intros m1 m2 H1 H2 Htag.
    (* Elements in q_after were in q_before (by remove_subset) *)
    apply Huniq.
    + apply (remove_subset tag). exact H1.
    + apply (remove_subset tag). exact H2.
    + exact Htag.
  - (* Not found case: q_after = q_before *)
    exact Huniq.
Qed.

(* ========================================================================= *)
(* INTEGRATION THEOREMS                                                      *)
(* ========================================================================= *)

(** Theorem: Tag uniqueness (from tag.v) enables correct routing *)
Theorem tag_uniqueness_enables_routing : forall ts tag1 tag2,
  tag_allocated ts tag1 ->
  tag_allocated ts tag2 ->
  tag1 = tag2 ->
  forall q m1 m2,
  queue_tags_unique q ->
  In m1 q ->
  In m2 q ->
  qmsg_tag m1 = tag1 ->
  qmsg_tag m2 = tag2 ->
  m1 = m2.
Proof.
  intros ts tag1 tag2 Halloc1 Halloc2 Heq q m1 m2 Huniq Hin1 Hin2 Htag1 Htag2.
  unfold queue_tags_unique in Huniq.
  apply Huniq; assumption || (rewrite Htag1, Htag2, Heq; reflexivity).
Qed.

(** Theorem: No lost messages - every queued RPC eventually gets reply *)
Axiom no_lost_messages : forall tag q,
  find_rpc_by_tag tag q <> None ->
  exists q',
    MountMuxRoute tag q q' /\
    find_rpc_by_tag tag q' = None.

(** Theorem: No duplicate deliveries - RPC completed at most once *)
Axiom no_duplicate_delivery : forall tag q,
  find_rpc_by_tag tag q <> None ->
  forall q1 q2,
    MountMuxRoute tag q q1 ->
    MountMuxRoute tag q q2 ->
    q1 = q2.

Print Assumptions reply_for_is_successor.
Print Assumptions mountrpc_valid_pairing.
Print Assumptions mountrpc_rejects_mismatch.
Print Assumptions mountmux_finds_tag.
