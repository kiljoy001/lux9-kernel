(** * Channel Model - Abstract 9P Chan and Qid structures
    *
    * Imports: None (foundational model)
    * Models: Chan (channel/file descriptor), Qid (unique file ID)
    *
    * Used by: message_routing.v, queue_invariants.v
    *
    * IMPLEMENTATION: kernel/include/portdat.h:179-209 (Chan)
    *                 kernel/include/lib.h:240-244 (Qid)
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.

Open Scope Z_scope.

(* ========================================================================= *)
(* QID - UNIQUE FILE IDENTIFIER                                             *)
(* ========================================================================= *)

(** Qid: Plan 9 unique file identifier (9P protocol)
    *
    * Uniquely identifies a file in the namespace.
    * path: Unique 64-bit identifier for the file
    * vers: Version number (incremented on modification)
    * type: File type flags (QTDIR, QTAPPEND, etc.)
    *)
Record Qid := mkQid {
  qid_path : Z;    (* uvlong: 64-bit unique path *)
  qid_vers : Z;    (* ulong: 32-bit version *)
  qid_type : Z     (* uchar: 8-bit type flags *)
}.

(** Qid type flags (from lib.h) *)
Definition QTDIR : Z := 0x80.      (* Directory *)
Definition QTAPPEND : Z := 0x40.   (* Append-only *)
Definition QTEXCL : Z := 0x20.     (* Exclusive use *)
Definition QTMOUNT : Z := 0x10.    (* Mount point *)
Definition QTAUTH : Z := 0x08.     (* Authentication file *)
Definition QTTMP : Z := 0x04.      (* Temporary file *)
Definition QTFILE : Z := 0x00.     (* Regular file *)

(** Qid equality *)
Definition qid_eq (q1 q2 : Qid) : Prop :=
  qid_path q1 = qid_path q2 /\
  qid_vers q1 = qid_vers q2 /\
  qid_type q1 = qid_type q2.

(** Qid equality is decidable *)
Lemma qid_eq_dec : forall q1 q2 : Qid,
  {qid_eq q1 q2} + {~ qid_eq q1 q2}.
Proof.
  intros q1 q2.
  destruct (Z.eq_dec (qid_path q1) (qid_path q2)) as [Hp | Hp].
  - destruct (Z.eq_dec (qid_vers q1) (qid_vers q2)) as [Hv | Hv].
    + destruct (Z.eq_dec (qid_type q1) (qid_type q2)) as [Ht | Ht].
      * left. unfold qid_eq. split; [| split]; assumption.
      * right. unfold qid_eq. intro H. destruct H as [_ [_ H]]. contradiction.
    + right. unfold qid_eq. intro H. destruct H as [_ [H _]]. contradiction.
  - right. unfold qid_eq. intro H. destruct H as [H _]. contradiction.
Qed.

(* ========================================================================= *)
(* CHAN - FILE DESCRIPTOR / CHANNEL                                         *)
(* ========================================================================= *)

(** Chan: Kernel representation of open file (channel)
    *
    * Represents an open file descriptor with associated state.
    * Used for both local and remote (mounted) file access.
    *
    * Simplified model focuses on fields relevant to mount operations.
    *)
Record Chan := mkChan {
  chan_ref : Z;        (* long: Reference count *)
  chan_offset : Z;     (* vlong: Current file offset *)
  chan_type : Z;       (* ushort: Device type *)
  chan_dev : Z;        (* ulong: Device instance ID *)
  chan_mode : Z;       (* ushort: Access mode (OREAD/OWRITE) *)
  chan_flag : Z;       (* ushort: Channel flags *)
  chan_qid : Qid;      (* Unique file identifier *)
  chan_fid : Z;        (* int: 9P file ID (for devmnt) *)
  chan_ismtpt : bool   (* int: Is this a mount point? *)
}.

(** Channel mode flags (from lib.h) *)
Definition OREAD : Z := 0.
Definition OWRITE : Z := 1.
Definition ORDWR : Z := 2.
Definition OEXEC : Z := 3.
Definition OTRUNC : Z := 16.
Definition OCEXEC : Z := 32.
Definition ORCLOSE : Z := 64.

(** Channel flags (from portdat.h) *)
Definition COPEN : Z := 0x0001.    (* Channel is open *)
Definition CCREATE : Z := 0x0002.  (* Created by create *)
Definition CCEXEC : Z := 0x0004.   (* Close on exec *)
Definition CFREE : Z := 0x0008.    (* On free list *)
Definition CRCLOSE : Z := 0x0010.  (* Remove on close *)
Definition CMSG : Z := 0x0020.     (* 9P message channel *)
Definition CCACHE : Z := 0x0080.   (* Caching enabled *)

(* ========================================================================= *)
(* CHANNEL VALIDITY PROPERTIES                                              *)
(* ========================================================================= *)

(** Valid reference count: must be > 0 for active channels *)
Definition valid_refcount (c : Chan) : Prop :=
  chan_ref c > 0.

(** Valid device ID: assigned by mntchan() starting at 1 *)
Definition valid_dev (c : Chan) : Prop :=
  chan_dev c > 0.

(** Valid FID: non-negative for 9P protocol *)
Definition valid_fid (c : Chan) : Prop :=
  chan_fid c >= 0.

(** Channel is open (COPEN flag set) *)
Definition chan_is_open (c : Chan) : Prop :=
  Z.land (chan_flag c) COPEN <> 0.

(** Channel is a 9P message channel (CMSG flag set) *)
Definition chan_is_msg (c : Chan) : Prop :=
  Z.land (chan_flag c) CMSG <> 0.

(** Well-formed channel *)
Definition ChanWellFormed (c : Chan) : Prop :=
  valid_refcount c /\
  valid_dev c /\
  valid_fid c /\
  chan_is_open c.

(* ========================================================================= *)
(* CHANNEL OPERATIONS                                                        *)
(* ========================================================================= *)

(** Increment reference count (incref) *)
Definition incref (c : Chan) : Chan :=
  mkChan
    (chan_ref c + 1)
    (chan_offset c)
    (chan_type c)
    (chan_dev c)
    (chan_mode c)
    (chan_flag c)
    (chan_qid c)
    (chan_fid c)
    (chan_ismtpt c).

(** Decrement reference count (decref) *)
Definition decref (c : Chan) : Chan :=
  mkChan
    (chan_ref c - 1)
    (chan_offset c)
    (chan_type c)
    (chan_dev c)
    (chan_mode c)
    (chan_flag c)
    (chan_qid c)
    (chan_fid c)
    (chan_ismtpt c).

(** Set channel flags *)
Definition set_flags (c : Chan) (flags : Z) : Chan :=
  mkChan
    (chan_ref c)
    (chan_offset c)
    (chan_type c)
    (chan_dev c)
    (chan_mode c)
    (Z.lor (chan_flag c) flags)
    (chan_qid c)
    (chan_fid c)
    (chan_ismtpt c).

(* ========================================================================= *)
(* CORRECTNESS PROPERTIES                                                    *)
(* ========================================================================= *)

(** Theorem: incref preserves well-formedness *)
Theorem incref_preserves_wellformed : forall c,
  ChanWellFormed c ->
  ChanWellFormed (incref c).
Proof.
  intros c H.
  destruct H as [Href [Hdev [Hfid Hopen]]].
  unfold ChanWellFormed, incref.
  split; [| split; [| split]].
  - unfold valid_refcount in *. simpl. lia.
  - unfold valid_dev in *. simpl. exact Hdev.
  - unfold valid_fid in *. simpl. exact Hfid.
  - unfold chan_is_open in *. simpl. exact Hopen.
Qed.

(** Theorem: decref maintains refcount >= 0 when wellformed *)
Theorem decref_nonnegative : forall c,
  ChanWellFormed c ->
  chan_ref (decref c) >= 0.
Proof.
  intros c H.
  destruct H as [Href _].
  unfold valid_refcount in Href.
  unfold decref. simpl. lia.
Qed.

(** Theorem: set_flags preserves well-formedness *)
Theorem set_flags_preserves_wellformed : forall c flags,
  ChanWellFormed c ->
  ChanWellFormed (set_flags c flags).
Proof.
  intros c flags H.
  destruct H as [Href [Hdev [Hfid Hopen]]].
  unfold ChanWellFormed, set_flags.
  split; [| split; [| split]].
  - unfold valid_refcount in *. simpl. exact Href.
  - unfold valid_dev in *. simpl. exact Hdev.
  - unfold valid_fid in *. simpl. exact Hfid.
  - unfold chan_is_open in *. simpl.
    intro Hcontra.
    (* Z.lor can only make bits 1, never clear them.
       If COPEN was set before (Hopen), it remains set after lor. *)
    assert (Hlor: Z.land (chan_flag c) COPEN <> 0 ->
                  Z.land (Z.lor (chan_flag c) flags) COPEN <> 0).
    { intro Hland.
      intro Hcontra'.
      (* COPEN bit is set in chan_flag c *)
      (* Z.lor preserves set bits, so COPEN remains set *)
      assert (Hbit: Z.testbit (Z.lor (chan_flag c) flags) 5 = true).
      { rewrite Z.lor_spec.
        (* Assuming COPEN = 0x0020 = bit 5 *)
        admit. (* This requires bit-level reasoning *)
      }
      admit.
    }
    apply Hlor in Hopen.
    contradiction.
Admitted.  (* TODO: Complete bit-level proof or simplify model *)

(* ========================================================================= *)
(* INTEGRATION WITH MOUNT PROOFS                                            *)
(* ========================================================================= *)

(** Theorem: Chan with dev ID from mntchan has valid_dev
    *
    * Links to: proofs/mnt/mntchk_safety.v
    * mntchan() allocates dev starting at 1, guaranteeing valid_dev
    *)
Theorem mntchan_creates_valid_dev : forall dev,
  dev > 0 ->
  forall c, chan_dev c = dev -> valid_dev c.
Proof.
  intros dev Hdev c Heq.
  unfold valid_dev.
  rewrite Heq.
  exact Hdev.
Qed.

Print Assumptions incref_preserves_wellformed.
Print Assumptions decref_nonnegative.
Print Assumptions set_flags_preserves_wellformed.
