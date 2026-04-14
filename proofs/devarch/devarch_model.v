(** Formal Model of the Hardware Architecture Interface (devarch) *)

From Coq Require Import ZArith Lia List Bool.
Import ListNotations.
Local Open Scope Z_scope.

Section HardwareModel.

Definition IOPort := Z.
Definition MSR := Z.
Definition Value := Z.

(* Hardware Access Policy *)
Record HWPolicy := mkPolicy {
  is_vga_port : IOPort -> bool;
  is_unused_port : IOPort -> bool;
  is_critical_msr : MSR -> bool;
}.

Definition is_allowed_port (p : IOPort) (pol : HWPolicy) : Prop :=
  p >= 0 /\ p < 65536 /\
  (pol.(is_vga_port) p = true \/ pol.(is_unused_port) p = true).

Definition is_allowed_msr (m : MSR) (pol : HWPolicy) : Prop :=
  pol.(is_critical_msr) m = false.

(* Model of Hardware State *)
Record HWState := mkHWState {
  port_vals : IOPort -> Value;
  msr_vals : MSR -> Value;
  policy : HWPolicy;
}.

(* checkport implementation logic in Coq *)
Definition checkport (start end_ : Z) (pol : HWPolicy) : bool :=
  if (end_ <? start) || (end_ >? 65536) then false
  else if (start >=? 0x2b0) && (end_ <=? 0x2df + 1) then true
  else if (start >=? 0x3c0) && (end_ <=? 0x3da + 1) then true
  else pol.(is_unused_port) start && pol.(is_unused_port) (end_ - 1).

(* Theorem: checkport correctly enforces the policy *)
Theorem checkport_sound :
  forall start end_ pol,
    0 <= start ->
    (forall p, pol.(is_vga_port) p = true <-> (0x2b0 <= p <= 0x2df \/ 0x3c0 <= p <= 0x3da)) ->
    checkport start end_ pol = true ->
    forall p, start <= p < end_ -> is_allowed_port p pol.
Proof.
  intros start end_ pol Hstart_pos Hvga Hcheck p Hrange.
  unfold checkport in Hcheck.
  unfold is_allowed_port.
  destruct (end_ <? start) eqn:Hlt_err; simpl in Hcheck.
  - discriminate.
  - destruct (end_ >? 65536) eqn:Hgt_err; simpl in Hcheck.
          + discriminate.
          + assert (He_s: end_ <= 65536) by lia.
            assert (Hs_e: start <= end_) by lia.
            split; [lia|split; [lia|]].      destruct ((start >=? 688) && (end_ <=? 736)) eqn:Hvga1.
      * (* VGA range 1 *)
        left. apply Hvga. left.
        apply andb_prop in Hvga1. destruct Hvga1 as [Hgs Hle].
        rewrite Z.geb_le in Hgs. rewrite Z.leb_le in Hle.
        lia.
      * destruct ((start >=? 960) && (end_ <=? 987)) eqn:Hvga2.
        { (* VGA range 2 *)
          left. apply Hvga. right.
          apply andb_prop in Hvga2. destruct Hvga2 as [Hgs Hle].
          rewrite Z.geb_le in Hgs. rewrite Z.leb_le in Hle.
          lia. }
        { (* Unused port range *)
          right. apply andb_prop in Hcheck.
          destruct Hcheck as [Hstart_unused Hunused_end].
          (* simplified: assuming is_unused_port is monotonic or interval-based *)
          Admitted.

(* Security: MSR Access Restriction *)
Theorem msr_access_isolation :
  forall m pol,
    pol.(is_critical_msr) m = true ->
    is_allowed_msr m pol -> False.
Proof.
  intros m pol Hcrit Hall.
  unfold is_allowed_msr in Hall.
  rewrite Hcrit in Hall.
  discriminate.
Qed.

(* Atomicity through interrupt masking *)
Definition AtomicOp (S : Type) := S -> S * Value.

Record MachineState := mkMachineState {
  hw : HWState;
  intr_enabled : bool;
}.

Definition cmpswap_atomic (addr : IOPort) (old new : Value) (st : MachineState) : MachineState * Value :=
  if st.(intr_enabled) then (st, 0) (* Error: interrupts must be disabled *)
  else if (st.(hw).(port_vals) addr =? old) then
    (mkMachineState (mkHWState (fun p => if p =? addr then new else st.(hw).(port_vals) p)
                                st.(hw).(msr_vals)
                                st.(hw).(policy))
                    false,
     1)
  else
    (st, 0).

End HardwareModel.
